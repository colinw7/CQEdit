#ifndef CEDIT_ED_H
#define CEDIT_ED_H

#include <CEd.h>

class CEditEd : public CEd {
 public:
  CEditEd(CEditFile *file);

  virtual ~CEditEd();

  virtual void output(const std::string &msg);
  virtual void error (const std::string &msg);

 protected:
  CEditEd(const CEditEd &ed);
  const CEditEd &operator=(const CEditEd &ed);

 protected:
  CEditFile *file_;
};

#endif
