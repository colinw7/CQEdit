#ifndef CSYNTAX_C_H
#define CSYNTAX_C_H

#include <CSyntax.h>

class CFile;

class CSyntaxC : public CSyntax {
 public:
  CSyntaxC();

  virtual ~CSyntaxC();

  void parseFile(CFile *file);

  void processLine(const std::string &line);

 private:
  CSyntaxToken findWord(const std::string &str);

 private:
  bool         continued_;
  CSyntaxToken last_token_;
};

#endif
