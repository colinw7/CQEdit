#ifndef CSYNTAX_C_H
#define CSYNTAX_C_H

#include <CSyntax.h>

class CFile;

class CSyntaxC : public CSyntax {
 public:
  CSyntaxC();

  virtual ~CSyntaxC();

  void processLine(const std::string &line) override;

 private:
  CSyntaxToken findWord(const std::string &str);

 private:
  bool         continued_ { false };
  CSyntaxToken last_token_;
};

#endif
