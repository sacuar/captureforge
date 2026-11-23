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
#include <QWidget>
#include <QAction>
#include <QGuiApplication>
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
    // Closing the sidebar only hides this optional control, never the tray.
    floating = new QWidget;
    floating->setObjectName("CaptureSidebar");
    floating->setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    floating->setAttribute(Qt::WA_QuitOnClose, false);
    floating->setFixedSize(73, 43);
    floating->setStyleSheet(
        "#CaptureSidebar{background:#222b35;border:1px solid #53616e;border-radius:12px;}"
        "#CaptureSidebar QPushButton{border:0;background:transparent;padding:0;border-radius:8px;}"
        "#CaptureSidebar QPushButton:hover{background:#37556e;}"
    );
    auto *row = new QHBoxLayout(floating);
    row->setContentsMargins(4,4,4,4);
    row->setSpacing(2);
    auto *camera = new QPushButton(floating);
    camera->setIcon(Icons::get("camera"));
    camera->setIconSize(QSize(24,24));
    camera->setFixedSize(38,33);
    camera->setAccessibleName("Capture region");
    camera->setToolTip("Capture region · Ctrl+Shift+S");
    row->addWidget(camera);
    connect(camera, &QPushButton::clicked, this, &Controller::capture);
    auto *dismiss = new QPushButton(floating);
    dismiss->setIcon(Icons::get("close"));
    dismiss->setIconSize(QSize(15,15));
    dismiss->setFixedSize(21,25);
    dismiss->setAccessibleName("Hide sidebar (keep tray running)");
    dismiss->setToolTip("Hide sidebar; CaptureForge stays in the system tray");
    row->addWidget(dismiss);
    connect(dismiss, &QPushButton::clicked, this, [this]{
        QSettings().setValue("ui/floating", false);
        floating->hide();
        if (floatingToggle) floatingToggle->setChecked(false);
        if (tray && !tray->isVisible()) tray->show();
    });
    const QRect a = QGuiApplication::primaryScreen()->availableGeometry();
    const QSettings s;
    const int x = qBound(a.left(), s.value("ui/sidebarX",a.right()-92).toInt(),a.right()-floating->width());
    const int y = qBound(a.top(), s.value("ui/sidebarY",a.center().y()).toInt(),a.bottom()-floating->height());
    floating->move(x,y);
    floating->setVisible(s.value("ui/floating",true).toBool());
}

void Controller::buildTray() {
    QIcon icon(appFile("camera.png"));
    if (icon.isNull()) icon = Icons::get("camera");
    tray = new QSystemTrayIcon(icon, this);
    auto *m = new QMenu;
    m->addAction(Icons::get("crop"), "Capture Region", this, &Controller::capture);
    floatingToggle = m->addAction(Icons::get("camera"), "Show sidebar camera");
    floatingToggle->setCheckable(true);
    floatingToggle->setChecked(QSettings().value("ui/floating",true).toBool());
    connect(floatingToggle,&QAction::toggled,this,[this](bool show){
        QSettings().setValue("ui/floating",show);
        if(floating)floating->setVisible(show);
        if(tray && !tray->isVisible())tray->show();
    });
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
    // Explorer may create the notification area after the app starts.
    auto *retry = new QTimer(this);
    connect(retry,&QTimer::timeout,this,[this]{
        if (tray && QSystemTrayIcon::isSystemTrayAvailable() && !tray->isVisible()) tray->show();
    });
    retry->start(5000);
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

void Controller::showHome() {
    // Desktop shortcut / second-instance activation reveals the sidebar.
    if (floating) {
        if (floatingToggle) floatingToggle->setChecked(true);
        floating->show();
        floating->raise();
        floating->activateWindow();
    }
}

void Controller::reloadSettings() {
    registerHotkeys();
    QSettings s;
    if (floatingToggle) floatingToggle->setChecked(s.value("ui/floating",true).toBool());
    floating->setVisible(s.value("ui/floating", true).toBool());
    if (tray && !tray->isVisible()) tray->show();
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
#ifdef Q_OS_LINUX
    // QScreen::grabWindow(0) cannot bypass Wayland's capture permissions.
    // Never produce a black screenshot or imply a successful capture.
    if (QGuiApplication::platformName().contains("wayland", Qt::CaseInsensitive)) {
        QMessageBox::information(nullptr, "Capture on Wayland",
            "Direct screen capture is restricted by Wayland. "
            "Run an X11 desktop session for region capture. "
            "Text editing and OCR of existing images are still available.");
        return;
    }
#endif
    if (overlay || capturePending)
        return;
    capturePending=true;

    floating->hide();
    if(home)home->hide();
    // Give the compositor time to remove the floating button before
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
             loaded.isEmpty() ? "(none — Tesseract OCR may be available)" : loaded.join("\n"));

    if (!errors.isEmpty())
        text += "\n\nLoad errors:\n" + errors.join("\n");

    QDialog d;d.setWindowTitle("CaptureForge Add-ons");d.resize(580,400);QVBoxLayout layout(&d);
    layout.addWidget(new QLabel("<h2>Available providers</h2>"));
    auto *info=new QPlainTextEdit(QString("Tesseract OCR: %1\nWindows OCR: available on Windows only\nImage editor: built in\nText editor: built in\n\n").arg(Ocr::executable().isEmpty()?"Not detected":Ocr::executable())+text);info->setReadOnly(true);layout.addWidget(info);
    auto *buttons=new QDialogButtonBox(QDialogButtonBox::Close);connect(buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);layout.addWidget(buttons);d.exec();
}

void Controller::openPluginFolder() {
    QDir().mkpath(pluginManager->pluginDirectory());
    QDesktopServices::openUrl(QUrl::fromLocalFile(pluginManager->pluginDirectory()));
}
