#ifndef CQEDIT_H
#define CQEDIT_H

#include <QWidget>

#include <CAutoPtr.h>
#include <CIBBox2D.h>
#include <CEvent.h>
#include <CQEditFile.h>
#include <CQWinWidget.h>

class QTimer;
class QPainter;

class CQEditArea;
class CQHistoryLineEdit;
class CQEditMarks;
class CQEditRegisters;

class CQEdit : public QWidget {
  Q_OBJECT

 private:
  CAutoPtr<CQEditFile>  file_;
  CAutoPtr<CQEditArea>  area_;
  CQHistoryLineEdit    *cmd_;
  CQEditMarks          *marks_;
  CQEditRegisters      *registers_;

 public:
  static void init();

  CQEdit(QWidget *parent=0);

  virtual ~CQEdit();

  CQEditFile *getFile() const { return file_; }
  CQEditArea *getArea() const { return area_; }

  QWidget *getCanvasWidget() const;

  void draw(QPainter *painter);

  void update();

  void keyPress  (const CKeyEvent   &event);
  void keyRelease(const CKeyEvent   &event);

  void mousePress  (const CMouseEvent &event);
  void mouseMotion (const CMouseEvent &event);
  void mouseRelease(const CMouseEvent &event);

  bool runEdCmd(const std::string &cmd);

  void selectionNotify(const CIBBox2D &bbox);

  void setFileName(const std::string &fileName);

  void sendOutputMsg(const std::string &msg);
  void sendErrorMsg (const std::string &msg);

  void setCmdText(const std::string &str);

  void sendStateChanged();

  void displayMarks();
  void displayRegisters();

  void gotoMark(const std::string &);
  void pasteBuffer(const std::string &);

  void quit();

  void setFocus();

 private slots:
  void runCmd(const QString &cmd);

 signals:
  void stateChanged();

  void fileNameChanged();

  void sizeChanged();

  void outputMsg(const QString &msg);
  void errorMsg (const QString &msg);

  void quitCommand();
};

class CQEditMarks : public CQWinTree {
  Q_OBJECT

 private:
  CQEdit *edit_;

 public:
  CQEditMarks(CQEdit *edit, QWidget *parent);

  void populate(CQEditFile *file);

 private slots:
  void itemClickedSlot(QTreeWidgetItem *);
};

class CQEditRegisters : public CQWinTree {
  Q_OBJECT

 private:
  CQEdit *edit_;

 public:
  CQEditRegisters(CQEdit *edit, QWidget *parent);

  void populate(CQEditFile *file);

 private slots:
  void itemClickedSlot(QTreeWidgetItem *);
};

#endif
