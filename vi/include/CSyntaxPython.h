#ifndef CSYNTAX_PYTHON_H
#define CSYNTAX_PYTHON_H

#include <CSyntax.h>

class CFile;

class CSyntaxPython : public CSyntax {
 public:
  CSyntaxPython();

  virtual ~CSyntaxPython();

  const char *language() const override { return "Python"; }

  void processLine(const std::string &line) override;

 private:
  CSyntaxToken findWord(const std::string &str);

 private:
  bool         continued_ { false };
  CSyntaxToken last_token_;
};

#endif
