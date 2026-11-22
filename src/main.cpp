#include <QApplication>
#include <QIcon>
#include <QCoreApplication>
#include "Controller.h"
#include "ocr/Ocr.h"
#include <QTimer>
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
    Controller controller;
    if(!app.arguments().contains("--background"))controller.showHome();
    if(app.arguments().contains("--smoke-test"))QTimer::singleShot(1500,&app,&QApplication::quit);
    return app.exec();
}
