#include <CQEdit.h>
#include <CEdit.h>
#include <CQUtil.h>
#include <CQEditCanvas.h>
#include <CQHistoryLineEdit.h>
#include <CQEditMgr.h>

#include <QApplication>
#include <QVBoxLayout>
#include <QHeaderView>
#include <QPainter>

#include <CEvent.h>

void
CQEdit::
init()
{
  CEditMgrInst->setFactory(new CQEditFactory);
}

CQEdit::
CQEdit(QWidget *parent) :
 QWidget(parent), cmd_(NULL), marks_(NULL), registers_(NULL)
{
  file_ = new CQEditFile(this);

  file_->init();

  //--------

  QVBoxLayout *vlayout = new QVBoxLayout(this);
  vlayout->setMargin(0); vlayout->setSpacing(0);

  area_ = new CQEditArea(this);

  connect(area_, SIGNAL(sizeChanged()), this, SIGNAL(sizeChanged()));

  vlayout->addWidget(area_);

  cmd_ = new CQHistoryLineEdit;

  connect(cmd_, SIGNAL(exec(const QString &)), this, SLOT(runCmd(const QString &)));

  vlayout->addWidget(cmd_);

  //--------

  file_->loadConfig("CQEdit");
}

CQEdit::
~CQEdit()
{
}

QWidget *
CQEdit::
getCanvasWidget() const
{
  return area_->getCanvas();
}

void
CQEdit::
draw(QPainter *painter)
{
  CQEditRenderer renderer(area_->getCanvas(), painter);

  CVEditMgrInst->setRenderer(&renderer);

  file_->draw(area_->getCanvas()->width(), area_->getCanvas()->height());

  CVEditMgrInst->setRenderer(NULL);
}

void
CQEdit::
update()
{
  area_->getCanvas()->update();

  emit stateChanged();
}

void
CQEdit::
keyPress(const CKeyEvent &event)
{
  file_->keyPress(event);
}

void
CQEdit::
keyRelease(const CKeyEvent &event)
{
  file_->keyRelease(event);
}

void
CQEdit::
mousePress(const CMouseEvent &event)
{
  file_->mousePress(event);
}

void
CQEdit::
mouseMotion(const CMouseEvent &event)
{
  file_->mouseMotion(event);
}

void
CQEdit::
mouseRelease(const CMouseEvent &event)
{
  file_->mouseRelease(event);
}

void
CQEdit::
runCmd(const QString &cmd)
{
  if (runEdCmd(cmd.toStdString())) {
    update();

    file_->getEdit()->getArea()->getCanvas()->setFocus();
  }
}

bool
CQEdit::
runEdCmd(const std::string &cmd)
{
  bool is_quit;

  bool rc = file_->runEdCmd(cmd, is_quit);

  if (is_quit)
    quit();

  return rc;
}

void
CQEdit::
selectionNotify(const CIBBox2D &bbox)
{
  file_->selectBBox(bbox, /*clear=*/true);

  update();
}

void
CQEdit::
setFileName(const std::string &)
{
  emit fileNameChanged();
}

void
CQEdit::
sendOutputMsg(const std::string &msg)
{
  emit outputMsg(QString(msg.c_str()));
}

void
CQEdit::
sendErrorMsg(const std::string &msg)
{
  emit errorMsg(QString(msg.c_str()));
}

void
CQEdit::
setCmdText(const std::string &str)
{
  if (cmd_) {
    cmd_->setText(str.c_str());

    cmd_->setFocus();
  }
}

void
CQEdit::
sendStateChanged()
{
  emit stateChanged();
}

void
CQEdit::
displayMarks()
{
  if (! marks_)
    marks_ = new CQEditMarks(this, area_->getCanvas());

  marks_->populate(file_);

  marks_->show();
}

void
CQEdit::
gotoMark(const std::string &str)
{
  uint line_num, char_num;

  if (file_->getMarkPos(str, &line_num, &char_num))
    file_->cursorTo(line_num, 0);

  file_->update();
}

void
CQEdit::
displayRegisters()
{
  if (! registers_)
    registers_ = new CQEditRegisters(this, area_->getCanvas());

  registers_->populate(file_);

  registers_->show();
}

void
CQEdit::
pasteBuffer(const std::string &)
{
}

void
CQEdit::
quit()
{
  file_->saveConfig("CQEdit");

  emit quitCommand();
}

void
CQEdit::
setFocus()
{
  area_->getCanvas()->setFocus();
}

//---------

CQEditMarks::
CQEditMarks(CQEdit *edit, QWidget *parent) :
 CQWinTree(parent), edit_(edit)
{
  connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
          this, SLOT(itemClickedSlot(QTreeWidgetItem *)));
}

void
CQEditMarks::
populate(CQEditFile *file)
{
  setUniformRowHeights(true);

  setColumnCount(3);

  setHeaderLabels(QStringList() << "Name" << "Row" << "Column");

  clear();

  uint numMarks = file->getNumMarks();

  CEditFile::MarkList::const_iterator p1, p2;

  for (p1 = file->getMarks().begin(),
         p2 = file->getMarks().end(); p1 != p2; ++p1) {
    const std::string &str = (*p1).first;
    const CIPoint2D   &pos = (*p1).second;

    QStringList strs;

    strs << QString(str.c_str());
    strs << QString(CStrUtil::toString(pos.y).c_str());
    strs << QString(CStrUtil::toString(pos.x).c_str());

    addTopLevelItem(new QTreeWidgetItem(strs));
  }

  int w = 0;

  for (int i = 0; i < 3; ++i)
    w += columnWidth(i);

  w += 8;

  int h = header()->height();

  for (int i = 0; i < 5 && i < (int) numMarks; ++i)
    h += 20;

  h += 8;

  setSize(w, h);

  CQWinTree::show();
}

void
CQEditMarks::
itemClickedSlot(QTreeWidgetItem *item)
{
  std::string str = item->text(0).toStdString();

  edit_->gotoMark(str);
}

//---------

CQEditRegisters::
CQEditRegisters(CQEdit *edit, QWidget *parent) :
 CQWinTree(parent), edit_(edit)
{
  connect(this, SIGNAL(itemClicked(QTreeWidgetItem *, int)),
          this, SLOT(itemClickedSlot(QTreeWidgetItem *)));
}

void
CQEditRegisters::
populate(CQEditFile *file)
{
  setUniformRowHeights(true);

  setColumnCount(2);

  setHeaderLabels(QStringList() << "Name" << "Text");

  clear();

  uint numBuffers = file->getNumBuffers();

  CEditFile::BufferMap::const_iterator p1, p2;

  for (p1 = file->getBufferMap().begin(),
         p2 = file->getBufferMap().end(); p1 != p2; ++p1) {
    char               c      = (*p1).first;
    const CEditBuffer &buffer = (*p1).second;

    std::string str1(&c, 1);

    QStringList strs;

    QString str2;

    uint numLines = buffer.getNumLines();

    for (uint i = 0; i < numLines; ++i) {
      if (i > 0) str2 += "\n";

      str2 += QString(buffer.getLine(i).getLine().c_str());
    }

    strs << QString(str1.c_str()) << str2;

    addTopLevelItem(new QTreeWidgetItem(strs));
  }

  int w = 0;

  for (int i = 0; i < 2; ++i)
    w += columnWidth(i);

  w += 8;

  int h = header()->height();

  for (int i = 0; i < 5 && i < (int) numBuffers; ++i)
    h += 20;

  h += 8;

  setSize(w, h);

  CQWinTree::show();
}

void
CQEditRegisters::
itemClickedSlot(QTreeWidgetItem *item)
{
  std::string str = item->text(0).toStdString();

  edit_->pasteBuffer(str);
}

//------

CQEditRenderer::
CQEditRenderer(QWidget *w, QPainter *painter) :
 w_(w), painter_(painter), ascent_(0), descent_(0)
{
}

void
CQEditRenderer::
fill(const CRGBA &bg)
{
  painter_->fillRect(w_->rect(), QBrush(CQUtil::rgbaToColor(bg)));
}

void
CQEditRenderer::
setFont(CFontPtr font)
{
  QFont qfont = CQUtil::toQFont(font);

  painter_->setFont(qfont);

  QFontMetrics fm(qfont);

  ascent_  = fm.ascent ();
  descent_ = fm.descent();
}

void
CQEditRenderer::
setForeground(const CRGBA &fg)
{
  painter_->setPen(CQUtil::rgbaToColor(fg));
}

void
CQEditRenderer::
fillRectangle(const CIBBox2D &rect, const CRGBA &rgba)
{
  painter_->fillRect(CQUtil::toQRect(rect), QBrush(QColor(CQUtil::rgbaToColor(rgba))));
}

void
CQEditRenderer::
drawChar(const CIPoint2D &p, char c)
{
  std::string str(&c, 1);

  drawString(p, str);
}

void
CQEditRenderer::
drawString(const CIPoint2D &p, const std::string &str)
{
  painter_->drawText(p.x, p.y + ascent_, str.c_str());
}
