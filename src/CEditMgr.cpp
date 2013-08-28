#include <CEditMgr.h>
#include <CEditFile.h>
#include <CEditLine.h>
#include <CEditChar.h>
#include <CEditCursor.h>
#include <CEditEd.h>
#include <CLineEdit.h>

CEditMgr *
CEditMgr::
getInstance()
{
  static CEditMgr *instance;

  if (! instance)
    instance = new CEditMgr;

  return instance;
}

CEditMgr::
CEditMgr() :
 factory_(NULL)
{
}

CEditMgr::
~CEditMgr()
{
}

void
CEditMgr::
setFactory(CEditFactory *factory)
{
  factory_ = factory;
}

CEditFactory *
CEditMgr::
getFactory()
{
  if (! factory_)
    factory_ = new CEditDefFactory;

  return factory_;
}

CEditFile *
CEditMgr::
createFile()
{
  return getFactory()->createFile();
}

CEditCursor *
CEditMgr::
createCursor(CEditFile *file)
{
  return getFactory()->createCursor(file);
}

CEditEd *
CEditMgr::
createEd(CEditFile *file)
{
  return getFactory()->createEd(file);
}

CEditLine *
CEditMgr::
createLine(CEditFile *file)
{
  return getFactory()->createLine(file);
}

CEditChar *
CEditMgr::
createChar(CEditLine *line)
{
  return getFactory()->createChar(line);
}

CLineEdit *
CEditMgr::
createLineEdit(CEditFile *file)
{
  return getFactory()->createLineEdit(file);
}

//----------

CEditFile *
CEditDefFactory::
createFile()
{
  return new CEditFile();
}

CEditCursor *
CEditDefFactory::
createCursor(CEditFile *file)
{
  return new CEditCursor(file);
}

CEditEd *
CEditDefFactory::
createEd(CEditFile *file)
{
  return new CEditEd(file);
}

CEditLine *
CEditDefFactory::
createLine(CEditFile *file)
{
  return new CEditLine(file);
}

CEditChar *
CEditDefFactory::
createChar(CEditLine *)
{
  return new CEditChar();
}

CLineEdit *
CEditDefFactory::
createLineEdit(CEditFile *)
{
  return new CLineEdit();
}
