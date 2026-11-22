#pragma once
#include <QMainWindow>
class QPlainTextEdit;
class TextEditor:public QMainWindow{
 Q_OBJECT
public: explicit TextEditor(QWidget *p=nullptr); void setText(const QString&);
private slots: void openFile();void save();void saveAs();void findText();void replaceText();
private: QPlainTextEdit *edit;QString fileName;
};
