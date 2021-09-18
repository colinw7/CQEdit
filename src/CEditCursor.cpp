#include <CEditCursor.h>
#include <CEditFile.h>
#include <CEditLine.h>

CEditCursor::
CEditCursor(CEditFile *file) :
 file_(file), pos_(0)
{
}

CEditCursor::
CEditCursor(const CEditCursor &c) :
 file_(c.file_), pos_(c.pos_)
{
}

CEditCursor::
~CEditCursor()
{
}

CEditCursor &
CEditCursor::
operator=(const CEditCursor &c)
{
  file_ = c.file_;
  pos_  = c.pos_;

  return *this;
}

CEditCursor *
CEditCursor::
dup() const
{
  return new CEditCursor(*this);
}

void
CEditCursor::
setPos(const CIPoint2D &pos)
{
  if (pos.y != pos_.y)
    setLastRow(pos.y);

  pos_ = pos;
}

void
CEditCursor::
setLastRow(uint row)
{
  last_line_.set = true;
  last_line_.row = row;

  const auto *line = file_->getEditLine(last_line_.row);

  last_line_.text = (line ? line->getString() : "");
}

void
CEditCursor::
updateLastLine()
{
  const auto *line = file_->getEditLine(last_line_.row);

  last_line_.text = line->getString();
}

void
CEditCursor::
print(std::ostream &os) const
{
  os << pos_;
}

std::ostream &
operator<<(std::ostream &os, const CEditCursor &c)
{
  c.print(os);

  return os;
}
