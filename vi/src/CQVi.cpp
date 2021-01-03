#include <CQVi.h>
#include <CQUtil.h>
#include <CKeyType.h>

#include <QGridLayout>
#include <QScrollBar>
#include <QMouseEvent>
#include <QPainter>

CQVi::
CQVi(QWidget *parent) :
 QWidget(parent)
{
//initFont(QFont("Courier", 18));
  initFont(QFont("Inconsolata", 18));

  vi_ = std::make_unique<CVi>();

  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin(0); layout->setSpacing(0);

  QGridLayout *grid = new QGridLayout;
  grid->setMargin(2); grid->setSpacing(2);

  canvas_  = new CQViCanvas(this);
  hscroll_ = new QScrollBar(Qt::Horizontal);
  vscroll_ = new QScrollBar(Qt::Vertical  );

  QObject::connect(hscroll_, SIGNAL(valueChanged(int)), this, SLOT(hscrollSlot(int)));
  QObject::connect(vscroll_, SIGNAL(valueChanged(int)), this, SLOT(vscrollSlot(int)));

  grid->addWidget(canvas_ , 0, 0);
  grid->addWidget(hscroll_, 1, 0);
  grid->addWidget(vscroll_, 0, 1);

  setFocusProxy(canvas_);

  layout->addLayout(grid);

  status_ = new CQViStatus(this);

  layout->addWidget(status_);
}

void
CQVi::
initFont(const QFont &font)
{
  setFont(font);

  QFontMetrics fm(this->font());

  char_rect_   = fm.boundingRect("X");
  char_width_  = fm.width("X");
  char_ascent_ = fm.ascent();
  char_height_ = fm.ascent() + fm.descent();
}

void
CQVi::
hscrollSlot(int)
{
  update();
}

void
CQVi::
vscrollSlot(int)
{
  update();
}

void
CQVi::
loadFile(const std::string &filename)
{
  setWindowTitle(filename.c_str());

  vi_->loadFile(filename);
}

void
CQVi::
paintEvent(QPaintEvent *)
{
}

void
CQVi::
draw(QPainter *painter)
{
  int h = canvas_->height();

  painter->setFont(font());

  painter->fillRect(rect(), QBrush(QColor(0, 0, 0)));

  painter->setPen(QColor(255, 255, 255));

  x_offset_ = hscroll_->value();
  y_offset_ = vscroll_->value();

  uint cx, cy;

  vi_->getPos(&cx, &cy);

  CVi::LinesCI pl1, pl2;

  char cc = ' ';
  uint cp = 0;

  uint iy  = 0;
  uint ix1 = 0;
  uint ix2 = 0;

  int x = -x_offset_;
  int y = -y_offset_;

  maxLineLength_ = 0;

  for (pl1 = vi_->linesBegin(), pl2 = vi_->linesEnd(); pl1 != pl2; ++pl1) {
    CViLine *line = *pl1;

    if      (y + char_height_ < 0)
      goto next_line;
    else if (y > h)
      break;

    yLineMap_[y + char_height_] = iy;

    {
      ix1 = 0;
      ix2 = 0;
      x   = -x_offset_;

      CViLine::CharsCI pc1, pc2;

      for (pc1 = line->beginChar(), pc2 = line->endChar(); pc1 != pc2; ++pc1) {
        char c = *pc1;

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
    }

 next_line:
    y += char_height_;

    ++iy;
  }

  int xc = cp*char_width_  - x_offset_;
  int yc = cy*char_height_ - y_offset_;

  painter->fillRect(QRect(xc, yc, char_width_, char_height_), QBrush(QColor(255, 255, 0)));

  painter->setPen(QColor(0, 0, 0));

  painter->drawText(xc, yc + char_ascent_, QString(cc));

  updateScrollbars();
}

void
CQVi::
updateStatus()
{
  uint cx, cy;

  vi_->getPos(&cx, &cy);

  QString text = QString("Row %1, Col %2").arg(cy + 1).arg(cx + 1);

  status_->setText(text);
}

void
CQVi::
resizeEvent(QResizeEvent *)
{
  updateScrollbars();
}

void
CQVi::
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
CQVi::
mousePressEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  for (const auto &yLine : yLineMap_) {
    if (press_pos_.y() < yLine.first) {
      int iy = yLine.second;

      vi_->setPos(0, iy);

      update();

      break;
    }
  }
}

void
CQVi::
mouseReleaseEvent(QMouseEvent *)
{
}

void
CQVi::
mouseMoveEvent(QMouseEvent *e)
{
  press_pos_ = e->pos();

  update();
}

void
CQVi::
wheelEvent(QWheelEvent *)
{
  update();
}

void
CQVi::
keyPressEvent(QKeyEvent *e)
{
#if 0
  CKeyType keyType = CQUtil::convertKey(e->key(), e->modifiers());

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

  if      (keyData.key == Qt::Key_Left    ) keyData.key = (int) CViKeyData::KeyCode::LEFT;
  else if (keyData.key == Qt::Key_Right   ) keyData.key = (int) CViKeyData::KeyCode::RIGHT;
  else if (keyData.key == Qt::Key_Up      ) keyData.key = (int) CViKeyData::KeyCode::UP;
  else if (keyData.key == Qt::Key_Down    ) keyData.key = (int) CViKeyData::KeyCode::DOWN;
  else if (keyData.key == Qt::Key_Home    ) keyData.key = (int) CViKeyData::KeyCode::HOME;
  else if (keyData.key == Qt::Key_End     ) keyData.key = (int) CViKeyData::KeyCode::END;
  else if (keyData.key == Qt::Key_PageUp  ) keyData.key = (int) CViKeyData::KeyCode::PAGE_UP;
  else if (keyData.key == Qt::Key_PageDown) keyData.key = (int) CViKeyData::KeyCode::PAGE_DOWN;
  else if (keyData.key == Qt::Key_Insert  ) keyData.key = (int) CViKeyData::KeyCode::INSERT;
  else if (keyData.key == Qt::Key_SysReq  ) keyData.key = (int) CViKeyData::KeyCode::SYS_REQ;
  else if (keyData.key == Qt::Key_Escape  ) keyData.key = (int) CViKeyData::KeyCode::ESCAPE;

  else if (keyData.key == Qt::Key_Shift   ) keyData.key = (int) CViKeyData::KeyCode::SHIFT;
  else if (keyData.key == Qt::Key_Control ) keyData.key = (int) CViKeyData::KeyCode::CONTROL;
  else if (keyData.key == Qt::Key_Meta    ) keyData.key = (int) CViKeyData::KeyCode::META;
  else if (keyData.key == Qt::Key_Alt     ) keyData.key = (int) CViKeyData::KeyCode::ALT;

  else if (keyData.key == Qt::Key_CapsLock) keyData.key = (int) CViKeyData::KeyCode::CAPS_LOCK;
  else if (keyData.key == Qt::Key_NumLock ) keyData.key = (int) CViKeyData::KeyCode::NUM_LOCK;

  keyData.is_shift   = (e->modifiers() & Qt::ShiftModifier  );
  keyData.is_control = (e->modifiers() & Qt::ControlModifier);
  keyData.is_alt     = (e->modifiers() & Qt::AltModifier    );
  keyData.is_meta    = (e->modifiers() & Qt::MetaModifier   );

  vi_->processChar(keyData);
#endif

  update();
}

QSize
CQVi::
sizeHint() const
{
  QSize s1 = canvas_->sizeHint();
  QSize s2 = status_->sizeHint();

  return QSize(s1.width() + vscroll_->width(),
               s1.height() + s2.height() + hscroll_->height() + 4);
}

QSize
CQVi::
canvasSizeHint() const
{
  return QSize(100*char_width_, 60*char_height_);
}

//------

CQViCanvas::
CQViCanvas(CQVi *vi) :
 QWidget(vi), vi_(vi)
{
  setFocusPolicy(Qt::StrongFocus);
}

void
CQViCanvas::
paintEvent(QPaintEvent *)
{
  QPainter painter(this);

  vi_->draw(&painter);

  vi_->updateStatus();
}

QSize
CQViCanvas::
sizeHint() const
{
  return vi_->canvasSizeHint();
}

//------

CQViStatus::
CQViStatus(CQVi *vi) :
 QLabel(vi), vi_(vi)
{
  QFontMetrics fm(font());

  setFixedHeight(fm.height() + 4);
}
