#include <CQViOptionsDlg.h>
#include <CQVi.h>

#include <CQColorChooser.h>
#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>

CQViOptionsDlg::
CQViOptionsDlg(QWidget *parent) :
 CQDialog(parent)
{
}

void
CQViOptionsDlg::
createWidgets(QWidget *frame)
{
  auto *layout = new QGridLayout(frame);

  bgColor_ = new CQColorChooser;
  fgColor_ = new CQColorChooser;

  listCheck_       = new QCheckBox;
  numberCheck_     = new QCheckBox;
  ignoreCaseCheck_ = new QCheckBox;

  syntaxCombo_ = new QComboBox;

  syntaxCombo_->addItem("None");
  syntaxCombo_->addItem("C");
  syntaxCombo_->addItem("C++");

  int row = 0;

  layout->addWidget(new QLabel("Bg"), row, 0);
  layout->addWidget(bgColor_        , row, 1); ++row;

  layout->addWidget(new QLabel("Fg"), row, 0);
  layout->addWidget(fgColor_        , row, 1); ++row;

  layout->addWidget(new QLabel("List"       ), row, 0);
  layout->addWidget(listCheck_               , row, 1); ++row;
  layout->addWidget(new QLabel("Number"     ), row, 0);
  layout->addWidget(numberCheck_             , row, 1); ++row;
  layout->addWidget(new QLabel("Ignore Case"), row, 0);
  layout->addWidget(ignoreCaseCheck_         , row, 1); ++row;

  layout->addWidget(new QLabel("Syntax"), row, 0);
  layout->addWidget(syntaxCombo_        , row, 1); ++row;

  layout->setRowStretch(row, 1);
}

void
CQViOptionsDlg::
updateOptions(CQVi::Widget *widget)
{
  auto *mgr = CQVi::Mgr::instance();

  bg_ = mgr->bg();
  fg_ = mgr->fg();

  bgColor_->setColor(bg_);
  fgColor_->setColor(fg_);

  listCheck_->setEnabled(widget);

  if (widget) {
    list_   = widget->app()->getListMode();
    number_ = widget->app()->getNumberMode();

    ignoreCase_ = ! widget->app()->getCaseSensitive();

    listCheck_  ->setChecked(list_);
    numberCheck_->setChecked(number_);

    ignoreCaseCheck_->setChecked(ignoreCase_);

    auto *syntax = widget->app()->syntax();

    auto lang = std::string(syntax ? syntax->language() : "");

    if      (lang == "C"  ) syntaxCombo_->setCurrentIndex(1);
    else if (lang == "C++") syntaxCombo_->setCurrentIndex(2);
    else                    syntaxCombo_->setCurrentIndex(0);
  }
}

void
CQViOptionsDlg::
accept()
{
  bg_ = bgColor_->color();
  fg_ = fgColor_->color();

  list_       = listCheck_      ->isChecked();
  number_     = numberCheck_    ->isChecked();
  ignoreCase_ = ignoreCaseCheck_->isChecked();

  syntaxName_ = syntaxCombo_->currentText();
}
