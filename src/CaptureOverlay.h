#pragma once
#include <QWidget>
#include <QImage>
#include <QPixmap>

class CaptureOverlay : public QWidget {
    Q_OBJECT
public:
    explicit CaptureOverlay();

signals:
    void captured(const QPixmap&);

protected:
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void keyPressEvent(QKeyEvent*) override;
    void paintEvent(QPaintEvent*) override;

private:
    QImage desktop;
    QPoint start;
    QPoint current;
    bool dragging = false;
};
