#include <CQEditCanvas.h>
#include <CQUtil.h>
#include <CQEdit.h>

#include <QGridLayout>
#include <QScrollBar>
#include <QPainter>

CQEditArea::
CQEditArea(CQEdit *edit) :
 QWidget(nullptr), edit_(edit)
{
  setObjectName("area");

  auto *grid = new QGridLayout(this);
  grid->setMargin(0); grid->setSpacing(0);

  canvas_ = new CQEditCanvas(this);

  hbar_ = new QScrollBar(Qt::Horizontal);
  vbar_ = new QScrollBar(Qt::Vertical);

  grid->addWidget(canvas_, 0, 0);
  grid->addWidget(vbar_  , 0, 1);
  grid->addWidget(hbar_  , 1, 0);

  connect(canvas_, SIGNAL(sizeChanged()), this, SIGNAL(sizeChanged()));

  connect(vbar_, SIGNAL(valueChanged(int)), this, SLOT(vscrollSlot(int)));
  connect(hbar_, SIGNAL(valueChanged(int)), this, SLOT(hscrollSlot(int)));
}

void
CQEditArea::
updateScrollbars()
{
  int xsize = canvas_->width ();
  int ysize = canvas_->height();

  const CISize2D &size = edit_->getFile()->getVSize();

  int dx = std::max(0, size.getWidth () - xsize);
  int dy = std::max(0, size.getHeight() - ysize);

  hbar_->setPageStep(xsize);

  hbar_->setMinimum(0);
  hbar_->setMaximum(dx);

  hbar_->setSingleStep(edit_->getFile()->getFont()->getCharWidth());

  hbar_->setValue(-edit_->getFile()->getXOffset());

  vbar_->setPageStep(ysize);

  vbar_->setMinimum(0);
  vbar_->setMaximum(dy);

  vbar_->setSingleStep(edit_->getFile()->getFont()->getCharHeight());

  vbar_->setValue(-edit_->getFile()->getYOffset());
}

void
CQEditArea::
hscrollSlot(int x)
{
  edit_->getFile()->setXOffset(-x);

  edit_->getFile()->setIgnoreChanged(true);

  canvas_->update();
}

void
CQEditArea::
vscrollSlot(int y)
{
  edit_->getFile()->setYOffset(-y);

  edit_->getFile()->setIgnoreChanged(true);

  canvas_->update();
}

//------

CQEditCanvas::
CQEditCanvas(CQEditArea *area) :
 QWidget(nullptr), area_(area), edit_(area->getEdit())
{
  setObjectName("canvas");

  setFocusPolicy(Qt::StrongFocus);
}

void
CQEditCanvas::
paintEvent(QPaintEvent *)
{
  QPainter ipainter(&qimage_);

  edit_->draw(&ipainter);

  QPainter painter(this);

  painter.drawImage(QPoint(0, 0), qimage_);

  area_->updateScrollbars();
}

void
CQEditCanvas::
resizeEvent(QResizeEvent *)
{
  qimage_ = QImage(QSize(width(), height()), QImage::Format_ARGB32);

  edit_->getFile()->setIgnoreChanged(true);

  emit sizeChanged();
}

void
CQEditCanvas::
keyPressEvent(QKeyEvent *e)
{
  CKeyEvent *event = CQUtil::convertEvent(e);

  edit_->getFile()->keyPress(*event);
}

void
CQEditCanvas::
keyReleaseEvent(QKeyEvent *e)
{
  CKeyEvent *event = CQUtil::convertEvent(e);

  edit_->getFile()->keyRelease(*event);
}

void
CQEditCanvas::
mousePressEvent(QMouseEvent *e)
{
  CMouseEvent *event = CQUtil::convertEvent(e);

  edit_->getFile()->mousePress(*event);

  pressed_ = true;
  button_  = event->getButton();
}

void
CQEditCanvas::
mouseMoveEvent(QMouseEvent *e)
{
  CMouseEvent *event = CQUtil::convertEvent(e);

  if (pressed_) {
    event->setButton(button_);

    edit_->getFile()->mouseMotion(*event);
  }
}

void
CQEditCanvas::
mouseReleaseEvent(QMouseEvent *e)
{
  CMouseEvent *event = CQUtil::convertEvent(e);

  edit_->getFile()->mouseRelease(*event);

  pressed_ = false;
}

void
CQEditCanvas::
selectionNotify(const QPoint &, const QPoint &)
{
  //const CIBBox2D &bbox = getSelectionRect();

  //edit_->selectionNotify(bbox);
}
