#ifndef CED_H
#define CED_H

#include <CRegExp.h>
#include <CAutoPtr.h>
#include <CIPoint2D.h>
#include <sys/types.h>

class CEd;
class CEditFile;

class CEdPointCondition {
 public:
  CEdPointCondition() { }

  virtual ~CEdPointCondition() { }

  virtual bool execute(CEd *) { return true; }
};

class CEdLineCondition : public CEdPointCondition {
 public:
  CEdLineCondition(int line=-1) :
   CEdPointCondition(), line_(line) {
  }

  void setLine(int line) { line_ = line; }
  int getLine() const { return line_; }

 private:
  int line_ { 0 };
};

class CEdRegExpCondition : public CEdPointCondition {
 public:
  CEdRegExpCondition(const std::string &expr) :
   CEdPointCondition(), expr_(expr) {
  }

 private:
  CRegExp expr_;
};

class CEd {
 private:
  typedef std::vector<std::string> StringList;

  // structure to represent an entered command
  class InputData {
   public:
    InputData() :
     start_(), end_(), cmd_('\0'), lines_() {
    }

    bool isCmd(char cmd) const {
      return (cmd_ == cmd);
    }

    void setCmd(char cmd) {
      cmd_ = cmd;
    }

    void setStartLine(int line) {
      start_.setLine(line);
    }

    void setEndLine(int line) {
      end_.setLine(line);
    }

    int getStartLine() {
      return start_.getLine();
    }

    int getEndLine() {
      return end_.getLine();
    }

    const StringList &getLines() const { return lines_; }

    void addLine(const std::string &line) {
      lines_.push_back(line);
    }

    void clearLines() {
      lines_.clear();
    }

   private:
    CEdLineCondition start_;        // start position
    CEdLineCondition end_;          // end position
    char             cmd_ { '\0' }; // command name
    StringList       lines_;        // lines (for a, c, i, ...)
  };

 public:
  enum Mode {
    COMMAND,
    INPUT
  };

 public:
  CEd(CEditFile *file);

  virtual ~CEd();

  void init();

  bool execFile(const std::string &fileName);

  bool execCmd(const std::string &cmd);

  bool findNext(const std::string &str, int *line_num, int *char_num);
  bool findPrev(const std::string &str, int *line_num, int *char_num);

  Mode getMode() const { return mode_; }

  bool isQuit() const { return quit_; }

  bool getEx() const { return ex_; }
  void setEx(bool ex) { ex_ = ex; }

  bool getCaseSensitive() const { return case_sensitive_; }
  void setCaseSensitive(bool flag) { case_sensitive_ = flag; }

  void doSubstitute(int i1, int i2, const std::string &find,
                    const std::string &replace, char mod);

  void doFindNext(int i1, int i2, const std::string &find);

  void doFindPrev(int i1, int i2, const std::string &find);

  void doGlob(int i1, int i2, const std::string &find, const std::string &cmd);

  void doJoin(int i1, int i2);

  void doMove(int i1, int i2, int i3);

  void doCopy(int i1, int i2, int i3);

  void doDelete(int i1, int i2);

  void doCopy(int i1, int i2);

  void doPaste(int i1);

  void doUndo();

  void doMark(int i1, char c);

  void doExecute(int i1, int i2, const std::string &cmdStr);

  void doPrint(int i1, int i2, bool numbered, bool eol);

  virtual void output(const std::string &msg);
  virtual void error (const std::string &msg);

  void addLine(uint row, const std::string &line);
  void deleteLine(uint row);

  virtual void setPos(const CIPoint2D &p);

  uint getRow() const;
  uint getCol() const;

  void edNotifyQuit(bool force);

 private:
  CEd(const CEd &rhs);
  CEd &operator=(const CEd &rhs);

 private:
  CEditFile *file_           { nullptr };
  Mode       mode_           { COMMAND }; // current mode, command or input
  InputData  input_data_;
  bool       ex_             { false };   // vi/ex mode
  bool       case_sensitive_ { true  };   // case sensitive
  bool       quit_           { false };   // quit
};

#endif
