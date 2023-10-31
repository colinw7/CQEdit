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

 private:
  CQEdit*       edit_   { nullptr };
  CQEditCanvas* canvas_ { nullptr };
  QScrollBar*   vbar_   { nullptr };
  QScrollBar*   hbar_   { nullptr };
};

//---

class CQEditCanvas : public QWidget {
  Q_OBJECT

 public:
  CQEditCanvas(CQEditArea *edit);

  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;

  void keyPressEvent(QKeyEvent *event) override;
  void keyReleaseEvent(QKeyEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

  //void selectionNotify(CButtonAction action);

  bool focusNextPrevChild(bool) override { return false; }

 private:
  void selectionNotify(const QPoint &, const QPoint &);

 signals:
  void sizeChanged();

 private:
  CQEditArea*  area_    { nullptr };
  CQEdit*      edit_    { nullptr };
  QImage       qimage_;
  bool         pressed_ { false };
  CMouseButton button_  { CBUTTON_NONE };
};

#endif
