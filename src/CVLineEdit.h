#ifndef CVLINE_EDIT
#define CVLINE_EDIT

#include <CLineEdit.h>
#include <CRGBA.h>
#include <CEvent.h>
#include <CIBBox2D.h>
#include <CFont.h>
#include <accessor.h>
#include <vector>

class CVLineEditRenderer {
 public:
  CVLineEditRenderer() { }

  virtual ~CVLineEditRenderer() { }

  virtual void setForeground(const CRGBA &rgba) = 0;

  virtual void fillRectangle(const CIBBox2D &rect, const CRGBA &rgba) = 0;

  virtual void drawChar(const CIPoint2D &p, char c) = 0;
};

class CVLineEdit : public CLineEdit {
 public:
  CVLineEdit();

  virtual ~CVLineEdit() { }

  ACCESSOR(Overwrite, bool, overwrite)

  virtual void setBackground(const CRGBA &bg);
  virtual void setForeground(const CRGBA &gg);

  virtual void setFont(CFontPtr font);

  void keyPress(const CKeyEvent &event);

  void buttonPress(const CMouseEvent &event);

  virtual void update() { }

  virtual void process() { }

  virtual void prevLine() { }
  virtual void nextLine() { }

  virtual void draw(CVLineEditRenderer *renderer, uint w, uint h);

  virtual void drawInside(CVLineEditRenderer *renderer, const CIBBox2D &bbox);

  int xyToPos(int x, int y);

  virtual void drawFilledChar(const CIBBox2D &bbox, char c, const CRGBA &bg,
                              const CRGBA &fg, bool filled=false) const;

 protected:
  CRGBA                  bg_, fg_;
  CRGBA                  cursor_color_;
  CFontPtr               font_;
  bool                   overwrite_;
  std::vector<CIBBox2D>  bboxes_;
  CVLineEditRenderer    *renderer_;
};

#endif
