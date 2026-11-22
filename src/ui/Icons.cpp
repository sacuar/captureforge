#include "Icons.h"
#include <QCoreApplication>
#include <QApplication>
#include <QFile>
#include <QPainter>
#include <QSvgRenderer>
#include <QSettings>
#include <QStyle>
namespace Icons {
QIcon get(const QString &name){
 QFile f(QCoreApplication::applicationDirPath()+"/icons/"+name+".svg");
 if(!f.open(QIODevice::ReadOnly))return qApp->style()->standardIcon(QStyle::SP_FileIcon);
 QByteArray svg=f.readAll(); QSettings s; QByteArray color=s.value("appearance/theme","Dark").toString()=="Light"?"#263440":"#e3e9f0";
 svg.replace("#000000",color);svg.replace("fill=\"black\"","fill=\""+color+"\"");
 // Google Material SVGs commonly omit fill; apply it at the root as well.
 if(!svg.left(svg.indexOf('>')).contains(" fill="))svg.replace("<svg ","<svg fill=\""+color+"\" ");
 QSvgRenderer renderer(svg);QIcon result;
 for(int n:{16,20,24,32,48,64}){QPixmap pix(n,n);pix.fill(Qt::transparent);QPainter p(&pix);renderer.render(&p);p.end();result.addPixmap(pix);}
 return result;
}
}
