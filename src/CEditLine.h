#ifndef CEDIT_LINE_H
#define CEDIT_LINE_H

#include <CPOptVal.h>
#include <vector>
#include <iostream>

class CRegExp;
class CEditChar;
class CEditFile;
class CEditLine;

class CEditLineChars {
 public:
  using CharList = std::vector<CEditChar *>;

  using iterator       = CharList::iterator;
  using const_iterator = CharList::const_iterator;

 public:
  CEditLineChars() :
   chars_() {
  }

  CEditLineChars(const CEditLineChars &chars);

 ~CEditLineChars();

  CEditLineChars &operator=(const CEditLineChars &chars);

  void clear();

  uint size() const { return chars_.size(); }

  bool empty() const { return chars_.empty(); }

  iterator begin() { return chars_.begin(); }
  iterator end  () { return chars_.end  (); }

  const_iterator begin() const { return chars_.begin(); }
  const_iterator end  () const { return chars_.end  (); }

  const CEditChar *getChar(uint char_num) const;

  iterator getCharI(uint char_num);

  void addChar(CEditChar *c);

  void addChars(uint pos, const std::vector<CEditChar *> &chars);

  void setChar(uint pos, CEditChar *c);

  void insertChar(uint pos, CEditChar *c);

  void deleteChars(uint pos, uint num);

  void deleteChar(uint pos);

  void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CEditLineChars &chars);

 private:
  CharList chars_;
};

class CEditLineUtil {
 public:
  CEditLineUtil(CEditLine *line) :
   line_(line) {
  }

  bool isBlank() const;

  bool isSentenceEnd(uint pos, uint *n) const;

  bool isSection(uint pos, uint *n) const;

  bool findNext(const std::string &str, int char_num1=0, int char_num2=-1, uint *char_num=0) const;
  bool findNext(const CRegExp &pattern, int char_num1=0,
                int char_num2=-1, uint *spos=0, uint *epos=0) const;

  bool findPrev(const std::string &str, int char_num1=0, int char_num2=-1, uint *char_num=0) const;
  bool findPrev(const CRegExp &pattern, int char_num1=0,
                int char_num2=-1, uint *spos=0, uint *epos=0) const;

 private:
  CEditLine *line_;
};

class CEditLine {
 public:
  using const_char_iterator = CEditLineChars::const_iterator;

 public:
  CEditLine(CEditFile *file);

  CEditLine(const CEditLine &line);

  virtual ~CEditLine();

  CEditLine &operator=(const CEditLine &line);

  virtual CEditLine *dup() const;

  // Chars
  virtual void addChars(uint pos, const std::string &line);
  virtual void addChar (uint pos, char c);
  virtual void addChars(uint pos, uint num, char c);

  virtual const_char_iterator beginChar() const;
  virtual const_char_iterator endChar  () const;

  virtual uint getLength() const;

  virtual bool isEmpty() const;

  bool isBlank() const;

  bool isSentenceEnd(uint pos, uint *n) const;

  bool isSection(uint pos, uint *n) const;

  virtual void clear();

  virtual const CEditChar *getCharP(uint pos) const;

  virtual char getChar(uint pos) const;
  virtual void setChar(uint pos, char c);

  virtual void insertChar(uint pos, char c);

  virtual void replaceChar(uint pos, char c);

  virtual void deleteChars(uint pos, uint num);

  virtual void deleteChar(uint pos);

  bool findNext(const std::string &str, int char_num1=0, int char_num2=-1, uint *char_num=0) const;
  bool findNext(const CRegExp &pattern, int char_num1=0,
                int char_num2=-1, uint *spos=0, uint *epos=0) const;

  bool findPrev(const std::string &str, int char_num1=0, int char_num2=-1, uint *char_num=0) const;
  bool findPrev(const CRegExp &pattern, int char_num1=0,
                int char_num2=-1, uint *spos=0, uint *epos=0) const;

  virtual void replace(const std::string &str);
  virtual void replace(int spos, int epos, const std::string &str);

  virtual void split(CEditLine *line, uint pos);

  virtual void join(CEditLine *line);

  virtual std::string getString() const;

  virtual std::string getSubString(int spos, int epos) const;

  virtual const char *getCString() const;

  virtual const char *getSubCString(int spos, int epos) const;

  // changed
  bool getChanged() const { return changed_; }

  virtual void setChanged(bool changed);

  // print
  virtual void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CEditLine &line);

 protected:
  CEditFile      *file_    { nullptr };
  CEditLineUtil   util_;
  CEditLineChars  chars_;
  bool            changed_ { false };
};

#endif
