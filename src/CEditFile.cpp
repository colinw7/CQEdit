#include <CEditFile.h>
#include <CEditMgr.h>
#include <CEditCursor.h>
#include <CEditEd.h>
#include <CEditFileUtil.h>
#include <CEditFileCharIterator.h>
#include <CFile.h>
#include <CRegExp.h>
#include <CStrUtil.h>
#include <CAssert.h>
#include <cstring>

CEditFile::
CEditFile() :
 util_         (NULL),
 fileName_     (),
 lines_        (),
 cursor_       (NULL),
 ed_           (NULL),
 cmd_mgr_      (this),
 undo_         (),
 group_list_   (),
 marks_        (),
 option_map_   (),
 options_      (),
 buffer_map_   (),
 findPattern_  (),
 extraLineChar_(false),
 changed_      (false),
 unsaved_      (false),
 msg_lines_    (),
 err_lines_    ()
{
  util_ = new CEditFileUtil(this);
}

CEditFile::
~CEditFile()
{
  resetUndo();

  //for_each(beginLine(), endLine(), CDeletePointer());

  delete cursor_;
  delete ed_;
  delete util_;
}

void
CEditFile::
init()
{
  cursor_ = CEditMgrInst->createCursor(this);

  addLine("");

  ed_ = CEditMgrInst->createEd(this);

  ed_->init();

  ed_->setEx(true);
}

void
CEditFile::
setFileName(const std::string &fileName)
{
  fileName_ = fileName;

  CEditBuffer &buffer = getBuffer('%');

  buffer.clear();

  buffer.addLine(fileName_, false);
}

const CIPoint2D &
CEditFile::
getPos() const
{
  return cursor_->getPos();
}

void
CEditFile::
setPos(const CIPoint2D &pos)
{
  if (! CASSERT(pos.y < (int) getNumLines() && pos.x <= (int) getLineEnd(pos.y) + 1,
                "Invalid Position")) return;

  if (pos != getPos())
    cursor_->setPos(pos);
}

bool
CEditFile::
isValidPos(const CIPoint2D &pos) const
{
  return (pos.y < (int) getNumLines() && pos.x <= (int) getLineEnd(pos.y));
}

uint
CEditFile::
getRow() const
{
  return getPos().y;
}

uint
CEditFile::
getCol() const
{
  return getPos().x;
}

void
CEditFile::
setRow(uint y)
{
  setPos(CIPoint2D(getCol(), y));
}

void
CEditFile::
setCol(uint x)
{
  setPos(CIPoint2D(x, getRow()));
}

void
CEditFile::
fixPos()
{
  bool changed = false;

  CIPoint2D pos = getPos();

  if (pos.y >= (int) getNumLines()) {
    pos.y = getNumLines() - 1;

    changed = true;
  }

  if (pos.y < 0) {
    pos.y = 0;

    changed = true;
  }

  if (pos.x > (int) getLineEnd(pos.y)) {
    pos.x = getLineEnd(pos.y);

    changed = true;
  }

  if (pos.x < 0) {
    pos.x = 0;

    changed = true;
  }

  if (changed)
    setPos(pos);
}

uint
CEditFile::
getNumLines() const
{
  return lines_.size();
}

bool
CEditFile::
isLinesEmpty() const
{
  return lines_.empty();
}

void
CEditFile::
setExtraLineChar(bool extraLineChar)
{
  extraLineChar_ = extraLineChar;
}

void
CEditFile::
setChanged(bool changed)
{
  changed_ = changed;
}

void
CEditFile::
setUnsaved(bool unsaved)
{
  unsaved_ = unsaved;
}

CEditFile::const_line_iterator
CEditFile::
beginLine() const
{
  return lines_.begin();
}

CEditFile::const_line_iterator
CEditFile::
endLine() const
{
  return lines_.end();
}

CEditFile::const_char_iterator
CEditFile::
beginChar() const
{
  return CharIterator(this);
}

CEditFile::const_char_iterator
CEditFile::
endChar() const
{
  return CharIterator(this).toEnd();
}

const CEditLine *
CEditFile::
getEditLine() const
{
  return getEditLine(getRow());
}

const CEditLine *
CEditFile::
getEditLine(uint line_num) const
{
  if (line_num >= getNumLines())
    return NULL;

  return lines_.getLine(line_num);
}

const CEditChar *
CEditFile::
getEditChar() const
{
  return getEditChar(getRow(), getCol());
}

const CEditChar *
CEditFile::
getEditChar(uint line_num, uint char_num) const
{
  if (line_num >= getNumLines())
    return NULL;

  return getEditLine(line_num)->getCharP(char_num);
}

std::string
CEditFile::
getLine() const
{
  return getEditLine()->getString();
}

std::string
CEditFile::
getLine(uint line_num) const
{
  return getEditLine(line_num)->getString();
}

char
CEditFile::
getChar() const
{
  return getChar(getRow(), getCol());
}

char
CEditFile::
getChar(uint line_num, uint char_num) const
{
  if (line_num >= getNumLines())
    return '\0';

  return getEditLine(line_num)->getChar(char_num);
}

bool
CEditFile::
loadLines(const std::string &fileName)
{
  subDeleteAllLines();

  if (fileName != "") {
    setFileName(fileName);

    CFile file(fileName);

    if (file.exists() && file.isRegular())
      addFileLines(fileName, 0);
  }

  if (getNumLines() == 0)
    addLine("");

  resetUndo();

  setUnsaved(false);

  return true;
}

bool
CEditFile::
addFileLines(const std::string &fileName)
{
  return addFileLines(fileName, getRow());
}

bool
CEditFile::
addFileLines(const std::string &fileName, uint line_num)
{
  CFile file(fileName);

  if (! file.exists() || ! file.isRegular())
    return false;

  startGroup();

  std::string str;

  while (file.readLine(str)) {
    uint len = str.size();

    if (len > 0 && str[len - 1] == '\r')
      str = str.substr(0, len - 1);

    addLine(line_num, str);

    ++line_num;
  }

  endGroup();

  return true;
}

bool
CEditFile::
saveLines(const std::string &fileName)
{
  CFile file(fileName);

  if (file.exists() && ! file.isRegular())
    return false;

  setFileName(fileName);

  LineList::const_iterator p1 = beginLine();
  LineList::const_iterator p2 = endLine  ();

  for ( ; p1 != p2; ++p1) {
    file.write((*p1)->getCString());

    file.putC('\n');
  }

  setUnsaved(true);

  return true;
}

void
CEditFile::
addLine(const std::string &str)
{
  addLine(getRow(), str);
}

void
CEditFile::
addLine(uint line_num, const std::string &str)
{
  CASSERT(line_num <= getNumLines(), "Invalid Line Num");

  CEditLine *line = CEditMgrInst->createLine(this);

  line->addChars(0, str);

  subAddLine(line_num, line);
}

void
CEditFile::
subAddLine(uint line_num, CEditLine *line)
{
  lines_.addLine(line_num, line);

  addUndo(new CEditDeleteLineCmd(&cmd_mgr_, line_num));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
addChars(uint line_num, uint char_num, const std::string &chars)
{
  CASSERT(line_num <= getNumLines(), "Invalid Line Num");

  subAddChars(line_num, char_num, chars);
}

void
CEditFile::
subAddChars(uint line_num, uint char_num, const std::string &chars)
{
  lines_.addLineChars(line_num, char_num, chars);

  addUndo(new CEditDeleteCharsCmd(&cmd_mgr_, line_num, char_num, chars.size()));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
moveLine(uint line_num1, int line_num2)
{
  CASSERT(line_num1 < getNumLines(), "Invalid Line Num");

  CASSERT(line_num2 >= -1 && line_num2 < (int) getNumLines(), "Invalid Line Num");

  subMoveLine(line_num1, line_num2);
}

void
CEditFile::
subMoveLine(uint line_num1, int line_num2)
{
  lines_.moveLine(line_num1, line_num2);

  addUndo(new CEditMoveLineCmd(&cmd_mgr_, line_num2, line_num1 - 1));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
copyLine(uint line_num1, uint line_num2)
{
  CASSERT(line_num1 < getNumLines(), "Invalid Line Num");

  CEditLine *line = getEditLine(line_num1)->dup();

  subAddLine(line_num2, line);
}

void
CEditFile::
deleteAllLines()
{
  subDeleteAllLines();

  addLine("");
}

void
CEditFile::
subDeleteAllLines()
{
  uint numLines = getNumLines();

  for (int i = numLines - 1; i >= 0; --i)
    subDeleteLine(i);
}

void
CEditFile::
deleteLine()
{
  deleteLine(getRow());
}

void
CEditFile::
deleteLine(uint line_num)
{
  if (! CASSERT(line_num < getNumLines(), "Invalid Line Num")) return;

  yankLines('\0', line_num, 1);

  subDeleteLine(line_num);

  if (isLinesEmpty())
    addLine("");

  fixPos();
}

void
CEditFile::
subDeleteLine(uint line_num)
{
  const std::string &str = getEditLine(line_num)->getString();

  lines_.deleteLine(line_num);

  addUndo(new CEditAddLineCmd(&cmd_mgr_, line_num, str));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
deleteWord()
{
  util_->deleteWord();
}

void
CEditFile::
deleteWord(uint line_num, uint char_num)
{
  util_->deleteWord(line_num, char_num);
}

void
CEditFile::
deleteEOL()
{
  util_->deleteEOL();
}

void
CEditFile::
deleteEOL(uint line_num, uint char_num)
{
  util_->deleteEOL(line_num, char_num);
}

void
CEditFile::
deleteTo(uint line_num, uint char_num)
{
  deleteTo(getRow(), getCol(), line_num, char_num);
}

void
CEditFile::
deleteTo(uint line_num1, uint char_num1, uint line_num2, uint char_num2)
{
  startGroup();

  if      (line_num1 < line_num2) {
    const CEditLine *line = getEditLine(line_num1);

    int num = line->getLength() - char_num1;

    deleteChars(line_num1, char_num1, num);

    for (uint i = 0; i < line_num2 - line_num1 - 1; ++i)
      deleteLine(line_num1 + i + 1);

    cursorDown(1);

    line_num2 = line_num1 + 1;

    deleteChars(line_num2, 0, char_num2);
  }
  else if (line_num2 < line_num1) {
    deleteChars(line_num1, 0, char_num1);

    for (uint i = 0; i < line_num1 - line_num2 - 1; ++i)
      deleteLine(line_num2 + i + 1);

    cursorUp(1);

    line_num2 = line_num1 - 1;

    const CEditLine *line = getEditLine(line_num2);

    int num = line->getLength() - char_num2;

    deleteChars(line_num2, char_num2, num);
  }
  else {
    if (char_num1 < char_num2) {
      int num = char_num2 - char_num1 + 1;

      deleteChars(line_num1, char_num1, num);
    }
    else {
      int num = char_num1 - char_num2 + 1;

      deleteChars(line_num1, char_num2, num);
    }
  }

  endGroup();
}

void
CEditFile::
deleteChar()
{
  deleteChar(getRow());
}

void
CEditFile::
deleteChar(uint line_num)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  const CEditLine *line = getEditLine(line_num);

  if (line->isEmpty())
    return;

  deleteChars(line_num, getCol(), 1);
}

void
CEditFile::
deleteChars(uint n)
{
  deleteChars(getRow(), getCol(), n);
}

void
CEditFile::
deleteChars(uint line_num, uint char_num, uint n)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  subDeleteChars(line_num, char_num, n);

  endGroup();
}

void
CEditFile::
subDeleteChars(uint line_num, uint char_num, uint n)
{
  const CEditLine *line = getEditLine(line_num);

  CASSERT(char_num + n <= line->getLength(), "Invalid Number of Chars");

  for (uint i = 0; i < n; ++i) {
    char c = line->getChar(char_num + i);

    addUndo(new CEditInsertCharCmd(&cmd_mgr_, line_num, char_num, c));
  }

  lines_.deleteLineChars(line_num, char_num, n);

  fixPos();

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
shiftLeft(uint line_num1, uint line_num2)
{
  CASSERT(line_num1 < getNumLines() && line_num2 < getNumLines(), "Invalid Line Num");

  util_->shiftLeft(line_num1, line_num2);
}

void
CEditFile::
shiftRight(uint line_num1, uint line_num2)
{
  CASSERT(line_num1 < getNumLines() && line_num2 < getNumLines(), "Invalid Line Num");

  util_->shiftRight(line_num1, line_num2);
}

void
CEditFile::
yankLines(char c, uint n)
{
  yankLines(c, getRow(), n);
}

void
CEditFile::
yankLines(char id, uint line_num, uint n)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  yankClear(id);

  for (uint i = 0; i < n; ++i) {
    uint line_num1 = line_num + i;

    const CEditLine *line = getEditLine(line_num1);

    uint len = line->getLength();

    subYankTo(id, line_num1, 0, line_num1, std::max(int(len) - 1, 0), true);
  }
}

void
CEditFile::
yankWords(char id, uint n)
{
  yankWords(id, getRow(), getCol(), n);
}

void
CEditFile::
yankWords(char id, uint line_num, uint char_num, uint n)
{
  yankClear(id);

  uint line_num1 = line_num;
  uint char_num1 = char_num;

  for (uint i = 0; i < n; ++i)
    endWord(&line_num1, &char_num1);

  subYankTo(id, line_num, char_num, line_num1, char_num1, false);
}

void
CEditFile::
yankChar(char id)
{
  yankChar(id, getRow(), getCol());
}

void
CEditFile::
yankChar(char id, uint line_num, uint char_num)
{
  yankClear(id);

  subYankTo(id, line_num, char_num, line_num, char_num, false);
}

void
CEditFile::
yankTo(char id, uint line_num, uint char_num, bool is_line)
{
  yankClear(id);

  subYankTo(id, getRow(), getCol(), line_num, char_num, is_line);
}

void
CEditFile::
yankTo(char id, uint line_num1, uint char_num1, uint line_num2, uint char_num2, bool is_line)
{
  yankClear(id);

  subYankTo(id, line_num1, char_num1, line_num2, char_num2, is_line);
}

void
CEditFile::
yankClear(char id)
{
  if (! inGroup()) {
    CEditBuffer &buffer = getBuffer(id);

    buffer.clear();
  }
}

void
CEditFile::
subYankTo(char id, uint line_num1, uint char_num1, uint line_num2, uint char_num2, bool is_line)
{
  CASSERT(line_num1 < getNumLines() && line_num2 < getNumLines(), "Invalid Line Num");

  std::vector<CEditBufferLine> lines;

  if      (line_num1 < line_num2) {
    const CEditLine *line1 = getEditLine(line_num1);

    const std::string &str1 = line1->getString();

    lines.push_back(CEditBufferLine(str1.substr(char_num1), is_line));

    for (uint i = line_num1 + 1; i < line_num2; ++i) {
      const CEditLine *line = getEditLine(i);

      lines.push_back(CEditBufferLine(line->getString(), true));
    }

    const CEditLine *line2 = getEditLine(line_num2);

    const std::string &str2 = line2->getString();

    lines.push_back(CEditBufferLine(str2.substr(0, char_num2), is_line));
  }
  else if (line_num2 < line_num1) {
    const CEditLine *line2 = getEditLine(line_num2);

    const std::string &str2 = line2->getString();

    lines.push_back(CEditBufferLine(str2.substr(char_num2), is_line));

    for (uint i = line_num2 + 1; i < line_num1; ++i) {
      const CEditLine *line = getEditLine(i);

      lines.push_back(CEditBufferLine(line->getString(), true));
    }

    const CEditLine *line1 = getEditLine(line_num1);

    const std::string &str1 = line1->getString();

    lines.push_back(CEditBufferLine(str1.substr(0, char_num1), is_line));
  }
  else {
    const CEditLine *line1 = getEditLine(line_num1);

    const std::string &str1 = line1->getString();

    std::string str2;

    if (char_num1 < char_num2)
      str2 = str1.substr(char_num1, char_num2 - char_num1 + 1);
    else
      str2 = str1.substr(char_num2, char_num1 - char_num2 + 1);

    lines.push_back(CEditBufferLine(str2, is_line));
  }

  std::vector<CEditBufferLine>::iterator p1 = lines.begin();
  std::vector<CEditBufferLine>::iterator p2 = lines.end  ();

  if (inGroup()) {
    CEditGroup *group = group_list_.back();

    for ( ; p1 != p2; ++p1)
      group->addLine((*p1).line, (*p1).newline);
  }
  else {
    CEditBuffer &buffer = getBuffer(id);

    for ( ; p1 != p2; ++p1)
      buffer.addLine((*p1).line, (*p1).newline);
  }
}

void
CEditFile::
pasteAfter(char id)
{
  pasteAfter(id, getRow(), getCol());
}

void
CEditFile::
pasteBefore(char id)
{
  pasteBefore(id, getRow(), getCol());
}

void
CEditFile::
pasteAfter(char id, uint line_num, uint char_num)
{
  CEditBuffer &buffer = getBuffer(id);

  uint num_lines = buffer.lines.size();

  if (num_lines == 0)
    return;

  CEditBufferLine *sline = &buffer.lines[0];
  CEditBufferLine *eline = NULL;

  if (num_lines > 1)
    eline = &buffer.lines[num_lines - 1];

  startGroup();

  if (! sline->newline) {
    if (eline)
      splitLine(line_num, char_num);

    const CEditLine *line = getEditLine(line_num);

    if (char_num < line->getLength())
      addChars(line_num, char_num + 1, sline->line);
    else
      addChars(line_num, char_num, sline->line);
  }
  else {
    ++line_num;

    addLine(line_num, sline->line);

    cursorDown  (1);
    cursorToLeft();

    ++line_num;
  }

  for (uint i = 1; i < num_lines - 1; ++i) {
    CEditBufferLine *mline = &buffer.lines[i];

    addLine(line_num, mline->line);

    cursorDown  (1);
    cursorToLeft();

    ++line_num;
  }

  if (eline) {
    if (! eline->newline) {
      cursorDown  (1);
      cursorToLeft();

      ++line_num;

      addChars(line_num, 0, eline->line);
    }
    else {
      addLine(line_num, eline->line);

      cursorDown  (1);
      cursorToLeft();
    }
  }

  endGroup();
}

void
CEditFile::
pasteBefore(char id, uint line_num, uint char_num)
{
  CEditBuffer &buffer = getBuffer(id);

  uint num_lines = buffer.lines.size();

  if (num_lines == 0)
    return;

  CEditBufferLine *sline = &buffer.lines[0];
  CEditBufferLine *eline = NULL;

  if (num_lines > 1)
    eline = &buffer.lines[num_lines - 1];

  startGroup();

  if (! sline->newline) {
    if (eline)
      splitLine(line_num, char_num);

    addChars(line_num, char_num, sline->line);
  }
  else
    addLine(line_num, sline->line);

  for (uint i = 1; i < num_lines - 1; ++i) {
    CEditBufferLine *mline = &buffer.lines[i];

    addLine(line_num + i, mline->line);
  }

  if (eline) {
    if (! eline->newline)
      addChars(line_num + num_lines - 1, 0, eline->line);
    else
      addLine(line_num + num_lines - 1, eline->line);
  }

  endGroup();
}

uint
CEditFile::
lineLength(uint line_num)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  const CEditLine *line = getEditLine(line_num);

  return line->getLength();
}

void
CEditFile::
setChar(char c)
{
  setChar(getRow(), getCol(), c);
}

void
CEditFile::
setChar(uint line_num, uint char_num, char c)
{
  if (isLinesEmpty())
    addLine(0, "");

  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  lines_.setLineChar(line_num, char_num, c);
}

void
CEditFile::
insertChar(char c)
{
  insertChar(getRow(), getCol(), c);

  cursorRight(1);
}

void
CEditFile::
insertChar(uint line_num, uint char_num, char c)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  subInsertChar(line_num, char_num, c);

  endGroup();
}

void
CEditFile::
subInsertChar(uint line_num, uint char_num, char c)
{
  if (isLinesEmpty()) {
    CEditLine *line = CEditMgrInst->createLine(this);

    subAddLine(0, line);

    line_num = 0;
  }

  lines_.addLineChar(line_num, char_num, c);

  addUndo(new CEditDeleteCharsCmd(&cmd_mgr_, line_num, char_num, 1));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
replaceChar(char c)
{
  replaceChar(getRow(), getCol(), c);

  cursorRight(1);
}

void
CEditFile::
replaceChar(uint line_num, uint char_num, char c)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  subReplaceChar(line_num, char_num, c);

  endGroup();
}

void
CEditFile::
subReplaceChar(uint line_num, uint char_num, char c)
{
  if (isLinesEmpty()) {
    CEditLine *line = CEditMgrInst->createLine(this);

    subAddLine(0, line);
  }

  const CEditLine *line = getEditLine(line_num);

  char c1 = line->getChar(char_num);

  lines_.replaceLineChar(line_num, char_num, c);

  addUndo(new CEditReplaceCharCmd(&cmd_mgr_, line_num, char_num, c1));

  setChanged(true);
  setUnsaved(true);
}

void
CEditFile::
splitLine()
{
  splitLine(getRow(), getCol());
}

void
CEditFile::
splitLine(uint line_num, uint char_num)
{
  startGroup();

  subSplitLine(line_num, char_num);

  endGroup();
}

void
CEditFile::
subSplitLine(uint line_num, uint char_num)
{
  CEditLine *line = CEditMgrInst->createLine(this);

  lines_.addLine(line_num + 1, line);

  lines_.splitLine(line_num, char_num);

  addUndo(new CEditJoinLineCmd(&cmd_mgr_, line_num));

  setChanged(true);
  setUnsaved(true);

  cursorDown  (1);
  cursorToLeft();
}

void
CEditFile::
joinLine()
{
  joinLine(getRow());
}

void
CEditFile::
joinLine(uint line_num)
{
  CASSERT(line_num < getNumLines() - 1, "Invalid Line Num");

  startGroup();

  subJoinLine(line_num);

  endGroup();
}

void
CEditFile::
subJoinLine(uint line_num)
{
  const CEditLine *line1 = getEditLine(line_num);

  uint len1 = line1->getLength();

  lines_.joinLine(line_num);

  addUndo(new CEditSplitLineCmd(&cmd_mgr_, line_num, len1));

  subDeleteLine(line_num + 1);
}

void
CEditFile::
newLineBelow()
{
  uint line_num = getRow();

  addLine(line_num + 1, "");

  cursorDown(1);

  cursorToLeft();
}

void
CEditFile::
newLineAbove()
{
  addLine(getRow(), "");

  cursorToLeft();
}

bool
CEditFile::
cursorLeft(uint n)
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  bool rc = cursorLeft(n, &y, &x);

  setPos(CIPoint2D(x, y));

  return rc;
}

bool
CEditFile::
cursorLeft(uint n, uint *, uint *char_num)
{
  for (uint i = 0; i < n; ++i) {
    if (*char_num <= 0)
      return false;

    --(*char_num);
  }

  return true;
}

bool
CEditFile::
cursorRight(uint n)
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  bool rc = cursorRight(n, &y, &x);

  setPos(CIPoint2D(x, y));

  return rc;
}

bool
CEditFile::
cursorRight(uint n, uint *line_num, uint *char_num)
{
  for (uint i = 0; i < n; ++i) {
    if (*char_num >= getLineEnd(*line_num))
      return false;

    ++(*char_num);
  }

  return true;
}

bool
CEditFile::
cursorUp(uint n)
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  bool rc = cursorUp(n, &y, &x);

  setPos(CIPoint2D(x, y));

  return rc;
}

bool
CEditFile::
cursorUp(uint n, uint *line_num, uint *char_num)
{
  bool rc = true;

  for (uint i = 0; i < n; ++i) {
    if (*line_num <= 0) {
      rc = false;
      break;
    }

    --(*line_num);
  }

  if (*char_num >= getLineEnd(*line_num))
    *char_num = getLineEnd(*line_num);

  return rc;
}

bool
CEditFile::
cursorDown(uint n)
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  bool rc = cursorDown(n, &y, &x);

  setPos(CIPoint2D(x, y));

  return rc;
}

bool
CEditFile::
cursorDown(uint n, uint *line_num, uint *char_num)
{
  if (n > 0 && isLinesEmpty())
    return false;

  bool rc = true;

  for (uint i = 0; i < n; ++i) {
    if (*line_num >= getNumLines() - 1) {
      rc = false;
      break;
    }

    ++(*line_num);
  }

  if (*char_num >= getLineEnd(*line_num))
    *char_num = getLineEnd(*line_num);

  return rc;
}

void
CEditFile::
cursorToLeft()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorToLeft(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorToLeft(uint *, uint *char_num)
{
  *char_num = 0;
}

void
CEditFile::
cursorToRight()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorToRight(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorToRight(uint *line_num, uint *char_num)
{
  *char_num = getLineEnd(*line_num);
}

void
CEditFile::
cursorSkipSpace()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorSkipSpace(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorSkipSpace(uint *line_num, uint *char_num)
{
  const CEditLine *line = getEditLine(*line_num);

  while (*char_num < line->getLength() - 1 && isspace(line->getChar(*char_num)))
    ++(*char_num);
}

void
CEditFile::
cursorFirstNonBlankUp()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorFirstNonBlankUp(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorFirstNonBlankUp(uint *line_num, uint *char_num)
{
  cursorUp(1, line_num, char_num);

  cursorFirstNonBlank(line_num, char_num);
}

void
CEditFile::
cursorFirstNonBlankDown()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorFirstNonBlankDown(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorFirstNonBlankDown(uint *line_num, uint *char_num)
{
  cursorDown(1, line_num, char_num);

  cursorFirstNonBlank(line_num, char_num);
}

void
CEditFile::
cursorFirstNonBlank()
{
  const CIPoint2D pos = getPos();

  uint x = pos.x;
  uint y = pos.y;

  cursorFirstNonBlank(&y, &x);

  setPos(CIPoint2D(x, y));
}

void
CEditFile::
cursorFirstNonBlank(uint *line_num, uint *char_num)
{
  cursorToLeft(line_num, char_num);

  cursorSkipSpace(line_num, char_num);
}

void
CEditFile::
cursorTo(uint line_num, uint char_num)
{
  CIPoint2D pos(char_num, line_num);

  setPos(pos);
}

void
CEditFile::
nextWord()
{
  util_->nextWord();
}

void
CEditFile::
nextWord(uint *line_num, uint *char_num)
{
  util_->nextWord(line_num, char_num);
}

void
CEditFile::
nextWORD()
{
  util_->nextWORD();
}

// a WORD is a series of non-blank characters
void
CEditFile::
nextWORD(uint *line_num, uint *char_num)
{
  util_->nextWORD(line_num, char_num);
}

void
CEditFile::
prevWord()
{
  util_->prevWord();
}

void
CEditFile::
prevWord(uint *line_num, uint *char_num)
{
  util_->prevWord(line_num, char_num);
}

void
CEditFile::
prevWORD()
{
  util_->prevWORD();
}

void
CEditFile::
prevWORD(uint *line_num, uint *char_num)
{
  util_->prevWord(line_num, char_num);
}

void
CEditFile::
endWord()
{
  util_->endWord();
}

void
CEditFile::
endWord(uint *line_num, uint *char_num)
{
  util_->endWord(line_num, char_num);
}

void
CEditFile::
endWORD()
{
  util_->endWORD();
}

void
CEditFile::
endWORD(uint *line_num, uint *char_num)
{
  util_->endWORD(line_num, char_num);
}

bool
CEditFile::
getWord(std::string &word)
{
  return util_->getWord(word);
}

bool
CEditFile::
getWord(uint line_num, uint char_num, std::string &word)
{
  return util_->getWord(line_num, char_num, word);
}

void
CEditFile::
nextSentence()
{
  util_->nextSentence();
}

void
CEditFile::
nextSentence(uint *line_num, uint *char_num)
{
  util_->nextSentence(line_num, char_num);
}

void
CEditFile::
prevSentence()
{
  util_->prevSentence();
}

void
CEditFile::
prevSentence(uint *line_num, uint *char_num)
{
  util_->prevSentence(line_num, char_num);
}

void
CEditFile::
nextParagraph()
{
  util_->nextParagraph();
}

void
CEditFile::
nextParagraph(uint *line_num, uint *char_num)
{
  util_->nextParagraph(line_num, char_num);
}

void
CEditFile::
prevParagraph()
{
  util_->prevParagraph();
}

void
CEditFile::
prevParagraph(uint *line_num, uint *char_num)
{
  util_->prevParagraph(line_num, char_num);
}

bool
CEditFile::
nextLine(uint *line_num, uint *char_num)
{
  const CEditLine *line = getEditLine(*line_num);

  *char_num = line->getLength() - 1;

  if (*line_num >= getNumLines() - 1)
    return false;

  ++(*line_num);

  *char_num = 0;

  return true;
}

bool
CEditFile::
prevLine(uint *line_num, uint *char_num)
{
  *char_num = 0;

  if (*line_num <= 0)
    return false;

  --(*line_num);

  const CEditLine *line = getEditLine(*line_num);

  *char_num = std::max((int) line->getLength() - 1, 0);

  return true;
}

void
CEditFile::
nextSection()
{
  util_->nextSection();
}

void
CEditFile::
nextSection(uint *line_num, uint *char_num)
{
  util_->nextSection(line_num, char_num);
}

void
CEditFile::
prevSection()
{
  util_->prevSection();
}

void
CEditFile::
prevSection(uint *line_num, uint *char_num)
{
  util_->prevSection(line_num, char_num);
}

void
CEditFile::
swapChar()
{
  swapChar(getRow(), getCol());
}

void
CEditFile::
swapChar(uint line_num, uint char_num)
{
  const CEditLine *line = getEditLine(line_num);

  char c = line->getChar(char_num);

  if      (islower(c)) c = toupper(c);
  else if (isupper(c)) c = tolower(c);

  replace(line_num, char_num, c);
}

bool
CEditFile::
findNext(const std::string &pattern)
{
  return findNext(pattern, getRow(), getCol() + 1);
}

bool
CEditFile::
findNext(const std::string &pattern, uint *fline_num, uint *fchar_num)
{
  return findNext(pattern, getRow(), getCol() + 1, fline_num, fchar_num);
}

bool
CEditFile::
findNext(const std::string &pattern, uint line_num, int char_num)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1);
}

bool
CEditFile::
findNext(const std::string &pattern, uint line_num, int char_num, uint *fline_num, uint *fchar_num)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1, fline_num, fchar_num);
}

bool
CEditFile::
findNext(const std::string &pattern, uint line_num1, int char_num1, int line_num2, int char_num2)
{
  uint fline_num, fchar_num;

  if (! findNext(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
CEditFile::
findNext(const std::string &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num)
{
  setFindPattern(CRegExp(pattern));

  if (getEditLine(line_num1)->findNext(pattern, char_num1, -1, fchar_num)) {
    *fline_num = line_num1;
    return true;
  }

  for (int i = line_num1 + 1; i <= line_num2 - 1; ++i) {
    if (getEditLine(i)->findNext(pattern, 0, -1, fchar_num)) {
      *fline_num = i;
      return true;
    }
  }

  if (getEditLine(line_num2)->findNext(pattern, 0, char_num2, fchar_num)) {
    *fline_num = line_num2;
    return true;
  }

  return false;
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint *len)
{
  return findNext(pattern, getRow(), getCol() + 1, len);
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len)
{
  return findNext(pattern, getRow(), getCol() + 1, fline_num, fchar_num, len);
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint line_num, int char_num, uint *len)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1, len);
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint line_num, int char_num,
         uint *fline_num, uint *fchar_num, uint *len)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1,
                  fline_num, fchar_num, len);
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *len)
{
  uint fline_num, fchar_num;

  if (! findNext(pattern, line_num1, char_num1, line_num2, char_num2,
                 &fline_num, &fchar_num, len))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
CEditFile::
findNext(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len)
{
  setFindPattern(pattern);

  uint spos, epos;

  if (getEditLine(line_num1)->findNext(pattern, char_num1, -1, &spos, &epos)) {
    *fline_num = line_num1;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  for (int i = (int) line_num1 + 1; i <= line_num2 - 1; ++i) {
    if (getEditLine(i)->findNext(pattern, 0, -1, &spos, &epos)) {
      *fline_num = i;
      *fchar_num = spos;
      if (len) *len = epos - spos + 1;
      return true;
    }
  }

  if (getEditLine(line_num2)->findNext(pattern, 0, char_num2, &spos, &epos)) {
    *fline_num = line_num2;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  return false;
}

bool
CEditFile::
findPrev(const std::string &pattern)
{
  return findPrev(pattern, getRow(), getCol() - 1);
}

bool
CEditFile::
findPrev(const std::string &pattern, uint *fline_num, uint *fchar_num)
{
  return findPrev(pattern, getRow(), getCol() - 1, fline_num, fchar_num);
}

bool
CEditFile::
findPrev(const std::string &pattern, uint line_num, int char_num)
{
  return findPrev(pattern, line_num, char_num, 0, 0);
}

bool
CEditFile::
findPrev(const std::string &pattern, uint line_num, int char_num, uint *fline_num, uint *fchar_num)
{
  return findPrev(pattern, line_num, char_num, 0, 0, fline_num, fchar_num);
}

bool
CEditFile::
findPrev(const std::string &pattern, uint line_num1, int char_num1, int line_num2, int char_num2)
{
  uint fline_num, fchar_num;

  if (! findPrev(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
CEditFile::
findPrev(const std::string &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num)
{
  setFindPattern(CRegExp(pattern));

  if (getEditLine(line_num1)->findPrev(pattern, char_num1, 0, fchar_num)) {
    *fline_num = line_num1;
    return true;
  }

  for (int i = line_num1 - 1; i >= line_num2 + 1; --i) {
    if (getEditLine(i)->findPrev(pattern, -1, 0, fchar_num)) {
      *fline_num = i;
      return true;
    }
  }

  if (getEditLine(line_num2)->findPrev(pattern, -1, char_num2, fchar_num)) {
    *fline_num = line_num2;
    return true;
  }

  return false;
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint *len)
{
  return findPrev(pattern, getRow(), getCol() - 1, len);
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len)
{
  return findPrev(pattern, getRow(), getCol() - 1, fline_num, fchar_num, len);
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint line_num, int char_num, uint *len)
{
  return findPrev(pattern, line_num, char_num, 0, 0, len);
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint line_num, int char_num,
         uint *fline_num, uint *fchar_num, uint *len)
{
  return findPrev(pattern, line_num, char_num, 0, 0, fline_num, fchar_num, len);
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *len)
{
  uint fline_num, fchar_num;

  if (! findPrev(pattern, line_num1, char_num1, line_num2, char_num2,
                 &fline_num, &fchar_num, len))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
CEditFile::
findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len)
{
  setFindPattern(pattern);

  uint spos, epos;

  if (getEditLine(line_num1)->findPrev(pattern, char_num1, 0, &spos, &epos)) {
    *fline_num = line_num1;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  for (int i = line_num1 - 1; i >= line_num2 + 1; --i) {
    if (getEditLine(i)->findPrev(pattern, -1, 0, &spos, &epos)) {
      *fline_num = i;
      *fchar_num = spos;
      if (len) *len = epos - spos + 1;
      return true;
    }
  }

  if (getEditLine(line_num2)->findPrev(pattern, -1, char_num2, &spos, &epos)) {
    *fline_num = line_num2;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  return false;
}

bool
CEditFile::
findNextChar(char c, bool multiline)
{
  return findNextChar(getRow(), getCol(), c, multiline);
}

bool
CEditFile::
findNextChar(const std::string &str, bool multiline)
{
  return findNextChar(getRow(), getCol(), str, multiline);
}

bool
CEditFile::
findNextChar(uint line_num, int char_num, char c, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  ++char_num;

  while (true) {
    const CEditLine *line = getEditLine(line_num);

    for (uint i = char_num; i < line->getLength(); ++i) {
      if (line->getChar(i) == c) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == getNumLines() - 1)
      break;

    ++line_num;

    char_num = 0;
  }

  return false;
}

bool
CEditFile::
findNextChar(uint line_num, int char_num, const std::string &str, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  ++char_num;

  while (true) {
    const CEditLine *line = getEditLine(line_num);

    const char *str1 = str.c_str();

    for (uint i = char_num; i < line->getLength(); ++i) {
      if (strchr(str1, line->getChar(i))) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == getNumLines() - 1)
      break;

    ++line_num;

    char_num = 0;
  }

  return false;
}

bool
CEditFile::
findPrevChar(char c, bool multiline)
{
  return findPrevChar(getRow(), getCol(), c, multiline);
}

bool
CEditFile::
findPrevChar(const std::string &str, bool multiline)
{
  return findPrevChar(getRow(), getCol(), str, multiline);
}

bool
CEditFile::
findPrevChar(uint line_num, int char_num, char c, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  --char_num;

  const CEditLine *line = getEditLine(line_num);

  while (true) {
    for (int i = char_num; i >= 0; --i) {
      if (line->getChar(i) == c) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == 0)
      break;

    --line_num;

    line = getEditLine(line_num);

    char_num = line->getLength() - 1;
  }

  return false;
}

bool
CEditFile::
findPrevChar(uint line_num, int char_num, const std::string &str, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  --char_num;

  const CEditLine *line = getEditLine(line_num);

  while (true) {
    const char *str1 = str.c_str();

    for (int i = char_num; i >= 0; --i) {
      if (strchr(str1, line->getChar(i))) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == 0)
      break;

    --line_num;

    line = getEditLine(line_num);

    char_num = line->getLength() - 1;
  }

  return false;
}

bool
CEditFile::
replace(uint line_num, uint char_num, char c)
{
  return replace(line_num, char_num, char_num, std::string(&c, 1));
}

bool
CEditFile::
replace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  bool rc = subReplace(line_num, char_num1, char_num2, replaceStr);

  endGroup();

  return rc;
}

bool
CEditFile::
subReplace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr)
{
  const CEditLine *line = getEditLine(line_num);

  std::string old = line->getSubString(char_num1, char_num2);

  lines_.replaceLineChars(line_num, char_num1, char_num2, replaceStr);

  char_num2 = char_num1 + replaceStr.size() - 1;

  addUndo(new CEditReplaceCmd(&cmd_mgr_, line_num, char_num1, char_num2, old));

  setChanged(true);
  setUnsaved(true);

  return true;
}

void
CEditFile::
markReturn()
{
  setMarkPos("'");
}

void
CEditFile::
setMarkPos(const std::string &mark)
{
  setMarkPos(mark, getRow(), getCol());
}

void
CEditFile::
setMarkPos(const std::string &mark, uint line_num, uint char_num)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  marks_[mark] = CIPoint2D(char_num, line_num);
}

bool
CEditFile::
getMarkPos(const std::string &mark, uint *line_num, uint *char_num) const
{
  MarkList::const_iterator p = marks_.find(mark);

  if (p == marks_.end())
    return false;

  CIPoint2D pos = (*p).second;

  if (pos.x < 0 && pos.y < 0)
    return false;

  *line_num = pos.y;
  *char_num = pos.x;

  return true;
}

void
CEditFile::
unsetMarkPos(const std::string &mark)
{
  marks_[mark] = CIPoint2D(-1, -1);
}

void
CEditFile::
clearLineMarks(uint line_num)
{
  MarkList::iterator p1 = marks_.begin();
  MarkList::iterator p2 = marks_.end  ();

  for ( ; p1 != p2; ++p1) {
    if ((*p1).second.y == (int) line_num)
      (*p1).second = CIPoint2D(-1, -1);
  }
}

void
CEditFile::
startGroup()
{
  CEditGroup *group = new CEditGroup;

  group_list_.push_back(group);

  undo_.startGroup();

  addUndo(new CEditMoveToCmd(&cmd_mgr_, getRow(), getCol()));
}

void
CEditFile::
endGroup()
{
  undo_.endGroup();

  if (! inGroup())
    return;

  CEditGroup *group = group_list_.back();

  group_list_.pop_back();

  CEditGroup::LineList::iterator p1 = group->lines.begin();
  CEditGroup::LineList::iterator p2 = group->lines.end  ();

  CEditBuffer &buffer = getBuffer('\0');

  for ( ; p1 != p2; ++p1)
    buffer.lines.push_back(*p1);

  delete group;
}

bool
CEditFile::
inGroup() const
{
  return ! group_list_.empty();
}

void
CEditFile::
addUndo(CEditCmd *cmd)
{
  if (! undo_.locked())
    undo_.addUndo(cmd);
  else
    delete cmd;
}

void
CEditFile::
undo()
{
  undo_.undo();

  fixPos();
}

void
CEditFile::
redo()
{
  undo_.redo();

  fixPos();
}

bool
CEditFile::
canUndo() const
{
  return undo_.canUndo();
}

bool
CEditFile::
canRedo() const
{
  return undo_.canRedo();
}

void
CEditFile::
undoLine()
{
  const CEditCursor::LastLine &last_line = cursor_->getLastLine();

  if (last_line.set) {
    std::string text = last_line.text;

    cursor_->updateLastLine();

    lines_.replaceLineChars(last_line.row, text);
  }
}

void
CEditFile::
resetUndo()
{
  undo_.clear();
}

bool
CEditFile::
isWordChar(char c)
{
  return (isalnum(c) || c == '_');
}

uint
CEditFile::
getLineEnd(uint line_num) const
{
  if (line_num >= getNumLines())
    return 0;

  const CEditLine *line = getEditLine(line_num);

  uint len = line->getLength();

  if (len > 0 && ! isExtraLineChar())
    --len;

  return len;
}

bool
CEditFile::
runEdCmd(const std::string &str, bool &quit)
{
  msg_lines_.clear();
  err_lines_.clear();

  quit = false;

  bool rc = true;

  std::vector<std::string> words;

  CStrUtil::toWords(str, words);

  uint num_words = words.size();

  std::string cmd = (num_words > 0 ? words[0] : "");

  if      (cmd == "set") {
    std::string name, arg;

    for (uint i = 1; i < num_words; ++i) {
      std::string::size_type pos = words[i].find('=');

      if (pos != std::string::npos) {
        name = words[i].substr(0, pos);
        arg  = words[i].substr(pos + 1);
      }
      else {
        name = words[i];
        arg  = "";
      }

      setOptionString(name, arg);
    }
  }
  else if (cmd == "registers") {
    addMsgLine("-- Registers --");

    BufferMap::const_iterator p1, p2;

    for (p1 = buffer_map_.begin(), p2 = buffer_map_.end(); p1 != p2; ++p1) {
      char               c      = (*p1).first;
      const CEditBuffer &buffer = (*p1).second;

      uint numLines = buffer.getNumLines();

      for (uint i = 0; i < numLines; ++i) {
        std::string msg;

        msg += std::string(&c, 1);
        msg += "    ";
        msg += buffer.getLine(i).getLine();

        addMsgLine(msg);
      }
    }
  }
  else if (cmd == "marks") {
    addMsgLine("-- Marks --");

    MarkList::const_iterator p1, p2;

    for (p1 = marks_.begin(), p2 = marks_.end(); p1 != p2; ++p1) {
      const std::string &str1 = (*p1).first;
      const CIPoint2D   &pos  = (*p1).second;

      std::string msg;

      msg += str1 + " ";
      msg += CStrUtil::toString(pos.y) + " ";
      msg += CStrUtil::toString(pos.x);

      addMsgLine(msg);
    }
  }
  else if (cmd == "exit" || cmd == "quit") {
    quit = true;
  }
  else if (cmd == "position") {
    const CIPoint2D pos = getPos();

    std::string msg = CStrUtil::toString(pos.x) + " " + CStrUtil::toString(pos.y);

    addMsgLine(msg);
  }
  else if (cmd == "replay") {
    if (num_words > 1)
      replayFile(words[1]);
  }
  else {
    rc = ed_->execCmd(str);

    if (rc)
      quit = ed_->isQuit();
  }

  if (! msg_lines_.empty())
    displayMessage(msg_lines_);

  if (! err_lines_.empty())
    displayError(err_lines_);

  return rc;
}

void
CEditFile::
setOptionString(const std::string &name, const std::string &arg)
{
  std::string name1 = name;
  std::string arg1  = arg;

  if (name1.size() > 2 && name1.substr(0, 2) == "no")  {
    if (arg1 == "")
      arg1 = "0";

    name1 = name.substr(2);
  }
  else {
    if (arg1 == "")
      arg1 = "1";
  }

  option_map_[name1] = arg1;

  if      (name1 == "ignorecase") {
    options_.ignorecase = CStrUtil::toBool(arg1);

    ed_->setCaseSensitive(! options_.ignorecase);
  }
  else if (name1 == "list")
    options_.list = CStrUtil::toBool(arg1);
  else if (name1 == "number")
    options_.number = CStrUtil::toBool(arg1);
  else if (name1 == "shiftwidth")
    options_.shiftwidth = CStrUtil::toInteger(arg1);
  else if (name1 == "showmatch")
    options_.showmatch = CStrUtil::toBool(arg1);

  optionChanged(name1);
}

bool
CEditFile::
getOptionString(const std::string &name, std::string &value) const
{
  OptionMap::const_iterator p = option_map_.find(name);

  if (p == option_map_.end())
    return false;

  value = (*p).second;

  return true;
}

void
CEditFile::
optionChanged(const std::string &)
{
}

CEditBuffer &
CEditFile::
getBuffer(char c)
{
  return buffer_map_[c];
}

void
CEditFile::
replayFile(const std::string &)
{
  CASSERT(0, "Not Implemented");
}

void
CEditFile::
addMsgLine(const std::string &msg)
{
  msg_lines_.push_back(msg);
}

void
CEditFile::
addErrLine(const std::string &msg)
{
  err_lines_.push_back(msg);
}

void
CEditFile::
displayMessage(const StringList &lines)
{
  uint num_lines = lines.size();

  for (uint i = 0; i < num_lines; ++i) {
    std::cerr << lines[i] << std::endl;
  }
}

void
CEditFile::
displayError(const StringList &lines)
{
  uint num_lines = lines.size();

  for (uint i = 0; i < num_lines; ++i) {
    std::cerr << lines[i] << std::endl;
  }
}

void
CEditFile::
print(std::ostream &os) const
{
  LineList::const_iterator pline1 = beginLine();
  LineList::const_iterator pline2 = endLine  ();

  for ( ; pline1 != pline2; ++pline1)
    os << **pline1 << std::endl;
}

std::ostream &
operator<<(std::ostream &os, const CEditFile &file)
{
  file.print(os);

  return os;
}

//--------

CEditFileLines::
~CEditFileLines()
{
  clear();
}

void
CEditFileLines::
clear()
{
  LineList::const_iterator p1 = begin();
  LineList::const_iterator p2 = end  ();

  for ( ; p1 != p2; ++p1)
    delete *p1;

  lines_.clear();
}

const CEditLine *
CEditFileLines::
getLine(uint line_num) const
{
  return lines_[line_num];
}

void
CEditFileLines::
addLine(uint line_num, CEditLine *line)
{
  if (line_num == lines_.size()) {
    lines_.push_back(line);

    line->setChanged(true);
  }
  else {
    lines_.push_back(lines_[lines_.size() - 1]);

    for (int i = (int) lines_.size() - 2; i >= (int) line_num; --i)
      lines_[i + 1] = lines_[i];

    lines_[line_num] = line;

    for (uint i = line_num; i < lines_.size(); ++i)
      lines_[i]->setChanged(true);
  }
}

void
CEditFileLines::
addLineChar(uint line_num, uint char_num, char c)
{
  CEditLine *line = lines_[line_num];

  line->insertChar(char_num, c);

  line->setChanged(true);
}

void
CEditFileLines::
addLineChars(uint line_num, uint char_num, const std::string &chars)
{
  CEditLine *line = lines_[line_num];

  line->addChars(char_num, chars);

  line->setChanged(true);
}

void
CEditFileLines::
setLineChar(uint line_num, uint char_num, char c)
{
  CEditLine *line = lines_[line_num];

  line->setChar(char_num, c);

  line->setChanged(true);
}

void
CEditFileLines::
replaceLineChar(uint line_num, uint char_num, char c)
{
  CEditLine *line = lines_[line_num];

  line->replaceChar(char_num, c);

  line->setChanged(true);
}

void
CEditFileLines::
replaceLineChars(uint line_num, const std::string &str)
{
  CEditLine *line = lines_[line_num];

  line->replace(str);

  line->setChanged(true);
}

void
CEditFileLines::
replaceLineChars(uint line_num, uint char_num1, uint char_num2, const std::string &str)
{
  CEditLine *line = lines_[line_num];

  line->replace(char_num1, char_num2, str);

  line->setChanged(true);
}

void
CEditFileLines::
moveLine(uint line_num1, int line_num2)
{
  CEditLine *line = lines_[line_num1];

  if      (line_num2 > (int) line_num1) {
    for (int i = line_num1 + 1; i <= line_num2; ++i)
      lines_[i - 1] = lines_[i];

    lines_[line_num2] = line;

    for (int i = line_num1; i <= line_num2; ++i)
      lines_[i]->setChanged(true);
  }
  else if (line_num2 < (int) line_num1) {
    for (int i = line_num1 - 1; i > line_num2; --i)
      lines_[i + 1] = lines_[i];

    lines_[line_num2 + 1] = line;

    for (int i = line_num2; i <= (int) line_num1; ++i)
      lines_[i]->setChanged(true);
  }
}

void
CEditFileLines::
splitLine(uint line_num, uint char_num)
{
  CEditLine *line1 = lines_[line_num    ];
  CEditLine *line2 = lines_[line_num + 1];

  line1->split(line2, char_num);
}

void
CEditFileLines::
joinLine(uint line_num)
{
  CEditLine *line1 = lines_[line_num    ];
  CEditLine *line2 = lines_[line_num + 1];

  line1->join(line2);
}

void
CEditFileLines::
deleteLine(uint line_num)
{
  CEditLine *line = lines_[line_num];

  uint num_lines = lines_.size();

  for (uint i = line_num + 1; i < num_lines; ++i)
    lines_[i - 1] = lines_[i];

  lines_.pop_back();

  delete line;

  num_lines = lines_.size();

  for (uint i = line_num; i < num_lines; ++i)
    lines_[i]->setChanged(true);
}

void
CEditFileLines::
deleteLineChars(uint line_num, uint char_num, uint n)
{
  CEditLine *line = lines_[line_num];

  for (uint i = 0; i < n; ++i)
    line->deleteChar(char_num);

  line->setChanged(true);
}

//------------

CEditFile::CharIterator::
CharIterator() :
 impl_()
{
  impl_ = new CEditFileCharIterator;
}

CEditFile::CharIterator::
CharIterator(const CEditFile *file) :
 impl_()
{
  impl_ = new CEditFileCharIterator(file);
}

CEditFile::CharIterator::
CharIterator(const CharIterator &rhs)
{
  impl_ = new CEditFileCharIterator;

  *impl_ = *rhs.impl_;
}

CEditFile::CharIterator::
~CharIterator()
{
  delete impl_;
}

CEditFile::CharIterator &
CEditFile::CharIterator::
operator=(const CharIterator &rhs)
{
  *impl_ = *rhs.impl_;

  return *this;
}

const CEditFile::CharIterator::value_type &
CEditFile::CharIterator::
operator->() const
{
  return **impl_;
}

const CEditFile::CharIterator::value_type &
CEditFile::CharIterator::
operator*() const
{
  return **impl_;
}

CEditFile::CharIterator &
CEditFile::CharIterator::
operator++()
{
  ++(*impl_);

  return *this;
}

bool
CEditFile::CharIterator::
operator==(const CharIterator &i)
{
  return (*impl_ == *i.impl_);
}

bool
CEditFile::CharIterator::
operator!=(const CharIterator &i)
{
  return (*impl_ != *i.impl_);
}

CEditFile::CharIterator &
CEditFile::CharIterator::
toEnd()
{
  impl_->toEnd();

  return *this;
}
