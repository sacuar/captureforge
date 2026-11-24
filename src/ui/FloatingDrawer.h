#pragma once
#include <QWidget>
class QToolButton;
class QFrame;

class FloatingDrawer : public QWidget {
    Q_OBJECT
public:
    explicit FloatingDrawer(QWidget *parent=nullptr);
    void setExpanded(bool expanded);
    bool isExpanded() const { return expanded_; }
    void refreshAppearance();
    void moveToScreenEdge();
signals:
    void captureRequested();
    void ocrRequested();
    void textRequested();
    void settingsRequested();
    void hideRequested();
protected:
    void paintEvent(QPaintEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
private:
    QToolButton *camera_=nullptr,*expandButton_=nullptr,*closeButton_=nullptr;
    QWidget *tools_=nullptr;
    bool expanded_=false;
    bool dragging_=false;
    QPoint dragStart_;
};
