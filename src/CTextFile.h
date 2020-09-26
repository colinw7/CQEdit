#ifndef CTEXT_FILE_H
#define CTEXT_FILE_H

#include <CRefPtr.h>

#include <string>
#include <vector>
#include <list>
#include <cassert>
#include <cstddef>
#include <cmath>
#include <sys/types.h>

class CTextFileNotifyMgr;

// notifier
class CTextFileNotifier {
 public:
  CTextFileNotifier();

  virtual ~CTextFileNotifier() { }

  virtual void fileOpened(const std::string &fileName);

  virtual void positionChanged(uint x, uint y);

  virtual void lineAdded   (const std::string &line, uint line_num);
  virtual void lineDeleted (const std::string &line, uint line_num);
  virtual void lineReplaced(const std::string &line1, const std::string &line2, uint line_num);

  virtual void charAdded   (char c, uint line_num, uint char_num);
  virtual void charDeleted (char c, uint line_num, uint char_num);
  virtual void charReplaced(char c1, char c2, uint line_num, uint char_num);

  virtual void startGroup();
  virtual void endGroup  ();
};

//------

// interface
class CTextFileIFace {
 public:
  class LineIteratorImpl {
   public:
    LineIteratorImpl(CTextFileIFace *file) : file_(file), end_(false) { }

    LineIteratorImpl(const LineIteratorImpl &i) : file_(i.file_), end_(i.end_) { }

    virtual void toEnd() { end_ = true; }

    virtual ~LineIteratorImpl() { }

    virtual void next() = 0;

    virtual bool equal(const LineIteratorImpl &impl) const = 0;

    virtual std::string &line() const = 0;

    virtual uint lineNum() const = 0;

   protected:
    CTextFileIFace *file_;
    bool            end_;
  };

  //------

  class SimpleLineIteratorImpl : public LineIteratorImpl {
   public:
    SimpleLineIteratorImpl(CTextFileIFace *file) : LineIteratorImpl(file), line_num_(0) { }

    SimpleLineIteratorImpl(const SimpleLineIteratorImpl &i) :
     LineIteratorImpl(i), line_num_(i.line_num_) {
    }

    void next() { ++line_num_; end_ = (line_num_ > file_->getNumLines()); }

    bool equal(const LineIteratorImpl &i) const {
      const SimpleLineIteratorImpl &i1 = static_cast<const SimpleLineIteratorImpl &>(i);

      if (end_ && i1.end_) return true;
      if (end_ || i1.end_) return false;

      return (line_num_ == i1.line_num_);
    }

    std::string &line() const { static std::string l; l = file_->getLine(line_num_); return l; }

    uint lineNum() const { return line_num_; }

   protected:
    uint line_num_;
  };

  typedef CRefPtr<LineIteratorImpl> LineIteratorImplP;

  //------

  class LineIterator {
   public:
    typedef std::forward_iterator_tag iterator_category;
    typedef std::string               value_type;
    typedef ptrdiff_t                 difference_type;
    typedef value_type *              pointer;
    typedef value_type &              reference;

    LineIterator() : impl_() { }

    LineIterator(LineIteratorImplP impl) : impl_(impl) { }

    LineIterator(const LineIterator &i) : impl_(i.impl_) { }

    LineIterator &toEnd() { impl_->toEnd(); return *this; }

    const value_type &operator->() const { return line(); }
    const value_type &operator* () const { return line(); }

    LineIterator &operator++() { impl_->next(); return *this; }

    bool operator==(const LineIterator &i) { return impl_->equal(*i.impl_); }
    bool operator!=(const LineIterator &i) { return ! (*this == i); }

    const value_type &line() const { return impl_->line(); }

    uint lineNum() const { return impl_->lineNum(); }

   public:
    LineIteratorImplP impl_;
  };

  //------

  CTextFileIFace() { }

  virtual ~CTextFileIFace() { }

  // read/write
  virtual bool read (const char *fileName) = 0;
  virtual bool write(const char *fileName) = 0;

  // reset
  virtual void removeAllLines() = 0;

  // move
  virtual void moveTo(uint x, uint y) = 0;
  virtual void rmoveTo(int dx, int dy) = 0;

  virtual void getPos(uint *x, uint *y) const = 0;

  // file info
  virtual void setFileName(const std::string &fileName) = 0;
  virtual const std::string &getFileName() = 0;

  // inquire
  virtual char               getChar() const = 0;
  virtual const std::string &getLine() const = 0;

  virtual const std::string &getLine(uint y) const = 0;

  virtual uint getLineLength() const = 0;
  virtual uint getNumLines  () const = 0;

  // edit
  virtual void addCharAfter (char c) = 0;
  virtual void addCharBefore(char c) = 0;

  virtual void addLineAfter (const std::string &l) = 0;
  virtual void addLineBefore(const std::string &l) = 0;

  virtual void deleteCharAt() = 0;
  virtual void deleteCharBefore() = 0;

  virtual void deleteLineAt() = 0;
  virtual void deleteLineBefore() = 0;

  virtual void replaceChar(char c) = 0;
  virtual void replaceLine(const std::string &l) = 0;

  // visual
  virtual uint getPageTop() const = 0;
  virtual void setPageTop(uint pos) = 0;

  virtual uint getPageBottom() const = 0;
  virtual void setPageBottom(uint pos) = 0;

  // grouping for undo/redo
  virtual void startGroup() { }
  virtual void endGroup  () { }

  // iteration
  virtual LineIterator beginLine() = 0;
  virtual LineIterator endLine  () = 0;
};

//------

class CTextFileCursor {
 public:
  CTextFileCursor() { }

  void moveTo(uint x, uint y) { x_ = x; y_ = y; }

  void rmoveTo(int dx, int dy) {
    assert((dx > 0 || std::abs(dx) <= int(x_)) || (dy > 0 || std::abs(dy) <= int(y_)));

    x_ += dx;
    y_ += dy;
  }

  void setX(uint x) { x_ = x; }
  void setY(uint y) { y_ = y; }

  uint getX() const { return x_; }
  uint getY() const { return y_; }

 private:
  uint x_ { 0 }, y_ { 0 };
};

//------

struct CTextFileInfo {
  std::string fileName;

  CTextFileInfo() :
   fileName("") {
  }
};

//------

class CTextLine {
 public:
  CTextLine(const std::string &line) : line_(line) { }

  void setLine(const std::string &line) { line_ = line; }

  uint getLength() const { return line_.size(); }

  char getChar(uint pos) const;

  const std::string &getString() const;

  void addCharAfter (uint pos, char c);
  void addCharBefore(uint pos, char c);

  void deleteCharAt    (uint pos);
  void deleteCharBefore(uint pos);

  void replaceChar(uint pos, char c);

  void replaceString(const std::string &str);

 private:
  std::string line_;
};

//------

class CTextFile : public CTextFileIFace {
 public:
  CTextFile(const char *fileName=nullptr);
 ~CTextFile();

  void addNotifier   (CTextFileNotifier *notifier);
  void removeNotifier(CTextFileNotifier *notifier);

  // read/write
  bool read (const char *fileName);
  bool write(const char *fileName);

  // read/write
  void removeAllLines();

  // move
  void moveTo(uint x, uint y);
  void rmoveTo(int dx, int dy);

  void getPos(uint *x, uint *y) const;

  // file info
  void setFileName(const std::string &fileName);
  const std::string &getFileName();

  char               getChar() const;
  const std::string &getLine() const;

  const std::string &getLine(uint y) const;

  uint getLineLength() const;
  uint getNumLines  () const;

  // edit
  void addCharAfter(char c);
  void addCharBefore(char c);

  void addLineAfter (const std::string &line);
  void addLineBefore(const std::string &line);

  void deleteCharAt();
  void deleteCharBefore();

  void deleteLineAt();
  void deleteLineBefore();

  void replaceChar(char c);
  void replaceLine(const std::string &l);

  // visual
  uint getPageTop   () const;
  void setPageTop   (uint pos);
  uint getPageBottom() const;
  void setPageBottom(uint pos);

  // grouping for undo/redo
  void startGroup();
  void endGroup  ();

  // iteration
  LineIterator beginLine();
  LineIterator endLine  ();

 private:
  bool getLine(CTextLine **line);
  bool getLine(const CTextLine **line) const;

  void moveToLine(int y);
  void moveToChar(int x);

 protected:
  virtual CTextLine *allocLine(const std::string &line);

 private:
  typedef std::vector<CTextLine *> LineList;

  CTextFileInfo       fileInfo_;
  CTextFileCursor     cursor_;
  LineList            oldLines_;
  LineList            lines_;
  int                 pageTop_    { -1 };
  int                 pageBottom_ { -1 };
  CTextFileNotifyMgr* notifyMgr_  { nullptr };
};

//------

class CTextFileNotifyMgr {
 public:
  CTextFileNotifyMgr(CTextFile *file);

  void addNotifier   (CTextFileNotifier *notifier);
  void removeNotifier(CTextFileNotifier *notifier);

  void notifyFileOpened();

  void notifyPositionChanged();

  void notifyLineAdded   (const std::string &line, uint line_num);
  void notifyLineDeleted (const std::string &line, uint line_num);
  void notifyLineReplaced(const std::string &line1, const std::string &line, uint line_num);
  void notifyCharAdded   (char c, uint line_num, uint char_num);
  void notifyCharDeleted (char c, uint line_num, uint char_num);
  void notifyCharReplaced(char c1, char c, uint line_num, uint char_num);

  void notifyStartGroup();
  void notifyEndGroup  ();

 private:
  typedef std::list<CTextFileNotifier *> NotifierList;

  CTextFile    *file_ { nullptr };
  NotifierList  notifierList_;
};

#endif
