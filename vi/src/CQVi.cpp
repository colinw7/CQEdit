#include <CQVi.h>
#include <CQUtil.h>
#include <CKeyType.h>

#include <QLineEdit>
#include <QScrollBar>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>

CQViApp::
CQViApp(QWidget *parent) :
 QWidget(parent)
{
}

void
CQViApp::
init()
{
  setObjectName("app");

//initFont(QFont("Courier", 18));
  initFont(QFont("Inconsolata", 18));

  vi_ = std::make_unique<CQVi>(this);

  vi_->init();

  auto *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  auto *grid = new QGridLayout;
  grid->setMargin(2); grid->setSpacing(2);

  canvas_  = new CQViCanvas(this);
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

  status_ = new CQViStatus(this);

  layout->addWidget(status_);
}

void
CQViApp::
initFont(const QFont &font)
{
  setFont(font);

  QFontMetrics fm(this->font());

  char_rect_   = fm.boundingRect("X");
  char_width_  = fm.horizontalAdvance("X");
  char_ascent_ = fm.ascent();
  char_height_ = fm.ascent() + fm.descent();
}

void
CQViApp::
hscrollSlot(int)
{
  update();
}

void
CQViApp::
vscrollSlot(int)
{
  update();
}

void
CQViApp::
loadFile(const std::string &filename)
{
  setWindowTitle(filename.c_str());

  vi_->loadLines(filename);
}

void
CQViApp::
paintEvent(QPaintEvent *)
{
}

void
CQViApp::
draw(QPainter *painter)
{
  int h = canvas_->height();

  painter->setFont(font());

  painter->fillRect(rect(), QBrush(QColor(0, 0, 0)));

  painter->setPen(QColor(255, 255, 255));

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

  auto drawLine = [&](CViLine *line) {
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
        painter->drawText(x, y + char_ascent_, QString(c));
      else if (c == '\t')
        n = 8 - (ix2 % 8);

      x += n*char_width_;

      ix2 += n;

      ++ix1;
    }

    maxLineLength_ = std::max(ix2, maxLineLength_);
  };

  //---

  // draw lines
  y1_ = y;

  for (auto *line : vi_->lines()) {
    if (y > h)
      break;

    if (y + char_height_ >= 0) {
      yLineMap_[y + char_height_] = iy;

      drawLine(line);
    }

    y += char_height_;

    ++iy;
  }

  y2_ = y1_ + vi_->getNumLines()*char_height_;

  //---

  // draw cursor
  int xc = cp*char_width_  - x_offset_;
  int yc = cy*char_height_ - y_offset_;

  painter->fillRect(QRect(xc, yc, char_width_, char_height_), QBrush(QColor(255, 255, 0)));

  painter->setPen(QColor(0, 0, 0));

  painter->drawText(xc, yc + char_ascent_, QString(cc));

  //---

  updateScrollbars();
}

void
CQViApp::
updateStatus()
{
  uint cx, cy;

  vi_->getPos(&cx, &cy);

  QString text = QString("Row %1, Col %2").arg(cy + 1).arg(cx + 1);

  status_->setText(text);
}

void
CQViApp::
resizeEvent(QResizeEvent *)
{
  updateScrollbars();
}

int
CQViApp::
pageTop() const
{
  return y_offset_/char_height_;
}

int
CQViApp::
pageBottom() const
{
  return pageTop() + pageLength();
}

int
CQViApp::
pageLength() const
{
  return canvas_->height()/char_height_;
}

void
CQViApp::
scrollTo(uint x, uint y, bool force)
{
  int x1 = x*char_width_  - x_offset_;
  int y1 = y*char_height_ - y_offset_;

  int w = canvas_->width ();
  int h = canvas_->height();

  if      (x1 < 0 || force)
    hscroll_->setValue(x*char_width_);
  else if (x1 >= w)
    hscroll_->setValue((x + 1)*char_width_ - w);

  if      (y1 < 0 || force)
    vscroll_->setValue(y*char_height_);
  else if (y1 >= h)
    vscroll_->setValue((y + 1)*char_height_ - h);
}

void
CQViApp::
updateScrollbars()
{
  int w = width ();
  int h = height();

  int aw = maxLineLength_*char_width_;
  int ah = vi_->getNumLines()*char_height_;

  int hs = std::min(w, aw);
  int vs = std::min(h, ah);

  hscroll_->setMinimum(0);
  hscroll_->setMaximum(aw - hs);
  hscroll_->setPageStep(hs);
  hscroll_->setSingleStep(char_width_);

  vscroll_->setMinimum(0);
  vscroll_->setMaximum(ah - vs);
  vscroll_->setPageStep(vs);
  vscroll_->setSingleStep(char_height_);
}

void
CQViApp::
mousePressEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  for (const auto &yLine : yLineMap_) {
    if (press_pos_.y() < yLine.first) {
      int iy = yLine.second;

      auto *line = vi_->getLine(iy);

      auto ix = (x_offset_ + press_pos_.x())/char_width_;

      ix = std::min(ix, int(line->getLength()));

      vi_->setPos(ix, iy);

      update();

      break;
    }
  }
}

void
CQViApp::
mouseReleaseEvent(QMouseEvent *)
{
}

void
CQViApp::
mouseMoveEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  update();
}

void
CQViApp::
wheelEvent(QWheelEvent *)
{
  update();
}

void
CQViApp::
keyPressEvent(QKeyEvent *e)
{
#if 0
  auto keyType = CQUtil::convertKey(e->key(), e->modifiers());

  if (uint(keyType) <= 0xFF)
    vi_->processChar(keyType);
#else
  CViKeyData keyData;

  keyData.key  = e->key();
  keyData.text = e->text().toStdString();

  if (keyData.key >= Qt::Key_A && keyData.key <= Qt::Key_Z) {
    if (e->modifiers() & Qt::ShiftModifier)
      keyData.key = 'A' + (keyData.key - Qt::Key_A);
    else
      keyData.key = 'a' + (keyData.key - Qt::Key_A);
  }

  if      (keyData.key == Qt::Key_Escape   ) keyData.key = int(CViKeyData::KeyCode::ESCAPE);
  else if (keyData.key == Qt::Key_Tab      ) keyData.key = int(CViKeyData::KeyCode::TAB);
  else if (keyData.key == Qt::Key_Backtab  ) keyData.key = int(CViKeyData::KeyCode::BACKTAB);
  else if (keyData.key == Qt::Key_Backspace) keyData.key = int(CViKeyData::KeyCode::BACKSPACE);
  else if (keyData.key == Qt::Key_Return   ) keyData.key = int(CViKeyData::KeyCode::RETURN);
  else if (keyData.key == Qt::Key_Enter    ) keyData.key = int(CViKeyData::KeyCode::ENTER);
  else if (keyData.key == Qt::Key_Insert   ) keyData.key = int(CViKeyData::KeyCode::INSERT);
  else if (keyData.key == Qt::Key_Delete   ) keyData.key = int(CViKeyData::KeyCode::DELETE);
  else if (keyData.key == Qt::Key_Pause    ) keyData.key = int(CViKeyData::KeyCode::PAUSE);
  else if (keyData.key == Qt::Key_Print    ) keyData.key = int(CViKeyData::KeyCode::PRINT);
  else if (keyData.key == Qt::Key_SysReq   ) keyData.key = int(CViKeyData::KeyCode::SYS_REQ);
  else if (keyData.key == Qt::Key_Clear    ) keyData.key = int(CViKeyData::KeyCode::CLEAR);
  else if (keyData.key == Qt::Key_Home     ) keyData.key = int(CViKeyData::KeyCode::HOME);
  else if (keyData.key == Qt::Key_End      ) keyData.key = int(CViKeyData::KeyCode::END);
  else if (keyData.key == Qt::Key_Left     ) keyData.key = int(CViKeyData::KeyCode::LEFT);
  else if (keyData.key == Qt::Key_Up       ) keyData.key = int(CViKeyData::KeyCode::UP);
  else if (keyData.key == Qt::Key_Right    ) keyData.key = int(CViKeyData::KeyCode::RIGHT);
  else if (keyData.key == Qt::Key_Down     ) keyData.key = int(CViKeyData::KeyCode::DOWN);
  else if (keyData.key == Qt::Key_PageUp   ) keyData.key = int(CViKeyData::KeyCode::PAGE_UP);
  else if (keyData.key == Qt::Key_PageDown ) keyData.key = int(CViKeyData::KeyCode::PAGE_DOWN);

  else if (keyData.key == Qt::Key_Shift  ) keyData.key = int(CViKeyData::KeyCode::SHIFT);
  else if (keyData.key == Qt::Key_Control) keyData.key = int(CViKeyData::KeyCode::CONTROL);
  else if (keyData.key == Qt::Key_Meta   ) keyData.key = int(CViKeyData::KeyCode::META);
  else if (keyData.key == Qt::Key_Alt    ) keyData.key = int(CViKeyData::KeyCode::ALT);
//else if (keyData.key == Qt::Key_Super  ) keyData.key = int(CViKeyData::KeyCode::SUPER);
//else if (keyData.key == Qt::Key_Hyper  ) keyData.key = int(CViKeyData::KeyCode::HYPER);

  else if (keyData.key == Qt::Key_CapsLock) keyData.key = int(CViKeyData::KeyCode::CAPS_LOCK);
  else if (keyData.key == Qt::Key_NumLock ) keyData.key = int(CViKeyData::KeyCode::NUM_LOCK);

  else if (keyData.key == Qt::Key_F1 ) keyData.key = int(CViKeyData::KeyCode::F1);
  else if (keyData.key == Qt::Key_F2 ) keyData.key = int(CViKeyData::KeyCode::F2);
  else if (keyData.key == Qt::Key_F3 ) keyData.key = int(CViKeyData::KeyCode::F3);
  else if (keyData.key == Qt::Key_F4 ) keyData.key = int(CViKeyData::KeyCode::F4);
  else if (keyData.key == Qt::Key_F5 ) keyData.key = int(CViKeyData::KeyCode::F5);
  else if (keyData.key == Qt::Key_F6 ) keyData.key = int(CViKeyData::KeyCode::F6);
  else if (keyData.key == Qt::Key_F7 ) keyData.key = int(CViKeyData::KeyCode::F7);
  else if (keyData.key == Qt::Key_F8 ) keyData.key = int(CViKeyData::KeyCode::F8);
  else if (keyData.key == Qt::Key_F9 ) keyData.key = int(CViKeyData::KeyCode::F9);
  else if (keyData.key == Qt::Key_F10) keyData.key = int(CViKeyData::KeyCode::F10);
  else if (keyData.key == Qt::Key_F11) keyData.key = int(CViKeyData::KeyCode::F11);
  else if (keyData.key == Qt::Key_F12) keyData.key = int(CViKeyData::KeyCode::F11);

  keyData.is_shift   = (e->modifiers() & Qt::ShiftModifier  );
  keyData.is_control = (e->modifiers() & Qt::ControlModifier);
  keyData.is_alt     = (e->modifiers() & Qt::AltModifier    );
  keyData.is_meta    = (e->modifiers() & Qt::MetaModifier   );

  vi_->processChar(keyData);
#endif

  update();
}

QSize
CQViApp::
sizeHint() const
{
  auto s1 = canvas_->sizeHint();
  auto s2 = status_->sizeHint();

  return QSize(s1.width() + vscroll_->width(), s1.height() + s2.height() + hscroll_->height() + 4);
}

QSize
CQViApp::
canvasSizeHint() const
{
  return QSize(100*char_width_, 60*char_height_);
}

//------

CQVi::
CQVi(CQViApp *app) :
 CVi(), app_(app)
{
  setObjectName("vi");

  setInterface(new CQViInterface(this));
}

CViCmdLine *
CQVi::
createCmdLine() const
{
  auto *cmdLine = new CQViCmdLine(const_cast<CQViApp *>(app_));

  app_->setCmdLine(cmdLine);

  return cmdLine;
}

int
CQVi::
getPageTop() const
{
  return app_->pageTop();
}

int
CQVi::
getPageBottom() const
{
  return app_->pageBottom();
}

int
CQVi::
getPageLength() const
{
  return app_->pageLength();
}

void
CQVi::
scrollTop()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy, /*force*/true);
}

void
CQVi::
scrollMiddle()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy - getPageLength()/2, /*force*/true);
}

void
CQVi::
scrollBottom()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy - getPageLength(), /*force*/true);
}

void
CQVi::
scrollCursor()
{
  uint cx, cy;
  getPos(&cx, &cy);

  app_->scrollTo(cx, cy);
}

//------

CQViCanvas::
CQViCanvas(CQViApp *app) :
 QWidget(app), app_(app)
{
  setObjectName("canvas");

  setFocusPolicy(Qt::StrongFocus);
}

void
CQViCanvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  app_->draw(&painter);

  app_->updateStatus();
}

QSize
CQViCanvas::
sizeHint() const
{
  return app_->canvasSizeHint();
}

//------

CQViCmdLine::
CQViCmdLine(CQViApp *app) :
 CViCmdLine(app->vi()), app_(app)
{
  setObjectName("cmdLine");

  auto *layout = new QHBoxLayout(this);

  edit_ = new QLineEdit;

  layout->addWidget(edit_);
}

void
CQViCmdLine::
setVisible(bool visible)
{
  QFrame::setVisible(visible);
}

void
CQViCmdLine::
setLine(const std::string &line)
{
  CViCmdLine::setLine(line);

  edit_->setText(QString::fromStdString(getLine()));
}

void
CQViCmdLine::
keyPress(char c)
{
  CViCmdLine::keyPress(c);

  edit_->setText(QString::fromStdString(getLine()));
}

//------

CQViStatus::
CQViStatus(CQViApp *app) :
 QLabel(app), app_(app)
{
  setObjectName("status");

  QFontMetrics fm(font());

  setFixedHeight(fm.height() + 4);
}

//------

CQViInterface::
CQViInterface(CQVi *vi) :
 vi_(vi)
{
}

int
CQViInterface::
getPageTop() const
{
  return vi_->getPageTop();
}

int
CQViInterface::
getPageBottom() const
{
  return vi_->getPageBottom();
}

int
CQViInterface::
getPageLength() const
{
  return vi_->getPageLength();
}

void
CQViInterface::
scrollTop()
{
  vi_->scrollTop();
}

void
CQViInterface::
scrollMiddle()
{
  vi_->scrollMiddle();
}

void
CQViInterface::
scrollBottom()
{
  vi_->scrollBottom();
}

void
CQViInterface::
positionChanged()
{
  vi_->scrollCursor();
}
