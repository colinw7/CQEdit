#ifndef CEDIT_FILE_H
#define CEDIT_FILE_H

class CEditFile;
class CEditLine;
class CEditChar;
class CEditCursor;
class CEditEd;
class CEditFileUtil;

#include <map>

#include <CIPoint2D.h>
#include <CEditCmd.h>
#include <CUndo.h>
#include <COptVal.h>
#include <CRegExp.h>
#include <CTextFile.h>

//---

struct CEditBufferLine {
  std::string line;
  bool        newline;

  CEditBufferLine(const std::string &line1, bool newline1) :
   line(line1), newline(newline1) {
  }

  const std::string &getLine() const { return line; }

  bool getNewLine() const { return newline; }
};

//---

struct CEditGroup {
  using LineList = std::vector<CEditBufferLine>;

  LineList lines;

  CEditGroup() :
   lines() {
  }

  void addLine(const std::string &line, bool newline) {
    lines.push_back(CEditBufferLine(line, newline));
  }
};

//---

struct CEditBuffer {
  using LineList = std::vector<CEditBufferLine>;

  LineList lines;

  CEditBuffer() { }

  void clear() { lines.clear(); }

  uint getNumLines() const { return uint(lines.size()); }

  const CEditBufferLine &getLine(uint i) const { return lines[i]; }

  void addLine(const std::string &line, bool newline) {
    lines.push_back(CEditBufferLine(line, newline));
  }
};

//---

class CEditFileLines {
 public:
  using LineList = std::vector<CEditLine *>;

  using iterator       = LineList::iterator;
  using const_iterator = LineList::const_iterator;

 public:
  CEditFileLines() :
   lines_() {
  }

 ~CEditFileLines();

  uint size() const { return uint(lines_.size()); }

  bool empty() const { return lines_.empty(); }

  void clear();

  const CEditLine *getLine(uint line_num) const;

  const_iterator begin() const { return lines_.begin(); }
  const_iterator end  () const { return lines_.end  (); }

  void addLine(uint line_num, CEditLine *line);

  void addLineChar (uint line_num, uint char_num, char c);
  void addLineChars(uint line_num, uint char_num, const std::string &chars);

  void setLineChar(uint line_num, uint char_num, char c);

  void replaceLineChar(uint line_num, uint char_num, char c);

  void replaceLineChars(uint line_num, const std::string &str);
  void replaceLineChars(uint line_num, uint char_num1, uint char_num2, const std::string &str);

  void moveLine(uint line_num1, int line_num2);

  void splitLine(uint line_num, uint char_num);
  void joinLine(uint line_num);

  void deleteLine(uint line_num);

  void deleteLineChars(uint line_num, uint char_num, uint n);

 private:
  LineList lines_;
};

//---

class CEditFileCharIterator;

class CEditFile {
 public:
  using LineList   = CEditFileLines::LineList;
  using CmdList    = std::vector<CEditCmd *>;
  using GroupList  = std::vector<CEditGroup *>;
  using MarkList   = std::map<std::string, CIPoint2D>;
  using OptionMap  = std::map<std::string, std::string>;
  using BufferMap  = std::map<char, CEditBuffer>;
  using StringList = std::vector<std::string>;

  using const_line_iterator = LineList::const_iterator;

  class CharIterator {
   public:
    using iterator_category = std::forward_iterator_tag;
    using value_type        = CEditChar*;
    using difference_type   = ptrdiff_t;
    using pointer           = value_type *;
    using reference         = value_type &;

    CharIterator();
    CharIterator(const CEditFile *file);
    CharIterator(const CharIterator &rhs);

   ~CharIterator();

    CharIterator &operator=(const CharIterator &rhs);

    const value_type &operator->() const;
    const value_type &operator* () const;

    CharIterator &operator++();

    bool operator==(const CharIterator &i);

    bool operator!=(const CharIterator &i);

    CharIterator &toEnd();

   private:
    CEditFileCharIterator *impl_;
  };

  using const_char_iterator = CharIterator;

 protected:
  struct Options {
    bool ignorecase;
    bool list;
    bool number;
    bool showmatch;
    uint shiftwidth;

    Options() :
     ignorecase(false),
     list      (false),
     number    (false),
     showmatch (false),
     shiftwidth(2) {
    }
  };

 public:
  CEditFile();

  virtual ~CEditFile();

  virtual void init();

  const std::string &getFileName() const { return fileName_; }
  virtual void setFileName(const std::string &fileName);

  CEditCursor *getCursor() const { return cursor_; }

  CEditEd *getEd() const { return ed_; }

  virtual const CIPoint2D &getPos() const;
  virtual void setPos(const CIPoint2D &pos);

  bool isValidPos(const CIPoint2D &pos) const;

  virtual void setRow(uint y);
  virtual void setCol(uint x);

  virtual uint getRow() const;
  virtual uint getCol() const;

  uint getNumLines() const;

  bool isLinesEmpty() const;

  bool hasFindPattern() const { return findPattern_.isValid(); }
  const CRegExp &getFindPattern() const { return findPattern_.getValue(); }
  void setFindPattern(const CRegExp &pattern) { findPattern_.setValue(pattern); }

  bool isExtraLineChar() const { return extraLineChar_; }
  virtual void setExtraLineChar(bool extraLineChar);

  bool getChanged() const { return changed_; }
  virtual void setChanged(bool changed);

  bool getUnsaved() const { return unsaved_; }
  virtual void setUnsaved(bool unsaved);

  virtual const_line_iterator beginLine() const;
  virtual const_line_iterator endLine  () const;

  virtual const_char_iterator beginChar() const;
  virtual const_char_iterator endChar  () const;

  virtual const CEditLine *getEditLine() const;
  virtual const CEditLine *getEditLine(uint line_num) const;

  virtual const CEditChar *getEditChar() const;
  virtual const CEditChar *getEditChar(uint line_num, uint char_num) const;

  virtual std::string getLine() const;
  virtual std::string getLine(uint line_num) const;

  virtual char getChar() const;
  virtual char getChar(uint line_num, uint char_num) const;

  uint lineLength(uint line_num);

  uint getNumMarks() const { return uint(marks_.size()); }

  const MarkList &getMarks() const { return marks_; }

  uint getNumBuffers() const { return uint(bufferMap_.size()); }

  const BufferMap &getBufferMap() const { return bufferMap_; }

  bool loadLines(const std::string &fileName);
  bool saveLines(const std::string &fileName);

  bool addFileLines(const std::string &fileName);
  bool addFileLines(const std::string &fileName, uint line_num);

  virtual void addLine(const std::string &line);
  virtual void addLine(uint line_num, const std::string &line);

  virtual void addChars(uint line_num, uint char_num, const std::string &chars);

  virtual void moveLine(uint line_num1, int line_num2);
  virtual void copyLine(uint line_num1, uint line_num2);

  virtual void setChar(char c);
  virtual void setChar(uint line_num, uint char_num, char c);

  virtual void insertChar(char c);
  virtual void insertChar(uint line_num, uint char_num, char c);

  virtual void replaceChar(char c);
  virtual void replaceChar(uint line_num, uint char_num, char c);

  void deleteAllLines();
  void subDeleteAllLines();

  virtual void deleteLine();
  virtual void deleteLine(uint line_num);

  void deleteWord();
  void deleteWord(uint line_num, uint char_num);

  void deleteEOL();
  void deleteEOL(uint line_num, uint char_num);

  void deleteChar();
  void deleteChar(uint line_num);

  void deleteChars(uint n);
  void deleteChars(uint line_num, uint char_num, uint n);

  void deleteTo(uint line_num, uint char_num);
  void deleteTo(uint line_num1, uint char_num1, uint line_num2, uint char_num2);

  void shiftLeft(uint line_num1, uint line_num2);
  void shiftRight(uint line_num1, uint line_num2);

  void yankLines(char id, uint n);
  void yankLines(char id, uint line_num, uint n);

  void yankWords(char id, uint n);
  void yankWords(char id, uint line_num, uint char_num, uint n);

  void yankChar(char id);
  void yankChar(char id, uint line_num, uint char_num);

  void yankTo(char id, uint line_num, uint char_num, bool is_line);
  void yankTo(char id, uint line_num1, uint char_num1,
              uint line_num2, uint char_num2, bool is_line);

  virtual void yankClear(char id);

  virtual void subYankTo(char id, uint line_num1, uint char_num1,
                         uint line_num2, uint char_num2, bool is_line);

  virtual void pasteAfter(char id);
  virtual void pasteAfter(char id, uint line_num, uint char_num);
  virtual void pasteBefore(char id);
  virtual void pasteBefore(char id, uint line_num, uint char_num);

  virtual void splitLine();
  virtual void splitLine(uint line_num, uint char_num);

  virtual void joinLine();
  virtual void joinLine(uint line_num);

  void newLineBelow();
  void newLineAbove();

  bool cursorLeft(uint n);
  bool cursorLeft(uint n, uint *line_num, uint *char_num);

  bool cursorUp(uint n);
  bool cursorUp(uint n, uint *line_num, uint *char_num);

  bool cursorRight(uint n);
  bool cursorRight(uint n, uint *line_num, uint *char_num);

  bool cursorDown(uint n);
  bool cursorDown(uint n, uint *line_num, uint *char_num);

  void cursorToLeft();
  void cursorToLeft(uint *line_num, uint *char_num);

  void cursorToRight();
  void cursorToRight(uint *line_num, uint *char_num);

  void cursorSkipSpace();
  void cursorSkipSpace(uint *line_num, uint *char_num);

  void cursorFirstNonBlankUp();
  void cursorFirstNonBlankUp(uint *line_num, uint *char_num);

  void cursorFirstNonBlankDown();
  void cursorFirstNonBlankDown(uint *line_num, uint *char_num);

  void cursorFirstNonBlank();
  void cursorFirstNonBlank(uint *line_num, uint *char_num);

  void cursorTo(uint line_num, uint char_num);

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

  bool nextLine(uint *line_num, uint *char_num);
  bool prevLine(uint *line_num, uint *char_num);

  void swapChar();
  void swapChar(uint line_num, uint char_num);

  bool findNext(const std::string &pattern);
  bool findNext(const std::string &pattern, uint *fline_num, uint *fchar_num);
  bool findNext(const std::string &pattern, uint line_num, int char_num=0);
  bool findNext(const std::string &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num);
  bool findNext(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2=-1);
  bool findNext(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num);

  bool findNext(const CRegExp &pattern, uint *len=nullptr);
  bool findNext(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len=nullptr);
  bool findNext(const CRegExp &pattern, uint line_num, int char_num=0, uint *len=nullptr);
  bool findNext(const CRegExp &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num, uint *len=nullptr);
  bool findNext(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2=-1, uint *len=nullptr);
  bool findNext(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len=nullptr);

  bool findPrev(const std::string &pattern);
  bool findPrev(const std::string &pattern, uint *fline_num, uint *fchar_num);
  bool findPrev(const std::string &pattern, uint line_num, int char_num=0);
  bool findPrev(const std::string &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num);
  bool findPrev(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2=-1);
  bool findPrev(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num);

  bool findPrev(const CRegExp &pattern, uint *len=nullptr);
  bool findPrev(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len=nullptr);
  bool findPrev(const CRegExp &pattern, uint line_num, int char_num=0, uint *len=nullptr);
  bool findPrev(const CRegExp &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num, uint *len=nullptr);
  bool findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2=-1, uint *len=nullptr);
  bool findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len=nullptr);

  bool findNextChar(char c, bool multiline);
  bool findNextChar(const std::string &str, bool multiline);
  bool findNextChar(uint line_num, int char_num, char c, bool multiline);
  bool findNextChar(uint line_num, int char_num, const std::string &str, bool multiline);

  bool findPrevChar(char c, bool multiline);
  bool findPrevChar(const std::string &str, bool multiline);
  bool findPrevChar(uint line_num, int char_num, char c, bool multiline);
  bool findPrevChar(uint line_num, int char_num, const std::string &str, bool multiline);

  virtual bool replace(uint line_num, uint char_num, char c);
  virtual bool replace(uint line_num, uint char_num1, uint char_num2, const std::string &replace);

  virtual void markReturn();

  virtual bool getMarkPos(const std::string &mark, uint *line_num, uint *char_num) const;
  virtual void setMarkPos(const std::string &mark);
  virtual void setMarkPos(const std::string &mark, uint line_num, uint char_num);
  virtual void unsetMarkPos(const std::string &mark);
  virtual void clearLineMarks(uint line_num);

  //---

  virtual void startGroup();
  virtual void endGroup();
  virtual bool inGroup() const;

  //---

  virtual void addUndo(CEditCmd *cmd);

  virtual void undo();
  virtual void redo();

  virtual bool canUndo() const;
  virtual bool canRedo() const;

  virtual void undoLine();

  virtual void resetUndo();

  //---

  virtual bool isWordChar(char c);

  uint getLineEnd(uint line_num) const;

  virtual bool runEdCmd(const std::string &cmd, bool &quit);

  virtual void setOptionString(const std::string &name, const std::string &arg);
  virtual bool getOptionString(const std::string &name, std::string &value) const;

  virtual void optionChanged(const std::string &name);

  virtual const Options &getOptions() const { return options_; }

  virtual CEditBuffer &getBuffer(char c='\0');

  virtual void replayFile(const std::string &filename);

  void addMsgLine(const std::string &msg);
  void addErrLine(const std::string &msg);

  virtual void displayMessage(const StringList &lines);
  virtual void displayError  (const StringList &lines);

  virtual void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CEditFile &file);

 protected:
  void subAddLine(uint line_num, CEditLine *line);
  void subAddChars(uint line_num, uint char_num, const std::string &chars);
  void subMoveLine(uint line_num1, int line_num2);

  void subDeleteLine(uint line_num);
  void subDeleteChars(uint line_num, uint char_num, uint n);
  void subInsertChar(uint line_num, uint char_num, char c);
  void subReplaceChar(uint line_num1, uint char_num, char c);
  void subSplitLine(uint line_num, uint char_num);
  void subJoinLine(uint line_num);
  bool subReplace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr);

  void fixPos();

 private:
  CEditFile(const CEditFile &rhs);
  CEditFile &operator=(const CEditFile &rhs);

 protected:
  using OptRegExp = COptValT<CRegExp>;

  CEditFileUtil *util_ { nullptr };

  // data
  std::string    fileName_;
  CEditFileLines lines_;

  CEditEd*    ed_      { nullptr };
  CEditCmdMgr cmdMgr_;
  CUndo       undo_;

  // cursor
  CEditCursor* cursor_ { nullptr };

  // state
  bool extraLineChar_ { false };
  bool changed_       { false };
  bool unsaved_       { false };

  // groups
  GroupList groupList_;

  // marks, buffers
  MarkList  marks_;
  BufferMap bufferMap_;

  // options
  OptionMap optionMap_;
  Options   options_;

  // find
  OptRegExp findPattern_;

  StringList msgLines_;
  StringList errLines_;
};

#endif
