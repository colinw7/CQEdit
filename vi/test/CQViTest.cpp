#include <CQVi.h>
#include <CQViTest.h>
#include <CQViOptionsDlg.h>
#include <CSyntaxC.h>
#include <CSyntaxCPP.h>

#include <CQApp.h>
#include <CQTabSplit.h>
#include <CQMenu.h>
#include <CQToolBar.h>
#include <CQFontChooser.h>
#include <CQColorChooser.h>
#include <CQPixmapCache.h>

#include <QVBoxLayout>
#include <QFileDialog>
#include <QStatusBar>
#include <QToolButton>
#include <QLabel>

#include <svg/add_file_svg.h>
#include <svg/open_svg.h>
#include <svg/save_svg.h>
#include <svg/save_as_svg.h>
#include <svg/copy_svg.h>
#include <svg/cut_svg.h>
#include <svg/paste_svg.h>
#include <svg/undo_svg.h>
#include <svg/redo_svg.h>

int
main(int argc, char **argv)
{
  CQApp app(argc, argv);

  std::string filename;

  auto *test = new CQViTest;

  for (int i = 1; i < argc; ++i) {
    if (argv[i][0] == '-') {
      continue;
    }
    else
      test->addFile(argv[i]);
  }

  test->show();

  test->resize(test->sizeHint());

  return app.exec();
}

//----

CQViTest::
CQViTest() :
 CQMainWindow("CQVi")
{
  setObjectName("test");

  auto *frame = new QWidget;

  frame->setObjectName("frame");

  auto *vlayout = new QVBoxLayout(frame);
  vlayout->setMargin(0); vlayout->setSpacing(0);

  //---

  fileTab_ = new CQTabSplit;

//fileTab_->setTabPosition(QTabWidget::North);
  fileTab_->setState(CQTabSplit::State::TAB);
  fileTab_->setGrouped(true);
  fileTab_->setFocusPolicy(Qt::NoFocus);

  connect(fileTab_, SIGNAL(currentIndexChanged(int)), this, SLOT(tabIndexChanged(int)));

  vlayout->addWidget(fileTab_);

  //----

  setCentralWidget(frame);

  createMenus    ();
  createToolBars ();
  createStatusBar();

  //updateStatus();
  //updateTitle ();
}

CQViTest::
~CQViTest()
{
}

void
CQViTest::
addFile(const QString &filename)
{
  auto *edit = new CQVi::Widget;

  connect(edit, SIGNAL(stateChanged()), this, SLOT(updateState()));
  connect(edit, SIGNAL(positionChanged()), this, SLOT(updateState()));
  connect(edit, SIGNAL(sizeChanged()), this, SLOT(updateState()));

  edit->init();

  edit->loadFile(filename.toStdString());

  fileTab_->addWidget(edit, filename);

  edits_.push_back(edit);

  updateState();

  currentEdit()->setFocus();
}

void
CQViTest::
createMenus()
{
  fileMenu_ = new CQMenu(this, "&File");

  //----

  newItem_ = new CQMenuItem(fileMenu_, "&New");

//newItem_->setShortcut("Ctrl+N");
  newItem_->setStatusTip("Create New file");

  connect(newItem_->getAction(), SIGNAL(triggered()), this, SLOT(newFileSlot()));

  //----

  loadItem_ = new CQMenuItem(fileMenu_, "&Add");

//loadItem_->setShortcut("Ctrl+A");
  loadItem_->setStatusTip("Add new file");
  loadItem_->setIcon(CQPixmapCacheInst->getIcon("ADD_FILE"));

  connect(loadItem_->getAction(), SIGNAL(triggered()), this, SLOT(addFileSlot()));

  //----

  loadItem_ = new CQMenuItem(fileMenu_, "&Replace");

//loadItem_->setShortcut("Ctrl+R");
  loadItem_->setStatusTip("Replace current file");
  loadItem_->setIcon(CQPixmapCacheInst->getIcon("OPEN"));

  connect(loadItem_->getAction(), SIGNAL(triggered()), this, SLOT(replaceFileSlot()));

  //----

  saveItem_ = new CQMenuItem(fileMenu_, "&Save");

//saveItem_->setShortcut("Ctrl+S");
  saveItem_->setStatusTip("Save current file");

  saveItem_->setIcon(CQPixmapCacheInst->getIcon("SAVE"));

  connect(saveItem_->getAction(), SIGNAL(triggered()), this, SLOT(saveFileSlot()));

  //----

  saveAsItem_ = new CQMenuItem(fileMenu_, "Save &As...");

//saveAsItem_->setShortcut("Ctrl+A");
  saveAsItem_->setStatusTip("Save current file with new name");

  saveAsItem_->setIcon(CQPixmapCacheInst->getIcon("SAVE_AS"));

  connect(saveAsItem_->getAction(), SIGNAL(triggered()), this, SLOT(saveFileAsSlot()));

  //----

  closeItem_ = new CQMenuItem(fileMenu_, "Close");

//closeItem_->setShortcut("Ctrl+C");
  closeItem_->setStatusTip("Close current file");

  //closeItem_->setIcon(save_as_data, SAVE_AS_DATA_LEN);

  connect(closeItem_->getAction(), SIGNAL(triggered()), this, SLOT(closeFileSlot()));

  //----

  fileMenu_->addSeparator();

  //----

  quitItem_ = new CQMenuItem(fileMenu_, "&Quit");

//quitItem_->setShortcut("Ctrl+Q");
  quitItem_->setStatusTip("Quit the application");

  connect(quitItem_->getAction(), SIGNAL(triggered()), this, SLOT(close()));

  //--------

  editMenu_ = new CQMenu(this, "&Edit");

  //----

  cutItem_ = new CQMenuItem(editMenu_, "C&ut");

//cutItem_->setShortcut("Ctrl+X");
  cutItem_->setStatusTip("Cut selected text");

  cutItem_->setIcon(CQPixmapCacheInst->getIcon("CUT"));

  copyItem_ = new CQMenuItem(editMenu_, "&Copy");

//copyItem_->setShortcut("Ctrl+C");
  copyItem_->setStatusTip("Copy selected text");

  copyItem_->setIcon(CQPixmapCacheInst->getIcon("COPY"));

  pasteItem_ = new CQMenuItem(editMenu_, "&Paste");

//pasteItem_->setShortcut("Ctrl+V");
  pasteItem_->setStatusTip("Paste selected text");

  pasteItem_->setIcon(CQPixmapCacheInst->getIcon("PASTE"));

  editMenu_->addSeparator();

  undoItem_ = new CQMenuItem(editMenu_, "&Undo");

//undoItem_->setShortcut("Ctrl+Z");
  undoItem_->setStatusTip("Undo last change");

  undoItem_->setIcon(CQPixmapCacheInst->getIcon("UNDO"));

  connect(undoItem_->getAction(), SIGNAL(triggered()), this, SLOT(undo()));

  redoItem_ = new CQMenuItem(editMenu_, "&Redo");

//redoItem_->setShortcut("Ctrl+Y");
  redoItem_->setStatusTip("Redo last undo");

  redoItem_->setIcon(CQPixmapCacheInst->getIcon("REDO"));

  connect(redoItem_->getAction(), SIGNAL(triggered()), this, SLOT(redo()));

  //--------

  viewMenu_ = new CQMenu(this, "&View");

  bgItem_ = new CQMenuItem(viewMenu_, "&Options ...");

  bgItem_->setStatusTip("Set Options");

  connect(bgItem_->getAction(), SIGNAL(triggered()), this, SLOT(setOptions()));

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
CQViTest::
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

  font_->setFont(CQVi::Mgr::instance()->font());
  font_->setStyle(CQFontChooser::FontButton);
  font_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  connect(font_, SIGNAL(fontChanged(const QFont &)), this, SLOT(setFont(const QFont &)));

  styleToolBar_->addWidget(font_);

  color_ = new CQColorChooser(this);

  color_->setStyles(CQColorChooser::ColorButton);
  color_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

  connect(color_, SIGNAL(colorApplied(const QColor &)),
          this, SLOT(setSelectionColor(const QColor &)));

  styleToolBar_->addWidget(color_);

  //----

#if 0
  modeToolBar_ = new CQToolBar(this, "&Mode");
//modeToolBar_->setIconSize(QSize(16, 16));

  mode_ = new QComboBox(this);

  mode_->setFocusPolicy(Qt::NoFocus);
  mode_->addItems(QStringList() << "Vi" << "Text");

  connect(mode_, SIGNAL(currentIndexChanged(const QString &)),
          this, SLOT(modeChanged(const QString &)));

  modeToolBar_->addWidget(mode_);
#endif
}

void
CQViTest::
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

  insButton_->setText("NORM");
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
CQViTest::
tabIndexChanged(int ind)
{
  auto *w = fileTab_->widget(ind);

  if (w)
    w->setFocus();
}

void
CQViTest::
updateState()
{
  auto *edit = currentEdit();

  if (! edit) return;

  QString text = "NORM";

  if      (edit->app()->getInsertMode())
    text = "INS";
  else if (edit->app()->getVisualMode() == CVi::App::VisualMode::CHAR)
    text = "VCHAR";
  else if (edit->app()->getVisualMode() == CVi::App::VisualMode::LINE)
    text = "VLINE";
  else if (edit->app()->getVisualMode() == CVi::App::VisualMode::BLOCK)
    text = "VBLOCK";
  else if (edit->app()->getCmdLineMode())
    text = "COMMAND";

  insButton_->setText(text);

  auto size = QString("(%1x%2)").arg(edit->rows()).arg(edit->cols());

  sizeLabel_->setText(size);

  auto pos = QString("(%1,%2)").
    arg(edit->app()->getRow() + 1).arg(currentEdit()->app()->getCol() + 1);

  positionLabel_->setText(pos);
}

void
CQViTest::
newFileSlot()
{
  addFile("");
}

void
CQViTest::
addFileSlot()
{
  QString fileName =
    QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

  if (fileName != "")
    addFile(fileName);
}

void
CQViTest::
replaceFileSlot()
{
  if (! currentEdit()) return;

  auto fileName = QFileDialog::getOpenFileName(this, "Open File", "", "Text Files (*.txt *.*)");

  if (fileName.length())
    currentEdit()->loadFile(fileName.toStdString());
}

void
CQViTest::
saveFileSlot()
{
  if (! currentEdit()) return;

  auto fileName = currentEdit()->getFilename();

  if (fileName.empty())
    saveFileAsSlot();
  else
    currentEdit()->saveFile(fileName);
}

void
CQViTest::
saveFileAsSlot()
{
  if (! currentEdit()) return;

  auto fileName = QFileDialog::getSaveFileName(this, "Save File", "", "Text Files (*.txt *.*)");

  if (fileName.length()) {
    currentEdit()->setFilename(fileName.toStdString());

    saveFileSlot();
  }
}

void
CQViTest::
closeFileSlot()
{
  int ind = fileTab_->currentIndex();

  if (ind >= 0)
    fileTab_->removeWidget(ind);
}

void
CQViTest::
undo()
{
  if (! currentEdit()) return;

  currentEdit()->app()->undo();
}

void
CQViTest::
redo()
{
  if (! currentEdit()) return;

  currentEdit()->app()->redo();
}

void
CQViTest::
setOptions()
{
  auto *dialog = new CQViOptionsDlg(this);

  dialog->init();
  dialog->setWindowTitle("Set Options");

  dialog->updateOptions(currentEdit());

  dialog->exec();

  if (dialog->isAccepted()) {
    auto *mgr = CQVi::Mgr::instance();

    mgr->setBg(dialog->getBg());
    mgr->setFg(dialog->getFg());

    if (currentEdit()) {
      currentEdit()->app()->setListMode  (dialog->getList  ());
      currentEdit()->app()->setNumberMode(dialog->getNumber());

      currentEdit()->app()->setCaseSensitive(! dialog->getIgnoreCase());

      auto oldSyntaxName = QString(currentEdit()->app()->syntax() ?
       currentEdit()->app()->syntax()->language() : "None");
      auto newSyntaxName = dialog->syntaxName();

      if (newSyntaxName != oldSyntaxName) {
        if      (newSyntaxName == "C"  ) currentEdit()->app()->setSyntax(new CSyntaxC);
        else if (newSyntaxName == "C++") currentEdit()->app()->setSyntax(new CSyntaxCPP);
        else                             currentEdit()->app()->setSyntax(nullptr);
      }
    }
  }

  delete dialog;
}

void
CQViTest::
setFont(const QFont &font)
{
  CQVi::Mgr::instance()->setFont(font);
}

void
CQViTest::
setSelectionColor(const QColor &)
{
}

CQVi::Widget *
CQViTest::
currentEdit() const
{
  auto *w = fileTab_->currentWidget();

  return qobject_cast<CQVi::Widget *>(w);
}

QSize
CQViTest::
sizeHint() const
{
  QFontMetrics fm(CQVi::Mgr::instance()->font());

  auto cw = fm.horizontalAdvance("X");
  auto ch = fm.height();

  return QSize(100*cw, 60*ch);
}
