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

 private:
  QTabWidget     *fileTab_ { nullptr };
  CQEdit         *edit_    { nullptr };
  CQEdit         *output_  { nullptr };
  CQFontChooser  *font_    { nullptr };
  CQColorChooser *color_   { nullptr };
  QComboBox      *mode_    { nullptr };

  // File Menu
  CQMenu *fileMenu { nullptr };

  CQMenuItem *newItem    { nullptr };
  CQMenuItem *loadItem   { nullptr };
  CQMenuItem *saveItem   { nullptr };
  CQMenuItem *saveAsItem { nullptr };
  CQMenuItem *closeItem  { nullptr };
  CQMenuItem *quitItem   { nullptr };

  // Edit Menu
  CQMenu *editMenu { nullptr };

  CQMenuItem *cutItem   { nullptr };
  CQMenuItem *copyItem  { nullptr };
  CQMenuItem *pasteItem { nullptr };
  CQMenuItem *undoItem  { nullptr };
  CQMenuItem *redoItem  { nullptr };

  // View Menu
  CQMenu *viewMenu { nullptr };

  CQMenuItem *bgItem { nullptr };

  // Help Menu
  CQMenu *helpMenu { nullptr };

  CQMenuItem *aboutItem { nullptr };

  CQToolBar *fileToolBar  { nullptr };
  CQToolBar *editToolBar  { nullptr };
  CQToolBar *styleToolBar { nullptr };
  CQToolBar *modeToolBar  { nullptr };

  QLabel      *statusLabel   { nullptr };
  QLabel      *messageLabel  { nullptr };
  QToolButton *insButton     { nullptr };
  QLabel      *sizeLabel     { nullptr };
  QLabel      *positionLabel { nullptr };
};
