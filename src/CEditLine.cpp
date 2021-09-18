#include <CEditLine.h>
#include <CEditChar.h>
#include <CEditMgr.h>
#include <CStrUtil.h>
#include <CRegExp.h>
#include <CAssert.h>
#include <CFuncs.h>
#include <algorithm>
#include <cstring>

CEditLine::
CEditLine(CEditFile *file) :
 file_(file), util_(this), chars_()
{
}

CEditLine::
CEditLine(const CEditLine &line) :
 file_(line.file_), util_(this), chars_(line.chars_)
{
}

CEditLine::
~CEditLine()
{
}

CEditLine &
CEditLine::
operator=(const CEditLine &line)
{
  file_    = line.file_;
  chars_   = line.chars_;
  changed_ = true;

  return *this;
}

CEditLine *
CEditLine::
dup() const
{
  return new CEditLine(*this);
}

void
CEditLine::
addChars(uint pos, const std::string &line)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  uint num = line.size();
  if (num == 0) return;

  std::vector<CEditChar *> chars;

  for (uint i = 0; i < num; ++i) {
    auto *c1 = CEditMgrInst->createChar(this);

    c1->setChar(line[i]);

    chars.push_back(c1);
  }

  chars_.addChars(pos, chars);

  setChanged(true);
}

void
CEditLine::
addChar(uint pos, char c)
{
  addChars(pos, std::string(&c, 1));
}

void
CEditLine::
addChars(uint pos, uint num, char c)
{
  if (num <= 0) return;

  std::string str;

  str.resize(num, c);

  addChars(pos, str);
}

uint
CEditLine::
getLength() const
{
  return chars_.size();
}

bool
CEditLine::
isEmpty() const
{
  return chars_.empty();
}

bool
CEditLine::
isBlank() const
{
  return util_.isBlank();
}

bool
CEditLine::
isSentenceEnd(uint pos, uint *n) const
{
  return util_.isSentenceEnd(pos, n);
}

bool
CEditLine::
isSection(uint pos, uint *n) const
{
  return util_.isSection(pos, n);
}

CEditLineChars::const_iterator
CEditLine::
beginChar() const
{
  return chars_.begin();
}

CEditLineChars::const_iterator
CEditLine::
endChar() const
{
  return chars_.end();
}

void
CEditLine::
clear()
{
  chars_.clear();

  setChanged(true);
}

const CEditChar *
CEditLine::
getCharP(uint pos) const
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return nullptr;

  if (pos == getLength())
    return nullptr;

  return chars_.getChar(pos);
}

char
CEditLine::
getChar(uint pos) const
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return '\0';

  if (pos == getLength())
    return '\0';

  return getCharP(pos)->getChar();
}

void
CEditLine::
setChar(uint pos, char c)
{
  if (! CASSERT(pos < getLength(), "Invalid Char Num")) return;

  auto *c1 = CEditMgrInst->createChar(this);

  c1->setChar(c);

  chars_.setChar(pos, c1);

  setChanged(true);
}

void
CEditLine::
insertChar(uint pos, char c)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  auto *c1 = CEditMgrInst->createChar(this);

  c1->setChar(c);

  chars_.insertChar(pos, c1);

  setChanged(true);
}

void
CEditLine::
replaceChar(uint pos, char c)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  if (pos < getLength())
    setChar(pos, c);
  else
    insertChar(pos, c);
}

void
CEditLine::
deleteChars(uint pos, uint num)
{
  if (! CASSERT(pos < getLength() + num - 1, "Invalid Char Num")) return;

  for (uint i = 0; i < num; ++i)
    deleteChar(pos);
}

void
CEditLine::
deleteChar(uint pos)
{
  if (! CASSERT(pos < getLength(), "Invalid Char Num")) return;

  chars_.deleteChar(pos);
}

bool
CEditLine::
findNext(const std::string &pattern, int char_num1, int char_num2, uint *char_num) const
{
  return util_.findNext(pattern, char_num1, char_num2, char_num);
}

bool
CEditLine::
findNext(const CRegExp &pattern, int char_num1, int char_num2, uint *spos, uint *epos) const
{
  return util_.findNext(pattern, char_num1, char_num2, spos, epos);
}

bool
CEditLine::
findPrev(const std::string &pattern, int char_num1, int char_num2, uint *char_num) const
{
  return util_.findPrev(pattern, char_num1, char_num2, char_num);
}

bool
CEditLine::
findPrev(const CRegExp &pattern, int char_num1, int char_num2, uint *spos, uint *epos) const
{
  return util_.findPrev(pattern, char_num1, char_num2, spos, epos);
}

void
CEditLine::
replace(const std::string &str)
{
  replace(0, getLength() - 1, str);
}

void
CEditLine::
replace(int spos, int epos, const std::string &str)
{
  if (epos < 0)
    epos = getLength() - 1;

  if (! CASSERT(spos >= 0 && spos < (int) getLength(), "Invalid Char Num"))
    return;

  if (! CASSERT(epos >= 0 && epos < (int) getLength(), "Invalid Char Num"))
    return;

  if (! CASSERT(spos <= epos, "Invalid Range"))
    return;

  int len1 = epos - spos + 1;
  int len2 = str.size();

  if      (len1 > len2)
    deleteChars(spos, len1 - len2);
  else if (len2 > len1)
    addChars(spos + len1 - 1, len2 - len1, '\0');

  for (int i = 0; i < len2; ++i)
    setChar(spos + i, str[i]);

  setChanged(true);
}

void
CEditLine::
split(CEditLine *line, uint pos)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  uint num_chars = getLength();

  for (int i = pos; i < (int) num_chars; ++i)
    line->chars_.addChar(chars_.getChar(i)->dup());

  for (int i = pos; i < (int) num_chars; ++i)
    chars_.deleteChar(getLength() - 1);

  setChanged(true);

  line->setChanged(true);
}

void
CEditLine::
join(CEditLine *line)
{
  uint num_chars = line->getLength();

  for (uint i = 0; i < num_chars; ++i)
    chars_.addChar(line->chars_.getChar(i)->dup());

  line->clear();

  setChanged(true);
}

std::string
CEditLine::
getString() const
{
  return getCString();
}

std::string
CEditLine::
getSubString(int spos, int epos) const
{
  if (epos < 0)
    epos = getLength() - 1;

  return getSubCString(spos, epos);
}

const char *
CEditLine::
getCString() const
{
  if (! isEmpty())
    return getSubCString(0, getLength() - 1);
  else
    return "";
}

const char *
CEditLine::
getSubCString(int spos, int epos) const
{
  if (! CASSERT(spos >= 0 && epos < (int) getLength() && spos <= epos,
                "Invalid Char Pos")) return nullptr;

  static char *buffer;
  static uint  buffer_size;

  uint len = epos - spos + 1;

  if (len >= buffer_size) {
    delete [] buffer;

    buffer_size = len + 1;
    buffer      = new char [buffer_size];
  }

  int j = 0;

  for (int i = spos; i <= epos; ++i)
    buffer[j++] = getChar(i);

  buffer[j] = 0;

  return buffer;
}

void
CEditLine::
setChanged(bool changed)
{
  changed_ = changed;
}

void
CEditLine::
print(std::ostream &os) const
{
  os << chars_;
}

std::ostream &
operator<<(std::ostream &os, const CEditLine &line)
{
  line.print(os);

  return os;
}

//-------

CEditLineChars::
~CEditLineChars()
{
  clear();
}

CEditLineChars::
CEditLineChars(const CEditLineChars &chars) :
 chars_()
{
  auto pchar1 = chars.chars_.begin();
  auto pchar2 = chars.chars_.end  ();

  for ( ; pchar1 != pchar2; ++pchar1)
    chars_.push_back((*pchar1)->dup());
}

CEditLineChars &
CEditLineChars::
operator=(const CEditLineChars &chars)
{
  clear();

  auto pchar1 = chars.chars_.begin();
  auto pchar2 = chars.chars_.end  ();

  for ( ; pchar1 != pchar2; ++pchar1)
    chars_.push_back((*pchar1)->dup());

  return *this;
}

void
CEditLineChars::
clear()
{
  std::for_each(chars_.begin(), chars_.end(), CDeletePointer());

  chars_.clear();
}

void
CEditLineChars::
setChar(uint pos, CEditChar *c)
{
  auto **p = &chars_[pos];

  delete *p;

  *p = c;
}

const CEditChar *
CEditLineChars::
getChar(uint char_num) const
{
  return chars_[char_num];
}

CEditLineChars::iterator
CEditLineChars::
getCharI(uint pos)
{
  auto p = begin();

  advance(p, pos);

  return p;
}

void
CEditLineChars::
addChar(CEditChar *c)
{
  chars_.push_back(c);
}

void
CEditLineChars::
addChars(uint pos, const std::vector<CEditChar *> &chars)
{
  uint num = chars.size();

  if (num == 0)
    return;

  uint old_len = chars_.size();

  for (uint i = 0; i < num; ++i)
    chars_.push_back(nullptr);

  uint new_len = old_len + num;

  for (int i = (int) old_len - 1, j = (int) new_len - 1; i >= (int) pos; --i, --j)
    std::swap(chars_[j], chars_[i]);

  for (uint i = 0; i < num; ++i)
    chars_[pos + i] = chars[i];

  for (uint i = pos; i < new_len; ++i)
    chars_[i]->setChanged(true);
}

void
CEditLineChars::
insertChar(uint pos, CEditChar *c)
{
  if (pos == chars_.size())
    chars_.push_back(c);
  else {
    chars_.push_back(chars_[chars_.size() - 1]);

    for (int i = (int) chars_.size() - 2; i >= (int) pos; --i)
      chars_[i + 1] = chars_[i];

    chars_[pos] = c;
  }

  // TODO: changed
}

void
CEditLineChars::
deleteChars(uint pos, uint num)
{
  for (uint i = 0; i < num; ++i)
    deleteChar(pos);
}

void
CEditLineChars::
deleteChar(uint pos)
{
  auto *c1 = chars_[pos];

  uint num_chars = chars_.size();

  for (uint i = pos + 1; i < num_chars; ++i)
    chars_[i - 1] = chars_[i];

  chars_.pop_back();

  delete c1;
}

void
CEditLineChars::
print(std::ostream &os) const
{
  auto pchar1 = chars_.begin();
  auto pchar2 = chars_.end  ();

  for ( ; pchar1 != pchar2; ++pchar1)
    os << **pchar1;
}

std::ostream &
operator<<(std::ostream &os, const CEditLineChars &chars)
{
  chars.print(os);

  return os;
}

//----------

bool
CEditLineUtil::
isBlank() const
{
  uint len = line_->getLength();

  for (uint i = 0; i < len; ++i) {
    char c = line_->getChar(i);

    if (! isspace(c))
      return false;
  }

  return true;
}

bool
CEditLineUtil::
isSentenceEnd(uint pos, uint *n) const
{
  *n = 0;

  uint len = line_->getLength();

  if (pos + *n >= len)
    return false;

  char c = line_->getChar(pos + *n);

  // check for . ! ?
  if (strchr(".?!", c) == nullptr)
    return false;

  (*n)++;

  // skip (, ], " and ' after . ! ?
  while (pos + *n < len - 1) {
    char c1 = line_->getChar(pos + *n);

    if (strchr(")]\"\'", c1) == nullptr)
      break;

    (*n)++;
  }

  // EOL is ok
  if (pos + *n >= len)
    return true;

  c = line_->getChar(pos + *n);

  // space is ok
  if (isspace(c))
    return true;

  return false;
}

bool
CEditLineUtil::
isSection(uint, uint *n) const
{
  uint len = line_->getLength();

  if (len > 0 && line_->getChar(0) == '{') {
    *n = 0;
    return true;
  }

  return false;
}

bool
CEditLineUtil::
findNext(const std::string &pattern, int char_num1, int char_num2, uint *char_num) const
{
  if (line_->isEmpty())
    return false;

  uint num_chars = line_->getLength();

  if (char_num1 >= (int) num_chars)
    return false;

  if (char_num2 < 0)
    char_num2 = num_chars - 1;

  const char *line = line_->getCString();

  const char *pattern1 = pattern.c_str();

  char *p = CStrUtil::strstr(&line[char_num1], &line[char_num2], pattern1);
  if (! p) return false;

  if (char_num)
    *char_num = p - line;

  return true;
}

bool
CEditLineUtil::
findNext(const CRegExp &pattern, int char_num1, int char_num2, uint *spos, uint *epos) const
{
  if (line_->isEmpty())
    return false;

  uint num_chars = line_->getLength();

  if (char_num1 >= (int) num_chars)
    return false;

  if (char_num2 < 0)
    char_num2 = num_chars - 1;

  auto line = line_->getString().substr(char_num1, char_num2 - char_num1 + 1);

  if (! pattern.find(line))
    return false;

  int spos1, epos1;

  if (! pattern.getMatchRange(&spos1, &epos1))
    return false;

  if (spos) *spos = spos1 + char_num1;
  if (epos) *epos = epos1 + char_num1;

  return true;
}

bool
CEditLineUtil::
findPrev(const std::string &pattern, int char_num1, int char_num2, uint *char_num) const
{
  if (line_->isEmpty())
    return false;

  uint num_chars = line_->getLength();

  if (char_num1 < 0)
    char_num1 = num_chars - 1;

  if (char_num2 >= (int) num_chars)
    return false;

  const char *line = line_->getCString();

  const char *pattern1 = pattern.c_str();

  char *p = CStrUtil::strrstr(&line[char_num1], &line[char_num2], pattern1);
  if (! p) return false;

  if (char_num)
    *char_num = p - line;

  return true;
}

bool
CEditLineUtil::
findPrev(const CRegExp &pattern, int char_num1, int char_num2, uint *spos, uint *epos) const
{
  if (line_->isEmpty())
    return false;

  uint num_chars = line_->getLength();

  if (char_num1 < 0)
    char_num1 = num_chars - 1;

  if (char_num2 >= (int) num_chars)
    return false;

  auto line = line_->getString().substr(char_num2, char_num1 - char_num2 + 1);

  if (! pattern.find(line))
    return false;

  int spos1, epos1;

  if (! pattern.getMatchRange(&spos1, &epos1))
    return false;

  if (spos) *spos = spos1 + char_num2;
  if (epos) *epos = epos1 + char_num2;

  return true;
}
