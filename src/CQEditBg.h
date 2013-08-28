#include <CQDialog.h>

class CQEditBg : public CQDialog {
  Q_OBJECT

 private:
  QString colorName_;

 public:
  CQEditBg(QWidget *parent) :
   CQDialog(parent) {
  }

  void createWidgets(QWidget *frame);

  const QString &getColorName() const { return colorName_; }

 public slots:
  void setColorName(const QString &colorName);
};
