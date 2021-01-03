#include <CVi.h>

#include <QLabel>
#include <memory>

class QScrollBar;

class CQVi;

class CQViCanvas : public QWidget {
 public:
  CQViCanvas(CQVi *vi);

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  CQVi *vi_ { nullptr };
};

//------

class CQViStatus : public QLabel {
 public:
  CQViStatus(CQVi *vi);

  CQVi *vi() const { return vi_; }

 private:
  CQVi *vi_ { nullptr };
};

//------

class CQVi : public QWidget {
  Q_OBJECT

 public:
  CQVi(QWidget *parent=nullptr);

  void loadFile(const std::string &filename);

  void initFont(const QFont &font);

  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;

  void mousePressEvent  (QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent   (QMouseEvent *) override;

  void wheelEvent(QWheelEvent *) override;

  void keyPressEvent(QKeyEvent *) override;

  QSize sizeHint() const override;

  QSize canvasSizeHint() const;

  void draw(QPainter *painter);

  void updateScrollbars();

  void updateStatus();

 private slots:
  void hscrollSlot(int);
  void vscrollSlot(int);

 private:
  using YLineMap = std::map<int, int>;

  using ViP = std::unique_ptr<CVi>;

  ViP         vi_;
  CQViCanvas* canvas_        { nullptr };
  QScrollBar* hscroll_       { nullptr };
  QScrollBar* vscroll_       { nullptr };
  CQViStatus* status_        { nullptr };
  QRect       char_rect_;
  int         char_width_    { 8 };
  int         char_ascent_   { 10 };
  int         char_height_   { 12 };
  int         x_offset_      { 0 };
  int         y_offset_      { 0 };
  YLineMap    yLineMap_;
  uint        maxLineLength_ { 0 };
  QPoint      press_pos_;
};
