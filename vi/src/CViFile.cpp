#include <CViFile.h>

CViFile::
CViFile()
{
}

bool
CViFile::
load(const std::string &filename)
{
}

bool
CViFile::
save()
{
}

uint
CViFile::
getNumLines() const
{
  return lines_.size();
}

bool
CViFile::
isLinesEmpty() const
{
  return lines_.empty();
}


CViFile::LinesCI::
CViFile::
linesBegin() const
{
  return lines_.begin();
}

CViFile::LinesCI::
CViFile::
linesEnd() const
{
  return lines_.end();
}

CViLine *
CViFile::
getLine(uint line_num)
{
  return lines_.getLine(line_num);
}

void
CViFile::
clearLines()
{
  lines_.clear();
}

CViLine *
CViFile::
addLineAfter(uint line_num, const std::string &str)
{
  CViLine *line = new CViLine(str);

  lines_.addLineAfter(line_num, line);

  return line;
}

CViLine *
CViFile::
addLineBefore(uint line_num, const std::string &str)
{
  CViLine *line = new CViLine(str);

  lines_.addLineAfter(line_num, line);

  return line;
}

void
CViFile::
deleteLine(uint line_num)
{
  lines_.deleteLine(line_num);
}

//------

CViLines::
CViLines() :
 lines_()
{
}

CViLines::
~CViLines()
{
  clear();
}

void
CViLines::
clear()
{
  LineList::const_iterator p1 = begin();
  LineList::const_iterator p2 = end  ();

  for ( ; p1 != p2; ++p1)
    delete *p1;

  lines_.clear();
}

CViLine *
CViLines::
getLine(uint line_num)
{
  return lines_[line_num];
}

void
CViLines::
addLineAfter(uint line_num, CViLine *line)
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
CViLines::
addLineBefore(uint line_num, CViLine *line)
{
  lines_.push_back(lines_[lines_.size() - 1]);

  for (int i = (int) lines_.size() - 2; i >= (int) line_num - 1; --i)
    lines_[i + 1] = lines_[i];

  lines_[line_num - 1] = line;

  for (uint i = line_num - 1; i < lines_.size(); ++i)
    lines_[i]->setChanged(true);
}

void
CViLines::
deleteLine(uint line_num)
{
}

//-------

typedef char CViChar;

class CViLine {
 public:
  typedef std::string::const_iterator CharsCI;

  CViLine(const std::string &line);

  uint getLength() const;

  bool isEmpty() const;

  CharsCI beginChar() const;
  CharsCI endChar  () const;

  CViChar &getChar(uint char_num);

  void clear();

  void addCharAfter (uint char_num, const CViChar &c);
  void addCharBefore(uint char_num, const CViChar &c);

  void deleteChar(uint char_num);

 private:
  typedef std::vector<CViChar> Chars;

  Chars chars_;
};
