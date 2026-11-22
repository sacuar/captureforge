#pragma once
#include <QDialog>
class QLineEdit; class QKeySequenceEdit; class QComboBox; class QCheckBox;
class SettingsDialog:public QDialog{
 Q_OBJECT
public: explicit SettingsDialog(QWidget *parent=nullptr);
signals: void settingsChanged();
private slots: void save();
private:
 QKeySequenceEdit *region,*ocr,*editor,*settingsKey;
 QComboBox *theme; QCheckBox *floating,*updates,*plugins; QLineEdit *ocrExe;
};
