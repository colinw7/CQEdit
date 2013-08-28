#ifndef CQEDIT_FILE_H
#define CQEDIT_FILE_H

#include <QFont>

#include <CVEditFile.h>
#include <CVEditCursor.h>

class CQEdit;

class CQEditFile : public CVEditFile {
 private:
  CQEdit *edit_;

 public:
  CQEditFile(CQEdit *edit);

  CQEdit *getEdit() const { return edit_; }

  const QFont &getQFont() const;

  void setQFont(const QFont &font);

  void setFileName(const std::string &fileName);

  void setCmdText(const std::string &str);

  void stateChanged();

  void selectionChanged(const std::string &str);

  void update();

  void saveAndQuit();

  void quit();

  void displayMessage(const StringList &lines);
  void displayError  (const StringList &lines);

  void displayMarks();
  void displayRegisters();

  CEditCursor *createCursor();
};

class CQEditCursor  : public CVEditCursor {
 private:
  CQEditFile *file_;

 public:
  CQEditCursor(CQEditFile *file);

  virtual void setPos(const CIPoint2D &pos);
};

#endif
