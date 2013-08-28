#ifndef CSYNTAX_H
#define CSYNTAX_H

#include <string>
#include <sys/types.h>

enum CSyntaxToken {
  TOKEN_NONE,
  TOKEN_PREPRO,
  TOKEN_KEYWORD,
  TOKEN_STRING,
  TOKEN_COMMENT
};

class CSyntaxNotifier {
 public:
  virtual ~CSyntaxNotifier() { }

  virtual void addToken(uint line_num, uint word_start,
                        const std::string &word, CSyntaxToken token) = 0;

  virtual void addText(uint, uint, const std::string &) { }
};

class CSyntax {
 protected:
  uint             line_num_;
  CSyntaxNotifier *notifier_;

 public:
  CSyntax();

  virtual ~CSyntax() { }

  void setNotifier(CSyntaxNotifier *notifier);

  virtual void init();
  virtual void term();

  virtual void processLine(const std::string &line) = 0;

  virtual void addToken(uint line_num, uint word_start,
                        const std::string &word, CSyntaxToken token);

  virtual void addText(uint line_num, uint word_start, const std::string &text);
};

#endif
