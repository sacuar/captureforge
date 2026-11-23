#pragma once
#include <QMainWindow>
#include <QImage>
#include <QPolygonF>

class QLabel;
class QScrollArea;
class QAction;

class ImageEditor : public QMainWindow {
    Q_OBJECT
public:
    explicit ImageEditor(const QPixmap&, QWidget *p = nullptr);
    const QImage& currentImage() const { return image; }

signals:
    void requestOCR(const QImage&);

private slots:
    void saveAs();
    void crop();
    void rotateLeft();
    void flip();
    void copy();
    void undo();
    void togglePerspective();
    void applyPerspective();
    void fitWindowToImage();

private:
    void refresh();
    void pushUndo();
    QPoint imagePoint(const QPoint&) const;
    void finishCrop(const QPoint&);

    QLabel *label = nullptr;
    QScrollArea *scroll = nullptr;
    QAction *cropAction = nullptr;
    QAction *penAction = nullptr;
    QAction *perspectiveAction = nullptr;
    QImage image;
    QList<QImage> history;
    bool drawing = false;
    QPoint last;

    bool perspective = false;
    int activeCorner = -1;
    QPolygonF corners;
    bool cropping = false;
    bool cropDragging = false;
    QPoint cropStart;
    QPoint cropCurrent;

protected:
    bool eventFilter(QObject*, QEvent*) override;
};
