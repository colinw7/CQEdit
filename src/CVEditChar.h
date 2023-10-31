#ifndef CVEDIT_CHAR_H
#define CVEDIT_CHAR_H

#include <CPOptVal.h>
#include <CRGBA.h>
#include <CIBBox2D.h>
#include <CEditChar.h>

class CVEditFile;
class CVEditLine;

struct CVEditCharStyle {
  CPOptValT<CRGBA> fg;
  CPOptValT<CRGBA> bg;
};

class CVEditChar : public CEditChar {
 public:
  CVEditChar();

  virtual ~CVEditChar();

  CVEditChar *dup() const override;

  // Style
  const CRGBA &getBg(const CVEditLine *vline) const;
  virtual void setBg(const CVEditLine *vline, const CRGBA &bg);

  const CRGBA &getFg(const CVEditLine *vline) const;
  virtual void setFg(const CVEditLine *vline, const CRGBA &fg);

  // Selected
  bool getSelected() const { return selected_; }
  virtual void setSelected(bool selected);

  // Draw
  virtual void draw(CVEditFile *file, const CVEditLine *vline, const CIBBox2D &bbox, bool fill);

 private:
  CVEditCharStyle style_;
  bool            selected_ { false };
};

#endif
