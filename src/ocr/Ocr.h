#pragma once
#include <QString>
#include <QImage>
namespace Ocr {
struct Result { QString text, error, provider; };
QString executable();
QString dataPath();
Result recognize(const QImage &image, bool windowsFallback=true);
int selfTest();
}
