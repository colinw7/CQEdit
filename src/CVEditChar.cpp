#include <CVEditChar.h>
#include <CVEditFile.h>
#include <CVEditLine.h>

CVEditChar::
CVEditChar() :
 CEditChar(), selected_(false)
{
}

CVEditChar::
~CVEditChar()
{
}

CVEditChar *
CVEditChar::
dup() const
{
  return new CVEditChar(*this);
}

const CRGBA &
CVEditChar::
getBg(const CVEditLine *vline) const
{
  if (style_.bg.isValid())
    return style_.bg.getValue();

  return vline->getBg();
}

void
CVEditChar::
setBg(const CVEditLine *, const CRGBA &bg)
{
  style_.bg.setValue(bg);
}

const CRGBA &
CVEditChar::
getFg(const CVEditLine *vline) const
{
  if (style_.fg.isValid())
    return style_.fg.getValue();

  return vline->getFg();
}

void
CVEditChar::
setFg(const CVEditLine *, const CRGBA &fg)
{
  style_.fg.setValue(fg);
}

void
CVEditChar::
setSelected(bool selected)
{
  selected_ = selected;

  setChanged(true);
}

void
CVEditChar::
draw(CVEditFile *file, const CVEditLine *line, const CIBBox2D &bbox, bool filled)
{
  CRGBA bg1, fg1;

  if (selected_) {
    bg1 = getFg(line);

    filled = false;
  }
  else {
    if (! filled)
      bg1 = getBg(line);
  }

  if (selected_)
    fg1 = getBg(line);
  else
    fg1 = getFg(line);

  file->drawFilledChar(bbox, getChar(), bg1, fg1, filled);
}
