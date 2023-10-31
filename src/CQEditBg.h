#include <CQDialog.h>

class CQEditBg : public CQDialog {
  Q_OBJECT

 public:
  CQEditBg(QWidget *parent) :
   CQDialog(parent) {
  }

  void createWidgets(QWidget *frame) override;

  const QString &getColorName() const { return colorName_; }

 public slots:
  void setColorName(const QString &colorName);

 private:
  QString colorName_;
};
