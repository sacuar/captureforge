#pragma once
#include <QObject>
#include <QNetworkAccessManager>
class Updater:public QObject {
 Q_OBJECT
public: explicit Updater(QObject *parent=nullptr);void check(bool manual);
private: QNetworkAccessManager network; bool busy=false;
};
