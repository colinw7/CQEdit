#include <CTextFile.h>
#include <CFile.h>

CTextFile::
CTextFile(const char *filename)
{
  notifyMgr_ = new CTextFileNotifyMgr(this);

  if (filename)
    read(filename);
  else
    addLineAfter("");
}

CTextFile::
~CTextFile()
{
}

void
CTextFile::
addNotifier(CTextFileNotifier *notifier)
{
  notifyMgr_->addNotifier(notifier);
}

void
CTextFile::
removeNotifier(CTextFileNotifier *notifier)
{
  notifyMgr_->removeNotifier(notifier);
}

bool
CTextFile::
read(const char *filename)
{
  assert(filename);

  fileInfo_.fileName = filename;

  CFile file(filename);

  if (! file.exists() || ! file.isRegular())
    return false;

  removeAllLines();

  std::vector<std::string> lines;

  file.toLines(lines);

  uint numLines = lines.size();

  for (uint i = 0; i < numLines; ++i) {
    addLineAfter(lines[i]);

    rmoveTo(0, 1);
  }

  moveTo(0, 0);

  notifyMgr_->notifyFileOpened();

  cursor_.moveTo(0, 0);

  notifyMgr_->notifyPositionChanged();

  return true;
}

bool
CTextFile::
write(const char *filename)
{
  assert(filename);

  fileInfo_.fileName = filename;

  CFile file(filename);

  if (file.exists() && ! file.isRegular())
    return false;

  uint numLines = getNumLines();

  for (uint y = 0; y < numLines; ++y) {
    file.write(lines_[y]->getString());

    file.putC('\n');
  }

  return false;
}

void
CTextFile::
removeAllLines()
{
  moveTo(0, 0);

  uint numLines = getNumLines();

  for (int y = numLines - 1; y >= 0; --y) {
    CTextLine *line = lines_[y];

    const std::string &str = line->getString();

    notifyMgr_->notifyLineDeleted(str, y);
  }

  lines_.clear();
}

void
CTextFile::
moveTo(uint x, uint y)
{
  uint numLines = getNumLines();

  if (numLines > 0)
    y = std::min(y, numLines - 1);
  else
    y = 0;

  moveToLine(y);

  uint lineLen = getLineLength();

  if (lineLen > 0)
    x = std::min(x, lineLen);
  else
    x = 0;

  moveToChar(x);

  notifyMgr_->notifyPositionChanged();
}

void
CTextFile::
rmoveTo(int dx, int dy)
{
  uint x, y;

  getPos(&x, &y);

  if (dx < 0 && uint(-dx) > x) dx = -x;
  if (dy < 0 && uint(-dy) > y) dy = -y;

  moveTo(x + dx, y + dy);
}

void
CTextFile::
moveToLine(int y)
{
  if (y == int(cursor_.getY())) return;

  cursor_.setY(y);

  notifyMgr_->notifyPositionChanged();
}

void
CTextFile::
moveToChar(int x)
{
  if (x == int(cursor_.getX())) return;

  cursor_.setX(x);

  notifyMgr_->notifyPositionChanged();
}

void
CTextFile::
getPos(uint *x, uint *y) const
{
  *x = cursor_.getX();
  *y = cursor_.getY();
}

void
CTextFile::
setFileName(const std::string &fileName)
{
  fileInfo_.fileName = fileName;
}

const std::string &
CTextFile::
getFileName()
{
  return fileInfo_.fileName;
}

char
CTextFile::
getChar() const
{
  const CTextLine *line;

  if (! getLine(&line))
    return '\0';

  uint x, y;

  getPos(&x, &y);

  return line->getChar(x);
}

const std::string &
CTextFile::
getLine() const
{
  const CTextLine *line;

  if (! getLine(&line)) {
    static std::string empty;

    return empty;
  }

  return line->getString();
}

const std::string &
CTextFile::
getLine(uint y) const
{
  static std::string empty;

  uint numLines = getNumLines();

  if (y >= numLines)
    return empty;

  return lines_[y]->getString();
}

uint
CTextFile::
getLineLength() const
{
  const CTextLine *line;

  if (! getLine(&line))
    return 0;

  return line->getLength();
}

uint
CTextFile::
getNumLines() const
{
  return lines_.size();
}

void
CTextFile::
addCharAfter(char c)
{
  CTextLine *line;

  if (! getLine(&line)) {
    std::string str(&c, 1);

    addLineAfter(str);

    return;
  }

  uint x, y;

  getPos(&x, &y);

  line->addCharAfter(x, c);

  notifyMgr_->notifyCharAdded(c, y, x);
}

void
CTextFile::
addCharBefore(char c)
{
  CTextLine *line;

  if (! getLine(&line)) {
    std::string str(&c, 1);

    addLineAfter(str);

    return;
  }

  uint x, y;

  getPos(&x, &y);

  if (x > 0) {
    line->addCharBefore(x, c);

    notifyMgr_->notifyCharAdded(c, y, x);
  }
  else {
    line->addCharBefore(0, c);

    notifyMgr_->notifyCharAdded(c, y, 0);
  }
}

void
CTextFile::
addLineAfter(const std::string &str)
{
  uint y        = cursor_.getY();
  uint numLines = getNumLines();

  CTextLine *line = allocLine(str);

  if (numLines == 0) {
    lines_.push_back(line);

    notifyMgr_->notifyLineAdded(str, 0);
  }
  else {
    lines_.push_back(NULL);

    for (int i = int(numLines) - 1; i >= int(y + 1); --i)
      lines_[i + 1] = lines_[i];

    lines_[y + 1] = line;

    notifyMgr_->notifyLineAdded(str, y + 1);
  }
}

void
CTextFile::
addLineBefore(const std::string &str)
{
  uint y        = cursor_.getY();
  uint numLines = getNumLines();

  CTextLine *line = allocLine(str);

  if (numLines == 0) {
    lines_.push_back(line);

    notifyMgr_->notifyLineAdded(str, 0);
  }
  else {
    lines_.push_back(NULL);

    for (int i = int(numLines) - 1; i >= int(y); --i)
      lines_[i + 1] = lines_[i];

    lines_[y] = line;

    notifyMgr_->notifyLineAdded(str, y);
  }
}

void
CTextFile::
deleteCharAt()
{
  CTextLine *line;

  if (! getLine(&line))
    return;

  if (line->getLength() == 0)
    return;

  uint x, y;

  getPos(&x, &y);

  char c = line->getChar(x);

  line->deleteCharAt(x);

  notifyMgr_->notifyCharDeleted(c, y, x);
}

void
CTextFile::
deleteCharBefore()
{
  CTextLine *line;

  if (! getLine(&line))
    return;

  uint x, y;

  getPos(&x, &y);

  if (line->getLength() == 0)
    return;

  if (x > 0) {
    char c = line->getChar(x);

    line->deleteCharBefore(x);

    notifyMgr_->notifyCharDeleted(c, y, x);
  }
  else {
    char c = line->getChar(0);

    line->deleteCharAt(0);

    notifyMgr_->notifyCharDeleted(c, y, 0);
  }
}

void
CTextFile::
deleteLineAt()
{
  uint y        = cursor_.getY();
  uint numLines = getNumLines();

  if (y >= numLines) return;

  CTextLine *line = lines_[y];

  std::string str = line->getString();

  for (int i = int(y); i < int(numLines) - 1; ++i)
    lines_[i] = lines_[i + 1];

  lines_.pop_back();

  oldLines_.push_back(line);

  notifyMgr_->notifyLineDeleted(str, y);
}

void
CTextFile::
deleteLineBefore()
{
  uint y        = cursor_.getY();
  uint numLines = getNumLines();

  if (y == 0 || y > numLines) return;

  CTextLine *line = lines_[y - 1];

  std::string str = line->getString();

  for (int i = int(y); i < int(numLines); ++i)
    lines_[i - 1] = lines_[i];

  lines_.pop_back();

  oldLines_.push_back(line);

  notifyMgr_->notifyLineDeleted(str, y - 1);
}

void
CTextFile::
replaceChar(char c)
{
  CTextLine *line;

  if (! getLine(&line))
    return;

  uint x, y;

  getPos(&x, &y);

  char c1 = line->getChar(x);

  line->replaceChar(x, c);

  notifyMgr_->notifyCharReplaced(c1, c, y, x);
}

void
CTextFile::
replaceLine(const std::string &str)
{
  uint x = cursor_.getX();
  uint y = cursor_.getY();

  CTextLine *line;

  if (! getLine(&line))
    return;

  std::string str1 = line->getString();

  line->replaceString(str);

  moveToChar(x);

  notifyMgr_->notifyLineReplaced(str1, str, y);
}

bool
CTextFile::
getLine(CTextLine **line)
{
  *line = NULL;

  uint y = cursor_.getY();

  uint numLines = getNumLines();

  if (y >= numLines)
    return false;

  *line = lines_[y];

  return true;
}

bool
CTextFile::
getLine(const CTextLine **line) const
{
  *line = NULL;

  uint y = cursor_.getY();

  uint numLines = getNumLines();

  if (y >= numLines)
    return false;

  *line = lines_[y];

  return true;
}

void
CTextFile::
startGroup()
{
  notifyMgr_->notifyStartGroup();
}

void
CTextFile::
endGroup()
{
  notifyMgr_->notifyEndGroup();
}

CTextFile::LineIterator
CTextFile::
beginLine()
{
  return LineIterator(LineIteratorImplP(new SimpleLineIteratorImpl(this)));
}

CTextFile::LineIterator
CTextFile::
endLine()
{
  return LineIterator(LineIteratorImplP(new SimpleLineIteratorImpl(this))).toEnd();
}

CTextLine *
CTextFile::
allocLine(const std::string &str)
{
  if (oldLines_.empty())
    return new CTextLine(str);

  CTextLine *line = oldLines_.back();

  line->setLine(str);

  oldLines_.pop_back();

  return line;
}

uint
CTextFile::
getPageTop() const
{
  if (pageTop_ >= 0)
    return pageTop_;
  else
    return 0;
}

void
CTextFile::
setPageTop(uint pos)
{
  pageTop_ = pos;
}

uint
CTextFile::
getPageBottom() const
{
  if (pageBottom_ >= 0)
    return pageBottom_;
  else
    return getNumLines() - 1;
}

void
CTextFile::
setPageBottom(uint pos)
{
  pageBottom_ = pos;
}

//------

char
CTextLine::
getChar(uint x) const
{
  return line_[x];
}

const std::string &
CTextLine::
getString() const
{
  return line_;
}

void
CTextLine::
addCharAfter(uint x, char c)
{
  uint len = line_.size();

  if (x >= len)
    line_ += c;
  else
    line_ = line_.substr(0, x + 1) + c + line_.substr(x + 1);
}

void
CTextLine::
addCharBefore(uint x, char c)
{
  uint len = line_.size();

  if (x >= len)
    line_ += c;
  else
    line_ = line_.substr(0, x) + c + line_.substr(x);
}

void
CTextLine::
deleteCharAt(uint x)
{
  uint len = line_.size();

  assert(x < len);

  line_ = line_.substr(0, x) + line_.substr(x + 1);
}

void
CTextLine::
deleteCharBefore(uint x)
{
  uint len = line_.size();

  assert(x < len);

  line_ = line_.substr(0, x - 1) + line_.substr(x);
}

void
CTextLine::
replaceChar(uint x, char c)
{
  uint len = line_.size();

  assert(x < len);

  line_[x] = c;
}

void
CTextLine::
replaceString(const std::string &str)
{
  line_ = str;
}

//--------

CTextFileNotifyMgr::
CTextFileNotifyMgr(CTextFile *file) :
 file_(file)
{
}

void
CTextFileNotifyMgr::
addNotifier(CTextFileNotifier *notifier)
{
  notifierList_.push_back(notifier);
}

void
CTextFileNotifyMgr::
removeNotifier(CTextFileNotifier *notifier)
{
  notifierList_.remove(notifier);
}

void
CTextFileNotifyMgr::
notifyFileOpened()
{
  uint x, y;

  file_->getPos(&x, &y);

  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->fileOpened(file_->getFileName());
}

void
CTextFileNotifyMgr::
notifyPositionChanged()
{
  uint x, y;

  file_->getPos(&x, &y);

  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->positionChanged(x, y);
}

void
CTextFileNotifyMgr::
notifyLineAdded(const std::string &line, uint line_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->lineAdded(line, line_num);
}

void
CTextFileNotifyMgr::
notifyLineDeleted(const std::string &line, uint line_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->lineDeleted(line, line_num);
}

void
CTextFileNotifyMgr::
notifyLineReplaced(const std::string &line1, const std::string &line2, uint line_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->lineReplaced(line1, line2, line_num);
}

void
CTextFileNotifyMgr::
notifyCharAdded(char c, uint line_num, uint char_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->charAdded(c, line_num, char_num);
}

void
CTextFileNotifyMgr::
notifyCharDeleted(char c, uint line_num, uint char_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->charDeleted(c, line_num, char_num);
}

void
CTextFileNotifyMgr::
notifyCharReplaced(char c1, char c2, uint line_num, uint char_num)
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->charReplaced(c1, c2, line_num, char_num);
}

void
CTextFileNotifyMgr::
notifyStartGroup()
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->startGroup();
}

void
CTextFileNotifyMgr::
notifyEndGroup()
{
  NotifierList::const_iterator p1, p2;

  for (p1 = notifierList_.begin(), p2 = notifierList_.end(); p1 != p2; ++p1)
    (*p1)->endGroup();
}

//------

CTextFileNotifier::
CTextFileNotifier()
{
}

void
CTextFileNotifier::
fileOpened(const std::string &)
{
}

void
CTextFileNotifier::
positionChanged(uint, uint)
{
}

void
CTextFileNotifier::
lineAdded(const std::string &, uint)
{
}

void
CTextFileNotifier::
lineDeleted(const std::string &, uint)
{
}

void
CTextFileNotifier::
lineReplaced(const std::string &, const std::string &, uint)
{
}

void
CTextFileNotifier::
charAdded(char, uint, uint)
{
}

void
CTextFileNotifier::
charDeleted(char, uint, uint)
{
}

void
CTextFileNotifier::
charReplaced(char, char, uint, uint)
{
}

void
CTextFileNotifier::
startGroup()
{
}

void
CTextFileNotifier::
endGroup()
{
}
