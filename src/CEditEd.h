#ifndef CEDIT_ED_H
#define CEDIT_ED_H

#include <CEd.h>

class CEditEd : public CEd {
 public:
  CEditEd(CEditFile *file);

  virtual ~CEditEd();

  void output(const std::string &msg) override;
  void error (const std::string &msg) override;

 protected:
  CEditEd(const CEditEd &ed);
  CEditEd &operator=(const CEditEd &ed);

 protected:
  CEditFile *file_;
};

#endif
