#ifndef CSYNTAX_CPP_H
#define CSYNTAX_CPP_H

#include <CSyntax.h>

class CFile;

class CSyntaxCPP : public CSyntax {
 public:
  CSyntaxCPP();

  virtual ~CSyntaxCPP();

  void processLine(const std::string &line) override;

 private:
  CSyntaxToken findWord(const std::string &str);

 private:
  bool         in_comment_ { false };
  bool         continued_  { false };
  CSyntaxToken last_token_;
};

#endif
