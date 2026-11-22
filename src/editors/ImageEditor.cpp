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

ImageEditor::ImageEditor(const QPixmap &p, QWidget *parent)
    : QMainWindow(parent), image(p.toImage()) {
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle("CaptureForge Image Editor");
    resize(1100, 760);

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

    label->setPixmap(QPixmap::fromImage(display));
    label->setFixedSize(image.size());
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
}

void ImageEditor::rotateLeft() {
    pushUndo();
    QTransform t;
    t.rotate(-90);
    image = image.transformed(t);
    refresh();
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
    statusBar()->showMessage("Undo applied.", 1800);
}

QPoint ImageEditor::imagePoint(const QPoint &p) const {
    return QPoint(qBound(0, p.x(), qMax(0, image.width() - 1)),
                  qBound(0, p.y(), qMax(0, image.height() - 1)));
}

bool ImageEditor::eventFilter(QObject *o, QEvent *e) {
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
