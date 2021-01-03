#ifndef CVI_H
#define CVI_H

#include <COptVal.h>
#include <CRegExp.h>

#include <vector>
#include <map>
#include <cassert>

class CVi;

class CViLastCommand {
 public:
  CViLastCommand();

  void clear();

  void addCount(uint n);

  void addKey(char key);

  void exec(CVi *vi);

 private:
  std::vector<char> keys_;
};

//---

class CViCmdLine {
 public:
  CViCmdLine() { }

  void setLine(const std::string &line) { line_ = line; }

  const std::string &getLine() const { return line_; }

  void cursorEnd() { }

  void keyPress(char c);

 private:
  std::string line_;
};

//---

// need abstract line and file ?
class CViLine {
 public:
  typedef std::string::const_iterator CharsCI;

  CViLine();
  CViLine(const CViLine &line);

  virtual ~CViLine();

  const CViLine &operator=(const CViLine &line);

  CViLine *dup() const;

  virtual void addChars(uint pos, const std::string &str);

  virtual void addChar(uint pos, char c);

  virtual void addChars(uint pos, uint num, char c);

  virtual uint getLength() const;

  virtual bool isEmpty() const;

  virtual CharsCI beginChar() const;
  virtual CharsCI endChar  () const;

  virtual void clear();

  virtual char getChar(uint pos) const;

  virtual void setChar(uint pos, char c);

  virtual void insertChar(uint pos, char c);

  virtual void replaceChar(uint pos, char c);

  virtual void deleteChars(uint pos, uint num);

  virtual void deleteChar(uint pos);

  virtual void replace(const std::string &str);

  virtual void replace(int spos, int epos, const std::string &str);

  // TODO: move out of class
  virtual void split(CViLine *line, uint pos);

  // TODO: move out of class
  virtual void join(CViLine *line);

  virtual uint getEnd(bool extraLineChar) const;

  virtual const std::string &getString() const;

  std::string getSubString(int spos, int epos) const;

  virtual const char *getCString() const;

  virtual void setChanged(bool value);

  virtual void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CViLine &line);

 private:
  const char *getSubCString(int spos, int epos) const;

 private:
  std::string chars_;
  bool        changed_ { false };
};

//---

class CViLines {
 public:
  typedef std::vector<CViLine *> LineList;

  typedef LineList::iterator       iterator;
  typedef LineList::const_iterator const_iterator;

 public:
  CViLines() :
   lines_() {
  }

 ~CViLines();

  uint size() const { return lines_.size(); }

  bool empty() const { return lines_.empty(); }

  void clear();

  const CViLine *getLine(uint line_num) const;

  CViLine *getLine(uint line_num);

  const_iterator begin() const { return lines_.begin(); }
  const_iterator end  () const { return lines_.end  (); }

  void addLine(uint line_num, CViLine *line);

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

class CViBufferLine {
 public:
  CViBufferLine(const std::string &line, bool new_line) :
   line_(line), new_line_(new_line) {
  }

  const std::string &getLine() const { return line_; }

  bool hasNewLine() const { return new_line_; }

 private:
  std::string line_;
  bool        new_line_;
};

//---

class CViBuffer {
 public:
  CViBuffer(char c='\0') : c_(c) { }

  char c() const { return c_; }

  void clear() { lines_.clear(); }

  uint getNumLines() const { return lines_.size(); }

  CViBufferLine *getLine(uint i) { return lines_[i]; }

  void addLine(const std::string &line, bool new_line) {
    lines_.push_back(new CViBufferLine(line, new_line));
  }

 private:
  typedef std::vector<CViBufferLine *> Lines;

  char  c_ { '\0' };
  Lines lines_;
};

//---

class CViOptions {
 public:
  CViOptions() : shift_width_(2) { }

  virtual ~CViOptions() { }

  virtual uint getShiftWidth() const { return shift_width_; }

 private:
  int shift_width_ { 2 };
};

//---

class CViInterface {
 public:
  CViInterface() { }

  virtual ~CViInterface() { }

  virtual void startGroup() { }
  virtual void endGroup  () { }
  virtual bool inGroup   () const { return false; }

  virtual void displayRegisters() const { }
  virtual void displayMarks() const { }

  virtual uint getTabStop() const { return 8; }

  virtual void saveLines(const std::string &fileName) { assert(fileName.size()); }

  virtual void undo() { }
  virtual void undoLine() { }
  virtual void redo() { }

  virtual void setIgnoreChanged(bool value) { assert(value); }

  virtual void stateChanged() { }

  virtual void updateSyntax() { }

  virtual void runEdCmd(const std::string &cmd, bool &quitted) {
    std::cerr << cmd << std::endl;

    quitted = false;
  }

  virtual void quit() { exit(0); }

  //------

  virtual int getPageTop() const { return 0; }
  virtual int getPageBottom() const { return 80; }
  virtual int getPageLength() const { return 80; }

  virtual void scrollTop() { }
  virtual void scrollMiddle() { }
  virtual void scrollBottom() { }
};

//---

struct CViKeyData {
  enum class KeyCode {
    NONE      = 0,
    ESCAPE    = 0x01000000,
    TAB       = 0x01000001,
    BACKTAB   = 0x01000002,
    BACKSPACE = 0x01000003,
    RETURN    = 0x01000004,
    ENTER     = 0x01000005,
    INSERT    = 0x01000006,
    DELETE    = 0x01000007,
    PAUSE     = 0x01000008,
    PRINT     = 0x01000009,
    SYS_REQ   = 0x0100000a,
    CLEAR     = 0x0100000b,
    HOME      = 0x01000010,
    END       = 0x01000011,
    LEFT      = 0x01000012,
    UP        = 0x01000013,
    RIGHT     = 0x01000014,
    DOWN      = 0x01000015,
    PAGE_UP   = 0x01000016,
    PAGE_DOWN = 0x01000017,
    SHIFT     = 0x01000020,
    CONTROL   = 0x01000021,
    META      = 0x01000022,
    ALT       = 0x01000023,
    CAPS_LOCK = 0x01000024,
    NUM_LOCK  = 0x01000025,
    F1        = 0x01000030,
    F2        = 0x01000031,
    F3        = 0x01000032,
    F4        = 0x01000033,
    F5        = 0x01000034,
    F6        = 0x01000035,
    F7        = 0x01000036,
    F8        = 0x01000036,
    F9        = 0x01000037,
    F10       = 0x01000038,
    F11       = 0x01000039,
    F12       = 0x0100003a,
    SUPER     = 0x01001103,
    HYPER     = 0x01001104,
  };

  int         key        { '\0' };
  std::string text;
  bool        is_shift   { false };
  bool        is_control { false };
  bool        is_alt     { false };
  bool        is_meta    { false };
};

class CVi : public CViInterface {
 public:
  using LinesCI = CViLines::const_iterator;

 public:
  CVi();

  virtual ~CVi();

  bool loadFile(const std::string &filename);

  bool getInsertMode() const { return insert_mode_; }
  void setInsertMode(bool insert_mode);

  bool getOverwriteMode() const { return overwrite_mode_; }
  void setOverwriteMode(bool value) { overwrite_mode_ = value; }

  bool getVisual() const { return visual_; }
  void setVisual(bool value) { visual_ = value; }

  void setCmdLineMode(bool cmd_line_mode, const std::string &str="");
  bool getCmdLineMode() const { return cmd_line_mode_; }

  std::string getCmdLineString() const;

  void processChar(const CViKeyData &keyData);
  void processChar(char key);

  void processInsertChar (const CViKeyData &key);
  void processCommandChar(const CViKeyData &key);
  void processNormalChar (const CViKeyData &key);
  void processControlChar(const CViKeyData &key);
  void processCmdLineChar(const CViKeyData &key);

  void normalInsertChar(char key);

  bool processMoveChar(const CViKeyData &keyData, int x, int y);

  bool doFindChar(char c, uint count, bool forward, bool till);

  void setExtraLineChar(bool extra_char) { extra_char_ = extra_char; }
  bool getExtraLineChar() const { return extra_char_; }

  void error(const std::string &msg) const;

  void getPos(uint *x, uint *y) const { *y = cursorPos_.row; *x = cursorPos_.col; }
  void setPos(uint x, uint y) { cursorPos_.row = y;  cursorPos_.col = x; }

  LinesCI linesBegin() const { return lines_.begin(); }
  LinesCI linesEnd  () const { return lines_.end  (); }

  uint getNumLines() const { return lines_.size(); }

 private:
  bool cursorLeft (uint n=1);
  bool cursorRight(uint n=1);
  bool cursorUp   (uint n=1);
  bool cursorDown (uint n=1);
  bool cursorLeft (uint n, uint *row, uint *col);
  bool cursorRight(uint n, uint *row, uint *col);
  bool cursorUp   (uint n, uint *row, uint *col);
  bool cursorDown (uint n, uint *row, uint *col);

  void cursorToLeft();
  void cursorToRight();
  void cursorToLeft(uint *row, uint *col);
  void cursorToRight(uint *row, uint *col);

  void cursorFirstNonBlankUp  ();
  void cursorFirstNonBlankDown();
  void cursorFirstNonBlankUp  (uint *row, uint *col);
  void cursorFirstNonBlankDown(uint *row, uint *col);

  void cursorFirstNonBlank();
  void cursorFirstNonBlank(uint *row, uint *col);

  void cursorSkipSpace();
  void cursorSkipSpace(uint *line_num, uint *char_num);

  void cursorTo(uint row, uint col);

  void deleteLine();
  void deleteLine(uint line_num);

  void deleteWord();
  void deleteWord(uint line_num, uint char_num);

  void deleteEOL();
  void deleteEOL(uint line_num, uint char_num);

  void deleteTo(uint line_num, uint char_num);
  void deleteTo(uint line_num1, uint char_num1, uint line_num2, uint char_num2);

  void deleteChar();
  void deleteChar(uint line_num);

  void shiftLeft (uint y1, uint y2);
  void shiftRight(uint y1, uint y2);

  void deleteChars(uint n);
  void deleteChars(uint line_num, uint char_num, uint n);

  void addLine(const std::string &str);
  void addLine(uint line_num, const std::string &str);

  void addChars(uint line_num, uint char_num, const std::string &chars);

  void insertChar(char c);
  void insertChar(uint line_num, uint char_num, char c);

  void replaceChar(char c);
  void replaceChar(uint line_num, uint char_num, char c);

  bool replace(uint line_num, uint char_num, char c);
  bool replace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr);

  void newLineAbove();
  void newLineBelow();

  void splitLine();
  void splitLine(uint line_num, uint char_num);

  void joinLine();
  void joinLine(uint line_num);

  void nextWord();
  void nextWord(uint *row, uint *col);
  void nextWORD();
  void nextWORD(uint *row, uint *col);

  void prevWord();
  void prevWord(uint *row, uint *col);
  void prevWORD();
  void prevWORD(uint *row, uint *col);

  void endWord();
  void endWord(uint *row, uint *col);
  void endWORD();
  void endWORD(uint *row, uint *col);

  bool getWord(std::string &word);
  bool getWord(uint line_num, uint char_num, std::string &word);

  void nextSentence();
  void nextSentence(uint *line_num, uint *char_num);
  void prevSentence();
  void prevSentence(uint *line_num, uint *char_num);

  bool isSentenceEnd(const CViLine *line, uint col, uint *num) const;

  void nextParagraph();
  void nextParagraph(uint *row, uint *col);
  void prevParagraph();
  void prevParagraph(uint *row, uint *col);

  void nextSection();
  void nextSection(uint *line_num, uint *char_num);
  void prevSection();
  void prevSection(uint *line_num, uint *char_num);

  bool isSection(const CViLine *line, uint col, uint *num) const;

  bool isBlank(const CViLine *line) const;

  bool nextLine(uint *line_num, uint *char_num);
  bool prevLine(uint *line_num, uint *char_num);

  void swapChar();
  void swapChar(uint line_num, uint char_num);

  bool findNext(const std::string &str);
  bool findNext(const std::string &pattern, uint *fline_num, uint *fchar_num);
  bool findNext(const std::string &pattern, uint line_num, int char_num);
  bool findNext(const std::string &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num);
  bool findNext(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2);
  bool findNext(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num);

  bool findNext(const CRegExp &pattern, uint *len=0);
  bool findNext(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len=0);
  bool findNext(const CRegExp &pattern, uint line_num, int char_num, uint *len=0);
  bool findNext(const CRegExp &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num, uint *len=0);
  bool findNext(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *len=0);
  bool findNext(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len=0);

  bool findPrev(const std::string &str);
  bool findPrev(const std::string &pattern, uint *fline_num, uint *fchar_num);
  bool findPrev(const std::string &pattern, uint line_num, int char_num);
  bool findPrev(const std::string &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num);
  bool findPrev(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2);
  bool findPrev(const std::string &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num);

  bool findPrev(const CRegExp &pattern, uint *len=0);
  bool findPrev(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len=0);
  bool findPrev(const CRegExp &pattern, uint line_num, int char_num, uint *len=0);
  bool findPrev(const CRegExp &pattern, uint line_num, int char_num,
                uint *fline_num, uint *fchar_num, uint *len=0);
  bool findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *len=0);
  bool findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
                int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len=0);

  bool findNextChar(char c, bool multiline);
  bool findNextChar(const std::string &str, bool multiline);
  bool findNextChar(uint line_num, int char_num, char c, bool multiline);
  bool findNextChar(uint line_num, int char_num, const std::string &str, bool multiline);

  bool findPrevChar(char c, bool multiline);
  bool findPrevChar(const std::string &str, bool multiline);
  bool findPrevChar(uint line_num, int char_num, char c, bool multiline);
  bool findPrevChar(uint line_num, int char_num, const std::string &str, bool multiline);

  bool findNext(const CViLine *line, const std::string &pattern, int char_num1,
                int char_num2, uint *char_num) const;
  bool findNext(const CViLine *line, const CRegExp &pattern, int char_num1,
                int char_num2, uint *spos, uint *epos) const;

  bool findPrev(const CViLine *line, const std::string &pattern, int char_num1,
                int char_num2, uint *char_num) const;
  bool findPrev(const CViLine *line, const CRegExp &pattern, int char_num1,
                int char_num2, uint *spos, uint *epos) const;

  void yankLines(char id, uint n);
  void yankLines(char id, uint line_num, uint n);

  void yankWords(char id, uint n);
  void yankWords(char id, uint line_num, uint char_num, uint n);

  void yankChar(char id);
  void yankChar(char id, uint line_num, uint char_num);

  void yankTo(char id, uint line_num, uint char_num, bool is_line);
  void yankTo(char id, uint line_num1, uint char_num1, uint line_num2,
              uint char_num2, bool is_line);

  void yankClear(char id);

  void pasteAfter (char c);
  void pasteBefore(char c);
  void pasteAfter (char id, uint line_num, uint char_num);
  void pasteBefore(char id, uint line_num, uint char_num);

  uint getRow() const;
  uint getCol() const;

  char getChar() const;
  char getChar(uint line_num, uint char_num) const;

  bool hasFindPattern() const { return findPattern_.isValid(); }
  const CRegExp &getFindPattern() const { return findPattern_.getValue(); }
  void setFindPattern(const CRegExp &pattern) { findPattern_.setValue(pattern); }

  bool getMarkPos(const std::string &name, uint *row, uint *col);
  void setMarkPos(const std::string &name);
  void markReturn();

  CViBuffer &getBuffer(char c);

  bool isWordChar(char c) const;

  const std::string &getFileName() const { return fileName_; }

  bool isLinesEmpty() const { return lines_.empty(); }

  const CViLine *getLine(uint line_num) const { return lines_.getLine(line_num); }

  CViLine *getLine(uint line_num) { return lines_.getLine(line_num); }

  void getSelectStart(int *row, int *col) const;
  void getSelectEnd(int *row, int *col) const;
  void setSelectRange(int row1, int col1, int row2, int col2);
  void clearSelection();
  void rangeSelect(int row1, int col1, int row2, int col2, bool select);

  void subAddLine(uint line_num, CViLine *line);
  void subAddChars(uint line_num, uint char_num, const std::string &chars);
  void subMoveLine(uint line_num1, int line_num2);
  void subDeleteAllLines();
  void subDeleteLine(uint line_num);
  void subDeleteChars(uint line_num, uint char_num, uint n);
  void subYankTo(char id, uint line_num1, uint char_num1, uint line_num2,
                 uint char_num2, bool is_line);
  void subInsertChar(uint line_num, uint char_num, char c);
  void subReplaceChar(uint line_num, uint char_num, char c);
  void subSplitLine(uint line_num, uint char_num);
  void subJoinLine(uint line_num);
  bool subReplace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr);

  void setChanged(bool value) { changed_ = value; }
  void setUnsaved(bool value) { unsaved_ = value; }

  CViOptions &getOptions() { return options_; }

 private:
  struct MarkPos {
    uint row { 0 };
    uint col { 0 };
    bool set { false };

    MarkPos(uint row1=0, uint col1=0, bool set1=false) :
     row(row1), col(col1), set(set1) {
    }
  };

  struct CursorPos {
    uint row { 0 };
    uint col { 0 };

    CursorPos(uint row1=0, uint col1=0) :
     row(row1), col(col1) {
    }
  };

  struct Selection {
    uint row1 { 0 };
    uint col1 { 0 };
    uint row2 { 0 };
    uint col2 { 0 };
    bool set  { false };

    Selection(uint row1_=0, uint col1_=0, uint row2_=9, uint col2_=0, bool set_=false) :
     row1(row1_), col1(col1_), row2(row2_), col2(col2_), set(set_) {
    }
  };

  using MarkPosMap = std::map<std::string,MarkPos>;
  using Buffers    = std::map<char,CViBuffer>;
  using Lines      = std::vector<CViLine>;

  std::string        fileName_;
  char               lastKey_        { '\0' };
  uint               count_          { 0 };
  bool               insert_mode_    { false };
  bool               overwrite_mode_ { false };
  bool               cmd_line_mode_  { false };
  bool               extra_char_     { false };
  bool               visual_         { false };
  bool               changed_        { false };
  bool               unsaved_        { false };
  CViCmdLine        *cmd_line_       { nullptr };
  char               register_       { '\0' };
  CViLastCommand     lastCommand_;
  char               find_char_      { '\0' };
  bool               find_forward_   { false };
  bool               find_till_      { false };
  COptValT<CRegExp>  findPattern_;
  CursorPos          cursorPos_;
  MarkPosMap         markPosMap_;
  Buffers            buffers_;
  CViLines           lines_;
  CViOptions         options_;
  Selection          selection_;
};

#endif
