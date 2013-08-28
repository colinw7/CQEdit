#ifndef CVEDIT_LINE_H
#define CVEDIT_LINE_H

#include <CEditLine.h>
#include <CRGBA.h>
#include <CIBBox2D.h>
#include <accessor.h>

class CVEditChar;
class CVEditFile;
class CVEditCursor;

struct CVEditLineStyle {
  CPOptValT<CRGBA> fg;
  CPOptValT<CRGBA> bg;
};

class CVEditLine : public CEditLine {
 public:
  CVEditLine(CVEditFile *vfile);

  virtual ~CVEditLine();

  virtual CVEditLine *dup() const;

  ACCESSOR(ExtraCharChanged, bool, extraCharChanged)

  const CRGBA &getBg() const;
  virtual void setBg(const CRGBA &bg);

  const CRGBA &getFg() const;
  virtual void setFg(const CRGBA &fg);

  void setBBox(const CIPoint2D &pos);

  const CIBBox2D &getBBox() const { return bbox_; }

  virtual void draw(const CIBBox2D &bbox, CVEditCursor *cursor, bool fill);

  virtual void clearSelection();

  virtual void selectInside(const CIBBox2D &bbox);

  virtual void selectAllChars();
  virtual void selectChar(int row);

  virtual void setSelectedCharColor(const CRGBA &color);

  virtual std::string getSelectedText() const;

  virtual bool pointToCol(const CIPoint2D &point, int *col) const;

  virtual bool colToPoint(int col, CIPoint2D &point) const;
  virtual bool colToRect (int col, CIBBox2D  &rect ) const;

  virtual bool overlaps(const CIBBox2D &bbox) const;

  uint getWidth () const { return bbox_.getWidth (); }
  uint getHeight() const { return bbox_.getHeight(); }

  void clearAnnotations();
  void addAnnotation(uint word_start, uint word_end,
                     const CRGBA &bg, const CRGBA &fg);

 private:
  CVEditFile      *vfile_;
  CIBBox2D         bbox_;
  CVEditLineStyle  style_;
  bool             extraCharChanged_;
};

#endif
