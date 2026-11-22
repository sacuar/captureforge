#pragma once
#include <QString>
class QApplication;
class ThemeManager {
public:
 static void applySavedTheme(QApplication &app);
 static void apply(QApplication &app,const QString &name);
};
