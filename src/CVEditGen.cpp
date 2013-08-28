#include <CVEditGen.h>
#include <CVEditFile.h>
#include <CVEditVi.h>
#include <CEvent.h>

CVEditGen::
CVEditGen(CVEditFile *file) :
 file_(file)
{
}

void
CVEditGen::
processChar(const CKeyEvent &event)
{
  CKeyType type = event.getType();

  if (CEvent::keyTypeIsAlpha(type) || CEvent::keyTypeIsDigit(type)) {
    if (file_->getOverwriteMode())
      file_->replaceChar(event.getText()[0]);
    else
      file_->insertChar(event.getText()[0]);

    return;
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
      if (file_->getOverwriteMode())
        file_->replaceChar(event.getText()[0]);
      else
        file_->insertChar(event.getText()[0]);

      break;

    case CKEY_TYPE_Escape:
      break;

    case CKEY_TYPE_BackSpace:
      if      (file_->cursorLeft(1))
        file_->deleteChar();
      else if (file_->cursorUp(1)) {
        file_->cursorToRight();

        file_->joinLine();
      }

      break;
    case CKEY_TYPE_DEL:
      file_->deleteChar();
      break;

    case CKEY_TYPE_KP_Enter:
    case CKEY_TYPE_Return:
      file_->splitLine();
      break;

    case CKEY_TYPE_Left:
      file_->cursorLeft(1);
      break;
    case CKEY_TYPE_Up:
      file_->cursorUp(1);
      break;
    case CKEY_TYPE_Right:
      file_->cursorRight(1);
      break;
    case CKEY_TYPE_Down:
      file_->cursorDown(1);
      break;

    case CKEY_TYPE_Page_Down: {
      int num = file_->getPageLength();

      file_->cursorDown(num);

      file_->scrollTop();

      break;
    }
    case CKEY_TYPE_Page_Up: {
      int num = file_->getPageLength();

      file_->cursorUp(num);

      file_->scrollBottom();

      break;
    }

   case CKEY_TYPE_Home: {
      file_->cursorToLeft();

      break;
    }
    case CKEY_TYPE_End: {
      file_->cursorToRight();

      break;
    }

    case CKEY_TYPE_Insert: {
      file_->setOverwriteMode(! file_->getOverwriteMode());

      break;
    }

    case CKEY_TYPE_TAB:
    case CKEY_TYPE_Tab:
    case CKEY_TYPE_KP_Tab: {
      uint tab_stop = file_->getTabStop();

      for (uint i = 0; i < tab_stop; ++i)
        file_->insertChar(' ');

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

    case CKEY_TYPE_SOH: // Select All (Ctrl+A)
      // TODO

      break;
    case CKEY_TYPE_EOT: // Delete Line (Ctrl+D)
      // TODO

      break;
    case CKEY_TYPE_Pause: // Save (Ctrl+S)
      file_->saveLines(file_->getFileName());

      break;
    case CKEY_TYPE_DC1: // Quit (Ctrl+Q)
      file_->quit();

      break;
    case CKEY_TYPE_EM: // Undo (Ctrl+Y)
      file_->redo();

      break;
    case CKEY_TYPE_SUB: // Undo (Ctrl+Z)
      file_->undo();

      break;
    default:
      file_->getVi()->error("Unsupported key " + CEvent::keyTypeName(type));
      break;
  }
}
