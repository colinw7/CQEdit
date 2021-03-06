#ifndef CVEDIT_VI_H
#define CVEDIT_VI_H

#include <CKeyType.h>
#include <CIPoint2D.h>
#include <CIBBox2D.h>

#include <string>
#include <vector>
#include <sys/types.h>

class CVEditFile;
class CKeyEvent;

class CVEditLastCommand {
 public:
  CVEditLastCommand(CVEditFile *file);

  void clear();

  void addCount(uint n);

  void addKey(CKeyType key);

  void exec();

 private:
  CVEditFile            *file_;
  std::vector<CKeyType>  keys_;
};

class CVLineEdit;

class CVEditVi {
 public:
  CVEditVi(CVEditFile *file);
 ~CVEditVi();

  void setInsertMode(bool insertMode);
  bool getInsertMode() const { return insertMode_; }

  void setCmdLineMode(bool cmdLineMode, const std::string &str="");
  bool getCmdLineMode() const { return cmdLineMode_; }

  std::string getCmdLineString() const;
  int         getCmdLinePos() const;

  void processChar(const CKeyEvent &event);

  void processInsertChar (const CKeyEvent &event);
  void processCommandChar(const CKeyEvent &event);
  void processNormalChar (const CKeyEvent &event);
  void processControlChar(const CKeyEvent &event);
  void processCmdLineChar(const CKeyEvent &event);

  void normalInsertChar(const CKeyEvent &event);

  bool processMoveChar(const CKeyEvent &event, CIPoint2D &newPos);

  bool doFindChar(char c, uint count, bool forward, bool till);

  void drawCmdLine(const CIBBox2D &bbox);

  void error(const std::string &mgs) const;

 private:
  CVEditFile        *file_        { nullptr };
  CKeyType           lastKey_     { CKEY_TYPE_NUL };
  uint               count_       { 0 };
  bool               insertMode_  { false };
  bool               cmdLineMode_ { false };
  CVLineEdit        *cmdLine_     { nullptr };
  char               register_    { '\0' };
  CVEditLastCommand  lastCommand_;
  char               findChar_    { '\0' };
  bool               findForward_ { true };
  bool               findTill_    { false };
};

#endif
