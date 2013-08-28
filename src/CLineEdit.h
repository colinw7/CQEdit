#ifndef CLINE_EDIT
#define CLINE_EDIT

#include <string>
#include <sys/types.h>

class CLineEdit {
 public:
  CLineEdit();

  virtual ~CLineEdit() { }

  void setLine(const std::string &line);

  const std::string &getLine() const { return line_; }

  int getPos() const { return pos_; }

  void insertChar(char c);
  void replaceChar(char c);
  void deleteChar();
  void clear();

  bool cursorLeft ();
  bool cursorRight();

  void cursorStart();
  void cursorEnd  ();

 protected:
  std::string line_;
  uint         pos_;
};

#endif
