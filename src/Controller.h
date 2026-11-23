#pragma once
#include <QObject>
#include <QImage>
#include <QSystemTrayIcon>

class QHotkey;
class QPushButton;
class QWidget;
class QAction;
class QDialog;
class CaptureOverlay;
class PluginManager;
class Updater;

class Controller : public QObject {
    Q_OBJECT
public:
    Controller();
    void showHome();

private slots:
    void capture();
    void openSettings();
    void reloadSettings();
    void openTextEditor();
    void runOCR(const QImage&);
    void trayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    void buildTray();
    void buildFloating();
    void registerHotkeys();
    void showOcrText(const QString &text, const QString &providerName);
    void reloadPlugins();
    void showPluginStatus();
    void openPluginFolder();

    QHotkey *hkCapture = nullptr;
    QHotkey *hkOCR = nullptr;
    QHotkey *hkText = nullptr;
    QHotkey *hkSettings = nullptr;
    QSystemTrayIcon *tray = nullptr;
    QWidget *floating = nullptr;
    QAction *floatingToggle = nullptr;
    CaptureOverlay *overlay = nullptr;
    PluginManager *pluginManager = nullptr;
    QImage lastImage;
    Updater *updater=nullptr;
    QDialog *home=nullptr;
    bool ocrBusy=false, capturePending=false;
};
