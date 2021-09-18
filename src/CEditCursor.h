#ifndef CEDIT_CURSOR_H
#define CEDIT_CURSOR_H

#include <CIPoint2D.h>
#include <string>
#include <sys/types.h>

class CEditFile;

class CEditCursor {
 public:
  // last row and text for undo line
  struct LastLine {
    bool        set;
    uint        row;
    std::string text;

    LastLine() :
     set(false), row(0), text() {
    }
  };

 public:
  CEditCursor(CEditFile *file);
  CEditCursor(const CEditCursor &c);

  virtual ~CEditCursor();

  CEditCursor &operator=(const CEditCursor &c);

  virtual CEditCursor *dup() const;

  const CIPoint2D &getPos() const { return pos_; }

  virtual void setPos(const CIPoint2D &pos);

  const LastLine &getLastLine() const { return last_line_; }

  void updateLastLine();

  virtual void print(std::ostream &os) const;

  friend std::ostream &operator<<(std::ostream &os, const CEditCursor &c);

 protected:
  friend class CEditFile;

  void setFile(CEditFile *file) { file_ = file; }

 private:
  void setLastRow(uint row);

 protected:
  CEditFile *file_;
  CIPoint2D  pos_;
  LastLine   last_line_;
};

#endif
