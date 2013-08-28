#ifndef CQFLOAT_EDIT_H
#define CQFLOAT_EDIT_H

#include <QLineEdit>

class CQFloatEdit : public QLineEdit {
  Q_OBJECT

 public:
  CQFloatEdit(QWidget *parent);

  void display(const QRect &rect, const QString &text);

  void hide();

 private:
  bool event(QEvent *e);

  bool eventFilter(QObject *obj, QEvent *event);

 private slots:
  void acceptSlot();

 signals:
  void valueChanged(const QString &text);

 private:
  bool    valid_;
  QString saveText_;
};

#endif
