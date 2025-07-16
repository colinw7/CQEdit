#ifndef CQViTest_H
#define CQViTest_H

#include <CQMainWindow.h>

#include <vector>

class CQTabSplit;
class CQMenu;
class CQMenuItem;
class CQToolBar;
class CQFontChooser;
class CQColorChooser;

class QToolButton;
class QLabel;

namespace CQVi {
class Widget;
}

class CQViTest : public CQMainWindow {
  Q_OBJECT

 public:
  CQViTest();
 ~CQViTest();

  void addFile(const QString &filename);

  QSize sizeHint() const override;

 private:
  void createMenus() override;
  void createToolBars() override;
  void createStatusBar() override;

 private Q_SLOTS:
  void tabIndexChanged(int ind);

  void updateState();

  void newFileSlot();
  void addFileSlot();
  void replaceFileSlot();

  void saveFileSlot();
  void saveFileAsSlot();

  void closeFileSlot();

  void undo();
  void redo();

  void setOptions();

  void setFont(const QFont &f);
  void setSelectionColor(const QColor &c);

  CQVi::Widget *currentEdit() const;

 private:
  using Edits = std::vector<CQVi::Widget *>;

  CQTabSplit* fileTab_ { nullptr };
  Edits       edits_;

  CQFontChooser  *font_  { nullptr };
  CQColorChooser *color_ { nullptr };

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
  CQMenuItem *fgItem_ { nullptr };

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

#endif
