#include <CQVi.h>
#include <CQUtil.h>
#include <CKeyType.h>

#include <QLineEdit>
#include <QScrollBar>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>

namespace CQVi {

Widget::
Widget(QWidget *parent) :
 QWidget(parent)
{
}

void
Widget::
init()
{
  setObjectName("app");

//setFont(QFont("Courier", 18));
  setFont(QFont("Inconsolata", 18));

  vi_ = std::make_unique<App>(this);

  vi_->init();

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

  status_ = new Status(this);

  layout->addWidget(status_);
}

void
Widget::
setFont(const QFont &font)
{
  fontData_.font = font;

  QWidget::setFont(fontData_.font);

  QFontMetrics fm(fontData_.font);

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

void
Widget::
loadFile(const std::string &filename)
{
  setWindowTitle(filename.c_str());

  vi_->loadLines(filename);
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
  int h = canvas_->height();

  painter->setFont(font());

  painter->fillRect(rect(), QBrush(bg()));

  painter->setPen(fg());

  x_offset_ = hscroll_->value();
  y_offset_ = vscroll_->value();

  // get cursor pos
  uint cx, cy;
  vi_->getPos(&cx, &cy);

  char cc = ' '; // cursor char
  uint cp = 0;   // cursor pos

  uint iy = 0;
  int  y  = -y_offset_;

  maxLineLength_ = 0;

  //---

  auto drawLine = [&](CVi::Line *line) {
    uint ix1 = 0;
    uint ix2 = 0;

    int x = -x_offset_;

    for (const auto &c : line->chars()) {
      if (ix1 == cx && iy == cy) {
        cc = c;
        cp = ix2;
      }

      uint n = 1;

      if (! isspace(c))
        painter->drawText(x, y + fontData_.char_ascent, QString(c));
      else if (c == '\t') {
        if (vi_->getListMode()) {
          painter->setPen(emptyFg());
          painter->drawText(x, y + fontData_.char_ascent, "^I");
          painter->setPen(fg());

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
      cc = (vi_->getListMode() ? '$' : ' ');
      cp = ix2;
    }

    if (vi_->getListMode()) {
      painter->setPen(emptyFg());
      painter->drawText(x, y + fontData_.char_ascent, QString("$"));
      painter->setPen(fg());

      ++ix2;
    }

    maxLineLength_ = std::max(ix2, maxLineLength_);
  };

  //---

  // draw lines
  y1_ = y;

  for (auto *line : vi_->lines()) {
    if (y > h)
      break;

    if (y + fontData_.char_height >= 0) {
      yLineMap_[y + fontData_.char_height] = iy;

      drawLine(line);
    }

    y += fontData_.char_height;

    ++iy;
  }

  y2_ = y1_ + vi_->getNumLines()*fontData_.char_height;

  //---

  // draw cursor
  int xc = cp*fontData_.char_width  - x_offset_;
  int yc = cy*fontData_.char_height - y_offset_;

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

  updateScrollbars();
}

void
Widget::
updateStatus()
{
  uint cx, cy;

  vi_->getPos(&cx, &cy);

  auto text = QString("Row %1, Col %2").arg(cy + 1).arg(cx + 1);

  if (vi_->getInsertMode())
    text += "  INS";

  if (vi_->count() > 1)
    text += "  #" + QString::number(vi_->count());

  status_->setText(text);
}

void
Widget::
resizeEvent(QResizeEvent *)
{
  updateScrollbars();
}

int
Widget::
pageTop() const
{
  return y_offset_/fontData_.char_height;
}

int
Widget::
pageBottom() const
{
  int pos = pageTop() + pageLength();

  int nl = vi_->getNumLines();

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
  int x1 = x*fontData_.char_width  - x_offset_;
  int y1 = y*fontData_.char_height - y_offset_;

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
  int ah = vi_->getNumLines()*fontData_.char_height;

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
    vi_->setPos(ix, iy);

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
    vi_->processChar(keyType);
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

  vi_->processChar(keyData);
#endif

  update();
}

QSize
Widget::
sizeHint() const
{
  auto s1 = canvas_->sizeHint();
  auto s2 = status_->sizeHint();

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

  auto *line = vi_->getLine(iy);

  int x1 = -x_offset_;

  for (const auto &c : line->chars()) {
    uint n = 1;

    if (c == '\t') {
      if (vi_->getListMode())
        n = 2;
      else
        n = 8 - (ix % 8);
    }

    int x2 = x1 + n*fontData_.char_width;

    if (pos.x() >= x1 && pos.x() < x2)
      break;

    ++ix;

    x1 = x2;
  }

  return true;
}

//------

App::
App(Widget *app) :
 CVi::App(), app_(app)
{
  setObjectName("vi");

  setInterface(new Interface(this));
}

CVi::CmdLine *
App::
createCmdLine() const
{
  auto *cmdLine = new CmdLine(const_cast<Widget *>(app_));

  app_->setCmdLine(cmdLine);

  return cmdLine;
}

int
App::
getPageTop() const
{
  return app_->pageTop();
}

int
App::
getPageBottom() const
{
  return app_->pageBottom();
}

int
App::
getPageLength() const
{
  return app_->pageLength();
}

void
App::
scrollTop()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy, /*force*/true);
}

void
App::
scrollMiddle()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy - getPageLength()/2, /*force*/true);
}

void
App::
scrollBottom()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy - getPageLength(), /*force*/true);
}

void
App::
scrollCursor()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy);
}

void
App::
update()
{
  app_->update();
}

//------

Canvas::
Canvas(Widget *app) :
 QWidget(app), app_(app)
{
  setObjectName("canvas");

  setFocusPolicy(Qt::StrongFocus);
}

void
Canvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  app_->draw(&painter);

  app_->updateStatus();
}

QSize
Canvas::
sizeHint() const
{
  return app_->canvasSizeHint();
}

//------

CmdLine::
CmdLine(Widget *app) :
 CVi::CmdLine(app->vi()), app_(app)
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

Status::
Status(Widget *app) :
 QLabel(app), app_(app)
{
  setObjectName("status");

  QFontMetrics fm(font());

  setFixedHeight(fm.height() + 4);
}

//------

Interface::
Interface(App *vi) :
 vi_(vi)
{
}

int
Interface::
getPageTop() const
{
  return vi_->getPageTop();
}

int
Interface::
getPageBottom() const
{
  return vi_->getPageBottom();
}

int
Interface::
getPageLength() const
{
  return vi_->getPageLength();
}

void
Interface::
scrollTop()
{
  vi_->scrollTop();
}

void
Interface::
scrollMiddle()
{
  vi_->scrollMiddle();
}

void
Interface::
scrollBottom()
{
  vi_->scrollBottom();
}

void
Interface::
stateChanged()
{
  vi_->update();
}

void
Interface::
positionChanged()
{
  vi_->scrollCursor();

  vi_->update();
}

}
