#ifndef CQEDIT_MGR_H
#define CQEDIT_MGR_H

#include <CVEditMgr.h>

class QPainter;

class CQEditFactory : public CVEditFactory {
 public:
  CEditCursor *createCursor(CEditFile *file) override {
    return new CQEditCursor(dynamic_cast<CQEditFile *>(file));
  }
};

//----

class CQEditRenderer : public CVEditRenderer {
 public:
  CQEditRenderer(QWidget *w, QPainter *painter);

 ~CQEditRenderer() { }

  void fill(const CRGBA &bg) override;

  void setFont(CFontPtr font) override;

  void setForeground(const CRGBA &fg) override;

  void fillRectangle(const CIBBox2D &rect, const CRGBA &rgba) override;

  void drawChar(const CIPoint2D &p, char c) override;

  void drawString(const CIPoint2D &p, const std::string &str) override;

 private:
  QWidget*  w_       { nullptr };
  QPainter* painter_ { nullptr };
  int       ascent_  { 10 };
  int       descent_ { 0 };
};

#endif
