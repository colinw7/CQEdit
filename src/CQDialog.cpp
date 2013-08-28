#include <CQDialog.h>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

CQDialog::
CQDialog(QWidget *parent) :
 QDialog(parent), frame_(NULL), accepted_(false)
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  layout->setMargin (2); layout->setSpacing(2);

  frame_ = new QWidget;

  layout->addWidget(frame_);

  QHBoxLayout *button_layout = new QHBoxLayout;

  QPushButton *ok     = new QPushButton("Ok"    );
  QPushButton *cancel = new QPushButton("Cancel");

  connect(ok    , SIGNAL(clicked()), this, SLOT(acceptSlot()));
  connect(cancel, SIGNAL(clicked()), this, SLOT(rejectSlot()));

  button_layout->addStretch();
  button_layout->addWidget(ok);
  button_layout->addWidget(cancel);

  layout->addLayout(button_layout);
}

void
CQDialog::
init()
{
  createWidgets(frame_);
}

void
CQDialog::
acceptSlot()
{
  accepted_ = true;

  accept();

  QDialog::accept();
}

void
CQDialog::
rejectSlot()
{
  accepted_ = false;

  reject();

  QDialog::reject();
}
