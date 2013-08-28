#include <CQEditBg.h>
#include <QVBoxLayout>
#include <CQColorChooser.h>

void
CQEditBg::
createWidgets(QWidget *frame)
{
  QVBoxLayout *layout = new QVBoxLayout(frame);

  CQColorChooser *color = new CQColorChooser;

  connect(color, SIGNAL(colorChanged(const QString &)),
          this, SLOT(setColorName(const QString &)));

  layout->addWidget(color);
}

void
CQEditBg::
setColorName(const QString &colorName)
{
  colorName_ = colorName;
}
