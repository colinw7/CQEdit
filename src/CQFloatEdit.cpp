#include <CQFloatEdit.h>

#include <QApplication>
#include <QKeyEvent>

CQFloatEdit::
CQFloatEdit(QWidget *parent) :
 QLineEdit(parent), valid_(false)
{
  connect(this, SIGNAL(editingFinished()), this, SLOT(acceptSlot()));

  hide();
}

void
CQFloatEdit::
display(const QRect &rect, const QString &text)
{
  valid_ = true;

  setText(text);

  saveText_ = text;

  move(rect.topLeft());

  resize(rect.size());

  show();
  raise();

  setFocus();

  selectAll();

  qApp->installEventFilter(this);
}

void
CQFloatEdit::
hide()
{
  qApp->removeEventFilter(this);

  QLineEdit::hide();
}

bool
CQFloatEdit::
event(QEvent *e)
{
  if (e->type() == QEvent::KeyPress) {
    QKeyEvent *ke = dynamic_cast<QKeyEvent *>(e);

    if (ke->key() == Qt::Key_Escape) {
      setText(saveText_);

      valid_ = false;

      hide();
    }
  }

  return QLineEdit::event(e);
}

bool
CQFloatEdit::
eventFilter(QObject *obj, QEvent *event)
{
  QEvent::Type type = event->type();

  // ignore if not a widget
  QWidget *w = qobject_cast<QWidget *>(obj);
  if (! w) return QObject::eventFilter(obj, event);

  bool validEvent = false;

  if (type == QEvent::MouseButtonPress || type == QEvent::MouseButtonDblClick)
    validEvent = true;

  if (validEvent) {
    if (w != this)
      hide();
  }

  // standard event processing
  return QObject::eventFilter(obj, event);
}

void
CQFloatEdit::
acceptSlot()
{
  if (valid_)
    emit valueChanged(text());

  hide();
}
