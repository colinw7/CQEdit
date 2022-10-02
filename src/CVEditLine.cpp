#include <CVEditLine.h>
#include <CVEditFile.h>
#include <CVEditChar.h>
#include <CVEditCursor.h>
#include <CAssert.h>

CVEditLine::
CVEditLine(CVEditFile *vfile) :
 CEditLine(vfile), vfile_(vfile), extraCharChanged_(false)
{
}

CVEditLine::
~CVEditLine()
{
}

CVEditLine *
CVEditLine::
dup() const
{
  return new CVEditLine(*this);
}

const CRGBA &
CVEditLine::
getBg() const
{
  if (style_.bg.isValid())
    return style_.bg.getValue();
  else
    return vfile_->getBg();
}

void
CVEditLine::
setBg(const CRGBA &bg)
{
  style_.bg.setValue(bg);
}

const CRGBA &
CVEditLine::
getFg() const
{
  if (style_.fg.isValid())
    return style_.fg.getValue();
  else
    return vfile_->getFg();
}

void
CVEditLine::
setFg(const CRGBA &fg)
{
  style_.fg.setValue(fg);
}

void
CVEditLine::
setBBox(const CIPoint2D &pos)
{
  int cw = vfile_->getCharWidth();
  int ch = vfile_->getCharHeight();

  //CIPoint2D pos1 = pos;

  int n = 0;
  int x = pos.x;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    int num = 1;

    if (vchar->getChar() == '\t')
      num = 8 - (n % 8);

    n += num;
    x += num*cw;
  }

  x += cw;

  bbox_.set(pos.x, pos.y, x, pos.y + ch);
}

void
CVEditLine::
draw(const CIBBox2D &bbox, CVEditCursor *cursor, bool filled)
{
  int cw = vfile_->getCharWidth();

  uint n  = 0;
  uint x1 = bbox.getXMin();
  uint x2 = 0;

  uint xmax = bbox.getXMax();

  bool changed = getChanged();

  if (! changed && vfile_->getIgnoreChanged())
    changed = true;

  int cx = -1;

  if (cursor)
    cx = cursor->getPos().x;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for (uint col = 0; pchar1 != pchar2; ++pchar1, ++col) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    //-----

    char c = vchar->getChar();

    uint num = 1;

    if (c == '\t')
      num = 8 - (n % 8);

    n  += num;
    x2  = x1 + num*cw;

    //-----

    if (x1 > xmax)
      continue;

    bool is_cursor = (cursor && cx == int(col));

    if (changed || vchar->getChanged() || is_cursor) {
      CIBBox2D cbbox(x1, bbox.getYMin(), x2, bbox.getYMax());

      vchar->draw(vfile_, this, cbbox, filled);

      if (is_cursor)
        cursor->draw(cbbox);
    }

    vchar->setChanged(false);

    //------

    x1 = x2;
  }

  bool is_cursor = (cursor && cx == int(n));

  if (changed || extraCharChanged_ || is_cursor) {
    x2 = x1 + cw;

    CIBBox2D cbbox(x1, bbox.getYMin(), x2, bbox.getYMax());

    if (vfile_->getOptions().list) {
      // draw $ at end

      if (is_cursor)
        cursor->draw(cbbox, '$');
      else {
        CRGBA bg1 = vfile_->getBg();
        CRGBA fg1 = CRGBA(1,0,0);

        vfile_->drawFilledChar(bbox, '$', bg1, fg1, filled);
      }
    }
    else {
      if (is_cursor)
        cursor->draw(cbbox);
    }

    extraCharChanged_ = false;
  }
}

void
CVEditLine::
clearSelection()
{
  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    vchar->setSelected(false);
  }
}

void
CVEditLine::
selectInside(const CIBBox2D &bbox)
{
  if (! bbox.overlaps(getBBox()))
    return;

  int cw = vfile_->getCharWidth();

  uint n  = 0;
  uint x1 = 0;
  uint x2 = 0;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    char c = vchar->getChar();

    uint num = 1;

    if (c == '\t')
      num = 8 - (n % 8);

    n  += num;
    x2  = x1 + num*cw;

    if (int(x1) > bbox.getXMax() || int(x2) < bbox.getXMin())
      continue;

    vchar->setSelected(true);

    x1 = x2;
  }
}

void
CVEditLine::
selectAllChars()
{
  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    vchar->setSelected(true);
  }
}

void
CVEditLine::
selectChar(int row)
{
  CASSERT(row >= 0 && row <= int(getLength()), "Invalid Char Num");

  if (row < int(getLength())) {
    CVEditChar *vchar = const_cast<CVEditChar *>(
      dynamic_cast<const CVEditChar *>(getCharP(row)));

    vchar->setSelected(true);
  }
}

void
CVEditLine::
setSelectedCharColor(const CRGBA &color)
{
  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    if (vchar->getSelected())
      vchar->setFg(this, color);
  }
}

std::string
CVEditLine::
getSelectedText() const
{
  std::string text;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    if (vchar->getSelected())
      text += vchar->getChar();
  }

  return text;
}

bool
CVEditLine::
pointToCol(const CIPoint2D &point, int *col) const
{
  if (point.y < bbox_.getYMin() || point.y > bbox_.getYMax())
    return false;

  int cw = vfile_->getCharWidth();

  uint n  = 0;
  uint x1 = 0;
  uint x2 = 0;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for (uint col1 = 0; pchar1 != pchar2; ++pchar1, ++col1) {
    char c = (*pchar1)->getChar();

    uint num = 1;

    if (c == '\t')
      num = 8 - (n % 8);

    n  += num;
    x2  = x1 + num*cw;

    if (point.x >= int(x1) && point.x <= int(x2)) {
      *col = col1;
      return true;
    }

    x1 = x2;
  }

  *col = 0;

  return true;
}

bool
CVEditLine::
colToPoint(int col, CIPoint2D &point) const
{
  CIBBox2D rect;

  if (! colToRect(col, rect))
    return false;

  point.x = rect.getXMin();
  point.y = rect.getYMax();

  return true;
}

bool
CVEditLine::
colToRect(int col, CIBBox2D &rect) const
{
  if (col < 0 || col >= int(getLength()))
    return false;

  int cw = vfile_->getCharWidth();

  uint n  = 0;
  uint x1 = 0;
  uint x2 = 0;

  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for (uint col1 = 0; pchar1 != pchar2; ++pchar1, ++col1) {
    char c = (*pchar1)->getChar();

    uint num = 1;

    if (c == '\t')
      num = 8 - (n % 8);

    n  += num;
    x2  = x1 + num*cw;

    if (col == int(col1)) {
      rect = CIBBox2D(x1, getBBox().getYMin(), x2, getBBox().getYMax());
      return true;
    }

    x1 = x2;
  }

  return false;
}

bool
CVEditLine::
overlaps(const CIBBox2D &bbox) const
{
  return bbox_.overlaps(bbox);
}

void
CVEditLine::
clearAnnotations()
{
  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for ( ; pchar1 != pchar2; ++pchar1) {
    CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

    vchar->setBg(this, vfile_->getBg());
    vchar->setFg(this, vfile_->getFg());
  }
}

void
CVEditLine::
addAnnotation(uint word_start, uint word_end, const CRGBA &bg, const CRGBA &fg)
{
  CEditLineChars::const_iterator pchar1 = beginChar();
  CEditLineChars::const_iterator pchar2 = endChar  ();

  for (uint i = 0; pchar1 != pchar2; ++pchar1, ++i) {
    if (i >= word_start && i <= word_end) {
      CVEditChar *vchar = dynamic_cast<CVEditChar *>(*pchar1);

      vchar->setBg(this, bg);
      vchar->setFg(this, fg);
    }
  }
}
