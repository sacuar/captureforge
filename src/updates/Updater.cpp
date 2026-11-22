#include "Updater.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVersionNumber>
#include <QApplication>
#include <QMessageBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QCryptographicHash>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QDesktopServices>
#include <QUrl>
#include <QTimer>
#include <QDir>
namespace {
const QString repo="https://github.com/sacuar/captureforge/releases";
QNetworkRequest request(const QUrl &url){QNetworkRequest r(url);r.setRawHeader("User-Agent","CaptureForge-Windows");r.setAttribute(QNetworkRequest::RedirectPolicyAttribute,QNetworkRequest::NoLessSafeRedirectPolicy);r.setTransferTimeout(30000);return r;}
}
Updater::Updater(QObject *parent):QObject(parent){}
void Updater::check(bool manual){
#ifdef Q_OS_LINUX
 if (manual) {
   if (QMessageBox::question(nullptr,"Ubuntu updates",
       "Ubuntu packages are available from the official Releases page. Open it now?")
       == QMessageBox::Yes)
     QDesktopServices::openUrl(QUrl(repo));
 }
 return;
#endif
 if(busy)return;busy=true;
 auto *reply=network.get(request(QUrl("https://api.github.com/repos/sacuar/captureforge/releases/latest")));
 connect(reply,&QNetworkReply::finished,this,[this,reply,manual]{
  busy=false;reply->deleteLater();
  if(reply->error()!=QNetworkReply::NoError){if(manual)QMessageBox::information(nullptr,"Updates","No published release found or GitHub is unavailable: "+reply->errorString());return;}
  auto obj=QJsonDocument::fromJson(reply->readAll()).object();QString tag=obj.value("tag_name").toString();
  if(!QRegularExpression("^v?[0-9]+\\.[0-9]+\\.[0-9]+$").match(tag).hasMatch())return;
  QString ver=tag.startsWith('v')?tag.mid(1):tag;
  if(QVersionNumber::fromString(ver)<=QVersionNumber::fromString(qApp->applicationVersion())){if(manual)QMessageBox::information(nullptr,"Updates","You have the latest version.");return;}
  QString name="CaptureForge-"+ver+"-Setup-x64.exe",url=repo+"/download/"+tag+"/"+name,digest; qint64 size=0;
  for(const auto &v:obj.value("assets").toArray()){auto a=v.toObject();if(a.value("name").toString()==name&&a.value("browser_download_url").toString()==url){digest=a.value("digest").toString();size=a.value("size").toInteger();break;}}
  bool verified=QRegularExpression("^sha256:[0-9a-fA-F]{64}$").match(digest).hasMatch()&&size>0&&size<=536870912;
  QDialog d;d.setWindowTitle("CaptureForge update");d.resize(570,460);QVBoxLayout l(&d);
  l.addWidget(new QLabel("Installed "+qApp->applicationVersion()+" → Available "+ver));l.addWidget(new QLabel("What's new"));auto *notes=new QPlainTextEdit(obj.value("body").toString());notes->setReadOnly(true);l.addWidget(notes);
  QDialogButtonBox buttons(QDialogButtonBox::Cancel);buttons.addButton(verified?"Download and install":"Open release page",QDialogButtonBox::AcceptRole);l.addWidget(&buttons);connect(&buttons,&QDialogButtonBox::accepted,&d,&QDialog::accept);connect(&buttons,&QDialogButtonBox::rejected,&d,&QDialog::reject);
  if(d.exec()!=QDialog::Accepted)return;if(!verified){QDesktopServices::openUrl(QUrl(repo));return;}
  busy=true;auto *temp=new QTemporaryDir; if(!temp->isValid()){delete temp;busy=false;return;}QString path=temp->filePath(name);auto *file=new QFile(path,this);
  if(!file->open(QIODevice::WriteOnly)){delete file;delete temp;busy=false;return;}
  auto *download=network.get(request(QUrl(url)));auto *dialog=new QProgressDialog("Downloading verified installer…","Cancel",0,100);dialog->setAutoClose(false);dialog->setAutoReset(false);dialog->setWindowModality(Qt::ApplicationModal);dialog->show();
  auto *hash=new QCryptographicHash(QCryptographicHash::Sha256);auto *count=new qint64(0);auto *bad=new bool(false);
  connect(dialog,&QProgressDialog::canceled,download,&QNetworkReply::abort);
  auto consume=[download,file,hash,count,bad,size]{QByteArray data=download->readAll();*count+=data.size();if(*count>size||file->write(data)!=data.size()){*bad=true;download->abort();return;}hash->addData(data);};
  connect(download,&QNetworkReply::readyRead,this,consume);
  connect(download,&QNetworkReply::downloadProgress,dialog,[dialog,size](qint64 n,qint64){dialog->setValue(int(qMin<qint64>(100,n*100/size)));});
  connect(download,&QNetworkReply::finished,this,[=]{
    if(download->bytesAvailable())consume();file->close();bool ok=!*bad&&download->error()==QNetworkReply::NoError&&*count==size&&hash->result().toHex()==digest.mid(7).toLatin1().toLower();
    delete hash;delete count;delete bad;file->deleteLater();download->deleteLater();dialog->deleteLater();busy=false;
    if(!ok){delete temp;QMessageBox::warning(nullptr,"Updates","Download cancelled or integrity verification failed.");return;}
    if(QMessageBox::question(nullptr,"Install update","Installer verified. Close CaptureForge and start installation?")!=QMessageBox::Yes){delete temp;return;}
    if(QProcess::startDetached(path,{"/NORESTART","/SP-"},temp->path())){temp->setAutoRemove(false);delete temp;qApp->quit();}else{delete temp;QMessageBox::warning(nullptr,"Updates","Could not launch installer.");}
  });
 });
}
