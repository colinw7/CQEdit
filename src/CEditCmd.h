#ifndef CEDIT_CMD_H
#define CEDIT_CMD_H

#include <map>

#include <CUndo.h>

class CEditCmd;
class CEditFile;

class CEditCmdMgr {
 public:
  CEditCmdMgr(CEditFile *file);

 ~CEditCmdMgr() { }

  CEditFile *getFile() const { return file_; }

  void addCmd(CEditCmd *cmd);

  CEditCmd *getCmd(const std::string &cmd) const;

  void execCmd(const char *cmdName, ...);

  bool getDebug() const { return debug_; }

 private:
  CEditCmdMgr(const CEditCmdMgr &rhs);
  CEditCmdMgr &operator=(const CEditCmdMgr &rhs);

 private:
  using CmdMap = std::map<std::string, CEditCmd *>;

  CEditFile *file_ { nullptr };
  CmdMap     cmds_;
  bool       debug_ { false };
};

//---

class CEditCmd : public CUndoData {
 public:
  CEditCmd(CEditCmdMgr *mgr);

  virtual ~CEditCmd() { }

  virtual const char *getName() const = 0;

  virtual bool exec(const std::vector<std::string> &argList) = 0;

 private:
  CEditCmd(const CEditCmd &rhs);
  CEditCmd &operator=(const CEditCmd &rhs);

  using CUndoData::exec;

 protected:
  CEditCmdMgr *mgr_ { nullptr };
};

//---

class CEditAddLineCmd : public CEditCmd {
 public:
  CEditAddLineCmd(CEditCmdMgr *mgr);

  CEditAddLineCmd(CEditCmdMgr *mgr, int line_num, const std::string &line);

  const char *getName() const override { return "add_line"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int         line_num_ { 0 };
  std::string line_;
};

//---

class CEditDeleteLineCmd : public CEditCmd {
 public:
  CEditDeleteLineCmd(CEditCmdMgr *mgr);

  CEditDeleteLineCmd(CEditCmdMgr *mgr, int line_num);

  const char *getName() const override { return "delete_line"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int         line_num_ { 0 };
  std::string chars_;
};

//---

class CEditMoveLineCmd : public CEditCmd {
 public:
  CEditMoveLineCmd(CEditCmdMgr *mgr);

  CEditMoveLineCmd(CEditCmdMgr *mgr, int line_num1, int line_num2);

  const char *getName() const override { return "move_line"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int line_num1_ { 0 };
  int line_num2_ { 0 };
};

//---

class CEditReplaceCmd : public CEditCmd {
 public:
  CEditReplaceCmd(CEditCmdMgr *mgr);

  CEditReplaceCmd(CEditCmdMgr *mgr, int line_num,
                  int char_num1, int char_num2, const std::string &str);

  const char *getName() const override { return "replace"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int         line_num_ { 0 };
  int         char_num1_ { 0 };
  int         char_num2_ { 0 };
  std::string str_;
};

//---

class CEditInsertCharCmd : public CEditCmd {
 public:
  CEditInsertCharCmd(CEditCmdMgr *mgr);

  CEditInsertCharCmd(CEditCmdMgr *mgr, int line_num, int char_num, char c);

  const char *getName() const override { return "insert_char"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int  line_num_ { 0 };
  int  char_num_ { 0 };
  char c_        { '\0' };
};

//---

class CEditReplaceCharCmd : public CEditCmd {
 public:
  CEditReplaceCharCmd(CEditCmdMgr *mgr);

  CEditReplaceCharCmd(CEditCmdMgr *mgr, int line_num, int char_num, char c);

  const char *getName() const override { return "replace_char"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int  line_num_ { 0 };
  int  char_num_ { 0 };
  char c_        { '\0' };
};

//---

class CEditDeleteCharsCmd : public CEditCmd {
 public:
  CEditDeleteCharsCmd(CEditCmdMgr *mgr);

  CEditDeleteCharsCmd(CEditCmdMgr *mgr, int line_num, int char_num, int num_chars);

  const char *getName() const override { return "delete_chars"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int         line_num_ { 0 };
  int         char_num_ { 0 };
  std::string chars_;
};

//---

class CEditSplitLineCmd : public CEditCmd {
 public:
  CEditSplitLineCmd(CEditCmdMgr *mgr);

  CEditSplitLineCmd(CEditCmdMgr *mgr, int line_num, int char_num);

  const char *getName() const override { return "split_line"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int line_num_ { 0 };
  int char_num_ { 0 };
};

//---

class CEditJoinLineCmd : public CEditCmd {
 public:
  CEditJoinLineCmd(CEditCmdMgr *mgr);

  CEditJoinLineCmd(CEditCmdMgr *mgr, int line_num);

  const char *getName() const override { return "join_line"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int line_num_ { 0 };
  int char_num_ { 0 };
};

//---

class CEditMoveToCmd : public CEditCmd {
 public:
  CEditMoveToCmd(CEditCmdMgr *mgr);

  CEditMoveToCmd(CEditCmdMgr *mgr, int line_num, int char_num);

  const char *getName() const override { return "move_to"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;

 private:
  int line_num_ { 0 };
  int char_num_ { 0 };
};

//---

class CEditUndoCmd : public CEditCmd {
 public:
  CEditUndoCmd(CEditCmdMgr *mgr);

  const char *getName() const override { return "undo"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;
};

//---

class CEditRedoCmd : public CEditCmd {
 public:
  CEditRedoCmd(CEditCmdMgr *mgr);

  const char *getName() const override { return "redo"; }

  bool exec(const std::vector<std::string> &argList) override;
  bool exec() override;
};

#endif
