#ifndef CSYNTAX_CPP_H
#define CSYNTAX_CPP_H

#include <CSyntax.h>

class CFile;

class CSyntaxCPP : public CSyntax {
 public:
  CSyntaxCPP();

  virtual ~CSyntaxCPP();

  void parseFile(CFile *file);

  void processLine(const std::string &line);

 private:
  CSyntaxToken findWord(const std::string &str);

 private:
  bool         in_comment_;
  bool         continued_;
  CSyntaxToken last_token_;
};

#endif
