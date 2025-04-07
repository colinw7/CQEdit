#include <CVi.h>

#include <QLabel>
#include <memory>

class QScrollBar;
class QLineEdit;

namespace CQVi {

class Widget;

class Mgr : public QObject {
  Q_OBJECT

  Q_PROPERTY(QColor bg READ bg WRITE setBg)
  Q_PROPERTY(QColor fg READ fg WRITE setFg)

  Q_PROPERTY(QFont font READ font WRITE setFont)

 public:
  static Mgr *instance();

 ~Mgr();

  const QColor &bg() const { return bg_; }
  void setBg(const QColor &c) { bg_ = c; update(); }

  const QColor &fg() const { return fg_; }
  void setFg(const QColor &c) { fg_ = c; update(); }

  const QFont &font() const { return font_; }
  void setFont(const QFont &font) { font_ = font; update(); }

  void addWidget   (Widget *w);
  void removeWidget(Widget *w);

  void update();

 private:
  Mgr();

 private:
  using Widgets = std::list<Widget *>;

  QColor bg_ {   0,   0,   0 };
  QColor fg_ { 255, 255, 255 };
  QFont  font_;

  Widgets widgets_;
};

class Canvas : public QWidget {
  Q_OBJECT

 public:
  Canvas(Widget *widget);

  void paintEvent(QPaintEvent *) override;

  QSize sizeHint() const override;

 private:
  Widget *widget_ { nullptr };
};

//------

#if 0
class Status : public QLabel {
  Q_OBJECT

 public:
  Status(Widget *widget_);

  Widget *app() const { return widget_; }

 private:
  Widget *widget_ { nullptr };
};
#endif

//------

class CmdLine : public QFrame, public CVi::CmdLine  {
  Q_OBJECT

 public:
  CmdLine(Widget *widget_);

  Widget *widget() const { return widget_; }

  void setVisible(bool visible) override;

  void updateLine() override;

 private:
  Widget*    widget_ { nullptr };
  QLineEdit* edit_   { nullptr };
};

//---

class App;

class Interface : public CVi::Interface {
 public:
  Interface(App *app);

  int getPageTop   () const override;
  int getPageBottom() const override;
  int getPageLength() const override;

  void scrollTop   () override;
  void scrollMiddle() override;
  void scrollBottom() override;

  void stateChanged    () override;
  void positionChanged () override;
  void selectionChanged() override;

 private:
  App* app_ { nullptr };
};

//---

class App : public QObject, public CVi::App {
  Q_OBJECT

 public:
  App(Widget *widget);

  CVi::CmdLine *createCmdLine() const override;

  int getPageTop   () const;
  int getPageBottom() const;
  int getPageLength() const;

  void scrollTop   ();
  void scrollMiddle();
  void scrollBottom();

  void scrollCursor();

  void stateChanged();
  void positionChanged();
  void selectionChanged();

  void update();

  QColor tokenColor(CSyntaxToken) const;

  void setNameValue(const std::string &name, const std::string &value) override;

 private:
  Widget *widget_ { nullptr };
};

//---

class Widget : public QWidget {
  Q_OBJECT

  Q_PROPERTY(QColor cursorBg READ cursorBg WRITE setCursorBg)
  Q_PROPERTY(QColor cursorFg READ cursorFg WRITE setCursorFg)
  Q_PROPERTY(QColor selBg    READ selBg    WRITE setSelBg   )
  Q_PROPERTY(QColor selFg    READ selFg    WRITE setSelFg   )
  Q_PROPERTY(QColor emptyFg  READ emptyFg  WRITE setEmptyFg )
  Q_PROPERTY(QColor numberFg READ numberFg WRITE setNumberFg)

  Q_PROPERTY(QColor preProFg  READ preProFg  WRITE setPreProFg )
  Q_PROPERTY(QColor keywordFg READ keywordFg WRITE setKeywordFg)
  Q_PROPERTY(QColor stringFg  READ stringFg  WRITE setStringFg )
  Q_PROPERTY(QColor commentFg READ commentFg WRITE setCommentFg)

 public:
  Widget(QWidget *parent=nullptr);
 ~Widget();

  void init();

  //---

  App *app() const { return app_.get(); }

  void setCmdLine(CmdLine *cmdLine) { cmdLine_ = cmdLine; }

  //---

  const QColor &cursorBg() const { return cursorBg_; }
  void setCursorBg(const QColor &c) { cursorBg_ = c; update(); }

  const QColor &cursorFg() const { return cursorFg_; }
  void setCursorFg(const QColor &c) { cursorFg_ = c; update(); }

  const QColor &selBg() const { return selBg_; }
  void setSelBg(const QColor &c) { selBg_ = c; update(); }

  const QColor &selFg() const { return selFg_; }
  void setSelFg(const QColor &c) { selFg_ = c; update(); }

  const QColor &emptyFg() const { return emptyFg_; }
  void setEmptyFg(const QColor &c) { emptyFg_ = c; update(); }

  const QColor &numberFg() const { return numberFg_; }
  void setNumberFg(const QColor &c) { numberFg_ = c; update(); }

  const QColor &preProFg() const { return preProFg_; }
  void setPreProFg(const QColor &c) { preProFg_ = c; }

  const QColor &keywordFg() const { return keywordFg_; }
  void setKeywordFg(const QColor &c) { keywordFg_ = c; }

  const QColor &stringFg() const { return stringFg_; }
  void setStringFg(const QColor &c) { stringFg_ = c; }

  const QColor &commentFg() const { return commentFg_; }
  void setCommentFg(const QColor &c) { commentFg_ = c; }

  //---

  void updateMgr();

  //---

  const std::string &getFilename() const;
  void setFilename(const std::string &filename);

  void loadFile(const std::string &filename);
  void saveFile(const std::string &filename);

  //---

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

//void updateStatus();

  bool mouseToPos(const QPoint &pos, int &ix, int &iy) const;

  int rows() const { return rows_; }
  int cols() const { return cols_; }

 Q_SIGNALS:
  void stateChanged();
  void positionChanged();
  void selectionChanged();
  void sizeChanged();

 private Q_SLOTS:
  void hscrollSlot(int);
  void vscrollSlot(int);

 private:
  using YLineMap = std::map<int, int>;

  using AppP = std::unique_ptr<App>;

  AppP app_;

  QColor cursorBg_ { 255, 255,   0 };
  QColor cursorFg_ {   0,   0,   0 };

  QColor selBg_ { 100, 100, 100 };
  QColor selFg_ { 255, 255, 255 };

  QColor emptyFg_  {   0,   0, 255 };
  QColor numberFg_ { 255, 255,   0 };

  QColor preProFg_  { 255, 127, 255 };
  QColor keywordFg_ { 255, 255, 127 };
  QColor stringFg_  { 255, 127,   0 };
  QColor commentFg_ { 127, 127, 255 };

  Canvas*     canvas_  { nullptr };
  QScrollBar* hscroll_ { nullptr };
  QScrollBar* vscroll_ { nullptr };
//Status*     status_  { nullptr };
  CmdLine*    cmdLine_ { nullptr };

  bool sizeChanged_ { true };

  struct FontData {
    QRect char_rect;
    int   char_width  { 8 };
    int   char_ascent { 10 };
    int   char_height { 12 };
  };

  FontData fontData_;

  int      xOffset_       { 0 };
  int      yOffset_       { 0 };
  YLineMap yLineMap_;
  uint     maxLineLength_ { 0 };
  int      y1_            { 0 };
  int      y2_            { 0 };
  int      rows_          { 1 };
  int      cols_          { 1 };

  QPoint press_pos_;

  int         lineDigits_ { -1 };
  int         lmargin_    { -1 };
  std::string numberFormat_;
};

}
