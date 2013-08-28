#ifndef CQEditCanvas_H
#define CQEditCanvas_H

#include <QWidget>
#include <CEvent.h>

class CQEdit;
class CQEditCanvas;

class QExposeEvent;
class QResizeEvent;
class QKeyEvent;
class QMouseEvent;
class QScrollBar;

class CQEditArea : public QWidget {
  Q_OBJECT

 private:
  CQEdit       *edit_;
  CQEditCanvas *canvas_;
  QScrollBar   *vbar_;
  QScrollBar   *hbar_;

 public:
  CQEditArea(CQEdit *edit);

  void updateScrollbars();

  CQEdit *getEdit() const { return edit_; }

  CQEditCanvas *getCanvas() const { return canvas_; }

 signals:
  void sizeChanged();

 private slots:
  void hscrollSlot(int x);
  void vscrollSlot(int y);
};

class CQEditCanvas : public QWidget {
  Q_OBJECT

 private:
  CQEditArea   *area_;
  CQEdit       *edit_;
  QImage        qimage_;
  bool          pressed_;
  CMouseButton  button_;

 public:
  CQEditCanvas(CQEditArea *edit);

  void paintEvent(QPaintEvent *);
  void resizeEvent(QResizeEvent *);

  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);

  void mousePressEvent(QMouseEvent *event);
  void mouseMoveEvent(QMouseEvent *event);
  void mouseReleaseEvent(QMouseEvent *event);

  //void selectionNotify(CButtonAction action);

  bool focusNextPrevChild(bool) { return false; }

 private:
  void selectionNotify(const QPoint &, const QPoint &);

 signals:
  void sizeChanged();
};

#endif
