#include <CQEditFile.h>
#include <CQEdit.h>
#include <CQFont.h>

#include <QApplication>
#include <QClipboard>

CQEditFile::
CQEditFile(CQEdit *edit) :
 CVEditFile(), edit_(edit)
{
}

const QFont &
CQEditFile::
getQFont() const
{
  CFontPtr font = getFont();

  CQFont *qfont = font.cast<CQFont>();

  return qfont->getQFont();
}

void
CQEditFile::
setQFont(const QFont &qfont)
{
  CFontPtr font = CQFontMgrInst->lookupFont(qfont);

  setFont(font);
}

void
CQEditFile::
setFileName(const std::string &fileName)
{
  CVEditFile::setFileName(fileName);

  edit_->setFileName(fileName);
}

void
CQEditFile::
setCmdText(const std::string &str)
{
  edit_->setCmdText(str);
}

void
CQEditFile::
stateChanged()
{
  edit_->sendStateChanged();
}

void
CQEditFile::
selectionChanged(const std::string &str)
{
  QClipboard *clipboard = QApplication::clipboard();

  clipboard->setText(str.c_str(), QClipboard::Selection);
}

void
CQEditFile::
update()
{
  edit_->update();
}

void
CQEditFile::
saveAndQuit()
{
  edit_->getFile()->saveLines(edit_->getFile()->getFileName());

  edit_->quit();
}

void
CQEditFile::
quit()
{
  edit_->quit();
}

void
CQEditFile::
displayMessage(const StringList &lines)
{
  uint num_lines = lines.size();

  for (uint i = 0; i < num_lines; ++i)
    edit_->sendOutputMsg(lines[i]);
}

void
CQEditFile::
displayError(const StringList &lines)
{
  uint num_lines = lines.size();

  for (uint i = 0; i < num_lines; ++i)
    edit_->sendErrorMsg(lines[i]);
}

void
CQEditFile::
displayMarks()
{
  edit_->displayMarks();
}

void
CQEditFile::
displayRegisters()
{
  edit_->displayRegisters();
}

//----------

CQEditCursor::
CQEditCursor(CQEditFile *file) :
 CVEditCursor(file), file_(file)
{
}

void
CQEditCursor::
setPos(const CIPoint2D &pos)
{
  CVEditCursor::setPos(pos);
}
