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
  void createMenus() override;
  void createToolBars() override;
  void createStatusBar() override;

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
  CQMenu *fileMenu_ { nullptr };

  CQMenuItem *newItem_    { nullptr };
  CQMenuItem *loadItem_   { nullptr };
  CQMenuItem *saveItem_   { nullptr };
  CQMenuItem *saveAsItem_ { nullptr };
  CQMenuItem *closeItem_  { nullptr };
  CQMenuItem *quitItem_   { nullptr };

  // Edit Menu
  CQMenu *editMenu_ { nullptr };

  CQMenuItem *cutItem_   { nullptr };
  CQMenuItem *copyItem_  { nullptr };
  CQMenuItem *pasteItem_ { nullptr };
  CQMenuItem *undoItem_  { nullptr };
  CQMenuItem *redoItem_  { nullptr };

  // View Menu
  CQMenu *viewMenu_ { nullptr };

  CQMenuItem *bgItem_ { nullptr };

  // Help Menu
  CQMenu *helpMenu_ { nullptr };

  CQMenuItem *aboutItem_ { nullptr };

  // Toolbar
  CQToolBar *fileToolBar_  { nullptr };
  CQToolBar *editToolBar_  { nullptr };
  CQToolBar *styleToolBar_ { nullptr };
  CQToolBar *modeToolBar_  { nullptr };

  // Status Bar
  QLabel      *statusLabel_   { nullptr };
  QLabel      *messageLabel_  { nullptr };
  QToolButton *insButton_     { nullptr };
  QLabel      *sizeLabel_     { nullptr };
  QLabel      *positionLabel_ { nullptr };
};
