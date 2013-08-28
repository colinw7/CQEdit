#ifndef CQHISTORY_LINE_EDIT_H
#define CQHISTORY_LINE_EDIT_H

#include <QLineEdit>
#include <CAutoPtr.h>

class CHistory;

class CQHistoryLineEdit : public QLineEdit {
  Q_OBJECT

 private:
  CAutoPtr<CHistory> history_;

 public:
  CQHistoryLineEdit(QWidget *parent=NULL);
 ~CQHistoryLineEdit();

 private slots:
  void execSlot();

 signals:
  void exec(const QString &cmd);

 private:
  void keyPressEvent(QKeyEvent *event);
};

#endif
