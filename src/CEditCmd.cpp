#include <CEditCmd.h>
#include <CEditFile.h>
#include <CEditLine.h>
#include <CStrUtil.h>
#include <cassert>
#include <iostream>

CEditCmdMgr::
CEditCmdMgr(CEditFile *file) :
 file_(file), debug_(false)
{
  addCmd(new CEditAddLineCmd    (this));
  addCmd(new CEditDeleteLineCmd (this));
  addCmd(new CEditMoveLineCmd   (this));
  addCmd(new CEditReplaceCmd    (this));
  addCmd(new CEditInsertCharCmd (this));
  addCmd(new CEditReplaceCharCmd(this));
  addCmd(new CEditDeleteCharsCmd(this));
  addCmd(new CEditSplitLineCmd  (this));
  addCmd(new CEditJoinLineCmd   (this));
  addCmd(new CEditMoveToCmd     (this));
  addCmd(new CEditUndoCmd       (this));
  addCmd(new CEditRedoCmd       (this));
}

void
CEditCmdMgr::
addCmd(CEditCmd *cmd)
{
  cmds_[cmd->getName()] = cmd;
}

CEditCmd *
CEditCmdMgr::
getCmd(const std::string &cmd) const
{
  auto p = cmds_.find(cmd);

  if (p == cmds_.end())
    return nullptr;

  return (*p).second;
}

void
CEditCmdMgr::
execCmd(const char *cmdName, ...)
{
  va_list args;

  va_start(args, cmdName);

  auto *cmd = getCmd(cmdName);
  assert(cmd);

  auto *arg = va_arg(args, char *);

  std::vector<std::string> argList;

  while (arg) {
    argList.push_back(arg);

    arg = va_arg(args, char *);
  }

  cmd->exec(argList);

  va_end(args);
}

//------

CEditAddLineCmd::
CEditAddLineCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr)
{
}

CEditAddLineCmd::
CEditAddLineCmd(CEditCmdMgr *mgr, int line_num, const std::string &line) :
 CEditCmd(mgr), line_num_(line_num), line_(line)
{
  if (mgr_->getDebug())
    std::cerr << "Add: Add Line " << line_num << " " << line << "\n";
}

bool
CEditAddLineCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int         pos  = CStrUtil::toInteger(argList[0]);
  const auto &line = argList[1];

  mgr_->getFile()->addLine(pos, line);

  return true;
}

bool
CEditAddLineCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (mgr_->getDebug())
      std::cerr << "Exec: Add Line " << line_num_ << " " << line_ << "\n";

    mgr_->getFile()->addLine(line_num_, line_);
  }
  else {
    if (mgr_->getDebug())
      std::cerr << "Exec: Delete Line " << line_num_ << "\n";

    mgr_->getFile()->deleteLine(line_num_);
  }

  return true;
}

//------

CEditDeleteLineCmd::
CEditDeleteLineCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0)
{
}

CEditDeleteLineCmd::
CEditDeleteLineCmd(CEditCmdMgr *mgr, int line_num) :
 CEditCmd(mgr), line_num_(line_num)
{
  if (mgr_->getDebug())
    std::cerr << "Add: Delete Line " << line_num_ << "\n";

  chars_ = mgr_->getFile()->getEditLine(line_num_)->getString();
}

bool
CEditDeleteLineCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 1);

  int pos = CStrUtil::toInteger(argList[0]);

  mgr_->getFile()->deleteLine(pos);

  return true;
}

bool
CEditDeleteLineCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (mgr_->getDebug())
      std::cerr << "Exec: Delete Line " << line_num_ << "\n";

    mgr_->getFile()->deleteLine(line_num_);
  }
  else {
    if (mgr_->getDebug())
      std::cerr << "Exec: Add Line " << line_num_ << " " << chars_ << "\n";

    mgr_->getFile()->addLine(line_num_, chars_);
  }

  return true;
}

//------

CEditMoveLineCmd::
CEditMoveLineCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num1_(0), line_num2_(0)
{
}

CEditMoveLineCmd::
CEditMoveLineCmd(CEditCmdMgr *mgr, int line_num1, int line_num2) :
 CEditCmd(mgr), line_num1_(line_num1), line_num2_(line_num2)
{
}

bool
CEditMoveLineCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int pos1 = CStrUtil::toInteger(argList[0]);
  int pos2 = CStrUtil::toInteger(argList[1]);

  mgr_->getFile()->moveLine(pos1, pos2);

  return true;
}

bool
CEditMoveLineCmd::
exec()
{
  if (getState() == UNDO_STATE)
    mgr_->getFile()->moveLine(line_num1_, line_num2_);
  else
    mgr_->getFile()->moveLine(line_num2_, line_num1_);

  return true;
}

//------

CEditReplaceCmd::
CEditReplaceCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0), char_num1_(0), char_num2_(0)
{
}

CEditReplaceCmd::
CEditReplaceCmd(CEditCmdMgr *mgr, int line_num,
                int char_num1, int char_num2, const std::string &str) :
 CEditCmd(mgr), line_num_(line_num), char_num1_(char_num1),
 char_num2_(char_num2), str_(str)
{
}

bool
CEditReplaceCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 4);

  int  line_num  = CStrUtil::toInteger(argList[0]);
  int  char_num1 = CStrUtil::toInteger(argList[1]);
  int  char_num2 = CStrUtil::toInteger(argList[2]);
  auto str       = argList[3];

  mgr_->getFile()->replace(line_num, char_num1, char_num2, str);

  return true;
}

bool
CEditReplaceCmd::
exec()
{
  auto str = mgr_->getFile()->getEditLine(line_num_)->getSubString(char_num1_, char_num2_);

  mgr_->getFile()->replace(line_num_, char_num1_, char_num2_, str_);

  str_ = str;

  return true;
}

//------

CEditInsertCharCmd::
CEditInsertCharCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0), char_num_(0), c_(0)
{
}

CEditInsertCharCmd::
CEditInsertCharCmd(CEditCmdMgr *mgr, int line_num, int char_num, char c) :
 CEditCmd(mgr), line_num_(line_num), char_num_(char_num), c_(c)
{
  if (mgr_->getDebug())
    std::cerr << "Add: Insert Char " << line_num_ << " " << char_num_ << " " << c_ << "\n";
}

bool
CEditInsertCharCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int  line_num = CStrUtil::toInteger(argList[0]);
  int  char_num = CStrUtil::toInteger(argList[1]);
  auto str      = argList[2];

  mgr_->getFile()->insertChar(line_num, char_num, str[0]);

  return true;
}

bool
CEditInsertCharCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (mgr_->getDebug())
      std::cerr << "Exec: Insert Char " << line_num_ << " " << char_num_ << " " << c_ << "\n";

    mgr_->getFile()->insertChar(line_num_, char_num_, c_);
  }
  else {
    if (mgr_->getDebug())
      std::cerr << "Exec: Delete Char " << line_num_ << " " << char_num_ << " " << "\n";

    mgr_->getFile()->deleteChars(line_num_, char_num_, 1);
  }

  return true;
}

//------

CEditReplaceCharCmd::
CEditReplaceCharCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0), char_num_(0), c_(0)
{
}

CEditReplaceCharCmd::
CEditReplaceCharCmd(CEditCmdMgr *mgr, int line_num, int char_num, char c) :
 CEditCmd(mgr), line_num_(line_num), char_num_(char_num), c_(c)
{
}

bool
CEditReplaceCharCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int  line_num = CStrUtil::toInteger(argList[0]);
  int  char_num = CStrUtil::toInteger(argList[1]);
  auto str      = argList[2];

  mgr_->getFile()->replaceChar(line_num, char_num, str[0]);

  return true;
}

bool
CEditReplaceCharCmd::
exec()
{
  auto str = mgr_->getFile()->getEditLine(line_num_)->getSubString(char_num_, char_num_);

  mgr_->getFile()->replaceChar(line_num_, char_num_, c_);

  c_ = str[0];

  return true;
}

//------

CEditDeleteCharsCmd::
CEditDeleteCharsCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0), char_num_(0)
{
}

CEditDeleteCharsCmd::
CEditDeleteCharsCmd(CEditCmdMgr *mgr, int line_num,
                    int char_num, int num_chars) :
 CEditCmd(mgr), line_num_(line_num), char_num_(char_num)
{
  chars_ = mgr_->getFile()->getEditLine(line_num_)->
             getSubString(char_num_, char_num_ + num_chars - 1);

  if (mgr_->getDebug())
      std::cerr << "Add: Delete Chars " << line_num_ << " " << char_num_ << " " << chars_ << "\n";
}

bool
CEditDeleteCharsCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int line_num  = CStrUtil::toInteger(argList[0]);
  int char_num  = CStrUtil::toInteger(argList[1]);
  int num_chars = CStrUtil::toInteger(argList[2]);

  mgr_->getFile()->deleteChars(line_num, char_num, num_chars);

  return true;
}

bool
CEditDeleteCharsCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (mgr_->getDebug())
      std::cerr << "Exec: Delete Chars " << line_num_ << " " << char_num_ << " " << chars_ << "\n";

    mgr_->getFile()->deleteChars(line_num_, char_num_, chars_.size());
  }
  else {
    if (mgr_->getDebug())
      std::cerr << "Exec: Add Chars " << line_num_ << " " << char_num_ << " " << chars_ << "\n";

    mgr_->getFile()->addChars(line_num_, char_num_, chars_);
  }

  return true;
}

//------

CEditSplitLineCmd::
CEditSplitLineCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0), char_num_(0)
{
}

CEditSplitLineCmd::
CEditSplitLineCmd(CEditCmdMgr *mgr, int line_num, int char_num) :
 CEditCmd(mgr), line_num_(line_num), char_num_(char_num)
{
}

bool
CEditSplitLineCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int line_num = CStrUtil::toInteger(argList[0]);
  int char_num = CStrUtil::toInteger(argList[1]);

  mgr_->getFile()->splitLine(line_num, char_num);

  return true;
}

bool
CEditSplitLineCmd::
exec()
{
  if (getState() == UNDO_STATE)
    mgr_->getFile()->splitLine(line_num_, char_num_);
  else
    mgr_->getFile()->joinLine(line_num_);

  return true;
}

//------

CEditJoinLineCmd::
CEditJoinLineCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr), line_num_(0)
{
}

CEditJoinLineCmd::
CEditJoinLineCmd(CEditCmdMgr *mgr, int line_num) :
 CEditCmd(mgr), line_num_(line_num)
{
  char_num_ = mgr_->getFile()->getCol();
}

bool
CEditJoinLineCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 1);

  int line_num = CStrUtil::toInteger(argList[0]);

  mgr_->getFile()->joinLine(line_num);

  return true;
}

bool
CEditJoinLineCmd::
exec()
{
  if (getState() == UNDO_STATE)
    mgr_->getFile()->joinLine(line_num_);
  else
    mgr_->getFile()->splitLine(line_num_, char_num_);

  return true;
}

//------

CEditMoveToCmd::
CEditMoveToCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr)
{
}

CEditMoveToCmd::
CEditMoveToCmd(CEditCmdMgr *mgr, int line_num, int char_num) :
 CEditCmd(mgr), line_num_(line_num), char_num_(char_num)
{
  if (mgr_->getDebug())
    std::cerr << "Add: Move To " << line_num << " " << char_num << "\n";
}

bool
CEditMoveToCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int line_num = CStrUtil::toInteger(argList[0]);
  int char_num = CStrUtil::toInteger(argList[1]);

  mgr_->getFile()->cursorTo(line_num, char_num);

  return true;
}

bool
CEditMoveToCmd::
exec()
{
  uint line_num = mgr_->getFile()->getRow();
  uint char_num = mgr_->getFile()->getCol();

  if (mgr_->getDebug())
    std::cerr << "Exec: Move To " << line_num_ << " " << char_num_ << "\n";

  mgr_->getFile()->cursorTo(line_num_, char_num_);

  line_num_ = line_num;
  char_num_ = char_num;

  return true;
}

//------

CEditUndoCmd::
CEditUndoCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr)
{
}

bool
CEditUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 0);

  mgr_->getFile()->undo();

  return true;
}

bool
CEditUndoCmd::
exec()
{
  mgr_->getFile()->undo();

  return true;
}

//------

CEditRedoCmd::
CEditRedoCmd(CEditCmdMgr *mgr) :
 CEditCmd(mgr)
{
}

bool
CEditRedoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 0);

  mgr_->getFile()->redo();

  return true;
}

bool
CEditRedoCmd::
exec()
{
  mgr_->getFile()->redo();

  return true;
}

//---------------

CEditCmd::
CEditCmd(CEditCmdMgr *mgr) :
 mgr_(mgr)
{
}
