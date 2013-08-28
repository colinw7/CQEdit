#include <CEditChar.h>

CEditChar::
CEditChar() :
 c_('\0'), changed_(false)
{
}

CEditChar::
CEditChar(const CEditChar &c) :
 c_(c.c_), changed_(false)
{
}

CEditChar::
~CEditChar()
{
}

const CEditChar &
CEditChar::
operator=(const CEditChar &c)
{
  c_ = c.c_;

  setChanged(true);

  return *this;
}

CEditChar *
CEditChar::
dup() const
{
  return new CEditChar(*this);
}

void
CEditChar::
setChar(char c)
{
  c_ = c;

  setChanged(true);
}

void
CEditChar::
setChanged(bool changed)
{
  changed_ = changed;
}

void
CEditChar::
print(std::ostream &os) const
{
  os << c_;
}

std::ostream &
operator<<(std::ostream &os, const CEditChar &c)
{
  c.print(os);

  return os;
}
