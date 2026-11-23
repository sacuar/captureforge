#include <QApplication>
#include <QIcon>
#include <QCoreApplication>
#include "Controller.h"
#include "ocr/Ocr.h"
#include <QTimer>
#include <QLockFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QStandardPaths>
#include <QDir>
#include <QCryptographicHash>
#include <QMessageBox>
#include "themes/ThemeManager.h"
int main(int argc,char *argv[]){
    qputenv("QT_ENABLE_HIGHDPI_SCALING","1");
    QApplication app(argc,argv);
    QApplication::setQuitOnLastWindowClosed(false);
    QApplication::setOrganizationName("CaptureForge");
    QApplication::setApplicationName("CaptureForge");
    app.setApplicationVersion(CAPTUREFORGE_VERSION);
    if(app.arguments().contains("--ocr-self-test"))return Ocr::selfTest();
    app.setWindowIcon(QIcon(QCoreApplication::applicationDirPath()+"/camera.png"));
    ThemeManager::applySavedTheme(app);
    const QString data=QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if(!QDir().mkpath(data))return 2;
    QLockFile lock(data+"/instance.lock");
    lock.setStaleLockTime(0);
    const QString endpoint="CaptureForge-"+QString::fromLatin1(QCryptographicHash::hash(data.toUtf8(),QCryptographicHash::Sha256).toHex().left(24));
    if(!lock.tryLock(0)){
        if(app.arguments().contains("--background"))return 0;
        QLocalSocket existing;existing.connectToServer(endpoint);
        if(existing.waitForConnected(1500)){existing.write("show");existing.flush();existing.waitForBytesWritten(1000);}
        else QMessageBox::information(nullptr,"CaptureForge","CaptureForge is already running. Open its existing window or system tray.");
        return 0;
    }
    Controller controller;
    QLocalServer local;
    local.setSocketOptions(QLocalServer::UserAccessOption);
    QLocalServer::removeServer(endpoint);
    if(local.listen(endpoint)){
        QObject::connect(&local,&QLocalServer::newConnection,&controller,[&]{
            while(local.hasPendingConnections()){
                auto *socket=local.nextPendingConnection();
                QObject::connect(socket,&QLocalSocket::disconnected,socket,&QObject::deleteLater);
                controller.showHome();socket->disconnectFromServer();
            }
        });
    }
    // Normal startup is unobtrusive: tray plus optional sidebar only.
    if(app.arguments().contains("--smoke-test"))QTimer::singleShot(1500,&app,&QApplication::quit);
    return app.exec();
}
