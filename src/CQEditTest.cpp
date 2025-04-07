#include <CQEditTest.h>

#include <QStatusBar>
#include <QLabel>
#include <QComboBox>
#include <QVBoxLayout>
#include <QFileDialog>
#include <QLineEdit>
#include <QToolButton>

#include <CQApp.h>
#include <CQEdit.h>
#include <CQFont.h>
#include <CQFontChooser.h>
#include <CQColorChooser.h>
#include <CQTabWidget.h>
#include <CQEditBg.h>
#include <CQUtil.h>
#include <CQUtilFont.h>
#include <CQUtilRGBA.h>
#include <CSyntaxCPP.h>
#include <CFile.h>
#include <CRGBName.h>

#include <svg/add_file_svg.h>
#include <svg/open_svg.h>
#include <svg/save_svg.h>
#include <svg/save_as_svg.h>
#include <svg/copy_svg.h>
#include <svg/cut_svg.h>
#include <svg/paste_svg.h>
#include <svg/undo_svg.h>
#include <svg/redo_svg.h>

CQEditTest::
CQEditTest() :
 CQMainWindow("CQEdit")
{
  auto *frame = new QWidget;

  auto *vlayout = new QVBoxLayout(frame);

  vlayout->setMargin(0); vlayout->setSpacing(0);

  //----

  fileTab_ = new CQTabWidget;

  fileTab_->setTabPosition(QTabWidget::North);

  vlayout->addWidget(fileTab_);

  //----

  setCentralWidget(frame);

  createMenus    ();
  createToolBars ();
  createStatusBar();

  updateStatus();
  updateTitle ();

  //CQAppInst->addObjEditFilter(this);

  //connect(qApp, SIGNAL(lastWindowClosed()), this, SLOT(saveConfig()));
}

CQEditTest::
~CQEditTest()
{
}

void
CQEditTest::
addFile(const std::string &fileName)
{
  auto *editTab = new QTabWidget;

  editTab->setTabPosition(QTabWidget::West);

  //----

  auto *edit = new CQEdit;

  edit->getFile()->loadLines(fileName);

  edit->getFile()->setSyntax(new CSyntaxCPP);

  connect(edit, SIGNAL(stateChanged   ()), this, SLOT(updateStatus()));
  connect(edit, SIGNAL(fileNameChanged()), this, SLOT(updateTitle ()));
  connect(edit, SIGNAL(sizeChanged    ()), this, SLOT(updateStatus()));
  connect(edit, SIGNAL(quitCommand    ()), this, SLOT(close       ()));

  connect(edit, SIGNAL(outputMsg(const QString &)), this, SLOT(outputMessage(const QString &)));
  connect(edit, SIGNAL(errorMsg (const QString &)), this, SLOT(errorMessage(const QString &)));

  editTab->addTab(edit, "Edit");

  //----

  auto *output = new CQEdit;

  editTab->addTab(output, "Output");

  //----

  fileTab_->addTab(editTab, fileName.c_str());

  connect(fileTab_, SIGNAL(currentChanged(int)), this, SLOT(currentFileChanged(int)));

  //----

  font_->setFont(edit->getFile()->getQFont());

  edit->setFocus();

  setCurrent(edit, output);
}

void
CQEditTest::
createMenus()
{
  fileMenu_ = new CQMenu(this, "&File");

  //----

  newItem_ = new CQMenuItem(fileMenu_, "&New");

  newItem_->setShortcut("Ctrl+N");
  newItem_->setStatusTip("Create New file");

  connect(newItem_->getAction(), SIGNAL(triggered()), this, SLOT(newFileSlot()));

  //----

  loadItem_ = new CQMenuItem(fileMenu_, "&Add");

  loadItem_->setShortcut("Ctrl+A");
  loadItem_->setStatusTip("Add new file");
  loadItem_->setIcon(CQPixmapCacheInst->getIcon("ADD_FILE"));

  connect(loadItem_->getAction(), SIGNAL(triggered()), this, SLOT(addFileSlot()));

  //----

  loadItem_ = new CQMenuItem(fileMenu_, "&Replace");

  loadItem_->setShortcut("Ctrl+R");
  loadItem_->setStatusTip("Replace current file");
  loadItem_->setIcon(CQPixmapCacheInst->getIcon("OPEN"));

  connect(loadItem_->getAction(), SIGNAL(triggered()), this, SLOT(replaceFileSlot()));

  //----

  saveItem_ = new CQMenuItem(fileMenu_, "&Save");

  saveItem_->setShortcut("Ctrl+S");
  saveItem_->setStatusTip("Save current file");

  saveItem_->setIcon(CQPixmapCacheInst->getIcon("SAVE"));

  connect(saveItem_->getAction(), SIGNAL(triggered()), this, SLOT(saveFileSlot()));

  //----

  saveAsItem_ = new CQMenuItem(fileMenu_, "Save &As...");

  saveAsItem_->setShortcut("Ctrl+A");
  saveAsItem_->setStatusTip("Save current file with new name");

  saveAsItem_->setIcon(CQPixmapCacheInst->getIcon("SAVE_AS"));

  connect(saveAsItem_->getAction(), SIGNAL(triggered()), this, SLOT(saveFileAsSlot()));

  //----

  closeItem_ = new CQMenuItem(fileMenu_, "Close");

  closeItem_->setShortcut("Ctrl+C");
  closeItem_->setStatusTip("Close current file");

  //closeItem_->setIcon(save_as_data, SAVE_AS_DATA_LEN);

  connect(closeItem_->getAction(), SIGNAL(triggered()), this, SLOT(closeFileSlot()));

  //----

  fileMenu_->addSeparator();

  //----

  quitItem_ = new CQMenuItem(fileMenu_, "&Quit");

  quitItem_->setShortcut("Ctrl+Q");
  quitItem_->setStatusTip("Quit the application");

  connect(quitItem_->getAction(), SIGNAL(triggered()), this, SLOT(close()));

  //--------

  editMenu_ = new CQMenu(this, "&Edit");

  //----

  cutItem_ = new CQMenuItem(editMenu_, "C&ut");

  cutItem_->setShortcut("Ctrl+X");
  cutItem_->setStatusTip("Cut selected text");

  cutItem_->setIcon(CQPixmapCacheInst->getIcon("CUT"));

  copyItem_ = new CQMenuItem(editMenu_, "&Copy");

  copyItem_->setShortcut("Ctrl+C");
  copyItem_->setStatusTip("Copy selected text");

  copyItem_->setIcon(CQPixmapCacheInst->getIcon("COPY"));

  pasteItem_ = new CQMenuItem(editMenu_, "&Paste");

  pasteItem_->setShortcut("Ctrl+V");
  pasteItem_->setStatusTip("Paste selected text");

  pasteItem_->setIcon(CQPixmapCacheInst->getIcon("PASTE"));

  editMenu_->addSeparator();

  undoItem_ = new CQMenuItem(editMenu_, "&Undo");

  undoItem_->setShortcut("Ctrl+Z");
  undoItem_->setStatusTip("Undo last change");

  undoItem_->setIcon(CQPixmapCacheInst->getIcon("UNDO"));

  connect(undoItem_->getAction(), SIGNAL(triggered()), this, SLOT(undo()));

  redoItem_ = new CQMenuItem(editMenu_, "&Redo");

  redoItem_->setShortcut("Ctrl+Y");
  redoItem_->setStatusTip("Redo last undo");

  redoItem_->setIcon(CQPixmapCacheInst->getIcon("REDO"));

  connect(redoItem_->getAction(), SIGNAL(triggered()), this, SLOT(redo()));

  //--------

  viewMenu_ = new CQMenu(this, "&View");

  bgItem_ = new CQMenuItem(viewMenu_, "Set &Background");

  bgItem_->setStatusTip("Set Background");

  connect(bgItem_->getAction(), SIGNAL(triggered()), this, SLOT(setBg()));

  //--------

  new CQMenu(this, "|");

  //--------

  helpMenu_ = new CQMenu(this, "&Help");

  //----

  aboutItem_ = new CQMenuItem(helpMenu_, "&About");

  aboutItem_->setStatusTip("Show the application's About box");

//connect(aboutItem_->getAction(), SIGNAL(triggered()), this, SLOT(about()));
}

void
CQEditTest::
createToolBars()
{
  fileToolBar_ = new CQToolBar(this, "&File");

  fileToolBar_->addItem(loadItem_);
  fileToolBar_->addItem(saveItem_);
  fileToolBar_->addItem(saveAsItem_);

  //----

  editToolBar_ = new CQToolBar(this, "&Edit");

  //editToolBar_->setIconSize(QSize(16, 16));

  editToolBar_->addItem(cutItem_);
  editToolBar_->addItem(copyItem_);
  editToolBar_->addItem(pasteItem_);

  editToolBar_->addSeparator();

  editToolBar_->addItem(undoItem_);
  editToolBar_->addItem(redoItem_);

  //----

  styleToolBar_ = new CQToolBar(this, "&Style");

  //styleToolBar_->setIconSize(QSize(16, 16));

  font_ = new CQFontChooser(this);

  font_->setStyle(CQFontChooser::FontButton);

  connect(font_, SIGNAL(fontChanged(const QFont &)), this, SLOT(setFont(const QFont &)));

  styleToolBar_->addWidget(font_);

  color_ = new CQColorChooser(this);

  color_->setStyles(CQColorChooser::ColorButton);

  connect(color_, SIGNAL(colorApplied(const QColor &)),
          this, SLOT(setSelectionColor(const QColor &)));

  styleToolBar_->addWidget(color_);

  //----

  modeToolBar_ = new CQToolBar(this, "&Mode");

  //modeToolBar_->setIconSize(QSize(16, 16));

  mode_ = new QComboBox(this);

  mode_->setFocusPolicy(Qt::NoFocus);
  mode_->addItems(QStringList() << "Vi" << "Text");

  connect(mode_, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(modeChanged(const QString &)));

  modeToolBar_->addWidget(mode_);
}

void
CQEditTest::
createStatusBar()
{
  statusLabel_ = new QLabel(" ");

  statusLabel_->setAlignment(Qt::AlignHCenter);
  statusLabel_->setMinimumSize(statusLabel_->sizeHint());

  statusBar()->addWidget(statusLabel_);

  messageLabel_ = new QLabel(" ");

  messageLabel_->setAlignment(Qt::AlignHCenter);
  messageLabel_->setMinimumSize(messageLabel_->sizeHint());

  statusBar()->addPermanentWidget(messageLabel_);

  insButton_ = new QToolButton();

  insButton_->setText("INS");

  insButton_->setMinimumSize(insButton_->sizeHint());

  statusBar()->addPermanentWidget(insButton_);

  sizeLabel_ = new QLabel(" ");

  sizeLabel_->setAlignment(Qt::AlignHCenter);
  sizeLabel_->setMinimumSize(sizeLabel_->sizeHint());

  statusBar()->addPermanentWidget(sizeLabel_);

  positionLabel_ = new QLabel(" ");

  positionLabel_->setAlignment(Qt::AlignHCenter);
  positionLabel_->setMinimumSize(positionLabel_->sizeHint());

  statusBar()->addPermanentWidget(positionLabel_);
}

void
CQEditTest::
currentFileChanged(int)
{
  auto *w = fileTab_->currentWidget();

  auto *editTab = qobject_cast<QTabWidget *>(w);

  if (! editTab) return;

  setCurrent(qobject_cast<CQEdit *>(editTab->widget(0)),
             qobject_cast<CQEdit *>(editTab->widget(1)));
}

void
CQEditTest::
setCurrent(CQEdit *edit, CQEdit *output)
{
  edit_   = edit;
  output_ = output;

  updateStatus();
  updateTitle ();
}

void
CQEditTest::
newFileSlot()
{
  addFile("");
}

void
CQEditTest::
addFileSlot()
{
  auto fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

  addFile(fileName.toStdString());
}

void
CQEditTest::
replaceFileSlot()
{
  auto fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

  if (fileName.length())
    edit_->getFile()->loadLines(fileName.toStdString());
}

void
CQEditTest::
saveFileSlot()
{
  std::string fileName = edit_->getFile()->getFileName();

  if (fileName.empty())
    saveFileAsSlot();
  else
    edit_->getFile()->saveLines(fileName);
}

void
CQEditTest::
saveFileAsSlot()
{
  QString fileName =
    QFileDialog::getSaveFileName(this, "Save File", "", "Text Files (*.txt *.*)");

  if (fileName.length()) {
    edit_->getFile()->setFileName(fileName.toStdString());

    saveFileSlot();
  }
}

void
CQEditTest::
closeFileSlot()
{
  int ind = fileTab_->currentIndex();

  if (ind >= 0)
    fileTab_->removeTab(ind);
}

void
CQEditTest::
undo()
{
  edit_->getFile()->undo();
}

void
CQEditTest::
redo()
{
  edit_->getFile()->redo();
}

void
CQEditTest::
setFont(const QFont &qfont)
{
  if (! edit_) return;

  QString    family;
  CFontStyle style;
  int        pixelSize;

  CQUtil::decodeFont(qfont, family, style, pixelSize);

  auto pfont = CQFontMgrInst->lookupFont(family.toStdString(), style, pixelSize);

  edit_->getFile()->setFont(pfont);

  edit_->getFile()->setIgnoreChanged(true);
}

void
CQEditTest::
setSelectionColor(const QColor &qcolor)
{
  CRGB rgb = CQUtil::colorToRGB(qcolor);

  edit_->getFile()->setSelectionColor(rgb);
}

void
CQEditTest::
modeChanged(const QString &mode)
{
  if (mode == "Vi")
    edit_->getFile()->setMode(CVEditFile::ModeVi);
  else
    edit_->getFile()->setMode(CVEditFile::ModeNormal);
}

void
CQEditTest::
setBg()
{
  auto *dialog = new CQEditBg(this);

  dialog->init();

  dialog->exec();

  if (dialog->isAccepted()) {
    CRGBA c = CRGBName::toRGBA(dialog->getColorName().toStdString());

    edit_->getFile()->setBg(c);
  }

  delete dialog;
}

void
CQEditTest::
updateStatus()
{
  static bool lastChanged;

  if (! edit_) return;

  auto *file = edit_->getFile();

  QString mode;

  if (file->getMode() == CVEditFile::ModeVi) {
    mode = "Vi ";

    mode += (file->getInsertMode() ? "(Insert)" : "(Command)");
  }
  else
    mode = "Text";

  auto msg = QString("Mode: %1").arg(mode);

  statusLabel_->setText(msg);

  if      (file->getInsertMode())
    insButton_->setText("INS");
  else if (file->getVisual())
    insButton_->setText("VIS");
  else
    insButton_->setText("OVR");

  auto size = QString("(%1x%2)").arg(file->getNumRows() + 1).arg(file->getNumCols() + 1);

  sizeLabel_->setText(size);

  auto pos = QString("(%1,%2)").arg(file->getRow() + 1).arg(file->getCol() + 1);

  positionLabel_->setText(pos);

  bool changed = file->getChanged();

  if (changed != lastChanged) {
    updateTitle();

    lastChanged = changed;
  }

  undoItem_->getAction()->setEnabled(file->canUndo());
  redoItem_->getAction()->setEnabled(file->canRedo());
}

void
CQEditTest::
outputMessage(const QString &msg)
{
  output_->getFile()->addLine(msg.toStdString());
}

void
CQEditTest::
errorMessage(const QString &msg)
{
  messageLabel_->setText(msg);
}

void
CQEditTest::
updateTitle()
{
  if (! edit_) return;

  const std::string &fileName = edit_->getFile()->getFileName();

  //std::string cwd = COS::getCurrentDir();

  CFile file(fileName);

  QString title =
    QString("%1 (%2)- CQEdit").arg(file.getName().c_str()).
                               arg(file.getDir ().c_str());

  if (edit_->getFile()->getUnsaved())
    title += "(*)";

  this->setWindowTitle(title);
}
