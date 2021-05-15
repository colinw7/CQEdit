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
  fileMenu = new CQMenu(this, "&File");

  //----

  newItem = new CQMenuItem(fileMenu, "&New");

  newItem->setShortcut("Ctrl+N");
  newItem->setStatusTip("Create New file");

  connect(newItem->getAction(), SIGNAL(triggered()), this, SLOT(newFileSlot()));

  //----

  loadItem = new CQMenuItem(fileMenu, "&Add");

  loadItem->setShortcut("Ctrl+A");
  loadItem->setStatusTip("Add new file");
  loadItem->setIcon(CQPixmapCacheInst->getIcon("ADD_FILE"));

  connect(loadItem->getAction(), SIGNAL(triggered()), this, SLOT(addFileSlot()));

  //----

  loadItem = new CQMenuItem(fileMenu, "&Replace");

  loadItem->setShortcut("Ctrl+R");
  loadItem->setStatusTip("Replace current file");
  loadItem->setIcon(CQPixmapCacheInst->getIcon("OPEN"));

  connect(loadItem->getAction(), SIGNAL(triggered()), this, SLOT(replaceFileSlot()));

  //----

  saveItem = new CQMenuItem(fileMenu, "&Save");

  saveItem->setShortcut("Ctrl+S");
  saveItem->setStatusTip("Save current file");

  saveItem->setIcon(CQPixmapCacheInst->getIcon("SAVE"));

  connect(saveItem->getAction(), SIGNAL(triggered()), this, SLOT(saveFileSlot()));

  //----

  saveAsItem = new CQMenuItem(fileMenu, "Save &As...");

  saveAsItem->setShortcut("Ctrl+A");
  saveAsItem->setStatusTip("Save current file with new name");

  saveAsItem->setIcon(CQPixmapCacheInst->getIcon("SAVE_AS"));

  connect(saveAsItem->getAction(), SIGNAL(triggered()), this, SLOT(saveFileAsSlot()));

  //----

  closeItem = new CQMenuItem(fileMenu, "Close");

  closeItem->setShortcut("Ctrl+C");
  closeItem->setStatusTip("Close current file");

  //closeItem->setIcon(save_as_data, SAVE_AS_DATA_LEN);

  connect(closeItem->getAction(), SIGNAL(triggered()), this, SLOT(closeFileSlot()));

  //----

  fileMenu->addSeparator();

  //----

  quitItem = new CQMenuItem(fileMenu, "&Quit");

  quitItem->setShortcut("Ctrl+Q");
  quitItem->setStatusTip("Quit the application");

  connect(quitItem->getAction(), SIGNAL(triggered()), this, SLOT(close()));

  //--------

  editMenu = new CQMenu(this, "&Edit");

  //----

  cutItem = new CQMenuItem(editMenu, "C&ut");

  cutItem->setShortcut("Ctrl+X");
  cutItem->setStatusTip("Cut selected text");

  cutItem->setIcon(CQPixmapCacheInst->getIcon("CUT"));

  copyItem = new CQMenuItem(editMenu, "&Copy");

  copyItem->setShortcut("Ctrl+C");
  copyItem->setStatusTip("Copy selected text");

  copyItem->setIcon(CQPixmapCacheInst->getIcon("COPY"));

  pasteItem = new CQMenuItem(editMenu, "&Paste");

  pasteItem->setShortcut("Ctrl+V");
  pasteItem->setStatusTip("Paste selected text");

  pasteItem->setIcon(CQPixmapCacheInst->getIcon("PASTE"));

  editMenu->addSeparator();

  undoItem = new CQMenuItem(editMenu, "&Undo");

  undoItem->setShortcut("Ctrl+Z");
  undoItem->setStatusTip("Undo last change");

  undoItem->setIcon(CQPixmapCacheInst->getIcon("UNDO"));

  connect(undoItem->getAction(), SIGNAL(triggered()), this, SLOT(undo()));

  redoItem = new CQMenuItem(editMenu, "&Redo");

  redoItem->setShortcut("Ctrl+Y");
  redoItem->setStatusTip("Redo last undo");

  redoItem->setIcon(CQPixmapCacheInst->getIcon("REDO"));

  connect(redoItem->getAction(), SIGNAL(triggered()), this, SLOT(redo()));

  //--------

  viewMenu = new CQMenu(this, "&View");

  bgItem = new CQMenuItem(viewMenu, "Set &Background");

  bgItem->setStatusTip("Set Background");

  connect(bgItem->getAction(), SIGNAL(triggered()), this, SLOT(setBg()));

  //--------

  new CQMenu(this, "|");

  //--------

  helpMenu = new CQMenu(this, "&Help");

  //----

  aboutItem = new CQMenuItem(helpMenu, "&About");

  aboutItem->setStatusTip("Show the application's About box");

//connect(aboutItem->getAction(), SIGNAL(triggered()), this, SLOT(about()));
}

void
CQEditTest::
createToolBars()
{
  fileToolBar = new CQToolBar(this, "&File");

  fileToolBar->addItem(loadItem);
  fileToolBar->addItem(saveItem);
  fileToolBar->addItem(saveAsItem);

  //----

  editToolBar = new CQToolBar(this, "&Edit");

  //editToolBar->setIconSize(QSize(16,16));

  editToolBar->addItem(cutItem);
  editToolBar->addItem(copyItem);
  editToolBar->addItem(pasteItem);

  editToolBar->addSeparator();

  editToolBar->addItem(undoItem);
  editToolBar->addItem(redoItem);

  //----

  styleToolBar = new CQToolBar(this, "&Style");

  //styleToolBar->setIconSize(QSize(16,16));

  font_ = new CQFontChooser(this);

  font_->setStyle(CQFontChooser::FontButton);

  connect(font_, SIGNAL(fontChanged(const QFont &)), this, SLOT(setFont(const QFont &)));

  styleToolBar->addWidget(font_);

  color_ = new CQColorChooser(this);

  color_->setStyles(CQColorChooser::ColorButton);

  connect(color_, SIGNAL(colorApplied(const QColor &)),
          this, SLOT(setSelectionColor(const QColor &)));

  styleToolBar->addWidget(color_);

  //----

  modeToolBar = new CQToolBar(this, "&Mode");

  //modeToolBar->setIconSize(QSize(16,16));

  mode_ = new QComboBox(this);

  mode_->setFocusPolicy(Qt::NoFocus);
  mode_->addItems(QStringList() << "Vi" << "Text");

  connect(mode_, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(modeChanged(const QString &)));

  modeToolBar->addWidget(mode_);
}

void
CQEditTest::
createStatusBar()
{
  statusLabel = new QLabel(" ");

  statusLabel->setAlignment(Qt::AlignHCenter);
  statusLabel->setMinimumSize(statusLabel->sizeHint());

  statusBar()->addWidget(statusLabel);

  messageLabel = new QLabel(" ");

  messageLabel->setAlignment(Qt::AlignHCenter);
  messageLabel->setMinimumSize(messageLabel->sizeHint());

  statusBar()->addPermanentWidget(messageLabel);

  insButton = new QToolButton();

  insButton->setText("INS");

  insButton->setMinimumSize(insButton->sizeHint());

  statusBar()->addPermanentWidget(insButton);

  sizeLabel = new QLabel(" ");

  sizeLabel->setAlignment(Qt::AlignHCenter);
  sizeLabel->setMinimumSize(sizeLabel->sizeHint());

  statusBar()->addPermanentWidget(sizeLabel);

  positionLabel = new QLabel(" ");

  positionLabel->setAlignment(Qt::AlignHCenter);
  positionLabel->setMinimumSize(positionLabel->sizeHint());

  statusBar()->addPermanentWidget(positionLabel);
}

void
CQEditTest::
currentFileChanged(int)
{
  QWidget *w = fileTab_->currentWidget();

  QTabWidget *editTab = qobject_cast<QTabWidget *>(w);

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
  QString fileName =
    QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

  addFile(fileName.toStdString());
}

void
CQEditTest::
replaceFileSlot()
{
  QString fileName =
    QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

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
  CQEditBg *dialog = new CQEditBg(this);

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

  CQEditFile *file = edit_->getFile();

  QString mode;

  if (file->getMode() == CVEditFile::ModeVi) {
    mode = "Vi ";

    mode += (file->getInsertMode() ? "(Insert)" : "(Command)");
  }
  else
    mode = "Text";

  QString msg = QString("Mode: %1").arg(mode);

  statusLabel->setText(msg);

  if      (file->getInsertMode())
    insButton->setText("INS");
  else if (file->getVisual())
    insButton->setText("VIS");
  else
    insButton->setText("OVR");

  QString size = QString("(%1x%2)").arg(file->getNumRows() + 1).arg(file->getNumCols() + 1);

  sizeLabel->setText(size);

  QString pos = QString("(%1,%2)").arg(file->getRow() + 1).arg(file->getCol() + 1);

  positionLabel->setText(pos);

  bool changed = file->getChanged();

  if (changed != lastChanged) {
    updateTitle();

    lastChanged = changed;
  }

  undoItem->getAction()->setEnabled(file->canUndo());
  redoItem->getAction()->setEnabled(file->canRedo());
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
  messageLabel->setText(msg);
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
