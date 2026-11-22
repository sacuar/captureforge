#include "SettingsDialog.h"
#include "ocr/Ocr.h"
#include "ui/Icons.h"
#include <QTabWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QFileDialog>
#include <QMessageBox>
#include <QtConcurrent>
#include <QFutureWatcher>
#include <QVBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QKeySequenceEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSettings>
#include <QLabel>
SettingsDialog::SettingsDialog(QWidget *p):QDialog(p){
 setWindowTitle("CaptureForge Settings");resize(640,480);
 auto *v=new QVBoxLayout(this);
 v->addWidget(new QLabel("<h2>Settings</h2><b>Shortcuts</b>"));
 auto *f=new QFormLayout;
 region=new QKeySequenceEdit;ocr=new QKeySequenceEdit;editor=new QKeySequenceEdit;settingsKey=new QKeySequenceEdit;
 QSettings s;
 region->setKeySequence(QKeySequence(s.value("shortcuts/region","Ctrl+Shift+S").toString()));
 ocr->setKeySequence(QKeySequence(s.value("shortcuts/ocr","Ctrl+Shift+O").toString()));
 editor->setKeySequence(QKeySequence(s.value("shortcuts/textEditor","Ctrl+Shift+E").toString()));
 settingsKey->setKeySequence(QKeySequence(s.value("shortcuts/settings","Ctrl+Shift+,").toString()));
 f->addRow("Capture region",region);f->addRow("OCR last image",ocr);f->addRow("Open text editor",editor);f->addRow("Settings",settingsKey);v->addLayout(f);
 v->addWidget(new QLabel("<b>Appearance / UI</b>"));
 theme=new QComboBox;theme->addItems({"Dark","Graphite","Light"});theme->setCurrentText(s.value("appearance/theme","Dark").toString());
 floating=new QCheckBox("Show floating capture button");floating->setChecked(s.value("ui/floating",true).toBool());
 v->addWidget(theme);v->addWidget(floating);
 updates=new QCheckBox("Check GitHub for updates on startup");updates->setChecked(s.value("updates/startup",true).toBool());v->addWidget(updates);
 plugins=new QCheckBox("Load trusted DLL add-ons automatically (restart required)");plugins->setChecked(s.value("plugins/enabled",false).toBool());v->addWidget(plugins);
 auto *tabs=new QTabWidget;auto *ocrTab=new QWidget;auto *ol=new QVBoxLayout(ocrTab);
 ol->addWidget(new QLabel("Bundled Tesseract is preferred. A custom executable is used if the bundle is missing."));
 ocrExe=new QLineEdit(s.value("ocr/executable").toString());ol->addWidget(ocrExe);
 auto *browse=new QPushButton(Icons::get("settings"),"Locate Tesseract executable");ol->addWidget(browse);connect(browse,&QPushButton::clicked,this,[this]{QString file=QFileDialog::getOpenFileName(this,"Locate Tesseract",{},"All files (*)");if(!file.isEmpty())ocrExe->setText(file);});
 auto *status=new QLabel("Detected: "+Ocr::executable());status->setWordWrap(true);ol->addWidget(status);
 auto *test=new QPushButton(Icons::get("document_scanner"),"Test installed OCR");ol->addWidget(test);connect(test,&QPushButton::clicked,this,[this,test,status]{test->setEnabled(false);status->setText("Testing English recognition using saved configuration…");auto *w=new QFutureWatcher<int>(this);connect(w,&QFutureWatcher<int>::finished,this,[w,test,status]{status->setText(w->result()==0?"PASS — test text recognised":"FAIL — engine, DLLs or English language data missing");test->setEnabled(true);w->deleteLater();});w->setFuture(QtConcurrent::run([]{return Ocr::selfTest();}));});
 ol->addStretch();tabs->addTab(ocrTab,"OCR diagnostics");v->addWidget(tabs,1);
 auto *reset=new QPushButton("Reset application settings");v->addWidget(reset);connect(reset,&QPushButton::clicked,this,[this]{if(QMessageBox::question(this,"Reset","Restore CaptureForge settings? Your files are not changed.")==QMessageBox::Yes){QSettings s;s.clear();s.sync();emit settingsChanged();reject();}});
 auto *bb=new QDialogButtonBox(QDialogButtonBox::Save|QDialogButtonBox::Cancel);
 connect(bb,&QDialogButtonBox::accepted,this,&SettingsDialog::save);connect(bb,&QDialogButtonBox::rejected,this,&QDialog::reject);v->addWidget(bb);
}
void SettingsDialog::save(){
 QSettings s;s.setValue("shortcuts/region",region->keySequence().toString());s.setValue("shortcuts/ocr",ocr->keySequence().toString());
 s.setValue("shortcuts/textEditor",editor->keySequence().toString());s.setValue("shortcuts/settings",settingsKey->keySequence().toString());
 s.setValue("appearance/theme",theme->currentText());s.setValue("ui/floating",floating->isChecked());s.setValue("updates/startup",updates->isChecked());s.setValue("plugins/enabled",plugins->isChecked());s.setValue("ocr/executable",ocrExe->text());emit settingsChanged();accept();
}
