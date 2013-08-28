#ifndef CEDIT_CHAR_H
#define CEDIT_CHAR_H

#include <iostream>

class CEditChar {
 public:
  CEditChar();
  CEditChar(const CEditChar &c);

  virtual ~CEditChar();

  const CEditChar &operator=(const CEditChar &c);

  virtual CEditChar *dup() const;

  char getChar() const { return c_; }

  virtual void setChar(char c);

  bool getChanged() const { return changed_; }

  virtual void setChanged(bool changed);

  virtual void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CEditChar &c);

 protected:
  char c_;
  bool changed_;
};

#endif
