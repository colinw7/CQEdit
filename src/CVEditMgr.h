#ifndef CVEDIT_MGR_H
#define CVEDIT_MGR_H

#include <CEditMgr.h>
#include <CVEditFile.h>
#include <CVEditCursor.h>
#include <CVEditLine.h>
#include <CVEditChar.h>
#include <CVLineEdit.h>

class CVEditFactory : public CEditDefFactory {
 public:
  virtual ~CVEditFactory() { }

  virtual CEditFile *createFile() {
    return new CVEditFile();
  }

  virtual CEditCursor *createCursor(CEditFile *file) {
    return new CVEditCursor(dynamic_cast<CVEditFile *>(file));
  }

  virtual CEditLine *createLine(CEditFile *file) {
    return new CVEditLine(dynamic_cast<CVEditFile *>(file));
  }

  virtual CEditChar *createChar(CEditLine *) {
    return new CVEditChar();
  }

  virtual CLineEdit *createLineEdit(CEditFile *) {
    return new CVLineEdit();
  }
};

class CVEditRenderer {
 public:
  CVEditRenderer() { }

  virtual ~CVEditRenderer() { }

  virtual void fill(const CRGBA &bg) = 0;

  virtual void setFont(CFontPtr font) = 0;

  virtual void setForeground(const CRGBA &fg) = 0;

  virtual void fillRectangle(const CIBBox2D &rect, const CRGBA &rgba) = 0;

  virtual void drawChar(const CIPoint2D &p, char c) = 0;

  virtual void drawString(const CIPoint2D &p, const std::string &str) = 0;
};

#define CVEditMgrInst CVEditMgr::getInstance()

class CVEditMgr {
 public:
  static CVEditMgr *getInstance();

 ~CVEditMgr();

  void setRenderer(CVEditRenderer *renderer);

  CVEditRenderer *getRenderer() const;

  void fill(const CRGBA &bg);

  void setFont(CFontPtr font);

  void setForeground(const CRGBA &fg);

  void fillRectangle(const CIBBox2D &rect, const CRGBA &rgba);

  void drawChar(const CIPoint2D &p, char c);

  void drawString(const CIPoint2D &p, const std::string &str);

 private:
  CVEditMgr();

 private:
  CVEditRenderer *renderer_;
};

#endif
