#include "TextEditor.h"
#include "ui/Icons.h"
#include <QToolBar>
#include <QAction>
#include <QPlainTextEdit>
#include <QMenuBar>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QInputDialog>
#include <QMessageBox>
TextEditor::TextEditor(QWidget*p):QMainWindow(p){
 setAttribute(Qt::WA_DeleteOnClose);setWindowTitle("CaptureForge Text Editor");resize(900,650);edit=new QPlainTextEdit;edit->setUndoRedoEnabled(true);setCentralWidget(edit);
 auto *tools=addToolBar("Text tools");tools->setMovable(false);tools->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);tools->addAction(Icons::get("save"),"Save",this,&TextEditor::save);tools->addAction(Icons::get("content_copy"),"Copy",edit,&QPlainTextEdit::copy);tools->addAction(Icons::get("search"),"Find",this,&TextEditor::findText);
 auto *file=menuBar()->addMenu("&File");file->addAction("New",[this]{edit->clear();fileName.clear();});file->addAction("Open",this,&TextEditor::openFile);
 file->addAction("Save",this,&TextEditor::save);file->addAction("Save As",this,&TextEditor::saveAs);file->addSeparator();file->addAction("Close",this,&QWidget::close);
 auto *ed=menuBar()->addMenu("&Edit");ed->addAction("Undo",edit,&QPlainTextEdit::undo);ed->addAction("Redo",edit,&QPlainTextEdit::redo);ed->addSeparator();
 ed->addAction("Cut",edit,&QPlainTextEdit::cut);ed->addAction("Copy",edit,&QPlainTextEdit::copy);ed->addAction("Paste",edit,&QPlainTextEdit::paste);
 ed->addAction("Select All",edit,&QPlainTextEdit::selectAll);ed->addAction("Find",this,&TextEditor::findText);ed->addAction("Replace",this,&TextEditor::replaceText);
 auto *fmt=menuBar()->addMenu("F&ormat");auto *wrap=fmt->addAction("Word Wrap");wrap->setCheckable(true);wrap->setChecked(true);
 connect(wrap,&QAction::toggled,this,[this](bool on){edit->setLineWrapMode(on?QPlainTextEdit::WidgetWidth:QPlainTextEdit::NoWrap);});
}
void TextEditor::setText(const QString&s){edit->setPlainText(s);}
void TextEditor::openFile(){auto f=QFileDialog::getOpenFileName(this,"Open text",{}, "Text (*.txt *.log *.md);;All (*.*)");if(f.isEmpty())return;QFile q(f);if(q.open(QIODevice::ReadOnly|QIODevice::Text)){edit->setPlainText(QString::fromUtf8(q.readAll()));fileName=f;}}
void TextEditor::save(){if(fileName.isEmpty()){saveAs();return;}QFile q(fileName);if(q.open(QIODevice::WriteOnly|QIODevice::Text))q.write(edit->toPlainText().toUtf8());}
void TextEditor::saveAs(){auto f=QFileDialog::getSaveFileName(this,"Save text","capture.txt","Text (*.txt);;Markdown (*.md)");if(!f.isEmpty()){fileName=f;save();}}
void TextEditor::findText(){bool ok=false;auto s=QInputDialog::getText(this,"Find","Text:",QLineEdit::Normal,{},&ok);if(ok&&!s.isEmpty()&&!edit->find(s))QMessageBox::information(this,"Find","No further match.");}
void TextEditor::replaceText(){bool ok=false;auto a=QInputDialog::getText(this,"Replace","Find:",QLineEdit::Normal,{},&ok);if(!ok||a.isEmpty())return;auto b=QInputDialog::getText(this,"Replace","Replace with:",QLineEdit::Normal,{},&ok);if(ok){QString t=edit->toPlainText();t.replace(a,b,Qt::CaseSensitive);edit->setPlainText(t);}}
