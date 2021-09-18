#include <CVi.h>

#include <QLabel>
#include <memory>

class QScrollBar;

class CQViApp;

class CQViCanvas : public QWidget {
  Q_OBJECT

 public:
  CQViCanvas(CQViApp *app);

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  CQViApp *app_ { nullptr };
};

//------

class CQViStatus : public QLabel {
  Q_OBJECT

 public:
  CQViStatus(CQViApp *app);

  CQViApp *app() const { return app_; }

 private:
  CQViApp *app_ { nullptr };
};

//------

class QLineEdit;

class CQViCmdLine : public QFrame, public CViCmdLine  {
  Q_OBJECT

 public:
  CQViCmdLine(CQViApp *app);

  CQViApp *app() const { return app_; }

  void setVisible(bool visible) override;

  void setLine(const std::string &line) override;

  void keyPress(char c) override;

 private:
  CQViApp*   app_  { nullptr };
  QLineEdit* edit_ { nullptr };
};

//---

class CQVi : public QObject, public CVi {
  Q_OBJECT

 public:
  CQVi(CQViApp *app);

  CViCmdLine *createCmdLine() const override;

 private:
  CQViApp *app_ { nullptr };
};

//---

class CQViApp : public QWidget {
  Q_OBJECT

 public:
  CQViApp(QWidget *parent=nullptr);

  void init();

  //---

  CQVi *vi() const { return vi_.get(); }

  void setCmdLine(CQViCmdLine *cmdLine) { cmdLine_ = cmdLine; }

  //---

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

  using ViP = std::unique_ptr<CQVi>;

  ViP          vi_;
  CQViCanvas*  canvas_        { nullptr };
  QScrollBar*  hscroll_       { nullptr };
  QScrollBar*  vscroll_       { nullptr };
  CQViStatus*  status_        { nullptr };
  CQViCmdLine* cmdLine_       { nullptr };
  QRect        char_rect_;
  int          char_width_    { 8 };
  int          char_ascent_   { 10 };
  int          char_height_   { 12 };
  int          x_offset_      { 0 };
  int          y_offset_      { 0 };
  YLineMap     yLineMap_;
  uint         maxLineLength_ { 0 };
  QPoint       press_pos_;
};
