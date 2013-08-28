#ifndef CEDIT_FILE_UTIL_H
#define CEDIT_FILE_UTIL_H

#include <string>
#include <sys/types.h>

class CEditFile;

class CEditFileUtil {
 public:
  CEditFileUtil(CEditFile *file);

  void deleteWord();
  void deleteWord(uint line_num, uint char_num);

  void deleteEOL();
  void deleteEOL(uint line_num, uint char_num);

  void shiftLeft(uint line_num1, uint line_num2);
  void shiftRight(uint line_num1, uint line_num2);

  void nextWord();
  void nextWord(uint *line_num, uint *char_num);
  void nextWORD();
  void nextWORD(uint *line_num, uint *char_num);

  void prevWord();
  void prevWord(uint *line_num, uint *char_num);
  void prevWORD();
  void prevWORD(uint *line_num, uint *char_num);

  void endWord();
  void endWord(uint *line_num, uint *char_num);
  void endWORD();
  void endWORD(uint *line_num, uint *char_num);

  bool getWord(std::string &word);
  bool getWord(uint line_num, uint char_num, std::string &word);

  void nextSentence();
  void nextSentence(uint *line_num, uint *char_num);
  void prevSentence();
  void prevSentence(uint *line_num, uint *char_num);

  void nextParagraph();
  void nextParagraph(uint *line_num, uint *char_num);
  void prevParagraph();
  void prevParagraph(uint *line_num, uint *char_num);

  void nextSection();
  void nextSection(uint *line_num, uint *char_num);
  void prevSection();
  void prevSection(uint *line_num, uint *char_num);

 private:
  CEditFile *file_;
};

#endif
