#include <CLineEdit.h>

CLineEdit::
CLineEdit() :
 pos_(0)
{
}

void
CLineEdit::
setLine(const std::string &line)
{
  line_ = line;
  pos_  = 0;
}

void
CLineEdit::
insertChar(char c)
{
  if (pos_ < line_.size())
    line_ = line_.substr(0, pos_) + c + line_.substr(pos_);
  else
    line_ += c;

  ++pos_;
}

void
CLineEdit::
replaceChar(char c)
{
  line_[pos_] = c;
}

void
CLineEdit::
deleteChar()
{
  if (pos_ < line_.size())
    line_ = line_.substr(0, pos_) + line_.substr(pos_ + 1);
}

void
CLineEdit::
clear()
{
  line_ = "";
  pos_  = 0;
}

bool
CLineEdit::
cursorLeft()
{
  if (pos_ > 0) {
    --pos_;

    return true;
  }
  else
    return false;
}

bool
CLineEdit::
cursorRight()
{
  if (pos_ < line_.size()) {
    ++pos_;

    return true;
  }
  else
    return false;
}

void
CLineEdit::
cursorStart()
{
  pos_ = 0;
}

void
CLineEdit::
cursorEnd()
{
  pos_ = line_.size();
}
