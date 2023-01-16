#ifndef CVEDIT_CURSOR_H
#define CVEDIT_CURSOR_H

#include <CEditCursor.h>
#include <CRGBA.h>
#include <CIBBox2D.h>

class CVEditFile;

class CVEditCursor : public CEditCursor {
 public:
  CVEditCursor(CVEditFile *file);

  virtual ~CVEditCursor() { }

  CVEditCursor *dup() const override;

  void setPos(const CIPoint2D &pos) override;

  const CRGBA &getBg() const { return bg_; }
  const CRGBA &getFg() const { return fg_; }

  virtual void setBg(const CRGBA &fg);
  virtual void setFg(const CRGBA &fg);

  void setBBox(const CIBBox2D &bbox) { bbox_ = bbox; }

  const CIBBox2D &getBBox() const { return bbox_; }

  virtual void draw(const CIBBox2D &bbox, char c='\0');

 private:
  CVEditFile *file_;
  CRGBA       fg_;
  CRGBA       bg_;
  CIBBox2D    bbox_;
};

#endif
