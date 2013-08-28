#include <CVEditCursor.h>
#include <CVEditFile.h>
#include <CVEditLine.h>
#include <CVEditChar.h>

CVEditCursor::
CVEditCursor(CVEditFile *file) :
 CEditCursor(file), file_(file)
{
  fg_ = CRGBA(1,1,1);
  bg_ = CRGBA(1,0,0);
}

CVEditCursor *
CVEditCursor::
dup() const
{
  return new CVEditCursor(*this);
}

void
CVEditCursor::
setPos(const CIPoint2D &pos)
{
  CEditChar *c = NULL;

  if (file_->isValidPos(pos_))
    c = const_cast<CEditChar *>(file_->getEditChar(pos_.y, pos_.x));

  if (c)
    c->setChanged(true);
  else {
    CEditLine *l = const_cast<CEditLine *>(file_->getEditLine(pos_.y));

    if (l) l->setChanged(true);
  }

  CEditCursor::setPos(pos);

  CIBBox2D rect;

  file_->posToRect(pos.y, pos.x, rect);

  file_->scrollTo(rect, CSCROLL_TYPE_VISIBLE);

  file_->positionChanged();
}

void
CVEditCursor::
setBg(const CRGBA &bg)
{
  bg_ = bg;
}

void
CVEditCursor::
setFg(const CRGBA &fg)
{
  fg_ = fg;
}

void
CVEditCursor::
draw(const CIBBox2D &bbox, char c)
{
  if (c == '\0') {
    const CIPoint2D &pos = getPos();

    c = file_->getChar(pos.y, pos.x);
  }

  if (c == '\0')
    c = ' ';

  CRGBA bg1 = bg_;
  CRGBA fg1 = fg_;

  file_->drawFilledChar(bbox, c, bg1, fg1, false);
}
