#include "Ocr.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QProcess>
#include <QSettings>
#include <QPainter>
#include <QFont>
namespace Ocr {
QString executable(){
 const QString bundled=QCoreApplication::applicationDirPath()+"/tesseract/tesseract.exe";
 if(QFileInfo::exists(bundled))return bundled;
 QSettings s; QString custom=s.value("ocr/executable").toString();if(QFileInfo(custom).isFile())return custom;
#ifdef Q_OS_WIN
 QString installed=qEnvironmentVariable("ProgramFiles","C:/Program Files")+"/Tesseract-OCR/tesseract.exe";
 if(QFileInfo::exists(installed))return installed;
#endif
 return QStandardPaths::findExecutable("tesseract");
}
QString dataPath(){
 QString exe=executable();if(exe.isEmpty())return {};
 QString dir=QFileInfo(exe).absolutePath()+"/tessdata";
 return QFileInfo::exists(dir+"/eng.traineddata")?dir:QString();
}
static Result process(const QString &exe, const QStringList &args,const QString &provider){
 QProcess p; p.setProgram(exe);p.setArguments(args);p.start();
 if(!p.waitForStarted(5000))return {{},p.errorString(),provider};
 if(!p.waitForFinished(45000)){p.kill();p.waitForFinished(3000);return {{},"OCR timed out",provider};}
 QString text=QString::fromUtf8(p.readAllStandardOutput()).trimmed();
 QString err=QString::fromUtf8(p.readAllStandardError()).trimmed();
 if(text.startsWith(QChar(0xfeff)))text.remove(0,1);
 if(p.exitStatus()!=QProcess::NormalExit||p.exitCode()!=0||text.isEmpty())return {{},err.isEmpty()?QString("No readable text or process failed"):err,provider};
 return {text,{},provider};
}
Result recognize(const QImage &image,bool windowsFallback){
 if(image.isNull())return {{},"Empty image",{}};
 QTemporaryDir dir; if(!dir.isValid())return {{},"Cannot create temporary directory",{}};
 QImage scaled=image;if(scaled.width()>4000||scaled.height()>4000)scaled=scaled.scaled(4000,4000,Qt::KeepAspectRatio,Qt::SmoothTransformation);
 QString file=dir.filePath("capture.png");if(!scaled.save(file,"PNG"))return {{},"Cannot save OCR image",{}};
 QString exe=executable();Result result={{},"Tesseract not installed", "Tesseract"};
 if(!exe.isEmpty()){
  QStringList args{file,"stdout","-l","eng","--psm","6"};QString data=dataPath();if(!data.isEmpty())args<<"--tessdata-dir"<<data;
  result=process(exe,args,"Tesseract");if(!result.text.isEmpty())return result;
 }
#ifdef Q_OS_WIN
 if(windowsFallback){
  if(scaled.width()>2400||scaled.height()>2400){scaled=scaled.scaled(2400,2400,Qt::KeepAspectRatio,Qt::SmoothTransformation);if(!scaled.save(file,"PNG"))return result;}
  QString script=QCoreApplication::applicationDirPath()+"/ocr.ps1";
  if(QFileInfo::exists(script)){
   QString ps=qEnvironmentVariable("SystemRoot","C:/Windows")+"/System32/WindowsPowerShell/v1.0/powershell.exe";
   Result fallback=process(ps,{"-NoProfile","-NonInteractive","-ExecutionPolicy","Bypass","-File",script,file},"Windows OCR");if(!fallback.text.isEmpty())return fallback;
   result.error += "\nWindows OCR: "+fallback.error;
  }
 }
#endif
 return result;
}
int selfTest(){
 QImage image(1000,220,QImage::Format_RGB32);image.fill(Qt::white);QPainter p(&image);p.setPen(Qt::black);QFont font("Arial");font.setPixelSize(64);p.setFont(font);p.drawText(image.rect(),Qt::AlignCenter,"CAPTUREFORGE OCR TEST");p.end();
 Result r=recognize(image,false);return r.text.toUpper().contains("CAPTUREFORGE")&&r.text.toUpper().contains("OCR TEST")?0:2;
}
}
