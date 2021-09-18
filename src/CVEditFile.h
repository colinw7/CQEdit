#ifndef CVEDIT_FILE_H
#define CVEDIT_FILE_H

class CVEditFile;
class CVEditLine;
class CVEditCursor;

class CMouseEvent;
class CSyntax;

#include <accessor.h>

#include <CEditFile.h>
#include <CPOptVal.h>
#include <CIBBox2D.h>
#include <CRGBA.h>
#include <CEvent.h>
#include <CScrollType.h>
#include <CConfig.h>
#include <CFont.h>
#include <memory>

class CVEditVi;
class CVEditGen;

struct CVEditFileStyle {
  CPOptValT<CRGBA>   fg;
  CPOptValT<CRGBA>   bg;
  COptValT<CFontPtr> font;
};

#define CVEditFileMgrInst CVEditFileMgr::getInstance()

class CVEditFileMgr {
 public:
  static CVEditFileMgr *getInstance() {
    static CVEditFileMgr *instance;

    if (! instance)
      instance = new CVEditFileMgr;

    return instance;
  }

  void loadConfig(CVEditFile *edit, const std::string &name);
  void loadConfig(CVEditFile *edit);

  void saveConfig(CVEditFile *edit, const std::string &name);
  void saveConfig(CVEditFile *edit);

 private:
  CVEditFileMgr();

  void loadConfig(CVEditFile *edit, CConfig &config);
  void saveConfig(CVEditFile *edit, CConfig &config);

 private:
  CConfig config_;
};

class CVLineEdit;

class CVEditFile : public CEditFile {
 public:
  enum Mode {
    ModeNormal,
    ModeVi
  };

 public:
  CVEditFile();

  virtual ~CVEditFile();

  void initFont();

  uint getCharWidth () const { return char_width_ ; }
  uint getCharHeight() const { return char_height_; }

  uint getNumRows() const { return num_rows_; }
  uint getNumCols() const { return num_cols_; }

  uint getWidth () const { return width_ ; }
  uint getHeight() const { return height_; }

  int getXOffset() const { return x_offset_; }
  int getYOffset() const { return y_offset_; }

  void setXOffset(int x_offset);
  void setYOffset(int y_offset);

  ACCESSOR(IgnoreChanged, bool  , ignore_changed)
  ACCESSOR(Visual       , bool  , visual        )
  ACCESSOR(TabStop      , uint  , tab_stop      )

  CVEditVi *getVi() const;

  virtual void loadConfig();
  virtual void loadConfig(const std::string &name);

  virtual void saveConfig();
  virtual void saveConfig(const std::string &name);

  virtual bool loadLines(const std::string &filename);
  virtual bool saveLines(const std::string &filename);

  const CIBBox2D &getBBox() const { return bbox_; }

  virtual void setBBox(const CIBBox2D &bbox) { bbox_ = bbox; }

  const CRGBA &getBg() const;
  virtual void setBg(const CRGBA &bg);

  const CRGBA &getFg() const;
  virtual void setFg(const CRGBA &fg);

  void setFontConf(const std::string &str);

  CFontPtr getFont() const;
  void setFont(CFontPtr font);

  const CRGBA &getCursorBg() const;
  void setCursorBg(const CRGBA &bg);

  const CRGBA &getCursorFg() const;
  void setCursorFg(const CRGBA &fg);

  const CISize2D &getVSize() const { return vsize_; }

  virtual void setMode(Mode mode);
  Mode getMode() const { return mode_; }

  virtual void setOverwriteMode(bool overwrite_mode_);
  bool getOverwriteMode() const { return overwrite_mode_; }

  bool getInsertMode() const;

  virtual void draw(uint width, uint height);

  virtual void drawStatus(const CIBBox2D &bbox);

  virtual void clearSelection();

  virtual void selectBBox(const CIBBox2D &bbox, bool clear);
  virtual void selectInside(const CIBBox2D &bbox, bool clear);

  virtual void rangeSelect(const CIBBox2D &bbox, bool clear);
  virtual void rangeSelect(int row1, int col1, int row2, int col2, bool clear);
  virtual void rangeSelect(const CIPoint2D &start, const CIPoint2D &end, bool clear);

  virtual void selectChar(int row, int col);
  virtual void selectLine(int row);

  virtual void setSelectionColor(const CRGBA &color);

  void setSelectRange(const CIPoint2D &start, const CIPoint2D &end);

  const CIPoint2D &getSelectStart() const;
  const CIPoint2D &getSelectEnd  () const;

  virtual std::string getSelectedText() const;

  virtual bool pointToPos(const CIPoint2D &point, int *row, int *col) const;
  virtual bool posToPoint(int row, int col, CIPoint2D &point) const;
  virtual bool posToRect (int row, int col, CIBBox2D  &rect ) const;

  void undo() override;
  void redo() override;

  void replayFile(const std::string &fileName) override;

  virtual void keyPress  (const CKeyEvent &event);
  virtual void keyRelease(const CKeyEvent &event);

  virtual void mousePress  (const CMouseEvent &event);
  virtual void mouseMotion (const CMouseEvent &event);
  virtual void mouseRelease(const CMouseEvent &event);

  virtual void scrollTop();
  virtual void scrollMiddle();
  virtual void scrollBottom();

  virtual void scrollToPos(const CIPoint2D &pos, CScrollType type);

  virtual void scrollTo(const CIBBox2D &, CScrollType);

  virtual uint getPageLength() const;

  virtual int getPageTop() const;
  virtual int getPageBottom() const;

  // file changed
  void setChanged(bool changed) override;

  // cursor changed
  virtual void positionChanged();

  // display ed command interface with initial text
  virtual void setCmdText(const std::string &) { }

  // stateChanged (insert/replace vi/normal ...) so update status
  virtual void stateChanged() { }

  // selection string changed
  virtual void selectionChanged(const std::string &) { }

  // redraw required
  virtual void update() { }

  // quit
  virtual void quit();

  // display marks
  virtual void displayMarks() { }

  // display marks
  virtual void displayRegisters() { }

  virtual void setSyntax(CSyntax *syntax);

  virtual void updateSyntax();

  void optionChanged(const std::string &name) override;

  // draw char
  virtual void drawFilledChar(const CIBBox2D &bbox, char c, const CRGBA &bg,
                              const CRGBA &fg, bool filled=false) const;

  virtual void applyOffset(CIBBox2D &bbox) const;
  virtual void applyOffset(CIPoint2D &p) const;

 protected:
  using EditViP  = std::unique_ptr<CVEditVi>;
  using EditGenP = std::unique_ptr<CVEditGen>;
  using SyntaxP  = std::unique_ptr<CSyntax>;

  using StyleP = CPOptValT<CVEditFileStyle>;

  CIBBox2D  bbox_;
  uint      indent_         { 0 };
  int       line_num1_      { -1 };
  int       line_num2_      { -1 };
  StyleP    style_;
  EditViP   vi_;
  EditGenP  gen_;
  SyntaxP   syntax_;
  uint      char_width_     { 8 };
  uint      char_height_    { 12 };
  uint      num_rows_       { 0 };
  uint      num_cols_       { 0 };
  uint      width_          { 100 };
  uint      height_         { 100 };
  int       x_offset_       { 0 };
  int       y_offset_       { 0 };
  CISize2D  vsize_;
  Mode      mode_           { ModeVi };
  bool      overwrite_mode_ { false };
  uint      tab_stop_       { 8 };
  bool      visual_         { false };
  CIPoint2D select_start_;
  CIPoint2D select_end_;
  bool      bbox_select_    { false };
  uint      press_row_      { 0 };
  uint      press_col_      { 0 };
  uint      release_row_    { 0 };
  uint      release_col_    { 0 };
  bool      ignore_changed_ { false };
};

#endif
