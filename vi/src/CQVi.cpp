#include <CQVi.h>
#include <CQUtil.h>
#include <CKeyType.h>
#include <CSyntaxC.h>

#include <QLineEdit>
#include <QScrollBar>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <cmath>

namespace CQVi {

Mgr *
Mgr::
instance()
{
  static Mgr *inst;

  if (! inst)
    inst = new Mgr;

  return inst;
}

Mgr::
Mgr()
{
//setFont(QFont("Courier", 18));
  setFont(QFont("Inconsolata", 18));
}

Mgr::
~Mgr()
{
}

void
Mgr::
addWidget(Widget *w)
{
  widgets_.push_back(w);

  w->updateMgr();
}

void
Mgr::
removeWidget(Widget *w)
{
  widgets_.remove(w);
}

void
Mgr::
update()
{
  for (auto *w : widgets_)
    w->updateMgr();
}

//---

Widget::
Widget(QWidget *parent) :
 QWidget(parent)
{
  Mgr::instance()->addWidget(this);
}

Widget::
~Widget()
{
  Mgr::instance()->removeWidget(this);
}

void
Widget::
init()
{
  setObjectName("app");

  app_ = std::make_unique<App>(this);

  app_->init();

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  auto *grid = new QGridLayout;
  grid->setMargin(2); grid->setSpacing(2);

  canvas_  = new Canvas(this);
  hscroll_ = new QScrollBar(Qt::Horizontal);
  vscroll_ = new QScrollBar(Qt::Vertical  );

  connect(hscroll_, SIGNAL(valueChanged(int)), this, SLOT(hscrollSlot(int)));
  connect(vscroll_, SIGNAL(valueChanged(int)), this, SLOT(vscrollSlot(int)));

  grid->addWidget(canvas_ , 0, 0);
  grid->addWidget(hscroll_, 1, 0);
  grid->addWidget(vscroll_, 0, 1);

  setFocusProxy(canvas_);

  layout->addLayout(grid);

  if (cmdLine_) {
    layout->addWidget(cmdLine_);

    cmdLine_->setVisible(false);
  }

//status_ = new Status(this);
//layout->addWidget(status_);
}

void
Widget::
updateMgr()
{
  auto font = Mgr::instance()->font();

  QWidget::setFont(font);

  QFontMetrics fm(font);

  fontData_.char_rect   = fm.boundingRect("X");
  fontData_.char_width  = fm.horizontalAdvance("X");
  fontData_.char_ascent = fm.ascent();
  fontData_.char_height = fm.ascent() + fm.descent();

  update();
}

void
Widget::
hscrollSlot(int)
{
  update();
}

void
Widget::
vscrollSlot(int)
{
  update();
}

//---

const std::string &
Widget::
getFilename() const
{
  return app_->getFileName();
}

void
Widget::
setFilename(const std::string &filename)
{
  app_->setFileName(filename);
}

void
Widget::
loadFile(const std::string &filename)
{
  setWindowTitle(filename.c_str());

  app_->loadLines(filename);
}

void
Widget::
saveFile(const std::string &filename)
{
  app_->saveLines(filename);
}

void
Widget::
paintEvent(QPaintEvent *)
{
}

void
Widget::
draw(QPainter *painter)
{
  auto bg   = Mgr::instance()->bg();
  auto fg   = Mgr::instance()->fg();
  auto font = Mgr::instance()->font();

  int w = canvas_->width ();
  int h = canvas_->height();

  painter->setFont(font);

  painter->fillRect(rect(), QBrush(bg));

  painter->setPen(fg);

  xOffset_ = hscroll_->value();
  yOffset_ = vscroll_->value();

  // get cursor pos
  uint cx, cy;
  app_->getPos(&cx, &cy);

  char cc = ' '; // cursor char
  uint cp = 0;   // cursor pos

  uint iy = 0;
  int  y  = -yOffset_;

  maxLineLength_ = 0;

  //---

  static char numberBuffer[256];

  lineDigits_   = -1;
  lmargin_      = 0;
  numberFormat_ = "";

  auto getLineNumberStr = [&](int i) {
    if (lineDigits_ < 0) {
      int n = app_->getNumLines();

      lineDigits_ = (n > 0 ? int(std::log10(n)) + 1 : 1);

      sprintf(numberBuffer, "%% %dd ", lineDigits_);

      numberFormat_ = numberBuffer;

      lmargin_ = (lineDigits_ + 1)*fontData_.char_width;
    }

    sprintf(numberBuffer, numberFormat_.c_str(), i);

    return QString(numberBuffer);
  };

  auto visualMode = app_->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  app_->getSelectStart(&selRow1, &selCol1);
  app_->getSelectEnd  (&selRow2, &selCol2);

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);
  }

  auto isSelected = [&](int row, int col) {
    if (visualMode == App::VisualMode::NONE) return false;

    if (row < selRow1 || row > selRow2) return false;

    if      (visualMode == App::VisualMode::CHAR) {
      if (row == selRow1) { if (col < selCol1) return false; }
      if (row == selRow2) { if (col > selCol2) return false; }
    }
    else if (visualMode == App::VisualMode::BLOCK) {
      if (col < selCol1 || col > selCol2) return false;
    }
    return true;
  };

  auto drawLine = [&](CVi::Line *line) {
    int x = -xOffset_;

    if (app_->getNumberMode()) {
      auto numberStr = getLineNumberStr(iy);

      painter->setPen(numberFg());
      painter->drawText(x, y + fontData_.char_ascent, numberStr);

      x += numberStr.size()*fontData_.char_width;
    }

    uint ix1 = 0; // char pos
    uint ix2 = 0; // adjusted char pos (tabs)

    for (const auto &c : line->chars()) {
      auto isSel = isSelected(iy, ix1);

      if (isSel)
        painter->fillRect(QRect(x, y, fontData_.char_width, fontData_.char_height),
                          QBrush(selBg()));

      if (ix1 == cx && iy == cy) {
        cc = c;
        cp = ix2;
      }

      uint n = 1;

      if (! isspace(c)) {
        CVi::Line::Style style;

        auto fgc = fg;

        if (line->getCharStyle(ix1, style))
          fgc = app_->tokenColor(style.token);

        painter->setPen(fgc);
        painter->drawText(x, y + fontData_.char_ascent, QString(c));
      }
      else if (c == '\t') {
        if (app_->getListMode()) {
          painter->setPen(emptyFg());
          painter->drawText(x, y + fontData_.char_ascent, "^I");

          n = 2;
        }
        else
          n = 8 - (ix2 % 8);
      }

      x += n*fontData_.char_width;

      ix2 += n;

      ++ix1;
    }

    if (ix1 == cx && iy == cy) {
      cc = (app_->getListMode() ? '$' : ' ');
      cp = ix2;
    }

    if (app_->getListMode()) {
      painter->setPen(emptyFg());
      painter->drawText(x, y + fontData_.char_ascent, QString("$"));

      ++ix2;
    }

    maxLineLength_ = std::max(ix2, maxLineLength_);
  };

  //---

  // draw lines
  y1_ = y;

  for (auto *line : app_->lines()) {
    if (y > h)
      break;

    if (y + fontData_.char_height >= 0) {
      yLineMap_[y + fontData_.char_height] = iy;

      drawLine(line);
    }

    y += fontData_.char_height;

    ++iy;
  }

  y2_ = y1_ + app_->getNumLines()*fontData_.char_height;

  //---

  // draw cursor
  int xc = lmargin_ + cp*fontData_.char_width  - xOffset_;
  int yc = cy*fontData_.char_height - yOffset_;

  painter->fillRect(QRect(xc, yc, fontData_.char_width, fontData_.char_height),
                    QBrush(cursorBg()));

  painter->setPen(cursorFg());
  painter->drawText(xc, yc + fontData_.char_ascent, QString(cc));

  //---

  // draw empty lines
  painter->setPen(emptyFg());

  while (y < h) {
    painter->drawText(0, y + fontData_.char_ascent, "~");

    y += fontData_.char_height;
  }

  //---

  rows_ = h/fontData_.char_height;
  cols_ = (w - lmargin_)/fontData_.char_width;

  updateScrollbars();

  //---

  if (sizeChanged_) {
    sizeChanged_ = false;

    Q_EMIT sizeChanged();
  }
}

#if 0
void
Widget::
updateStatus()
{
  uint cx, cy;

  app_->getPos(&cx, &cy);

  auto text = QString("Row %1, Col %2").arg(cy + 1).arg(cx + 1);

  if (app_->getInsertMode())
    text += "  INS";

  if (app_->count() > 1)
    text += "  #" + QString::number(app_->count());

  status_->setText(text);
}
#endif

void
Widget::
resizeEvent(QResizeEvent *)
{
  sizeChanged_ = true;

  updateScrollbars();
}

int
Widget::
pageTop() const
{
  return yOffset_/fontData_.char_height;
}

int
Widget::
pageBottom() const
{
  int pos = pageTop() + pageLength();

  int nl = app_->getNumLines();

  if (pos >= nl)
    pos = nl - 1;

  return pos;
}

int
Widget::
pageLength() const
{
  return canvas_->height()/fontData_.char_height;
}

void
Widget::
scrollTo(uint x, uint y, bool force)
{
  int x1 = x*fontData_.char_width  - xOffset_;
  int y1 = y*fontData_.char_height - yOffset_;

  int w = canvas_->width ();
  int h = canvas_->height();

  if      (x1 < 0 || force)
    hscroll_->setValue(x*fontData_.char_width);
  else if (x1 >= w)
    hscroll_->setValue((x + 1)*fontData_.char_width - w);

  if      (y1 < 0 || force)
    vscroll_->setValue(y*fontData_.char_height);
  else if (y1 >= h)
    vscroll_->setValue((y + 1)*fontData_.char_height - h);
}

void
Widget::
updateScrollbars()
{
  int w = canvas_->width ();
  int h = canvas_->height();

  int aw = maxLineLength_*fontData_.char_width;
  int ah = app_->getNumLines()*fontData_.char_height;

  int hs = std::min(w, aw);
  int vs = std::min(h, ah);

  hscroll_->setMinimum(0);
  hscroll_->setMaximum(aw - hs);
  hscroll_->setPageStep(hs);
  hscroll_->setSingleStep(fontData_.char_width);

  vscroll_->setMinimum(0);
  vscroll_->setMaximum(ah - vs);
  vscroll_->setPageStep(vs);
  vscroll_->setSingleStep(fontData_.char_height);
}

void
Widget::
mousePressEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  int ix, iy;

  if (mouseToPos(press_pos_, ix, iy)) {
    app_->setPos(ix, iy);

    update();
  }
}

void
Widget::
mouseReleaseEvent(QMouseEvent *)
{
}

void
Widget::
mouseMoveEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  update();
}

void
Widget::
wheelEvent(QWheelEvent *e)
{
  auto delta = e->angleDelta().y();

  int value = vscroll_->value();

  if      (delta < 0)
    value = std::min(value + fontData_.char_height, vscroll_->maximum());
  else if (delta > 0)
    value = std::max(value - fontData_.char_height, vscroll_->minimum());

  vscroll_->setValue(value);
}

void
Widget::
keyPressEvent(QKeyEvent *e)
{
#if 0
  auto keyType = CQUtil::convertKey(e->key(), e->modifiers());

  if (uint(keyType) <= 0xFF)
    app_->processChar(keyType);
#else
  CVi::KeyData keyData;

  keyData.key  = e->key();
  keyData.text = e->text().toStdString();

  if (keyData.key >= Qt::Key_A && keyData.key <= Qt::Key_Z) {
    if (e->modifiers() & Qt::ShiftModifier)
      keyData.key = 'A' + (keyData.key - Qt::Key_A);
    else
      keyData.key = 'a' + (keyData.key - Qt::Key_A);
  }

  if      (keyData.key == Qt::Key_Escape   ) keyData.key = int(CVi::KeyData::KeyCode::ESCAPE);
  else if (keyData.key == Qt::Key_Tab      ) keyData.key = int(CVi::KeyData::KeyCode::TAB);
  else if (keyData.key == Qt::Key_Backtab  ) keyData.key = int(CVi::KeyData::KeyCode::BACKTAB);
  else if (keyData.key == Qt::Key_Backspace) keyData.key = int(CVi::KeyData::KeyCode::BACKSPACE);
  else if (keyData.key == Qt::Key_Return   ) keyData.key = int(CVi::KeyData::KeyCode::RETURN);
  else if (keyData.key == Qt::Key_Enter    ) keyData.key = int(CVi::KeyData::KeyCode::ENTER);
  else if (keyData.key == Qt::Key_Insert   ) keyData.key = int(CVi::KeyData::KeyCode::INSERT);
  else if (keyData.key == Qt::Key_Delete   ) keyData.key = int(CVi::KeyData::KeyCode::DELETE);
  else if (keyData.key == Qt::Key_Pause    ) keyData.key = int(CVi::KeyData::KeyCode::PAUSE);
  else if (keyData.key == Qt::Key_Print    ) keyData.key = int(CVi::KeyData::KeyCode::PRINT);
  else if (keyData.key == Qt::Key_SysReq   ) keyData.key = int(CVi::KeyData::KeyCode::SYS_REQ);
  else if (keyData.key == Qt::Key_Clear    ) keyData.key = int(CVi::KeyData::KeyCode::CLEAR);
  else if (keyData.key == Qt::Key_Home     ) keyData.key = int(CVi::KeyData::KeyCode::HOME);
  else if (keyData.key == Qt::Key_End      ) keyData.key = int(CVi::KeyData::KeyCode::END);
  else if (keyData.key == Qt::Key_Left     ) keyData.key = int(CVi::KeyData::KeyCode::LEFT);
  else if (keyData.key == Qt::Key_Up       ) keyData.key = int(CVi::KeyData::KeyCode::UP);
  else if (keyData.key == Qt::Key_Right    ) keyData.key = int(CVi::KeyData::KeyCode::RIGHT);
  else if (keyData.key == Qt::Key_Down     ) keyData.key = int(CVi::KeyData::KeyCode::DOWN);
  else if (keyData.key == Qt::Key_PageUp   ) keyData.key = int(CVi::KeyData::KeyCode::PAGE_UP);
  else if (keyData.key == Qt::Key_PageDown ) keyData.key = int(CVi::KeyData::KeyCode::PAGE_DOWN);

  else if (keyData.key == Qt::Key_Shift  ) keyData.key = int(CVi::KeyData::KeyCode::SHIFT);
  else if (keyData.key == Qt::Key_Control) keyData.key = int(CVi::KeyData::KeyCode::CONTROL);
  else if (keyData.key == Qt::Key_Meta   ) keyData.key = int(CVi::KeyData::KeyCode::META);
  else if (keyData.key == Qt::Key_Alt    ) keyData.key = int(CVi::KeyData::KeyCode::ALT);
//else if (keyData.key == Qt::Key_Super  ) keyData.key = int(CVi::KeyData::KeyCode::SUPER);
//else if (keyData.key == Qt::Key_Hyper  ) keyData.key = int(CVi::KeyData::KeyCode::HYPER);

  else if (keyData.key == Qt::Key_CapsLock) keyData.key = int(CVi::KeyData::KeyCode::CAPS_LOCK);
  else if (keyData.key == Qt::Key_NumLock ) keyData.key = int(CVi::KeyData::KeyCode::NUM_LOCK);

  else if (keyData.key == Qt::Key_F1 ) keyData.key = int(CVi::KeyData::KeyCode::F1);
  else if (keyData.key == Qt::Key_F2 ) keyData.key = int(CVi::KeyData::KeyCode::F2);
  else if (keyData.key == Qt::Key_F3 ) keyData.key = int(CVi::KeyData::KeyCode::F3);
  else if (keyData.key == Qt::Key_F4 ) keyData.key = int(CVi::KeyData::KeyCode::F4);
  else if (keyData.key == Qt::Key_F5 ) keyData.key = int(CVi::KeyData::KeyCode::F5);
  else if (keyData.key == Qt::Key_F6 ) keyData.key = int(CVi::KeyData::KeyCode::F6);
  else if (keyData.key == Qt::Key_F7 ) keyData.key = int(CVi::KeyData::KeyCode::F7);
  else if (keyData.key == Qt::Key_F8 ) keyData.key = int(CVi::KeyData::KeyCode::F8);
  else if (keyData.key == Qt::Key_F9 ) keyData.key = int(CVi::KeyData::KeyCode::F9);
  else if (keyData.key == Qt::Key_F10) keyData.key = int(CVi::KeyData::KeyCode::F10);
  else if (keyData.key == Qt::Key_F11) keyData.key = int(CVi::KeyData::KeyCode::F11);
  else if (keyData.key == Qt::Key_F12) keyData.key = int(CVi::KeyData::KeyCode::F11);

  keyData.is_shift   = (e->modifiers() & Qt::ShiftModifier  );
  keyData.is_control = (e->modifiers() & Qt::ControlModifier);
  keyData.is_alt     = (e->modifiers() & Qt::AltModifier    );
  keyData.is_meta    = (e->modifiers() & Qt::MetaModifier   );

  app_->processChar(keyData);
#endif

  update();
}

QSize
Widget::
sizeHint() const
{
  auto s1 = canvas_->sizeHint();
//auto s2 = status_->sizeHint();
  auto s2 = QSize(0, 0);

  return QSize(s1.width () +               vscroll_->width (),
               s1.height() + s2.height() + hscroll_->height() + 4);
}

QSize
Widget::
canvasSizeHint() const
{
  return QSize(100*fontData_.char_width, 60*fontData_.char_height);
}

bool
Widget::
mouseToPos(const QPoint &pos, int &ix, int &iy) const
{
  ix = 0;
  iy = -1;

  for (const auto &yLine : yLineMap_) {
    if (pos.y() < yLine.first) {
      iy = yLine.second;
      break;
    }
  }

  if (iy < 0) {
    iy = 0;
    return false;
  }

  bool found = false;

  auto *line = app_->getLine(iy);

  int x1 = lmargin_ - xOffset_;

  for (const auto &c : line->chars()) {
    uint n = 1;

    if (c == '\t') {
      if (app_->getListMode())
        n = 2;
      else
        n = 8 - (ix % 8);
    }

    int x2 = x1 + n*fontData_.char_width;

    if (pos.x() >= x1 && pos.x() < x2) {
      found = true;
      break;
    }

    ++ix;

    x1 = x2;
  }

  return found;
}

//------

App::
App(Widget *widget) :
 CVi::App(), widget_(widget)
{
  setObjectName("vi");

  setInterface(new Interface(this));
}

CVi::CmdLine *
App::
createCmdLine() const
{
  auto *cmdLine = new CmdLine(const_cast<Widget *>(widget_));

  widget_->setCmdLine(cmdLine);

  return cmdLine;
}

int
App::
getPageTop() const
{
  return widget_->pageTop();
}

int
App::
getPageBottom() const
{
  return widget_->pageBottom();
}

int
App::
getPageLength() const
{
  return widget_->pageLength();
}

void
App::
scrollTop()
{
  uint cx, cy;
  getPos(&cx, &cy);

  widget_->scrollTo(cx, cy, /*force*/true);
}

void
App::
scrollMiddle()
{
  uint cx, cy;
  getPos(&cx, &cy);

  widget_->scrollTo(cx, cy - getPageLength()/2, /*force*/true);
}

void
App::
scrollBottom()
{
  uint cx, cy;
  getPos(&cx, &cy);

  widget_->scrollTo(cx, cy - getPageLength(), /*force*/true);
}

void
App::
scrollCursor()
{
  uint cx, cy;
  getPos(&cx, &cy);

  widget_->scrollTo(cx, cy);
}

void
App::
stateChanged()
{
  update();

  Q_EMIT widget_->stateChanged();
}

void
App::
positionChanged()
{
  scrollCursor();

  update();

  Q_EMIT widget_->positionChanged();
}

void
App::
selectionChanged()
{
  update();

  Q_EMIT widget_->selectionChanged();
}

void
App::
update()
{
  widget_->update();
}

QColor
App::
tokenColor(CSyntaxToken token) const
{
  switch (token) {
    case CSyntaxToken::PREPRO : return widget_->preProFg();
    case CSyntaxToken::KEYWORD: return widget_->keywordFg();
    case CSyntaxToken::STRING : return widget_->stringFg();
    case CSyntaxToken::COMMENT: return widget_->commentFg();
    default                   : return Mgr::instance()->fg();
  }
}

void
App::
setNameValue(const std::string &name, const std::string &value)
{
  if      (name == "bgColor")
    Mgr::instance()->setBg(QColor(QString::fromStdString(value)));
  else if (name == "fgColor")
    Mgr::instance()->setFg(QColor(QString::fromStdString(value)));
  else if (name == "cursorBgColor")
    widget_->setCursorBg(QColor(QString::fromStdString(value)));
  else if (name == "cursorFgColor")
    widget_->setCursorFg(QColor(QString::fromStdString(value)));
  else if (name == "emptyFgColor")
    widget_->setEmptyFg(QColor(QString::fromStdString(value)));
  else if (name == "numberFgColor")
    widget_->setNumberFg(QColor(QString::fromStdString(value)));
  else if (name == "keywordFgColor")
    widget_->setKeywordFg(QColor(QString::fromStdString(value)));
  else if (name == "font")
    Mgr::instance()->setFont(QFont(QString::fromStdString(value)));

  CVi::App::setNameValue(name, value);
}

//------

Canvas::
Canvas(Widget *widget) :
 QWidget(widget), widget_(widget)
{
  setObjectName("canvas");

  setFocusPolicy(Qt::StrongFocus);
}

void
Canvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  widget_->draw(&painter);

  //widget_->updateStatus();
}

QSize
Canvas::
sizeHint() const
{
  return widget_->canvasSizeHint();
}

//------

CmdLine::
CmdLine(Widget *widget) :
 CVi::CmdLine(widget->app()), widget_(widget)
{
  setObjectName("cmdLine");

  auto *layout = new QHBoxLayout(this);

  edit_ = new QLineEdit;

  layout->addWidget(edit_);
}

void
CmdLine::
setVisible(bool visible)
{
  QFrame::setVisible(visible);
}

void
CmdLine::
updateLine()
{
  edit_->setText(QString::fromStdString(getLine()));
}

//------

#if 0
Status::
Status(Widget *widget) :
 QLabel(widget), widget_(widget)
{
  setObjectName("status");

  QFontMetrics fm(font());

  setFixedHeight(fm.height() + 4);
}
#endif

//------

Interface::
Interface(App *app) :
 app_(app)
{
}

int
Interface::
getPageTop() const
{
  return app_->getPageTop();
}

int
Interface::
getPageBottom() const
{
  return app_->getPageBottom();
}

int
Interface::
getPageLength() const
{
  return app_->getPageLength();
}

void
Interface::
scrollTop()
{
  app_->scrollTop();
}

void
Interface::
scrollMiddle()
{
  app_->scrollMiddle();
}

void
Interface::
scrollBottom()
{
  app_->scrollBottom();
}

void
Interface::
stateChanged()
{
  app_->stateChanged();
}

void
Interface::
positionChanged()
{
  app_->positionChanged();
}

void
Interface::
selectionChanged()
{
  app_->selectionChanged();
}

}
