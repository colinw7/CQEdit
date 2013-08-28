#include <CVEditMgr.h>

CVEditMgr *
CVEditMgr::
getInstance()
{
  static CVEditMgr *instance;

  if (! instance)
    instance = new CVEditMgr;

  return instance;
}

CVEditMgr::
CVEditMgr() :
 renderer_(NULL)
{
}

CVEditMgr::
~CVEditMgr()
{
}

void
CVEditMgr::
setRenderer(CVEditRenderer *renderer)
{
  renderer_ = renderer;
}

CVEditRenderer *
CVEditMgr::
getRenderer() const
{
  return renderer_;
}

void
CVEditMgr::
fill(const CRGBA &bg)
{
  renderer_->fill(bg);
}

void
CVEditMgr::
setFont(CFontPtr font)
{
  renderer_->setFont(font);
}

void
CVEditMgr::
setForeground(const CRGBA &fg)
{
  renderer_->setForeground(fg);
}

void
CVEditMgr::
fillRectangle(const CIBBox2D &rect, const CRGBA &rgba)
{
  renderer_->fillRectangle(rect, rgba);
}

void
CVEditMgr::
drawChar(const CIPoint2D &p, char c)
{
  renderer_->drawChar(p, c);
}

void
CVEditMgr::
drawString(const CIPoint2D &p, const std::string &str)
{
  renderer_->drawString(p, str);
}
