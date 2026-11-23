#include "ImageEditor.h"
#include "ui/Icons.h"
#include <QMessageBox>
#include <QLabel>
#include <QScrollArea>
#include <QToolBar>
#include <QSettings>
#include <QMenu>
#include <QKeySequence>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QPainter>
#include <QMouseEvent>
#include <QAction>
#include <QStatusBar>
#include <QSignalBlocker>
#include <QGuiApplication>
#include <QScreen>
#include <QTransform>
#include <QtMath>

ImageEditor::ImageEditor(const QPixmap &p, QWidget *parent)
    : QMainWindow(parent), image(p.toImage()) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("CaptureForge Image Editor");
    resize(640, 480);

    label = new QLabel;
    label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    label->setMouseTracking(true);
    label->installEventFilter(this);

    scroll = new QScrollArea;
    scroll->setWidget(label);
    scroll->setWidgetResizable(false); // mouse coordinates now match image pixels
    scroll->setAlignment(Qt::AlignCenter);
    setCentralWidget(scroll);

    auto *t = addToolBar("Image Tools");
    t->setMovable(false); t->setIconSize(QSize(18,18)); t->setObjectName("ImageTools"); t->setToolButtonStyle(QSettings().value("ui/toolbarLabels",false).toBool()?Qt::ToolButtonTextBesideIcon:Qt::ToolButtonIconOnly);
    t->addAction(Icons::get("save"),"Save As", this, &ImageEditor::saveAs);
    t->addAction(Icons::get("content_copy"),"Copy", this, &ImageEditor::copy);
    t->addSeparator();

    cropAction = t->addAction(Icons::get("crop"),"Crop");
    cropAction->setCheckable(true);
    connect(cropAction, &QAction::triggered, this, &ImageEditor::crop);

    perspectiveAction = t->addAction(Icons::get("crop"),"Straighten document");
    perspectiveAction->setCheckable(true);
    connect(perspectiveAction,&QAction::triggered,this,&ImageEditor::togglePerspective);
    t->addAction("Apply straighten",this,&ImageEditor::applyPerspective);
    t->addAction(Icons::get("rotate_left"),"Rotate Left", this, &ImageEditor::rotateLeft);
    t->addAction(Icons::get("flip"),"Flip", this, &ImageEditor::flip);

    penAction = t->addAction(Icons::get("edit"),"Pen");
    penAction->setCheckable(true);
    connect(penAction, &QAction::toggled, this, [this](bool v) {
        drawing = v;
        if (v && cropping) {
            cropping = false;
            cropDragging = false;
            QSignalBlocker block(cropAction);
            cropAction->setChecked(false);
            refresh();
        }
        statusBar()->showMessage(v ? "Pen: drag on the image to draw." : "Ready");
    });

    t->addAction(Icons::get("undo"),"Undo", this, &ImageEditor::undo);
    t->addSeparator();
    t->addAction(Icons::get("document_scanner"),"OCR → Text Editor", [this] { emit requestOCR(image); });

    for(auto *action:t->actions())if(!action->isSeparator()){action->setToolTip(action->text());}
    t->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(t,&QToolBar::customContextMenuRequested,this,[t](const QPoint &point){
        QMenu menu(t);auto *labels=menu.addAction("Show toolbar labels");labels->setCheckable(true);labels->setChecked(t->toolButtonStyle()!=Qt::ToolButtonIconOnly);
        if(menu.exec(t->mapToGlobal(point))==labels){QSettings().setValue("ui/toolbarLabels",labels->isChecked());t->setToolButtonStyle(labels->isChecked()?Qt::ToolButtonTextBesideIcon:Qt::ToolButtonIconOnly);}
    });
    statusBar()->showMessage("Ready");
    refresh();
    fitWindowToImage();
}

void ImageEditor::refresh() {
    QImage display = image;
    if (cropping && cropDragging) {
        display = image.copy();
        QPainter p(&display);
        const QRect r(cropStart, cropCurrent);
        const QRect cropRect = r.normalized().intersected(image.rect());
        if (cropRect.isValid()) {
            p.fillRect(QRect(0, 0, display.width(), cropRect.top()), QColor(0, 0, 0, 90));
            p.fillRect(QRect(0, cropRect.bottom() + 1, display.width(), display.height() - cropRect.bottom() - 1), QColor(0, 0, 0, 90));
            p.fillRect(QRect(0, cropRect.top(), cropRect.left(), cropRect.height()), QColor(0, 0, 0, 90));
            p.fillRect(QRect(cropRect.right() + 1, cropRect.top(), display.width() - cropRect.right() - 1, cropRect.height()), QColor(0, 0, 0, 90));
            p.setPen(QPen(QColor(0, 170, 255), 2));
            p.drawRect(cropRect.adjusted(0, 0, -1, -1));
        }
    }

    if(perspective && corners.size()==4){
        QPainter p(&display);
        p.setRenderHint(QPainter::Antialiasing);
        p.setPen(QPen(QColor(0,175,255),3));
        p.drawPolygon(corners);
        p.setBrush(QColor(255,255,255));
        for(const QPointF &corner:corners)p.drawEllipse(corner,6,6);
    }
    label->setPixmap(QPixmap::fromImage(display));
    label->setFixedSize(image.size());
}

void ImageEditor::fitWindowToImage() {
    if(image.isNull())return;
    const QRect screen=QGuiApplication::screenAt(mapToGlobal(rect().center()))
        ? QGuiApplication::screenAt(mapToGlobal(rect().center()))->availableGeometry()
        : QGuiApplication::primaryScreen()->availableGeometry();
    // Fit small captures exactly; do not open a full-screen blank canvas.
    const int frameW=36, frameH=100;
    resize(qBound(340,image.width()+frameW,qMax(340,screen.width()-100)),
           qBound(180,image.height()+frameH,qMax(180,screen.height()-100)));
}

void ImageEditor::togglePerspective() {
    perspective=perspectiveAction->isChecked();
    if(perspective){
        if(penAction->isChecked())penAction->setChecked(false);
        if(cropAction->isChecked())cropAction->setChecked(false);
        cropping=false;
        corners=QPolygonF({QPointF(0,0),QPointF(image.width()-1,0),
            QPointF(image.width()-1,image.height()-1),QPointF(0,image.height()-1)});
        statusBar()->showMessage("Drag the four blue corner handles to document corners, then Apply straighten.");
    }else{activeCorner=-1;statusBar()->showMessage("Straightening cancelled.");}
    refresh();
}

void ImageEditor::applyPerspective() {
    if(!perspective || corners.size()!=4 || image.isNull())return;
    const qreal top=QLineF(corners[0],corners[1]).length();
    const qreal bottom=QLineF(corners[3],corners[2]).length();
    const qreal left=QLineF(corners[0],corners[3]).length();
    const qreal right=QLineF(corners[1],corners[2]).length();
    const int w=qRound(qMax(top,bottom)),h=qRound(qMax(left,right));
    if(w<3 || h<3 || qint64(w)*h>100000000){
        QMessageBox::warning(this,"Straighten","Document corners are invalid or the output is too large.");
        return;
    }
    // Qt constructs the projective homography from the selected corners.
    // Inverse-mapped resampling avoids gaps between source pixels.
    QTransform destinationToSource;
    const QPolygonF destination({QPointF(0,0),QPointF(w,0),QPointF(w,h),QPointF(0,h)});
    if(!QTransform::quadToQuad(destination,corners,destinationToSource)){
        QMessageBox::warning(this,"Straighten","Document corners cannot form a valid perspective transform.");
        return;
    }
    bool invertible=false;
    const QTransform sourceToDestination=destinationToSource.inverted(&invertible);
    if(!invertible)return;
    pushUndo();
    QImage corrected(QSize(w,h),QImage::Format_ARGB32_Premultiplied);
    corrected.fill(Qt::white);
    QPainter painter(&corrected);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setTransform(sourceToDestination);
    painter.drawImage(QPointF(0,0),image);
    painter.end();
    image=corrected;
    perspective=false;
    activeCorner=-1;
    perspectiveAction->setChecked(false);
    refresh();
    fitWindowToImage();
    statusBar()->showMessage("Perspective correction applied. Undo is available.",3000);
}

void ImageEditor::pushUndo() {
    history.append(image);
    if (history.size() > 20)
        history.removeFirst();
}

void ImageEditor::saveAs() {
    const QString f = QFileDialog::getSaveFileName(this, "Export image", "Capture.png",
        "PNG (*.png);;JPEG (*.jpg *.jpeg);;WebP (*.webp)");
    if (!f.isEmpty() && !image.save(f)) QMessageBox::warning(this,"Export","Could not save image.");
}

void ImageEditor::copy() {
    QApplication::clipboard()->setImage(image);
    statusBar()->showMessage("Image copied to clipboard.", 2500);
}

void ImageEditor::crop() {
    cropping = cropAction->isChecked();
    cropDragging = false;
    if (cropping) {
        if (penAction->isChecked())
            penAction->setChecked(false);
        statusBar()->showMessage("Crop: drag a rectangle on the image. Release to apply.");
    } else {
        statusBar()->showMessage("Crop cancelled.", 1800);
    }
    refresh();
}

void ImageEditor::finishCrop(const QPoint &point) {
    cropCurrent = imagePoint(point);
    const QRect r = QRect(cropStart, cropCurrent).normalized().intersected(image.rect());
    cropDragging = false;
    cropping = false;
    {
        QSignalBlocker block(cropAction);
        cropAction->setChecked(false);
    }

    if (r.width() >= 3 && r.height() >= 3) {
        pushUndo();
        image = image.copy(r);
        statusBar()->showMessage(QString("Cropped to %1 × %2.").arg(image.width()).arg(image.height()), 2500);
    } else {
        statusBar()->showMessage("Crop cancelled: selection was too small.", 2500);
    }
    refresh();
    fitWindowToImage();
}

void ImageEditor::rotateLeft() {
    pushUndo();
    QTransform t;
    t.rotate(-90);
    image = image.transformed(t);
    refresh();
    fitWindowToImage();
}

void ImageEditor::flip() {
    pushUndo();
    image = image.mirrored(true, false);
    refresh();
}

void ImageEditor::undo() {
    if (history.isEmpty())
        return;
    image = history.takeLast();
    refresh();
    fitWindowToImage();
    statusBar()->showMessage("Undo applied.", 1800);
}

QPoint ImageEditor::imagePoint(const QPoint &p) const {
    return QPoint(qBound(0, p.x(), qMax(0, image.width() - 1)),
                  qBound(0, p.y(), qMax(0, image.height() - 1)));
}

bool ImageEditor::eventFilter(QObject *o, QEvent *e) {
    if(o==label && perspective){
        if(e->type()==QEvent::MouseButtonPress){
            const auto *m=static_cast<QMouseEvent*>(e);
            if(m->button()==Qt::LeftButton){
                const QPointF position=m->position();
                qreal distance=1e20;
                activeCorner=-1;
                for(int i=0;i<corners.size();++i){
                    const qreal d=QLineF(position,corners[i]).length();
                    if(d<distance && d<28){distance=d;activeCorner=i;}
                }
                return true;
            }
        }
        if(e->type()==QEvent::MouseMove && activeCorner>=0){
            const auto *m=static_cast<QMouseEvent*>(e);
            corners[activeCorner]=imagePoint(m->position().toPoint());
            refresh();return true;
        }
        if(e->type()==QEvent::MouseButtonRelease){
            activeCorner=-1;return true;
        }
    }
    if (o == label && cropping) {
        if (e->type() == QEvent::MouseButtonPress) {
            auto *m = static_cast<QMouseEvent*>(e);
            if (m->button() == Qt::LeftButton) {
                cropStart = cropCurrent = imagePoint(m->position().toPoint());
                cropDragging = true;
                refresh();
                return true;
            }
        }
        if (e->type() == QEvent::MouseMove && cropDragging) {
            auto *m = static_cast<QMouseEvent*>(e);
            cropCurrent = imagePoint(m->position().toPoint());
            refresh();
            return true;
        }
        if (e->type() == QEvent::MouseButtonRelease && cropDragging) {
            auto *m = static_cast<QMouseEvent*>(e);
            if (m->button() == Qt::LeftButton) {
                finishCrop(m->position().toPoint());
                return true;
            }
        }
    }

    if (o == label && drawing) {
        if (e->type() == QEvent::MouseButtonPress) {
            auto *m = static_cast<QMouseEvent*>(e);
            if (m->button() == Qt::LeftButton) {
                pushUndo();
                last = imagePoint(m->position().toPoint());
                return true;
            }
        }
        if (e->type() == QEvent::MouseMove) {
            auto *m = static_cast<QMouseEvent*>(e);
            if (m->buttons() & Qt::LeftButton) {
                const QPoint now = imagePoint(m->position().toPoint());
                QPainter p(&image);
                p.setRenderHint(QPainter::Antialiasing);
                p.setPen(QPen(Qt::red, 4, Qt::SolidLine, Qt::RoundCap));
                p.drawLine(last, now);
                last = now;
                refresh();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(o, e);
}
