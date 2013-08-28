#ifndef CQDIALOG_H
#define CQDIALOG_H

#include <QDialog>

class CQDialog : public QDialog {
  Q_OBJECT

 private:
  QWidget *frame_;
  bool     accepted_;

 public:
  CQDialog(QWidget *parent = NULL);

  virtual ~CQDialog() { }

  void init();

  bool accepted() const { return accepted_; }

 protected:
  virtual void createWidgets(QWidget *) { }

  virtual void accept() { }
  virtual void reject() { }

 protected slots:
  void acceptSlot();
  void rejectSlot();
};

#endif
