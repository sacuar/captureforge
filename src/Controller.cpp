#include "Controller.h"
#include "ocr/Ocr.h"
#include "updates/Updater.h"
#include "ui/Icons.h"
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QPointer>
#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QToolButton>
#include <QSizePolicy>
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
        home=new QDialog;home->setWindowTitle("CaptureForge");home->resize(410,220);
        auto *layout=new QVBoxLayout(home);layout->setContentsMargins(14,14,14,14);layout->setSpacing(8);
        auto *heading=new QLabel("<span style='font-size:20px;font-weight:600'>CaptureForge</span><br>Capture, annotate and recognise text.");layout->addWidget(heading);
        auto *row=new QHBoxLayout;row->setSpacing(5);layout->addLayout(row);
        auto make=[this,row](const QString &name,const QString &icon,auto action){
            auto *button=new QToolButton(home);button->setText(name);button->setToolTip(name);button->setAccessibleName(name);
            button->setIcon(Icons::get(icon));button->setIconSize(QSize(20,20));button->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);button->setMinimumHeight(52);
            row->addWidget(button);connect(button,&QToolButton::clicked,this,action);
        };
        make("Capture","crop",[this]{capture();});
        make("OCR","document_scanner",[this]{if(lastImage.isNull())QMessageBox::information(home,"OCR","Capture an image first.");else runOCR(lastImage);});
        make("Text editor","edit",[this]{openTextEditor();});
        auto *footer=new QHBoxLayout;footer->setSpacing(4);layout->addLayout(footer);
        auto *hint=new QLabel("Version "+qApp->applicationVersion());footer->addWidget(hint);footer->addStretch();
        auto *settings=new QToolButton(home);settings->setIcon(Icons::get("settings"));settings->setText("Settings");settings->setToolTip("Settings");settings->setAccessibleName("Settings");settings->setIconSize(QSize(18,18));
        connect(settings,&QToolButton::clicked,this,&Controller::openSettings);footer->addWidget(settings);
        auto *more=new QToolButton(home);more->setText("More");more->setToolButtonStyle(Qt::ToolButtonTextOnly);more->setPopupMode(QToolButton::InstantPopup);more->setAccessibleName("More tools");
        auto *menu=new QMenu(more);menu->addAction(Icons::get("extension"),"Manage add-ons",this,[this]{showPluginStatus();});menu->addAction(Icons::get("update"),"Check for updates",this,[this]{updater->check(true);});more->setMenu(menu);footer->addWidget(more);
        layout->addWidget(new QLabel("Capture shortcut: "+QSettings().value("shortcuts/region","Ctrl+Shift+S").toString()));layout->addStretch();
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
