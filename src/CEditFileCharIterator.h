#ifndef CEditFileCharIterator_H
#define CEditFileCharIterator_H

#include <CEditLine.h>

class CEditFileCharIterator {
 public:
  typedef std::forward_iterator_tag iterator_category;
  typedef CEditChar*                value_type;
  typedef ptrdiff_t                 difference_type;
  typedef value_type *              pointer;
  typedef value_type &              reference;

  CEditFileCharIterator() :
   pline_    (),
   pline_end_(),
   pchar_    (),
   pchar_end_() {
  }

  CEditFileCharIterator(const CEditFile *file) :
   pline_    (),
   pline_end_(),
   pchar_    (),
   pchar_end_() {
    pline_     = file->beginLine();
    pline_end_ = file->endLine();

    if (pline_ != pline_end_) {
      pchar_     = (*pline_)->beginChar();
      pchar_end_ = (*pline_)->endChar();
    }
  }

  const value_type &operator->() const { return *pchar_; }
  const value_type &operator* () const { return *pchar_; }

  CEditFileCharIterator &operator++() {
    if (pline_ == pline_end_) return *this;

    if      (pchar_ != pchar_end_)
      ++pchar_;
    else {
      ++pline_;

      if (pline_ == pline_end_) return *this;

      pchar_     = (*pline_)->beginChar();
      pchar_end_ = (*pline_)->endChar();
    }

    while (pchar_ == pchar_end_) {
      ++pline_;

      if (pline_ == pline_end_) return *this;

      pchar_     = (*pline_)->beginChar();
      pchar_end_ = (*pline_)->endChar();
    }

    return *this;
  }

  bool operator==(const CEditFileCharIterator &i) {
    if (pline_ == pline_end_ && i.pline_ == i.pline_end_) return true;

    return (pline_ == i.pline_ && pchar_ == i.pchar_);
  }

  bool operator!=(const CEditFileCharIterator &i) {
    return ! (*this == i);
  }

  CEditFileCharIterator &toEnd() {
    pline_ = pline_end_;

    return *this;
  }

 private:
  CEditFile::const_line_iterator pline_;
  CEditFile::const_line_iterator pline_end_;
  CEditLine::const_char_iterator pchar_;
  CEditLine::const_char_iterator pchar_end_;
};

#endif
