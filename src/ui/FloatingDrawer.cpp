#include "FloatingDrawer.h"
#include "Icons.h"
#include <QToolButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QLinearGradient>
#include <QSettings>
#include <QGuiApplication>
#include <QScreen>
#include <QMouseEvent>
#include <QCloseEvent>
#include <QSizePolicy>

namespace {
constexpr int DrawerWidth=62;
constexpr int ClosedHeight=104;
constexpr int OpenHeight=244;
void styleTool(QToolButton *b,const QString &colour) {
    b->setFixedSize(39,36);
    b->setIconSize(QSize(20,20));
    b->setToolButtonStyle(Qt::ToolButtonIconOnly);
    b->setStyleSheet(QString(
       "QToolButton {background:%1; border:0; border-radius:12px; padding:5px;}"
       "QToolButton:hover {background:%2;}"
       "QToolButton:pressed {background:#134c69;}"
    ).arg(colour,colour=="#134c69"?"#20738f":"#28566e"));
}
}

FloatingDrawer::FloatingDrawer(QWidget *parent):QWidget(parent) {
    setObjectName("captureDrawer");
    setWindowFlags(Qt::Tool|Qt::FramelessWindowHint|Qt::WindowStaysOnTopHint|
                   Qt::WindowDoesNotAcceptFocus);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFixedSize(DrawerWidth,ClosedHeight);
    setToolTip("CaptureForge quick tools. Close hides the drawer; tray remains running.");
    auto *root=new QVBoxLayout(this);
    root->setContentsMargins(10,5,9,8);
    root->setSpacing(3);
    auto *header=new QHBoxLayout;
    header->setContentsMargins(0,0,0,0);
    header->addStretch();
    closeButton_=new QToolButton(this);
    closeButton_->setObjectName("drawerClose");
    closeButton_->setText(QString::fromUtf8("×"));
    closeButton_->setFixedSize(20,18);
    closeButton_->setToolTip("Hide floating camera (CaptureForge stays in the tray)");
    closeButton_->setAccessibleName("Hide floating camera");
    closeButton_->setStyleSheet(
        "QToolButton{border:0;border-radius:9px;color:white;background:rgba(9,29,44,185);padding:0;}"
        "QToolButton:hover{background:#dd6642;}");
    connect(closeButton_,&QToolButton::clicked,this,&FloatingDrawer::hideRequested);
    header->addWidget(closeButton_);
    root->addLayout(header);
    camera_=new QToolButton(this);
    camera_->setObjectName("drawerCapture");
    camera_->setIcon(Icons::get("crop"));
    camera_->setToolTip("Capture region");
    camera_->setAccessibleName("Capture region");
    styleTool(camera_,"#e67e3d");
    root->addWidget(camera_,0,Qt::AlignHCenter);
    connect(camera_,&QToolButton::clicked,this,&FloatingDrawer::captureRequested);
    tools_=new QWidget(this);
    auto *toolLayout=new QVBoxLayout(tools_);
    toolLayout->setContentsMargins(0,0,0,0);toolLayout->setSpacing(3);
    auto add=[this,toolLayout](const QString &icon,const QString &tip,const QString &colour) {
        auto *b=new QToolButton(tools_);
        b->setIcon(Icons::get(icon));b->setToolTip(tip);b->setAccessibleName(tip);
        styleTool(b,colour);
        toolLayout->addWidget(b,0,Qt::AlignHCenter);
        return b;
    };
    connect(add("document_scanner","OCR last capture","#0eaaa4"),&QToolButton::clicked,
            this,&FloatingDrawer::ocrRequested);
    connect(add("edit","Open text editor","#287fe4"),&QToolButton::clicked,
            this,&FloatingDrawer::textRequested);
    connect(add("settings","Settings","#134c69"),&QToolButton::clicked,
            this,&FloatingDrawer::settingsRequested);
    root->addWidget(tools_);
    expandButton_=new QToolButton(this);
    expandButton_->setObjectName("drawerExpand");
    expandButton_->setFixedSize(38,17);
    expandButton_->setText(QString::fromUtf8("⌄"));
    expandButton_->setToolTip("Expand quick tools");
    expandButton_->setAccessibleName("Expand quick tools");
    expandButton_->setStyleSheet(
        "QToolButton{border:0;border-radius:6px;color:#e1f5fa;background:transparent;padding:0;}"
        "QToolButton:hover{background:rgba(255,255,255,55);}");
    root->addWidget(expandButton_,0,Qt::AlignHCenter);
    connect(expandButton_,&QToolButton::clicked,this,[this]{setExpanded(!expanded_);});
    root->addStretch(1);
    setExpanded(false);
}

void FloatingDrawer::setExpanded(bool enabled) {
    expanded_=enabled;
    tools_->setVisible(enabled);
    expandButton_->setText(enabled?QString::fromUtf8("⌃"):QString::fromUtf8("⌄"));
    expandButton_->setToolTip(enabled?"Collapse quick tools":"Expand quick tools");
    expandButton_->setAccessibleName(expandButton_->toolTip());
    const QRect old=geometry();
    setFixedSize(DrawerWidth,enabled?OpenHeight:ClosedHeight);
    if(old.isValid() && old.left()>0)move(old.topLeft());
    update();
}

void FloatingDrawer::refreshAppearance(){update();}
void FloatingDrawer::moveToScreenEdge() {
    const QScreen *screen=QGuiApplication::screenAt(frameGeometry().center());
    if(!screen)screen=QGuiApplication::primaryScreen();
    if(!screen)return;
    const QRect a=screen->availableGeometry();
    move(a.right()-width()-7,a.top()+(a.height()-height())/2);
}
void FloatingDrawer::paintEvent(QPaintEvent *) {
    QPainter p(this);p.setRenderHint(QPainter::Antialiasing);
    const QRectF outline=rect().adjusted(3,2,-3,-2);
    QPainterPath path;path.addRoundedRect(outline,19,19);
    QSettings settings;const QString theme=settings.value("appearance/theme","Ocean").toString();
    QLinearGradient gradient(outline.topLeft(),outline.bottomRight());
    if(theme=="Light"){
        gradient.setColorAt(0,QColor(247,252,253,236));
        gradient.setColorAt(1,QColor(204,226,237,236));
    }else if(theme=="Graphite"){
        gradient.setColorAt(0,QColor(40,47,58,228));
        gradient.setColorAt(1,QColor(30,39,52,228));
    }else{
        gradient.setColorAt(0,QColor(12,59,77,232));
        gradient.setColorAt(.52,QColor(18,51,75,233));
        gradient.setColorAt(1,QColor(32,37,66,231));
    }
    p.fillPath(path,gradient);p.setClipPath(path);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(244,135,64,58));
    p.drawEllipse(QRectF(-11,-17,65,63));
    p.setBrush(QColor(18,186,181,70));
    p.drawEllipse(QRectF(32,37,65,83));
    p.setBrush(QColor(44,137,237,70));
    p.drawEllipse(QRectF(-25,height()-80,79,81));
    p.setClipping(false);
    p.setPen(QPen(QColor(126,219,226,105),1));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(outline,19,19);
}
void FloatingDrawer::closeEvent(QCloseEvent *event) {
    event->ignore();
    emit hideRequested();
}
void FloatingDrawer::mousePressEvent(QMouseEvent *event){
    if(event->button()==Qt::LeftButton){dragStart_=event->globalPosition().toPoint()-pos();dragging_=true;event->accept();return;}
    QWidget::mousePressEvent(event);
}
void FloatingDrawer::mouseMoveEvent(QMouseEvent *event){
    if(dragging_){move(event->globalPosition().toPoint()-dragStart_);event->accept();return;}
    QWidget::mouseMoveEvent(event);
}
void FloatingDrawer::mouseReleaseEvent(QMouseEvent *event){
    dragging_=false;QWidget::mouseReleaseEvent(event);
}
