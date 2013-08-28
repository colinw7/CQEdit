#include <CQMainWindow.h>
#include <CQMenu.h>
#include <CQToolBar.h>

class CQEdit;
class CQFontChooser;
class CQColorChooser;
class QTabWidget;
class QLabel;
class QToolButton;
class QComboBox;
class QLineEdit;

class CQEditTest : public CQMainWindow {
  Q_OBJECT

 private:
  QTabWidget     *fileTab_;
  CQEdit         *edit_;
  CQEdit         *output_;
  CQFontChooser  *font_;
  CQColorChooser *color_;
  QComboBox      *mode_;

  // File Menu
  CQMenu *fileMenu;

  CQMenuItem *newItem;
  CQMenuItem *loadItem;
  CQMenuItem *saveItem;
  CQMenuItem *saveAsItem;
  CQMenuItem *closeItem;
  CQMenuItem *quitItem;

  // Edit Menu
  CQMenu *editMenu;

  CQMenuItem *cutItem;
  CQMenuItem *copyItem;
  CQMenuItem *pasteItem;
  CQMenuItem *undoItem;
  CQMenuItem *redoItem;

  // View Menu
  CQMenu *viewMenu;

  CQMenuItem *bgItem;

  // Help Menu
  CQMenu *helpMenu;

  CQMenuItem *aboutItem;

  CQToolBar *fileToolBar;
  CQToolBar *editToolBar;
  CQToolBar *styleToolBar;
  CQToolBar *modeToolBar;

  QLabel      *statusLabel;
  QLabel      *messageLabel;
  QToolButton *insButton;
  QLabel      *sizeLabel;
  QLabel      *positionLabel;

 public:
  CQEditTest();
 ~CQEditTest();

  void addFile(const std::string &filename);

  CQEdit *getEdit() const { return edit_; }

 private:
  void createMenus();
  void createToolBars();
  void createStatusBar();

  void setCurrent(CQEdit *edit, CQEdit *output);

 private slots:
  void currentFileChanged(int);

  void newFileSlot();
  void addFileSlot();
  void replaceFileSlot();
  void saveFileSlot();
  void saveFileAsSlot();
  void closeFileSlot();

  void undo();
  void redo();

  void setFont(const QFont&);
  void setSelectionColor(const QColor &qcolor);

  void modeChanged(const QString &modeName);

  void setBg();

  void updateStatus();
  void outputMessage(const QString &msg);
  void errorMessage(const QString &msg);
  void updateTitle();
};
