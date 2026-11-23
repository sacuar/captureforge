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
 if(!f.open(QIODevice::ReadOnly)){
  // Built-in line icons keep toolbar actions distinct even without an icons/ directory.
  QPixmap pm(24,24);pm.fill(Qt::transparent);
  QPainter p(&pm);p.setRenderHint(QPainter::Antialiasing);
  const bool light=QSettings().value("appearance/theme","Dark").toString()=="Light";
  p.setPen(QPen(light?QColor("#263440"):QColor("#e3e9f0"),2,Qt::SolidLine,Qt::RoundCap,Qt::RoundJoin));
  if(name=="close"){p.drawLine(6,6,18,18);p.drawLine(18,6,6,18);}
  else if(name=="camera"){p.drawRoundedRect(3,7,18,13,2,2);p.drawEllipse(QPoint(12,13),4,4);p.drawLine(7,7,9,4);p.drawLine(9,4,15,4);p.drawLine(15,4,17,7);}
  else if(name=="crop"){p.drawLine(5,3,5,18);p.drawLine(5,18,20,18);p.drawLine(3,6,18,6);p.drawLine(18,6,18,21);}
  else if(name=="save"){p.drawRoundedRect(4,3,16,18,2,2);p.drawRect(8,4,8,6);p.drawRect(8,14,8,7);}
  else if(name=="content_copy"){p.drawRect(7,6,12,15);p.drawLine(4,17,4,3);p.drawLine(4,3,16,3);}
  else if(name=="rotate_left"){p.drawArc(QRect(5,5,14,14),30*16,280*16);p.drawLine(5,4,5,10);p.drawLine(5,4,11,4);}
  else if(name=="flip"){p.drawLine(12,3,12,21);p.drawLine(5,7,5,17);p.drawLine(5,7,10,12);p.drawLine(5,17,10,12);p.drawLine(19,7,19,17);p.drawLine(19,7,14,12);p.drawLine(19,17,14,12);}
  else if(name=="edit"){p.drawLine(5,18,16,7);p.drawLine(8,20,19,9);p.drawLine(15,6,18,3);p.drawLine(18,3,21,6);}
  else if(name=="undo"){p.drawArc(QRect(5,7,16,12),10*16,225*16);p.drawLine(5,5,5,12);p.drawLine(5,12,12,12);}
  else if(name=="document_scanner"){p.drawLine(4,9,4,4);p.drawLine(4,4,9,4);p.drawLine(15,4,20,4);p.drawLine(20,4,20,9);p.drawLine(4,15,4,20);p.drawLine(4,20,9,20);p.drawLine(15,20,20,20);p.drawLine(20,20,20,15);p.drawLine(7,12,17,12);}
  else if(name=="settings"){p.drawEllipse(QPoint(12,12),8,8);p.drawEllipse(QPoint(12,12),3,3);}
  else if(name=="extension"){p.drawRoundedRect(4,4,16,16,3,3);p.drawLine(12,7,12,17);p.drawLine(7,12,17,12);}
  else if(name=="update"){p.drawArc(QRect(4,4,16,16),40*16,280*16);p.drawLine(18,3,20,10);p.drawLine(20,10,14,9);}
  else if(name=="history"){p.drawEllipse(QPoint(12,12),9,9);p.drawLine(12,6,12,12);p.drawLine(12,12,17,14);}
  else {p.drawRoundedRect(4,4,16,16,3,3);p.drawLine(8,12,16,12);}
  p.end();return QIcon(pm);
 }
 QByteArray svg=f.readAll(); QSettings s; QByteArray color=s.value("appearance/theme","Dark").toString()=="Light"?"#263440":"#e3e9f0";
 svg.replace("#000000",color);svg.replace("fill=\"black\"","fill=\""+color+"\"");
 // Google Material SVGs commonly omit fill; apply it at the root as well.
 if(!svg.left(svg.indexOf('>')).contains(" fill="))svg.replace("<svg ","<svg fill=\""+color+"\" ");
 QSvgRenderer renderer(svg);QIcon result;
 for(int n:{16,20,24,32,48,64}){QPixmap pix(n,n);pix.fill(Qt::transparent);QPainter p(&pix);renderer.render(&p);p.end();result.addPixmap(pix);}
 return result;
}
}
