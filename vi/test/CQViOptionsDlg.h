#include <CQDialog.h>

namespace CQVi {
class Widget;
};

class CQColorChooser;
class QCheckBox;
class QComboBox;

class CQViOptionsDlg : public CQDialog {
  Q_OBJECT

 public:
  CQViOptionsDlg(QWidget *parent);

  void createWidgets(QWidget *frame) override;

  void updateOptions(CQVi::Widget *widget);

  const QColor &getBg() const { return bg_; }
  const QColor &getFg() const { return fg_; }

  bool getList      () const { return list_      ; }
  bool getNumber    () const { return number_    ; }
  bool getIgnoreCase() const { return ignoreCase_; }

  const QString &syntaxName() const { return syntaxName_; }

 private:
  void accept() override;

 private:
  CQColorChooser* bgColor_ { nullptr };
  CQColorChooser* fgColor_ { nullptr };

  QCheckBox* listCheck_       { nullptr };
  QCheckBox* numberCheck_     { nullptr };
  QCheckBox* ignoreCaseCheck_ { nullptr };

  QComboBox *syntaxCombo_ { nullptr };

  QColor bg_;
  QColor fg_;

  bool list_       { false };
  bool number_     { false };
  bool ignoreCase_ { false };

  QString syntaxName_;
};
