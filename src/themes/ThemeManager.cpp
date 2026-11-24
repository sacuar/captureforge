#include "ThemeManager.h"
#include <QApplication>
#include <QSettings>
void ThemeManager::applySavedTheme(QApplication &app){QSettings s;apply(app,s.value("appearance/theme","Ocean").toString());}
void ThemeManager::apply(QApplication &app,const QString &name){
 bool light=name=="Light";bool ocean=name=="Ocean";QString bg=light?"#f5f8fa":ocean?"#102735":name=="Graphite"?"#25282d":"#1c2027", panel=light?"#ffffff":ocean?"#163748":"#272e38", fg=light?"#213044":"#edf2f7", edge=light?"#d4dbe5":ocean?"#376275":"#3a4655", hover=light?"#e5edf7":ocean?"#1c586b":"#354253";
 app.setStyleSheet(QString(R"(
 QWidget {background:%1;color:%3;font-family:'Segoe UI';font-size:13px;}
 QToolBar {border:0;spacing:2px;padding:3px 5px;}
 QToolBar::separator {width:1px;background:%4;margin:4px 3px;}
 QToolBar QToolButton {padding:4px;border-radius:5px;min-width:20px;min-height:20px;}
 QToolBar QToolButton::menu-indicator {width:9px;}
 QMenuBar::item {padding:4px 9px;}
 QToolButton,QPushButton {background:transparent;border:1px solid transparent;border-radius:6px;padding:7px;}
 QToolButton:hover,QPushButton:hover {background:%5;} QToolButton:checked {background:#246bc0;color:white;}
 QLineEdit,QPlainTextEdit,QTextEdit,QComboBox,QKeySequenceEdit {background:%2;border:1px solid %4;border-radius:6px;padding:7px;selection-background-color:#246bc0;}
 QScrollArea {border:0;} QScrollBar:vertical {background:transparent;width:12px;margin:3px;}
 QScrollBar:horizontal {background:transparent;height:12px;margin:3px;}
 QScrollBar::handle {background:%4;border-radius:3px;min-height:28px;min-width:28px;}
 QScrollBar::handle:hover {background:#6485aa;} QScrollBar::add-line,QScrollBar::sub-line {width:0;height:0;}
 QScrollBar::add-page,QScrollBar::sub-page {background:transparent;}
 QTabBar::tab {padding:9px 15px;border-radius:6px;} QTabBar::tab:selected {background:%2;border-bottom:2px solid #246bc0;}
 QMenu {background:%2;border:1px solid %4;padding:4px;border-radius:7px;} QMenu::item {padding:5px 15px;} QMenu::item:selected {background:%5;}
 QWidget#captureDrawer {background:transparent;}
 QStatusBar {border-top:1px solid %4;} QDialogButtonBox QPushButton {background:%2;border:1px solid %4;min-width:75px;}
 )").arg(bg,panel,fg,edge,hover));
}
