#include "CaptureOverlay.h"
#include <QGuiApplication>
#include <QScreen>
#include <QPainter>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QFontMetrics>

static QRect virtualDesktopGeometry() {
    QRect vr;
    for (QScreen *screen : QGuiApplication::screens())
        vr = vr.united(screen->geometry());
    return vr;
}

// Build one 1:1 logical-coordinate desktop image.  Qt screen geometry and mouse
// events are both expressed in logical coordinates; keeping the snapshot in the
// same coordinate space avoids the old DPI/devicePixelRatio crop mismatch.
static QImage grabDesktopLogical(const QRect &virtualRect) {
    QImage out(virtualRect.size(), QImage::Format_ARGB32_Premultiplied);
    out.fill(Qt::black);

    QPainter painter(&out);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, false);

    for (QScreen *screen : QGuiApplication::screens()) {
        const QPixmap shot = screen->grabWindow(0);
        if (shot.isNull())
            continue;

        const QRect target(screen->geometry().topLeft() - virtualRect.topLeft(),
                           screen->geometry().size());
        const QImage source = shot.toImage();
        painter.drawImage(target, source, source.rect());
    }
    return out;
}

CaptureOverlay::CaptureOverlay() {
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_DeleteOnClose);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setCursor(Qt::CrossCursor);
    setFocusPolicy(Qt::StrongFocus);

    const QRect vr = virtualDesktopGeometry();
    setGeometry(vr);
    desktop = grabDesktopLogical(vr);

    show();
    raise();
    activateWindow();
    setFocus(Qt::ActiveWindowFocusReason);
}

void CaptureOverlay::mousePressEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton)
        return;
    start = current = e->position().toPoint();
    dragging = true;
    update();
}

void CaptureOverlay::mouseMoveEvent(QMouseEvent *e) {
    if (!dragging)
        return;
    current = e->position().toPoint();
    update();
}

void CaptureOverlay::mouseReleaseEvent(QMouseEvent *e) {
    if (e->button() != Qt::LeftButton || !dragging)
        return;

    current = e->position().toPoint();
    dragging = false;

    QRect selection(start, current);
    selection = selection.normalized().intersected(rect());

    if (selection.width() >= 3 && selection.height() >= 3) {
        const QImage cut = desktop.copy(selection);
        if (!cut.isNull())
            emit captured(QPixmap::fromImage(cut));
    }
    close();
}

void CaptureOverlay::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        close();
        return;
    }
    QWidget::keyPressEvent(e);
}

void CaptureOverlay::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.drawImage(rect(), desktop);
    p.fillRect(rect(), QColor(0, 0, 0, 92));

    if (!dragging) {
        p.setPen(Qt::white);
        const QString hint = "Drag to capture  •  Esc to cancel";
        const QRect textRect = QFontMetrics(p.font()).boundingRect(hint).adjusted(-12, -8, 12, 8);
        QRect box = textRect;
        box.moveTop(20);
        box.moveCenter(QPoint(rect().center().x(), box.center().y()));
        p.fillRect(box, QColor(0, 0, 0, 160));
        p.drawText(box, Qt::AlignCenter, hint);
        return;
    }

    QRect selection(start, current);
    selection = selection.normalized().intersected(rect());
    if (!selection.isValid())
        return;

    // Reveal the selected pixels at full brightness.
    p.drawImage(selection, desktop, selection);
    p.setPen(QPen(QColor(0, 170, 255), 2));
    p.drawRect(selection.adjusted(0, 0, -1, -1));

    const QString sizeText = QString("%1 × %2").arg(selection.width()).arg(selection.height());
    QRect sizeBox = QFontMetrics(p.font()).boundingRect(sizeText).adjusted(-8, -5, 8, 5);
    QPoint pos = selection.topLeft() + QPoint(0, -sizeBox.height() - 5);
    if (pos.y() < 2)
        pos.setY(selection.top() + 5);
    if (pos.x() + sizeBox.width() > width())
        pos.setX(width() - sizeBox.width() - 2);
    sizeBox.moveTopLeft(pos);
    p.fillRect(sizeBox, QColor(0, 0, 0, 185));
    p.setPen(Qt::white);
    p.drawText(sizeBox, Qt::AlignCenter, sizeText);
}
