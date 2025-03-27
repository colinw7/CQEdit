#include <CVi.h>

#include <QLabel>
#include <memory>

class QScrollBar;
class QLineEdit;

namespace CQVi {

class Widget;

class Canvas : public QWidget {
  Q_OBJECT

 public:
  Canvas(Widget *app);

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  Widget *app_ { nullptr };
};

//------

class Status : public QLabel {
  Q_OBJECT

 public:
  Status(Widget *app);

  Widget *app() const { return app_; }

 private:
  Widget *app_ { nullptr };
};

//------

class CmdLine : public QFrame, public CVi::CmdLine  {
  Q_OBJECT

 public:
  CmdLine(Widget *app);

  Widget *app() const { return app_; }

  void setVisible(bool visible) override;

  void updateLine() override;

 private:
  Widget*    app_  { nullptr };
  QLineEdit* edit_ { nullptr };
};

//---

class App;

class Interface : public CVi::Interface {
 public:
  Interface(App *vi);

  int getPageTop   () const override;
  int getPageBottom() const override;
  int getPageLength() const override;

  void scrollTop   () override;
  void scrollMiddle() override;
  void scrollBottom() override;

  void stateChanged() override;
  void positionChanged() override;

 private:
  App* vi_ { nullptr };
};

//---

class App : public QObject, public CVi::App {
  Q_OBJECT

 public:
  App(Widget *app);

  CVi::CmdLine *createCmdLine() const override;

  int getPageTop   () const;
  int getPageBottom() const;
  int getPageLength() const;

  void scrollTop   ();
  void scrollMiddle();
  void scrollBottom();

  void scrollCursor();

  void update();

 private:
  Widget *app_ { nullptr };
};

//---

class Widget : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QColor bg       READ bg       WRITE setBg      )
  Q_PROPERTY(QColor fg       READ fg       WRITE setFg      )
  Q_PROPERTY(QColor cursorBg READ cursorBg WRITE setCursorBg)
  Q_PROPERTY(QColor cursorFg READ cursorFg WRITE setCursorFg)
  Q_PROPERTY(QColor emptyFg  READ emptyFg  WRITE setEmptyFg )
  Q_PROPERTY(QFont  font     READ font     WRITE setFont    )

 public:
  Widget(QWidget *parent=nullptr);

  void init();

  //---

  App *vi() const { return vi_.get(); }

  void setCmdLine(CmdLine *cmdLine) { cmdLine_ = cmdLine; }

  //---

  const QColor &bg() const { return bg_; }
  void setBg(const QColor &c) { bg_ = c; update(); }

  const QColor &fg() const { return fg_; }
  void setFg(const QColor &c) { fg_ = c; update(); }

  const QColor &cursorBg() const { return cursorBg_; }
  void setCursorBg(const QColor &c) { cursorBg_ = c; update(); }

  const QColor &cursorFg() const { return cursorFg_; }
  void setCursorFg(const QColor &c) { cursorFg_ = c; update(); }

  const QColor &emptyFg() const { return emptyFg_; }
  void setEmptyFg(const QColor &c) { emptyFg_ = c; update(); }

  //---

  const QFont &font() const { return fontData_.font; }
  void setFont(const QFont &font);

  //---

  void loadFile(const std::string &filename);

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

  int pageTop   () const;
  int pageBottom() const;

  int pageLength() const;

  void scrollTo(uint x, uint y, bool force=false);

  void updateScrollbars();

  void updateStatus();

  bool mouseToPos(const QPoint &pos, int &ix, int &iy) const;

 private slots:
  void hscrollSlot(int);
  void vscrollSlot(int);

 private:
  using YLineMap = std::map<int, int>;

  using ViP = std::unique_ptr<App>;

  ViP vi_;

  QColor bg_ {   0,   0,   0 };
  QColor fg_ { 255, 255, 255 };

  QColor cursorBg_ { 255, 255,   0 };
  QColor cursorFg_ {   0,   0,   0 };

  QColor emptyFg_ { 0, 0, 255 };

  Canvas*     canvas_  { nullptr };
  QScrollBar* hscroll_ { nullptr };
  QScrollBar* vscroll_ { nullptr };
  Status*     status_  { nullptr };
  CmdLine*    cmdLine_ { nullptr };

  struct FontData {
    QFont font;
    QRect char_rect;
    int   char_width  { 8 };
    int   char_ascent { 10 };
    int   char_height { 12 };
  };

  FontData fontData_;

  int      x_offset_      { 0 };
  int      y_offset_      { 0 };
  YLineMap yLineMap_;
  uint     maxLineLength_ { 0 };
  int      y1_            { 0 };
  int      y2_            { 0 };

  QPoint press_pos_;
};

}
