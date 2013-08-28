#include <CEditEd.h>
#include <CEditFile.h>

CEditEd::
CEditEd(CEditFile *file) :
 CEd(file), file_(file)
{
}

CEditEd::
~CEditEd()
{
}

void
CEditEd::
output(const std::string &msg)
{
  file_->addMsgLine(msg);
}

void
CEditEd::
error(const std::string &msg)
{
  file_->addErrLine(msg);
}
