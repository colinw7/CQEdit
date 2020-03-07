#include <CVEditFile.h>
#include <CVEditGen.h>
#include <CVEditVi.h>
#include <CVEditCursor.h>
#include <CVEditMgr.h>
#include <CVLineEdit.h>
#include <CFile.h>
#include <CEvent.h>
#include <CFontMgr.h>
#include <CStrUtil.h>
#include <CSyntax.h>
#include <CRGBName.h>
#include <CAssert.h>

CVEditFileMgr::
CVEditFileMgr() :
 config_("CEdit")
{
}

void
CVEditFileMgr::
loadConfig(CVEditFile *file, const std::string &name)
{
  CConfig config(name);

  loadConfig(file, config);
}

void
CVEditFileMgr::
loadConfig(CVEditFile *file)
{
  loadConfig(file, config_);
}

void
CVEditFileMgr::
loadConfig(CVEditFile *file, CConfig &config)
{
  std::string str;

  if (config.getValue("font", "", str)) {
    std::string family = "courier";
    CFontStyle  style  = CFONT_STYLE_NORMAL;
    uint        size   = 12;

    if (! CFont::decodeFontName(str, family, style, size)) {
      uint x_res, y_res;

      if (! CFont::decodeXFontName(str, family, style, size, x_res, y_res)) {
        family = "courier";
        style  = CFONT_STYLE_NORMAL;
        size   = 12;
      }
    }

    CFontPtr font = CFontMgrInst->lookupFont(family, style, size);

    if (! font.isValid()) {
      std::cerr << "Invalid font: " << str << std::endl;

      font = CFontMgrInst->lookupFont("courier", CFONT_STYLE_NORMAL, 12);
    }

    file->setFont(font);
  }

  if (config.getValue("bg", "", str)) {
    CRGBA bg = CRGBName::toRGBA(str, CRGBA(0,0,0));

    file->setBg(bg);
  }

  if (config.getValue("fg", "", str)) {
    CRGBA fg = CRGBName::toRGBA(str, CRGBA(1,1,1));

    file->setFg(fg);
  }

  if (config.getValue("cursorBg", "", str)) {
    CRGBA bg = CRGBName::toRGBA(str, CRGBA(1,0,0));

    file->setCursorBg(bg);
  }

  if (config.getValue("cursorFg", "", str)) {
    CRGBA fg = CRGBName::toRGBA(str, CRGBA(0,1,0));

    file->setCursorFg(fg);
  }
}

void
CVEditFileMgr::
saveConfig(CVEditFile *file)
{
  saveConfig(file, config_);
}

void
CVEditFileMgr::
saveConfig(CVEditFile *file, const std::string &name)
{
  CConfig config(name);

  saveConfig(file, config);
}

void
CVEditFileMgr::
saveConfig(CVEditFile *file, CConfig &config)
{
  std::string fontStr = file->getFont()->encodeFontName();

  config.setValue("font", "", fontStr);

  config.setValue("bg", "", file->getBg().stringEncode());
  config.setValue("fg", "", file->getFg().stringEncode());

  config.setValue("cursorBg", "", file->getCursorBg().stringEncode());
  config.setValue("cursorFg", "", file->getCursorFg().stringEncode());

  config.save();
}

//------------------

CVEditFile::
CVEditFile()
{
  CFontPtr font = CFontMgrInst->lookupFont("courier", CFONT_STYLE_NORMAL, 12);

  setFont(font);

  setBg(CRGBA(1,1,1));
  setFg(CRGBA(0,0,0));

  gen_ = std::make_unique<CVEditGen>(this);
  vi_  = std::make_unique<CVEditVi >(this);
}

CVEditFile::
~CVEditFile()
{
}

void
CVEditFile::
setXOffset(int x_offset)
{
  x_offset_ = x_offset;
}

void
CVEditFile::
setYOffset(int y_offset)
{
  y_offset_ = y_offset;
}

CVEditVi *
CVEditFile::
getVi() const
{
  return vi_.get();
}

void
CVEditFile::
loadConfig(const std::string &name)
{
  CVEditFileMgrInst->loadConfig(this, name);
}

void
CVEditFile::
loadConfig()
{
  CVEditFileMgrInst->loadConfig(this);
}

void
CVEditFile::
saveConfig(const std::string &name)
{
  CVEditFileMgrInst->saveConfig(this, name);
}

void
CVEditFile::
saveConfig()
{
  CVEditFileMgrInst->saveConfig(this);
}

bool
CVEditFile::
loadLines(const std::string &filename)
{
  if (! CEditFile::loadLines(filename))
    return false;

  update();

  return true;
}

bool
CVEditFile::
saveLines(const std::string &filename)
{
  if (! CEditFile::saveLines(filename))
    return false;

  update();

  return true;
}

const CRGBA &
CVEditFile::
getBg() const
{
  style_.initValue();

  const CVEditFileStyle &style = style_.getValue();

  style.bg.initValue();

  return style.bg.getValue();
}

void
CVEditFile::
setBg(const CRGBA &bg)
{
  style_.initValue();

  CVEditFileStyle &style = style_.getValue();

  style.bg.setValue(bg);

  update();
}

const CRGBA &
CVEditFile::
getFg() const
{
  style_.initValue();

  const CVEditFileStyle &style = style_.getValue();

  style.fg.initValue();

  return style.fg.getValue();
}

void
CVEditFile::
setFg(const CRGBA &fg)
{
  style_.initValue();

  CVEditFileStyle &style = style_.getValue();

  style.fg.setValue(fg);

  update();
}

CFontPtr
CVEditFile::
getFont() const
{
  style_.initValue();

  const CVEditFileStyle &style = style_.getValue();

  return style.font.getValue();
}

void
CVEditFile::
setFont(CFontPtr font)
{
  style_.initValue();

  CVEditFileStyle &style = style_.getValue();

  style.font.setValue(font);

  char_width_  = font->getStringWidth("X");
  char_height_ = font->getCharHeight();

  update();
}

const CRGBA &
CVEditFile::
getCursorBg() const
{
  CVEditCursor *cursor = dynamic_cast<CVEditCursor *>(getCursor());

  return cursor->getBg();
}

void
CVEditFile::
setCursorBg(const CRGBA &bg)
{
  CVEditCursor *cursor = dynamic_cast<CVEditCursor *>(getCursor());

  if (cursor)
    cursor->setBg(bg);

  update();
}

const CRGBA &
CVEditFile::
getCursorFg() const
{
  CVEditCursor *cursor = dynamic_cast<CVEditCursor *>(getCursor());

  return cursor->getFg();
}

void
CVEditFile::
setCursorFg(const CRGBA &fg)
{
  CVEditCursor *cursor = dynamic_cast<CVEditCursor *>(getCursor());

  cursor->setFg(fg);

  update();
}

void
CVEditFile::
setMode(Mode mode)
{
  mode_ = mode;

  if (mode_ == ModeVi)
    setExtraLineChar(vi_->getInsertMode());
  else
    setExtraLineChar(true);

  stateChanged();
}

void
CVEditFile::
setOverwriteMode(bool overwrite_mode)
{
  if (overwrite_mode == overwrite_mode_)
    return;

  overwrite_mode_ = overwrite_mode;

  stateChanged();
}

bool
CVEditFile::
getInsertMode() const
{
  return vi_->getInsertMode();
}

void
CVEditFile::
draw(uint width, uint height)
{
  width_  = width;
  height_ = height;

  CVEditMgrInst->setFont(getFont());

  bool cmd_line = false;

  if (getMode() == ModeVi) {
    CVEditVi *vi = getVi();

    cmd_line = vi->getCmdLineMode();
  }

  //------

  int w = getWidth ();
  int h = getHeight();

  //------

  CFontPtr font = getFont();

  char_width_  = font->getStringWidth("X");
  char_height_ = font->getCharHeight();

  num_rows_ = h/char_height_;
  num_cols_ = w/char_width_;

  //------

  bool filled = false;

  if (getIgnoreChanged()) {
    CVEditMgrInst->fill(getBg());

    filled = true;
  }

  bool number = getOptions().number;

  uint num_lines = getNumLines();

  indent_ = 0;

  if (number)
    indent_ = char_width_*((int) log10(num_lines) + 2);

  CIPoint2D p(indent_ - x_offset_, -y_offset_);

  line_num1_ = p.y/int(char_height_);
  line_num2_ = line_num1_ + num_rows_ - 1;

  int dy = p.y - char_height_*line_num1_;

  CIBBox2D bbox(p.x, p.y, p.x + w, p.y + h);

  setBBox(bbox);

  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  uint max_x = indent_;

  CVEditCursor *cursor = dynamic_cast<CVEditCursor *>(getCursor());

  const CIPoint2D &cpos = cursor->getPos();

  CIPoint2D pos;

  pos.x = indent_;
  pos.y = dy;

  for (uint line_num = 0; pline1 != pline2; ++pline1, ++line_num) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    line->setBBox(pos);

    if (int(line_num) >= line_num1_ && int(line_num) <= line_num2_) {
      const CIBBox2D &lbbox = line->getBBox();

      bool line_filled = filled;

      if (! line_filled && line->getChanged()) {
        CIBBox2D flbbox = lbbox;

        flbbox.setXMax(w);

        applyOffset(flbbox);

        CVEditMgrInst->fillRectangle(flbbox, getBg());

        line_filled = true;
      }

      if (number && (getIgnoreChanged() || line->getChanged())) {
        CVEditMgrInst->setForeground(CRGBA(1,0,1));

        CIPoint2D p(0, pos.y);

        applyOffset(p);

        CVEditMgrInst->drawString(p, CStrUtil::toString(line_num));
      }

      if (cpos.y == (int) line_num) {
        if (! cmd_line)
          line->draw(lbbox, cursor, line_filled);
        else
          line->draw(lbbox, NULL, line_filled);
      }
      else
        line->draw(lbbox, NULL, line_filled);

      line->setChanged(false);

      max_x = std::max(max_x, indent_ + lbbox.getWidth());
    }

    pos.y += char_height_;
  }

  vsize_ = CISize2D(max_x, (num_lines + 1)*char_height_);

  if (getIgnoreChanged() || getChanged()) {
    CIBBox2D bbox(indent_, pos.y, w, pos.y + h);

    applyOffset(bbox);

    CVEditMgrInst->fillRectangle(bbox, getBg());

    setChanged(false);
  }

  int y1 = h - char_height_ + 1;
  int y2 = h - 1;

  CIPoint2D p1(0    , y1);
  CIPoint2D p2(w - 1, y2);

  CIBBox2D bbox1(p1, p2);

  if (cmd_line) {
    CVEditVi *vi = getVi();

    vi->drawCmdLine(bbox1);
  }
  else
    drawStatus(bbox1);

  setIgnoreChanged(false);
}

void
CVEditFile::
drawStatus(const CIBBox2D &)
{
}

void
CVEditFile::
clearSelection()
{
  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    line->clearSelection();
  }
}

void
CVEditFile::
selectBBox(const CIBBox2D &bbox, bool clear)
{
  if (bbox_select_)
    selectInside(bbox, clear);
  else
    rangeSelect(bbox, clear);

  update();
}

void
CVEditFile::
selectInside(const CIBBox2D &bbox, bool clear)
{
  if (clear)
    clearSelection();

  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    line->selectInside(bbox);
  }
}

void
CVEditFile::
rangeSelect(const CIBBox2D &bbox, bool clear)
{
  int row1, col1, row2, col2;

  if (! pointToPos(bbox.getMin(), &row1, &col1) ||
      ! pointToPos(bbox.getMax(), &row2, &col2))
    return;

  rangeSelect(row1, col1, row2, col2, clear);
}

void
CVEditFile::
rangeSelect(int row1, int col1, int row2, int col2, bool clear)
{
  rangeSelect(CIPoint2D(col1, row1), CIPoint2D(col2, row2), clear);
}

void
CVEditFile::
rangeSelect(const CIPoint2D &start, const CIPoint2D &end, bool clear)
{
  if (clear)
    clearSelection();

  if      (end.y > start.y) {
    for (int col = start.x; col < (int) lineLength(start.y); ++col)
      selectChar(start.y, col);

    for (int row = start.y + 1; row < end.y; ++row)
      selectLine(row);

    for (int col = 0; col <= end.x; ++col)
      selectChar(end.y, col);
  }
  else if (start.y > end.y) {
    for (int col = start.x; col >= 0; --col)
      selectChar(start.y, col);

    for (int row = start.y - 1; row > end.y; --row)
      selectLine(row);

    for (int col = (int) lineLength(end.y) - 1; col >= end.x; --col)
      selectChar(end.y, col);
  }
  else if (end.y == start.y) {
    int x1 = std::min(start.x, end.x);
    int x2 = std::max(start.x, end.x);

    for (int col = x1; col <= x2; ++col)
      selectChar(start.y, col);
  }

  std::string str = getSelectedText();

  selectionChanged(str);

  update();
}

void
CVEditFile::
selectChar(int row, int col)
{
  CASSERT(row >= 0 && row <= (int) getNumLines(), "Invalid Line Num");

  CVEditLine *line = const_cast<CVEditLine *>(
    dynamic_cast<const CVEditLine *>(getEditLine(row)));

  line->selectChar(col);
}

void
CVEditFile::
selectLine(int row)
{
  CASSERT(row >= 0 && row <= (int) getNumLines(), "Invalid Line Num");

  CVEditLine *line = const_cast<CVEditLine *>(
    dynamic_cast<const CVEditLine *>(getEditLine(row)));

  line->selectAllChars();
}

void
CVEditFile::
setSelectionColor(const CRGBA &color)
{
  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    line->setSelectedCharColor(color);
  }

  update();
}

void
CVEditFile::
setSelectRange(const CIPoint2D &start, const CIPoint2D &end)
{
  select_start_ = start;
  select_end_   = end;
}

const CIPoint2D &
CVEditFile::
getSelectStart() const
{
  return select_start_;
}

const CIPoint2D &
CVEditFile::
getSelectEnd() const
{
  return select_end_;
}

std::string
CVEditFile::
getSelectedText() const
{
  std::string text;

  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    text += line->getSelectedText();
  }

  return text;
}

void
CVEditFile::
keyPress(const CKeyEvent &event)
{
  if      (mode_ == ModeNormal)
    gen_->processChar(event);
  else if (mode_ == ModeVi)
    vi_->processChar(event);

  update();
}

void
CVEditFile::
keyRelease(const CKeyEvent &)
{
}

void
CVEditFile::
mousePress(const CMouseEvent &event)
{
  if (event.getButton() == CBUTTON_LEFT) {
    int row, col;

    if (! pointToPos(event.getPosition(), &row, &col))
      return;

    press_row_ = row;
    press_col_ = col;

    update();
  }
}

void
CVEditFile::
mouseMotion(const CMouseEvent &event)
{
  if (event.getButton() == CBUTTON_LEFT) {
    int x, y;

    if (! pointToPos(event.getPosition(), &x, &y))
      return;

    release_row_ = x;
    release_col_ = y;

    rangeSelect(press_row_, press_col_, release_row_, release_col_, true);

    update();
  }
}

void
CVEditFile::
mouseRelease(const CMouseEvent &event)
{
  if (event.getButton() == CBUTTON_LEFT) {
    int x, y;

    if (! pointToPos(event.getPosition(), &x, &y))
      return;

    release_row_ = x;
    release_col_ = y;

    cursorTo(release_row_, release_col_);

    if (event.getCount() != 2)
      rangeSelect(press_row_, press_col_, release_row_, release_col_, true);
    else {
      CIPoint2D end = getPos();

      if (isWordChar(getChar(end.y, end.x))) {
        uint x = end.x;
        uint y = end.y;

        endWord(&y, &x);

        end = CIPoint2D(x, y);

        prevWord(&y, &x);

        CIPoint2D start(x, y);

        rangeSelect(start, end, true);
      }
      else
        rangeSelect(end, end, true);
    }

    update();
  }
}

void
CVEditFile::
scrollTop()
{
  const CIPoint2D &pos = getPos();

  scrollToPos(pos, CSCROLL_TYPE_TOP);
}

void
CVEditFile::
scrollMiddle()
{
  const CIPoint2D &pos = getPos();

  scrollToPos(pos, CSCROLL_TYPE_CENTER);
}

void
CVEditFile::
scrollBottom()
{
  const CIPoint2D &pos = getPos();

  scrollToPos(pos, CSCROLL_TYPE_BOTTOM);
}

void
CVEditFile::
scrollToPos(const CIPoint2D &pos, CScrollType type)
{
  CIBBox2D rect;

  posToRect(pos.y, pos.x, rect);

  scrollTo(rect, type);
}

void
CVEditFile::
scrollTo(const CIBBox2D &bbox, CScrollType type)
{
  //std::cerr << "Old: " << line_num1_ << " -> " << line_num2_ << std::endl;

  if      (type == CSCROLL_TYPE_TOP) {
    int py = bbox.getYMin();

    if (py <  0                 ) py = 0;
    if (py >= vsize_.getHeight()) py = vsize_.getHeight() - 1;

    int line_num = py/int(char_height_);

    //std::cerr << "Line Num: " << line_num << std::endl;

    y_offset_ = -line_num*char_height_ - 1;

    //std::cerr << "Y Offset: " <<  y_offset_ << std::endl;

    setIgnoreChanged(true);

    update();
  }
  else if (type == CSCROLL_TYPE_BOTTOM) {
    int py = bbox.getYMax();

    if (py <  0                 ) py = 0;
    if (py >= vsize_.getHeight()) py = vsize_.getHeight() - 1;

    int line_num = py/int(char_height_);

    //std::cerr << "Line Num: " << line_num << std::endl;

    y_offset_ = (num_rows_ - 1 - line_num)*char_height_ - 1;

    if (y_offset_ > 0) y_offset_ = 0;

    //std::cerr << "Y Offset: " <<  y_offset_ << std::endl;

    setIgnoreChanged(true);

    update();
  }
  else if (type == CSCROLL_TYPE_VISIBLE) {
    int py = bbox.getYMid();

    if (py <  0                 ) py = 0;
    if (py >= vsize_.getHeight()) py = vsize_.getHeight() - 1;

    int line_num = py/int(char_height_);

    //std::cerr << "Line Num: " << line_num << std::endl;

    if      (line_num < line_num1_) {
      y_offset_ = -line_num*char_height_ - 1;

      //std::cerr << "Y Offset: " << y_offset_ << std::endl;

      setIgnoreChanged(true);

      update();
    }
    else if (line_num > line_num2_) {
      y_offset_ = (num_rows_ - 1 - line_num)*char_height_ - 1;

      if (y_offset_ > 0) y_offset_ = 0;

      //std::cerr << "Y Offset: " << y_offset_ << std::endl;

      setIgnoreChanged(true);

      update();
    }
  }
}

uint
CVEditFile::
getPageLength() const
{
  return num_rows_;
}

int
CVEditFile::
getPageTop() const
{
  int row, col;

  CIPoint2D point(1, getCharHeight()/2);

  if (! pointToPos(point, &row, &col)) {
    row = 0;
    col = 0;
  }

  return row;
}

int
CVEditFile::
getPageBottom() const
{
  int row, col;

  CIPoint2D point(1, getHeight() - getCharHeight()/2);

  if (! pointToPos(point, &row, &col)) {
    row = line_num2_;
    col = 0;
  }

  if (row >= (int) getNumLines())
    row = getNumLines() - 1;

  return row;
}

bool
CVEditFile::
pointToPos(const CIPoint2D &point, int *row, int *col) const
{
  *col = 0;

  if (point.y < bbox_.getYMin()) {
    *row = line_num1_;
    return false;
  }

  if (point.y > bbox_.getYMax()) {
    *row = line_num2_;
    return false;
  }

  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for (uint line_num = 0; pline1 != pline2; ++pline1, ++line_num) {
    if (int(line_num) >= line_num1_ && int(line_num) <= line_num2_) {
      CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

      if (line->pointToCol(point, col)) {
        *row = line_num;
        return true;
      }
    }
  }

  return false;
}

bool
CVEditFile::
posToPoint(int row, int col, CIPoint2D &point) const
{
  CIBBox2D rect;

  if (! posToRect(row, col, rect))
    return false;

  point.x = rect.getXMin();
  point.y = rect.getYMax();

  return true;
}

bool
CVEditFile::
posToRect(int row, int col, CIBBox2D &rect) const
{
  const CVEditLine *line = dynamic_cast<const CVEditLine *>(getEditLine(row));

  if (line == NULL)
    return false;

  if (! line->colToRect(col, rect))
    rect = line->getBBox();

  return true;
}

void
CVEditFile::
setChanged(bool changed)
{
  CEditFile::setChanged(changed);
}

void
CVEditFile::
positionChanged()
{
  if (visual_) {
    select_end_ = getPos();

    rangeSelect(select_start_, select_end_, true);
  }
}

void
CVEditFile::
undo()
{
  CEditFile::undo();

  update();
}

void
CVEditFile::
redo()
{
  CEditFile::redo();

  update();
}

void
CVEditFile::
replayFile(const std::string &fileName)
{
  CFile file(fileName);

  if (file.exists() && ! file.isRegular())
    return;

  CKeyEvent event;

  event.setPosition(CIPoint2D(0,0));

  std::string str;

  while (file.readLine(str)) {
    uint len = str.size();

    if (len <= 0) continue;

    if (str[0] == ':') {
      bool quit1;

      runEdCmd(str.substr(1), quit1);

      if (quit1)
        quit();

      continue;
    }

    if (str[0] == '/') {
      bool quit1;

      runEdCmd(str, quit1);

      continue;
    }

    for (uint i = 0; i < len; ++i) {
      char c = str[i];

      event.setType(CEvent::charKeyType(c));
      event.setText(str.substr(i, 1));

      if (c == '\\') {
        ++i;

        char c1 = '\0';

        if (i < len)
          c1 = str[i];

        switch (c1) {
          case 'h': event.setType(CKEY_TYPE_BackSpace); break;
          case 'i': event.setType(CKEY_TYPE_TAB      ); break;
          case 'l': event.setType(CKEY_TYPE_FF       ); break;
          case 'n': event.setType(CKEY_TYPE_Return   ); break;
          case '@': event.setType(CKEY_TYPE_Escape   ); break;
          default : break;
        }
      }

      keyPress(event);
    }
  }
}

void
CVEditFile::
quit()
{
  exit(0);
}

void
CVEditFile::
setSyntax(CSyntax *syntax)
{
  syntax_ = std::unique_ptr<CSyntax>(syntax);

  updateSyntax();
}

void
CVEditFile::
updateSyntax()
{
  if (! syntax_) return;

  class Notifier : public CSyntaxNotifier {
   private:
    CVEditFile *file_;
    CVEditLine *line_;
    CRGBA       bg_;
    CRGBA       fg_[10];

   public:
    Notifier(CVEditFile *file) :
     file_(file) {
      assert(file_);

      bg_ = CRGBA(0,0,0);

      fg_[TOKEN_PREPRO ] = CRGBA(1.0,0.5,1.0);
      fg_[TOKEN_KEYWORD] = CRGBA(1.0,1.0,0.5);
      fg_[TOKEN_STRING ] = CRGBA(1.0,0.5,0.0);
      fg_[TOKEN_COMMENT] = CRGBA(0.5,0.5,1.0);
    }

    void setLine(CVEditLine *line) {
      line_ = line;
    }

    void addToken(uint, uint word_start, const std::string &word, CSyntaxToken token) {
      line_->addAnnotation(word_start, word_start + word.size() - 1, bg_, fg_[token]);
    }
  };

  Notifier notifier(this);

  syntax_->init();

  syntax_->setNotifier(&notifier);

  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1) {
    CVEditLine *line = dynamic_cast<CVEditLine *>(*pline1);

    notifier.setLine(line);

    line->clearAnnotations();

    syntax_->processLine(line->getString());
  }

  syntax_->term();
}

void
CVEditFile::
optionChanged(const std::string &name)
{
  if (name == "number" || name == "list") {
    setIgnoreChanged(true);

    update();
  }
}

void
CVEditFile::
drawFilledChar(const CIBBox2D &bbox, char c, const CRGBA &bg, const CRGBA &fg, bool filled) const
{
  if (! filled) {
    CIBBox2D bbox1 = bbox;

    applyOffset(bbox1);

    CVEditMgrInst->fillRectangle(bbox1, bg);
  }

  if (c != '\0') {
    CVEditMgrInst->setForeground(fg);

    CIPoint2D p(bbox.getXMin(), bbox.getYMin());

    applyOffset(p);

    CVEditMgrInst->drawChar(p, c);
  }
}

void
CVEditFile::
applyOffset(CIBBox2D &bbox) const
{
  CIPoint2D p1 = bbox.getLL();
  CIPoint2D p2 = bbox.getUR();

  applyOffset(p1);
  applyOffset(p2);

  bbox = CIBBox2D(p1, p2);
}
void
CVEditFile::
applyOffset(CIPoint2D &p) const
{
  p.x += x_offset_;
  p.y += y_offset_;
}
