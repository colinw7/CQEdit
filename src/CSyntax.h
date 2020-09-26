#ifndef CSYNTAX_H
#define CSYNTAX_H

#include <string>
#include <sys/types.h>

enum class CSyntaxToken {
  NONE,
  PREPRO,
  KEYWORD,
  STRING,
  COMMENT
};

class CFile;

class CSyntaxNotifier {
 public:
  virtual ~CSyntaxNotifier() { }

  virtual void init() { }
  virtual void term() { }

  virtual void preProcessLine(const std::string &) { }
  virtual void postProcessLine(const std::string &) { }

  virtual void addToken(uint line_num, uint word_start,
                        const std::string &word, CSyntaxToken token) = 0;

  virtual void addText(uint, uint, const std::string &) { }
};

class CSyntax {
 public:
  CSyntax();

  virtual ~CSyntax() { }

  void setNotifier(CSyntaxNotifier *notifier);

  void parseFile(CFile *file);

  virtual void init();
  virtual void term();

  virtual void preProcessLine(const std::string &line);

  virtual void processLine(const std::string &line) = 0;

  virtual void postProcessLine(const std::string &line);

  virtual void addToken(uint line_num, uint word_start,
                        const std::string &word, CSyntaxToken token);

  virtual void addText(uint line_num, uint word_start, const std::string &text);

 protected:
  uint             line_num_ { 0 };
  CSyntaxNotifier *notifier_ { nullptr };
};

#endif
