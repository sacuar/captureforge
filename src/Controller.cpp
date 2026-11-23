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
#include <QMimeData>
#include <QHotkey>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QPushButton>
#include <QWidget>
#include <QAction>
#include <QGuiApplication>
#include <QInputDialog>
#include <QCheckBox>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QDateTime>
#include <QProgressDialog>
#include <QTextStream>
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
    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, [this]{
        if (!QSettings().value("clipboard/historyEnabled",false).toBool() || updatingClipboard) return;
        const auto *mime=QApplication::clipboard()->mimeData();
        if (!mime || !mime->hasText()) return;
        QString value=mime->text();
        if(value.isEmpty() || value.size()>16384) return;
        clipboardHistory.removeAll(value);
        clipboardHistory.prepend(value);
        while(clipboardHistory.size()>qBound(1,QSettings().value("clipboard/maxItems",20).toInt(),100))
            clipboardHistory.removeLast();
    });
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
    m->addAction(Icons::get("document_scanner"), "Scan from scanner…", this, &Controller::scanDocument);
    m->addAction(Icons::get("history"), "Clipboard history…", this, &Controller::showClipboardHistory);
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

void Controller::showClipboardHistory() {
    QDialog d;
    d.setWindowTitle("Clipboard history · this session only");
    auto *layout=new QVBoxLayout(&d);
    layout->addWidget(new QLabel("Optional local text history. Disabled by default; never uploaded or saved to disk."));
    auto *enabled=new QCheckBox("Remember text copied while CaptureForge is running");
    enabled->setChecked(QSettings().value("clipboard/historyEnabled",false).toBool());
    layout->addWidget(enabled);
    auto *items=new QListWidget;
    for(const QString &value: clipboardHistory) {
        auto *item=new QListWidgetItem(value.left(130).replace('\\n',' '),items);
        item->setToolTip(value.left(1000));
        item->setData(Qt::UserRole,value);
    }
    items->setMinimumSize(420,175);
    layout->addWidget(items);
    auto *row=new QHBoxLayout;
    auto *restore=new QPushButton("Copy selected");
    auto *clear=new QPushButton("Clear history");
    auto *close=new QPushButton("Close");
    row->addWidget(restore);row->addWidget(clear);row->addStretch();row->addWidget(close);
    layout->addLayout(row);
    connect(enabled,&QCheckBox::toggled,&d,[this](bool v){
        QSettings().setValue("clipboard/historyEnabled",v);
        if(!v)clipboardHistory.clear();
    });
    connect(restore,&QPushButton::clicked,&d,[this,items]{
        if(!items->currentItem())return;
        updatingClipboard=true;
        QApplication::clipboard()->setText(items->currentItem()->data(Qt::UserRole).toString());
        updatingClipboard=false;
    });
    connect(clear,&QPushButton::clicked,&d,[this,items]{clipboardHistory.clear();items->clear();});
    connect(close,&QPushButton::clicked,&d,&QDialog::accept);
    d.exec();
}

void Controller::scanDocument() {
#ifndef Q_OS_WIN
    QMessageBox::information(nullptr,"Scanner support",
        "Scanner acquisition currently uses Windows Image Acquisition (WIA). "
        "On this platform, import a scanner image through the image editor.");
#else
    // Windows enumerates locally installed WIA scanners, including network
    // devices exposed via compatible WIA drivers. No arbitrary network probing.
    auto *process=new QProcess(this);
    const QString output=QStandardPaths::writableLocation(QStandardPaths::TempLocation)
        +"/CaptureForge-scan-"+QUuid::createUuid().toString(QUuid::WithoutBraces)+".png";
    const QString script=QString::fromUtf8(R"PS(
$ErrorActionPreference='Stop'
try {
 $manager=New-Object -ComObject WIA.DeviceManager
 $scanners=@($manager.DeviceInfos | Where-Object { $_.Type -eq 1 })
 if ($scanners.Count -eq 0) { throw 'No WIA-compatible scanners were found. Install your scanner driver in Windows first.' }
 $devices=@()
 foreach($deviceInfo in $scanners){
   $name=try{$deviceInfo.Properties.Item('Name').Value}catch{'Scanner'}
   $devices+=("$name")
 }
 $chosen=0
 if($devices.Count -gt 1){
   Add-Type -AssemblyName Microsoft.VisualBasic
   $reply=[Microsoft.VisualBasic.Interaction]::InputBox(
       ('Available scanners: '+ (($devices | ForEach-Object { [array]::IndexOf($devices,$_).ToString()+': '+$_ }) -join '; ')),
       'CaptureForge - choose scanner index','0')
   if($reply -notmatch '^\d+$'){exit 3}
   $chosen=[int]$reply
   if($chosen -ge $scanners.Count){throw 'Invalid scanner selection'}
 }
 $dev=$scanners[$chosen].Connect()
 $item=$dev.Items.Item(1)
 $scan=$item.Transfer('{B96B3CAF-0728-11D3-9D7B-0000F81EF32E}')
 $scan.SaveFile('%1')
 Write-Output '%1'
}catch{Write-Error $_.Exception.Message; exit 1}
)PS").arg(QDir::toNativeSeparators(output).replace("'","''"));
    auto *progress=new QProgressDialog("Discovering WIA scanners and acquiring image…","Cancel",0,0);
    progress->setWindowTitle("CaptureForge Scan");
    progress->setAttribute(Qt::WA_DeleteOnClose, false);
    progress->setMinimumDuration(0);
    progress->setWindowModality(Qt::NonModal);
    connect(progress,&QProgressDialog::canceled,process,&QProcess::kill);
    connect(process,QOverload<int,QProcess::ExitStatus>::of(&QProcess::finished),
        this,[this,process,progress,output](int code,QProcess::ExitStatus status){
        progress->close();
        progress->deleteLater();
        process->deleteLater();
        if(code==0 && status==QProcess::NormalExit && QFileInfo::exists(output)){
            QImage image(output);
            QFile::remove(output);
            if(!image.isNull()){
                lastImage=image;
                auto *editor=new ImageEditor(QPixmap::fromImage(image));
                connect(editor,&ImageEditor::requestOCR,this,&Controller::runOCR);
                editor->show();return;
            }
        }
        QFile::remove(output);
        if(code!=3 && status==QProcess::NormalExit)
            QMessageBox::warning(nullptr,"Scan failed",
                "Unable to acquire an image. Check scanner power, WIA driver and permissions.\\n"+
                QString::fromLocal8Bit(process->readAllStandardError()).left(1500));
    });
    process->start("powershell.exe",{"-NoProfile","-STA","-Command",script});
    progress->show();
#endif
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
