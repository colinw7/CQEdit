#include <CVLineEdit.h>

CVLineEdit::
CVLineEdit() :
 CLineEdit    (),
 bg_          (0,0,0),
 fg_          (1,1,1),
 cursor_color_(1,1,0),
 overwrite_   (false)
{
}

void
CVLineEdit::
setBackground(const CRGBA &bg)
{
  bg_ = bg;
}

void
CVLineEdit::
setForeground(const CRGBA &fg)
{
  fg_ = fg;
}

void
CVLineEdit::
setFont(CFontPtr font)
{
  font_ = font;
}

void
CVLineEdit::
keyPress(const CKeyEvent &event)
{
  CKeyType type = event.getType();

  if (CEvent::keyTypeIsAlpha(type) ||
      CEvent::keyTypeIsDigit(type)) {
    if (getOverwrite())
      replaceChar(event.getText()[0]);
    else
      insertChar(event.getText()[0]);

    goto done;
  }

  switch (type) {
    case CKEY_TYPE_Space:
    case CKEY_TYPE_Exclam:
    case CKEY_TYPE_QuoteDbl:
    case CKEY_TYPE_NumberSign:
    case CKEY_TYPE_Dollar:
    case CKEY_TYPE_Percent:
    case CKEY_TYPE_Ampersand:
    case CKEY_TYPE_Apostrophe:
    case CKEY_TYPE_ParenLeft:
    case CKEY_TYPE_ParenRight:
    case CKEY_TYPE_Asterisk:
    case CKEY_TYPE_Plus:
    case CKEY_TYPE_Comma:
    case CKEY_TYPE_Minus:
    case CKEY_TYPE_Period:
    case CKEY_TYPE_Slash:

    case CKEY_TYPE_Colon:
    case CKEY_TYPE_Semicolon:
    case CKEY_TYPE_Less:
    case CKEY_TYPE_Equal:
    case CKEY_TYPE_Greater:
    case CKEY_TYPE_Question:
    case CKEY_TYPE_At:

    case CKEY_TYPE_BracketLeft:
    case CKEY_TYPE_Backslash:
    case CKEY_TYPE_BracketRight:
    case CKEY_TYPE_AsciiCircum:
    case CKEY_TYPE_Underscore:
    case CKEY_TYPE_QuoteLeft:
    case CKEY_TYPE_BraceLeft:
    case CKEY_TYPE_Bar:
    case CKEY_TYPE_BraceRight:
    case CKEY_TYPE_AsciiTilde:
      if (getOverwrite())
        replaceChar(event.getText()[0]);
      else
        insertChar(event.getText()[0]);

      break;

    case CKEY_TYPE_Escape:
      break;

    case CKEY_TYPE_BackSpace:
      if (cursorLeft())
        deleteChar();

      break;
    case CKEY_TYPE_DEL:
      deleteChar();
      break;
    case CKEY_TYPE_Clear:
      clear();
      break;

    case CKEY_TYPE_KP_Enter:
    case CKEY_TYPE_Return:
      process();
      break;

    case CKEY_TYPE_Left:
      cursorLeft();
      break;
    case CKEY_TYPE_Up:
      prevLine();
      break;
    case CKEY_TYPE_Right:
      cursorRight();
      break;
    case CKEY_TYPE_Down:
      nextLine();
      break;

    case CKEY_TYPE_Page_Down:
      break;
    case CKEY_TYPE_Page_Up:
      break;

    case CKEY_TYPE_Home:
    case CKEY_TYPE_SOH: // Ctrl+A
      cursorStart();
      break;
    case CKEY_TYPE_End:
    case CKEY_TYPE_ENQ: // Ctrl+E
      cursorEnd();
      break;

    case CKEY_TYPE_TAB:
    case CKEY_TYPE_Tab:
    case CKEY_TYPE_KP_Tab: {
      for (int i = 0; i < 8; ++i)
        insertChar(' ');

      break;
    }

    case CKEY_TYPE_Shift_L  : case CKEY_TYPE_Shift_R:
    case CKEY_TYPE_Control_L: case CKEY_TYPE_Control_R:
    case CKEY_TYPE_Caps_Lock: case CKEY_TYPE_Shift_Lock:
    case CKEY_TYPE_Meta_L   : case CKEY_TYPE_Meta_R:
    case CKEY_TYPE_Alt_L    : case CKEY_TYPE_Alt_R:
    case CKEY_TYPE_Super_L  : case CKEY_TYPE_Super_R:
    case CKEY_TYPE_Hyper_L  : case CKEY_TYPE_Hyper_R:
      break;

    default:
      std::cerr << "Unsupported key " << CEvent::keyTypeName(type) << std::endl;
      break;
  }

 done:
  update();
}

void
CVLineEdit::
buttonPress(const CMouseEvent &event)
{
  int pos = xyToPos(event.getX(), event.getY());

  if (pos >= 0)
    pos_ = pos;
}

void
CVLineEdit::
draw(CVLineEditRenderer *renderer, uint w, uint h)
{
  renderer_ = renderer;

  CIBBox2D bbox(0, 0, w - 1, h - 1);

  drawInside(renderer_, bbox);
}

void
CVLineEdit::
drawInside(CVLineEditRenderer *renderer, const CIBBox2D &bbox)
{
  renderer_ = renderer;

  renderer_->fillRectangle(bbox, bg_);

  uint len = line_.size();

  bboxes_.clear();

  int x1 = bbox.getXMin(); int y1 = bbox.getYMin();
  int x2 = bbox.getXMax(); int y2 = bbox.getYMax();

  int xs = x1;

  for (uint i = 0; i < len; ++i) {
    int cw1 = font_->getStringWidth(line_.substr(i, 1));

    int xe = xs + cw1 - 1;

    if (xs < x2) {
      CIBBox2D bbox(xs, y1, xe, y2);

      if (i == pos_)
        drawFilledChar(bbox, line_[i], cursor_color_, bg_, false);
      else
        drawFilledChar(bbox, line_[i], bg_, fg_, true);
    }

    bboxes_.push_back(CIBBox2D(xs, y1, xe, y2));

    xs = xe + 1;
  }

  if (pos_ == len) {
    int cw1 = font_->getStringWidth(" ");

    int xe = xs + cw1 - 1;

    if (xs < x2) {
      CIBBox2D bbox(xs, y1, xe, y2);

      drawFilledChar(bbox, ' ', cursor_color_, bg_, false);
    }
  }
}

int
CVLineEdit::
xyToPos(int x, int /*y*/)
{
  uint len = line_.size();

  if (len != bboxes_.size()) return -1;

  int xs = 0, xe = 0;

  for (uint i = 0; i < len; ++i) {
    xs = bboxes_[i].getXMin();
    xe = bboxes_[i].getXMax();

    if (x >= xs && x < xe) return i;
  }

  if (x >= xs)
    return len;

  return -1;
}

void
CVLineEdit::
drawFilledChar(const CIBBox2D &bbox, char c, const CRGBA &bg, const CRGBA &fg, bool filled) const
{
  if (! filled)
    renderer_->fillRectangle(bbox, bg);

  if (c != '\0') {
    renderer_->setForeground(fg);

    renderer_->drawChar(CIPoint2D(bbox.getXMin(), bbox.getYMin()), c);
  }
}
