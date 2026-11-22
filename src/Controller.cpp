#include "Controller.h"
#include "ocr/Ocr.h"
#include "updates/Updater.h"
#include "ui/Icons.h"
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPointer>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include "CaptureOverlay.h"
#include "editors/ImageEditor.h"
#include "editors/TextEditor.h"
#include "settings/SettingsDialog.h"
#include "themes/ThemeManager.h"
#include "plugins/PluginManager.h"
#include "plugins/PluginInterface.h"
#include <QApplication>
#include <QClipboard>
#include <QHotkey>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QScreen>
#include <QProcess>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QMessageBox>
#include <QIcon>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QUrl>
#include <QUuid>
#include <QtMath>

static QString appFile(const QString &name) {
    return QCoreApplication::applicationDirPath() + "/" + name;
}

Controller::Controller() {
    pluginManager = new PluginManager(this, this);
    QSettings preferences;
    if (preferences.value("plugins/enabled", false).toBool()) pluginManager->reload();
    buildFloating();
    buildTray();
    registerHotkeys();
    updater = new Updater(this);
    if (preferences.value("updates/startup",true).toBool()) QTimer::singleShot(2500,updater,[this]{updater->check(false);});
}

void Controller::buildFloating() {
    floating = new QPushButton;
    floating->setIcon(QIcon(appFile("camera.png")));
    floating->setIconSize(QSize(34, 34));
    floating->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    floating->setFixedSize(58, 58);
    floating->setToolTip("CaptureForge — click camera to capture region");
    floating->setStyleSheet("QPushButton{background:rgba(25,25,28,225);border:2px solid #8a929e;border-radius:29px}"
                            "QPushButton:hover{background:#087fb9;border-color:white}");
    connect(floating, &QPushButton::clicked, this, &Controller::capture);
    auto a = QGuiApplication::primaryScreen()->availableGeometry();
    floating->move(a.right() - 80, a.center().y());
    QSettings s;
    floating->setVisible(s.value("ui/floating", true).toBool());
}

void Controller::buildTray() {
    QIcon icon(appFile("camera.png"));
    tray = new QSystemTrayIcon(icon, this);
    auto *m = new QMenu;
    m->addAction(icon, "Open CaptureForge", this, &Controller::showHome);
    m->addAction(icon, "Capture Region", this, &Controller::capture);
    m->addAction("OCR Last Capture", this, [this] {
        if (lastImage.isNull())
            QMessageBox::information(nullptr, "OCR", "Capture an image first.");
        else
            runOCR(lastImage);
    });
    m->addAction("Open Text Editor", this, &Controller::openTextEditor);
    m->addAction(Icons::get("settings"), "Settings", this, &Controller::openSettings);
    m->addAction(Icons::get("update"), "Check for updates", this, [this]{updater->check(true);});

    auto *pluginsMenu = m->addMenu(Icons::get("extension"),"Add-ons");
    pluginsMenu->addAction("Reload Plugins", this, [this] { reloadPlugins(); });
    pluginsMenu->addAction("Open Plugins Folder", this, [this] { openPluginFolder(); });
    pluginsMenu->addAction("Manage add-ons", this, [this] { showPluginStatus(); });

    m->addSeparator();
    m->addAction("Quit", qApp, &QApplication::quit);
    tray->setContextMenu(m);
    tray->setToolTip("CaptureForge — click to capture, right-click for menu");
    connect(tray, &QSystemTrayIcon::activated, this, &Controller::trayActivated);
    tray->show();
}

void Controller::trayActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick)
        capture();
}

void Controller::registerHotkeys() {
    delete hkCapture;
    delete hkOCR;
    delete hkText;
    delete hkSettings;

    QSettings s;
    hkCapture = new QHotkey(QKeySequence(s.value("shortcuts/region", "Ctrl+Shift+S").toString()), true, this);
    connect(hkCapture, &QHotkey::activated, this, &Controller::capture);

    hkOCR = new QHotkey(QKeySequence(s.value("shortcuts/ocr", "Ctrl+Shift+O").toString()), true, this);
    connect(hkOCR, &QHotkey::activated, this, [this] {
        if (lastImage.isNull())
            QMessageBox::information(nullptr, "OCR", "Capture an image first.");
        else
            runOCR(lastImage);
    });

    hkText = new QHotkey(QKeySequence(s.value("shortcuts/textEditor", "Ctrl+Shift+E").toString()), true, this);
    connect(hkText, &QHotkey::activated, this, &Controller::openTextEditor);

    hkSettings = new QHotkey(QKeySequence(s.value("shortcuts/settings", "Ctrl+Shift+,").toString()), true, this);
    connect(hkSettings, &QHotkey::activated, this, &Controller::openSettings);
}

void Controller::showHome(){
    if(!home){
        home=new QDialog;home->setWindowTitle("CaptureForge");home->resize(550,420);
        auto *layout=new QVBoxLayout(home);layout->setContentsMargins(24,24,24,24);layout->setSpacing(10);
        auto *heading=new QLabel("<h1>CaptureForge</h1><p>Capture, annotate and recognise text locally.</p>");layout->addWidget(heading);
        auto make=[this,layout](const QString &name,const QString &icon,auto action){auto *button=new QPushButton(Icons::get(icon),name,home);button->setMinimumHeight(42);button->setIconSize(QSize(24,24));layout->addWidget(button);connect(button,&QPushButton::clicked,this,action);};
        make("Capture region","crop",[this]{capture();});
        make("OCR last capture","document_scanner",[this]{if(lastImage.isNull())QMessageBox::information(home,"OCR","Capture an image first.");else runOCR(lastImage);});
        make("Text editor","edit",[this]{openTextEditor();});
        make("Settings","settings",[this]{openSettings();});
        make("Manage add-ons","extension",[this]{showPluginStatus();});
        make("Check for updates","update",[this]{updater->check(true);});
        layout->addStretch();layout->addWidget(new QLabel("Version "+qApp->applicationVersion()+" • Ctrl+Shift+S to capture"));
    }
    home->show();home->raise();home->activateWindow();
}

void Controller::reloadSettings() {
    registerHotkeys();
    QSettings s;
    floating->setVisible(s.value("ui/floating", true).toBool());
    ThemeManager::apply(*qApp, s.value("appearance/theme", "Dark").toString());
}

void Controller::openSettings() {
    auto *d = new SettingsDialog;
    d->setAttribute(Qt::WA_DeleteOnClose);
    connect(d, &SettingsDialog::settingsChanged, this, &Controller::reloadSettings);
    d->show();
}

void Controller::openTextEditor() {
    (new TextEditor)->show();
}

void Controller::capture() {
    if (overlay || capturePending)
        return;
    capturePending=true;

    floating->hide();
    if(home)home->hide();
    // Give Windows time to remove the floating button from the compositor before
    // the desktop snapshot is taken.
    QTimer::singleShot(180, this, [this] {
        capturePending=false;
        overlay = new CaptureOverlay;
        connect(overlay, &CaptureOverlay::captured, this, [this](const QPixmap &p) {
            if (p.isNull())
                return;
            lastImage = p.toImage();
            auto *e = new ImageEditor(p);
            connect(e, &ImageEditor::requestOCR, this, &Controller::runOCR);
            e->show();
            e->raise();
            e->activateWindow();
        });
        connect(overlay, &QObject::destroyed, this, [this] {
            overlay = nullptr;
            QSettings s;
            if (s.value("ui/floating", true).toBool())
                floating->show();
        });
    });
}

void Controller::showOcrText(const QString &text, const QString &providerName) {
    const QString clean = text.trimmed();
    if (clean.isEmpty())
        return;

    auto *t = new TextEditor;
    t->setText(clean);
    t->setWindowTitle(QString("CaptureForge OCR — %1").arg(providerName));
    t->show();
    t->raise();
    t->activateWindow();
    QApplication::clipboard()->setText(clean);
}

void Controller::runOCR(const QImage &img) {
    if (img.isNull()) {
        QMessageBox::warning(nullptr, "OCR", "The image is empty.");
        return;
    }

    if (ocrBusy) { QMessageBox::information(nullptr,"OCR","Recognition already running.");return; }
    QStringList providerErrors;
    for (auto *provider: pluginManager->ocrPlugins()) {
        QString text,error;
        if(provider->recognize(img,text,error)&&!text.trimmed().isEmpty()){showOcrText(text,provider->ocrProviderName());return;}
        if(!error.isEmpty())providerErrors<<error;
    }
    ocrBusy=true;
    if(tray)tray->showMessage("CaptureForge","Recognising text locally…");
    auto *watcher=new QFutureWatcher<Ocr::Result>(this);
    connect(watcher,&QFutureWatcher<Ocr::Result>::finished,this,[this,watcher,providerErrors]{
        auto result=watcher->result();watcher->deleteLater();ocrBusy=false;
        if(!result.text.isEmpty())showOcrText(result.text,result.provider);
        else QMessageBox::warning(nullptr,"OCR failed",result.error+"\n"+providerErrors.join("\n")+"\nCheck Settings → OCR diagnostics.");
    });
    watcher->setFuture(QtConcurrent::run([img]{return Ocr::recognize(img);}));
}

void Controller::reloadPlugins() {
    if(QMessageBox::question(nullptr,"Trusted add-ons","Load all installed DLL add-ons? They run with your permissions.")!=QMessageBox::Yes)return;
    pluginManager->reload();
    if (tray)
        tray->showMessage("CaptureForge", QString("Plugins reloaded: %1 loaded").arg(pluginManager->loadedDescriptions().size()),
                          QSystemTrayIcon::Information, 2500);
}

void Controller::showPluginStatus() {
    const QStringList loaded = pluginManager->loadedDescriptions();
    const QStringList errors = pluginManager->loadErrors();

    QString text = QString("Plugin folder:\n%1\n\nLoaded:\n%2")
        .arg(QDir::toNativeSeparators(pluginManager->pluginDirectory()),
             loaded.isEmpty() ? "(none — built-in Windows OCR is still available)" : loaded.join("\n"));

    if (!errors.isEmpty())
        text += "\n\nLoad errors:\n" + errors.join("\n");

    QDialog d;d.setWindowTitle("CaptureForge Add-ons");d.resize(580,400);QVBoxLayout layout(&d);
    layout.addWidget(new QLabel("<h2>Available providers</h2>"));
    auto *info=new QPlainTextEdit(QString("Tesseract OCR: %1\nWindows OCR: built-in fallback\nImage editor: built in\nText editor: built in\n\n").arg(Ocr::executable().isEmpty()?"Not detected":Ocr::executable())+text);info->setReadOnly(true);layout.addWidget(info);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);layout.addWidget(buttons);d.exec();
}

void Controller::openPluginFolder() {
    QDir().mkpath(pluginManager->pluginDirectory());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginManager->pluginDirectory()));
}
