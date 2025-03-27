#include <string>

class CViLine;

class CViFile {
 public:
  typedef CViLines::const_iterator LinesCI;

 public:
  CViFile();

  bool load(const std::string &filename);
  bool save();

  const std::string &getFileName() const { return filename_; }

  void setFileName(const std::string &filename);

  uint getNumLines() const;

  bool isLinesEmpty() const;

  LinesCI linesBegin() const;
  LinesCI linesEnd  () const;

  CViLine *getLine(uint line_num);

  void clearLines();

  CViLine *addLineAfter (uint line_num, const std::string &line);
  CViLine *addLineBefore(uint line_num, const std::string &line);

  void deleteLine(uint line_num);

 private:
  std::string filename_;
  CViLines    lines_;
};

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

  const_iterator begin() const { return lines_.begin(); }
  const_iterator end  () const { return lines_.end  (); }

  CViLine *getLine(uint line_num);

  void clear();

  void addLineAfter (uint line_num, CViLine *line);
  void addLineBefore(uint line_num, CViLine *line);

  void deleteLine(uint line_num);

 private:
  LineList lines_;
};

typedef char CViChar;

class CViLine {
 public:
  typedef std::string::const_iterator CharsCI;

  CViLine(const std::string &line);

  uint getLength() const;

  bool isEmpty() const;

  CharsCI beginChar() const;
  CharsCI endChar  () const;

  CViChar &getChar(uint char_num);

  void clear();

  void addCharAfter (uint char_num, const CViChar &c);
  void addCharBefore(uint char_num, const CViChar &c);

  void deleteChar(uint char_num);

 private:
  typedef std::vector<CViChar> Chars;

  Chars chars_;
};
