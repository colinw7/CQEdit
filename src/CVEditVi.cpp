#include <CVEditVi.h>
#include <CVEditMgr.h>
#include <CEvent.h>
#include <CStrUtil.h>
#include <CVLineEdit.h>
#include <cstring>

class LineEditRenderer : public CVLineEditRenderer {
  void setForeground(const CRGBA &rgba) {
    return CVEditMgrInst->setForeground(rgba);
  }

  void fillRectangle(const CIBBox2D &rect, const CRGBA &rgba) {
    return CVEditMgrInst->fillRectangle(rect, rgba);
  }

  void drawChar(const CIPoint2D &p, char c) {
    return CVEditMgrInst->drawChar(p, c);
  }
};

//------

CVEditVi::
CVEditVi(CVEditFile *file) :
 file_       (file),
 lastKey_    (CKEY_TYPE_NUL),
 count_      (0),
 insertMode_ (false),
 cmdLineMode_(false),
 cmdLine_    (NULL),
 register_   ('\0'),
 lastCommand_(file),
 findChar_   ('\0'),
 findForward_(true),
 findTill_   (false)
{
}

CVEditVi::
~CVEditVi()
{
  delete cmdLine_;
}

std::string
CVEditVi::
getCmdLineString() const
{
  return cmdLine_->getLine();
}

void
CVEditVi::
processChar(const CKeyEvent &event)
{
  if      (getInsertMode())
    processInsertChar(event);
  else if (getCmdLineMode())
    processCmdLineChar(event);
  else
    processCommandChar(event);
}

void
CVEditVi::
processCommandChar(const CKeyEvent &event)
{
  CKeyType key = event.getType();

  switch (key) {
    case CKEY_TYPE_Shift_L  : case CKEY_TYPE_Shift_R:
    case CKEY_TYPE_Control_L: case CKEY_TYPE_Control_R:
    case CKEY_TYPE_Caps_Lock: case CKEY_TYPE_Shift_Lock:
    case CKEY_TYPE_Meta_L   : case CKEY_TYPE_Meta_R:
    case CKEY_TYPE_Alt_L    : case CKEY_TYPE_Alt_R:
    case CKEY_TYPE_Super_L  : case CKEY_TYPE_Super_R:
    case CKEY_TYPE_Hyper_L  : case CKEY_TYPE_Hyper_R:
      return;
    default:
      break;
  }

  //------

  if (lastKey_ != CKEY_TYPE_NUL) {
    char c = CEvent::keyTypeChar(key);

    switch (lastKey_) {
      case CKEY_TYPE_QuoteLeft: { // goto mark line and char
        file_->markReturn();

        uint line_num, char_num;

        if (file_->getMarkPos(std::string(&c, 1), &line_num, &char_num))
          file_->cursorTo(line_num, char_num);

        goto done;
      }
      case CKEY_TYPE_QuoteDbl: { // set register
        if      (key == CKEY_TYPE_TAB ||
                 key == CKEY_TYPE_Tab ||
                 key == CKEY_TYPE_KP_Tab) {
          file_->displayRegisters();
        }
        else if (isalnum(c) || strchr("#/", c))
          register_ = c;
        else
          error("Invalid register name '" + std::string(&c, 1) + "'");

        goto done;
      }
      case CKEY_TYPE_Apostrophe: { // goto mark line
        if (key == CKEY_TYPE_TAB ||
            key == CKEY_TYPE_Tab ||
            key == CKEY_TYPE_KP_Tab)
          file_->displayMarks();
        else {
          file_->markReturn();

          uint line_num, char_num;

          if (file_->getMarkPos(std::string(&c, 1), &line_num, &char_num))
            file_->cursorTo(line_num, 0);
        }

        goto done;
      }
      case CKEY_TYPE_c: { // change
        if      (key == CKEY_TYPE_c) {
          setInsertMode(true);

          file_->cursorToLeft();
          file_->deleteEOL();
        }
        else if (key == CKEY_TYPE_w) {
          setInsertMode(true);

          file_->deleteWord();
        }
        else if (key == CKEY_TYPE_l) {
          setInsertMode(true);

          for (uint i = 0; i < std::max(count_, 1U); ++i)
            file_->deleteChar();
        }
        else {
          CIPoint2D start = file_->getPos();
          CIPoint2D end   = start;

          bool rc = processMoveChar(event, end);

          if (rc) {
            setInsertMode(true);

            file_->deleteTo(start.y, start.x, end.y, end.x);
          }
        }

        lastCommand_.clear();
        lastCommand_.addCount(count_);
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case CKEY_TYPE_d: { // delete
        if      (key == CKEY_TYPE_d)
          file_->deleteLine();
        else if (key == CKEY_TYPE_w)
          file_->deleteWord();
        else if (key == CKEY_TYPE_l)
          file_->deleteChar();
        else {
          CIPoint2D start = file_->getPos();
          CIPoint2D end   = start;

          bool rc = processMoveChar(event, end);

          if (rc)
            file_->deleteTo(start.y, start.x, end.y, end.x);
        }

        lastCommand_.clear();
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case CKEY_TYPE_f: { // find next char
        doFindChar(c, std::max(count_, 1U), true, false);

        break;
      }
      case CKEY_TYPE_F: { // find prev char
        doFindChar(c, std::max(count_, 1U), false, false);

        break;
      }
      case CKEY_TYPE_g: { // test code !!!
        file_->setSelectRange(file_->getPos(), file_->getPos());

        CIPoint2D select_end = file_->getSelectEnd();

        bool rc = processMoveChar(event, select_end);

        if (rc)
          file_->rangeSelect(file_->getSelectStart(), select_end, true);
        else
          file_->clearSelection();
      }
      case CKEY_TYPE_m: { // mark
        file_->setMarkPos(std::string(&c, 1));

        goto done;
      }
      case CKEY_TYPE_r: { // replace char
        file_->replaceChar(c);

        file_->cursorLeft(1);

        lastCommand_.clear();
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case CKEY_TYPE_t: { // till next char
        doFindChar(c, std::max(count_, 1U), true, true);

        break;
      }
      case CKEY_TYPE_T: { // till prev char
        doFindChar(c, std::max(count_, 1U), false, true);

        break;
      }
      case CKEY_TYPE_y: { // yank
        if      (key == CKEY_TYPE_y)
          file_->yankLines(register_, std::max(count_, 1U));
        else if (key == CKEY_TYPE_w)
          file_->yankWords(register_, std::max(count_, 1U));
        else {
          CIPoint2D start = file_->getPos();
          CIPoint2D end   = start;

          bool rc = processMoveChar(event, end);

          if (rc)
            file_->yankTo(register_, start.y, start.x, end.y, end.x, false);
        }

        break;
      }
      case CKEY_TYPE_z: { // scroll to
        if      (key == CKEY_TYPE_Return ||
                 key == CKEY_TYPE_Plus)
          file_->scrollTop();
        else if (key == CKEY_TYPE_Period ||
                 key == CKEY_TYPE_z)
          file_->scrollMiddle();
        else if (key == CKEY_TYPE_b ||
                 key == CKEY_TYPE_Minus)
          file_->scrollBottom();
        else if (key == CKEY_TYPE_t)
          file_->scrollTop();

        break;
      }
      case CKEY_TYPE_Z: {
        if      (key == CKEY_TYPE_Q)
          file_->quit();
        else if (key == CKEY_TYPE_Z) {
          file_->saveLines(file_->getFileName());

          file_->quit();
        }

        break;
      }
      case CKEY_TYPE_Exclam: {
        std::string str;

        if      (key == CKEY_TYPE_Exclam)
          str = ":.!";
        else {
          CIPoint2D pos = file_->getPos();

          file_->setSelectRange(pos, pos);

          bool rc = processMoveChar(event, pos);

          if (rc) {
            int d = abs(file_->getSelectStart().y - pos.y);

            if (d != 0) {
              file_->cursorTo(file_->getSelectEnd().y, pos.x);

              str = ":.,+" + CStrUtil::toString(d) + "!";
            }
            else
              str = ":.!";
          }
        }

        setCmdLineMode(true, str);

        break;
      }
      case CKEY_TYPE_Less: {
        CIPoint2D start = file_->getPos();
        CIPoint2D end   = start;

        if      (key == CKEY_TYPE_Less)
          file_->shiftLeft(start.y, end.y);
        else {
          bool rc = processMoveChar(event, end);

          if (rc)
            file_->shiftRight(start.y, end.y);
        }

        break;
      }
      case CKEY_TYPE_Greater: {
        CIPoint2D start = file_->getPos();
        CIPoint2D end   = start;

        if      (key == CKEY_TYPE_Greater)
          file_->shiftRight(start.y, end.y);
        else {
          bool rc = processMoveChar(event, end);

          if (rc)
            file_->shiftRight(start.y, end.y);
        }

        break;
      }
      case CKEY_TYPE_BracketLeft: {
        if (key == CKEY_TYPE_BracketLeft)
          file_->prevSection();

        break;
      }
      case CKEY_TYPE_BracketRight: {
        if (key == CKEY_TYPE_BracketRight)
          file_->nextSection();

        break;
      }
      default:
        break;
    }

    goto done;
  }

  //------

  if      (key == CKEY_TYPE_0) {
    if (count_ == 0) {
      file_->cursorToLeft();

      goto done;
    }
    else {
      count_ = count_*10 + (key - CKEY_TYPE_0);

      //count_str_ += CEvent::keyTypeChar(key);

      return;
    }
  }
  else if (key >= CKEY_TYPE_1 && key <= CKEY_TYPE_9) {
    count_ = count_*10 + (key - CKEY_TYPE_0);

    //count_str_ += CEvent::keyTypeChar(key);

    return;
  }

  //------

  if (event.isControlKey() || event.isMetaKey())
    processControlChar(event);
  else
    processNormalChar(event);

  return;

 done:
  count_    = 0;
  lastKey_ = CKEY_TYPE_NUL;
}

void
CVEditVi::
processNormalChar(const CKeyEvent &event)
{
  CKeyType key = event.getType();

  switch (key) {
    // cursor movement
    case CKEY_TYPE_h:
    case CKEY_TYPE_BackSpace: {
      file_->cursorLeft(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_Left: {
      if (event.isShiftKey()) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          file_->prevWord();
      }
      else
        file_->cursorLeft(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_j: {
      file_->cursorDown(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_Down: {
      if (event.isShiftKey()) {
        int num = file_->getPageLength();

        file_->cursorDown(num);

        file_->scrollTop();
      }
      else
       file_->cursorDown(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_k: {
      file_->cursorUp(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_Up: {
      if (event.isShiftKey()) {
        int num = file_->getPageLength();

        file_->cursorUp(num);

        file_->scrollBottom();
      }
      else
        file_->cursorUp(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_l:
    case CKEY_TYPE_Space:
    case CKEY_TYPE_FF: {
      file_->cursorRight(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_Right: {
      if (event.isShiftKey()) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          file_->nextWord();
      }
      else
        file_->cursorRight(std::max(count_, 1U));

      break;
    }
    case CKEY_TYPE_Insert: {
      file_->setOverwriteMode(! file_->getOverwriteMode());

      break;
    }
    case CKEY_TYPE_Home: {
      file_->cursorTo(0, 0);

      break;
    }
    case CKEY_TYPE_End: {
      file_->cursorTo(file_->getNumLines() - 1, 0);

      break;
    }
    case CKEY_TYPE_b: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->prevWord();

      break;
    }
    case CKEY_TYPE_B: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->prevWORD();

      break;
    }
    case CKEY_TYPE_w: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->nextWord();

      break;
    }
    case CKEY_TYPE_W: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->nextWORD();

      break;
    }
    case CKEY_TYPE_e: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->endWord();

      break;
    }
    case CKEY_TYPE_E: {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->endWORD();

      break;
    }

    //----------

    // scroll
    case CKEY_TYPE_Page_Down:
    case CKEY_TYPE_ACK: {
      int num = file_->getPageLength();

      file_->cursorDown(num);

      file_->scrollTop();

      break;
    }
    case CKEY_TYPE_Page_Up:
    case CKEY_TYPE_STX: {
      int num = file_->getPageLength();

      file_->cursorUp(num);

      file_->scrollBottom();

      break;
    }
    case CKEY_TYPE_Sys_Req: {
      int num = file_->getPageLength() / 2;

      file_->cursorUp(num);

      file_->scrollBottom();

      break;
    }
    case CKEY_TYPE_EOT: {
      int num = file_->getPageLength() / 2;

      file_->cursorDown(num);

      file_->scrollTop();

      break;
    }
    case CKEY_TYPE_ENQ: {
      int row1 = file_->getPageTop();
      int row2 = file_->getRow();

      int num = row2 - row1 - 1;

      if (num > 0) {
        file_->cursorUp(num);

        file_->scrollTop();

        file_->cursorDown(num);
      }
      else {
        file_->cursorDown(1);

        file_->scrollTop();
      }

      break;
    }
    case CKEY_TYPE_EM: {
      int row1 = file_->getPageBottom();
      int row2 = file_->getRow();

      int num = row1 - row2 - 1;

      if (num > 0) {
        file_->cursorDown(num);

        file_->scrollBottom();

        file_->cursorUp(num);
      }
      else {
        file_->cursorUp(1);

        file_->scrollBottom();
      }

      break;
    }

    //----------

    case CKEY_TYPE_BEL: {
      std::cout << file_->getFileName() << " " << file_->getNumLines() << " lines" << std::endl;

      break;
    }
    case CKEY_TYPE_DC2: {
      file_->redo();

      break;
    }

    //----------

    // Multi-char commands
    case CKEY_TYPE_c:
    case CKEY_TYPE_d:
    case CKEY_TYPE_f:
    case CKEY_TYPE_g:
    case CKEY_TYPE_m:
    case CKEY_TYPE_r:
    case CKEY_TYPE_t:
    case CKEY_TYPE_y:
    case CKEY_TYPE_z:
    case CKEY_TYPE_F:
    case CKEY_TYPE_T:
    case CKEY_TYPE_Z:
    case CKEY_TYPE_QuoteLeft:
    case CKEY_TYPE_QuoteDbl:
    case CKEY_TYPE_Apostrophe:
    case CKEY_TYPE_Less:
    case CKEY_TYPE_Greater:
    case CKEY_TYPE_BracketLeft:
    case CKEY_TYPE_BracketRight:
    case CKEY_TYPE_Exclam: {
      lastKey_ = key;
      return;
    }

    //----------

    case CKEY_TYPE_Minus: // first non-blank up
      file_->cursorFirstNonBlankUp();
      break;

    case CKEY_TYPE_Plus: // first non-blank down
    case CKEY_TYPE_Return:
      file_->cursorFirstNonBlankDown();
      break;

    case CKEY_TYPE_Underscore: // first non-blank
    case CKEY_TYPE_AsciiCircum:
      file_->cursorFirstNonBlank();
      break;

    case CKEY_TYPE_AsciiTilde:
      file_->swapChar();

      file_->cursorRight(1);

      break;

    case CKEY_TYPE_NumberSign: { // find string under cursor backward
      std::string word;

      if (file_->getWord(word))
        file_->findPrev(word);

      break;
    }

    case CKEY_TYPE_Asterisk: { // find string under cursor forward
      std::string word;

      if (file_->getWord(word))
        file_->findNext(word);

      break;
    }

    case CKEY_TYPE_Dollar: {
      uint num = std::max(count_, 1U) - 1;

      file_->cursorDown(num);

      file_->cursorToRight();

      break;
    }

    case CKEY_TYPE_Percent: { // percent of file (count)
                              // find matching pair (no count)
      if (count_ > 0) {
        if (count_ <= 100) {
          int pos = (count_*(file_->getNumLines() - 1))/100;

          file_->cursorTo(pos, 0);
        }
      }
      else {
        char c = file_->getChar();

        if (! strchr("([{}])", c)) {
          if (file_->findNextChar("([{}])", false))
            c = file_->getChar();
          else
            break;
        }

        if      (c == '(')
          file_->findNextChar(')', true);
        else if (c == '[')
          file_->findNextChar(']', true);
        else if (c == '{')
          file_->findNextChar('}', true);
        else if (c == '}')
          file_->findPrevChar('{', true);
        else if (c == ']')
          file_->findPrevChar('[', true);
        else if (c == ')')
          file_->findPrevChar('(', true);
      }

      break;
    }

    case CKEY_TYPE_Ampersand: // repeat last search/replace
      error("Unimplemented");
      break;

    case CKEY_TYPE_ParenLeft: // sentence backward
      file_->prevSentence();
      break;
    case CKEY_TYPE_ParenRight: // sentence forward
      file_->nextSentence();
      break;
    case CKEY_TYPE_BraceLeft: // paragraph backward
      file_->prevParagraph();
      break;
    case CKEY_TYPE_BraceRight: // paragraph forward
      file_->nextParagraph();
      break;

    case CKEY_TYPE_At: // execute command in buffer
      error("Unimplemented");
      break;

    case CKEY_TYPE_Equal: // filter through format command
      error("Unimplemented");
      break;

    case CKEY_TYPE_p: {
      file_->startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->pasteAfter(register_);

      file_->endGroup();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;
    }
    case CKEY_TYPE_P: {
      file_->startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i)
        file_->pasteBefore(register_);

      file_->endGroup();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;
    }

    case CKEY_TYPE_Q: // ex mode
      error("Unimplemented");
      break;

    case CKEY_TYPE_R: // replace mode
      setInsertMode(true);

      file_->setOverwriteMode(true);

      break;

    case CKEY_TYPE_Y:
      file_->yankLines(register_, count_);

      break;

    case CKEY_TYPE_Bar: { // to column
      file_->cursorToLeft();

      int num = count_ - 1;

      file_->cursorRight(num);

      break;
    }

    case CKEY_TYPE_S: // 'cc' - TODO: count
      setInsertMode(true);

      file_->deleteLine();

      file_->newLineAbove();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case CKEY_TYPE_D:
      file_->deleteEOL();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case CKEY_TYPE_G:
      file_->markReturn();

      // goto line
      if (count_ == 0) {
        file_->cursorTo(file_->getNumLines() - 1, 0);

        file_->cursorToRight();
      }
      else
        file_->cursorTo(count_ - 1, 0);

      break;
    case CKEY_TYPE_H: {
      int row = file_->getPageTop();

      file_->cursorTo(row, 0);

      break;
    }
    case CKEY_TYPE_M: {
      int row = (file_->getPageBottom() + file_->getPageTop())/2;

      file_->cursorTo(row, 0);

      break;
    }
    case CKEY_TYPE_L: {
      int row = file_->getPageBottom();

      file_->cursorTo(row, 0);

      break;
    }

    case CKEY_TYPE_J:
      file_->joinLine();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case CKEY_TYPE_Colon:
      setCmdLineMode(true, ":");

      break;

    case CKEY_TYPE_Slash:
      setCmdLineMode(true, "/");

      break;
    case CKEY_TYPE_Question:
      setCmdLineMode(true, "?");

      break;
    case CKEY_TYPE_n:
      if (file_->hasFindPattern())
        file_->findNext(file_->getFindPattern());

      break;
    case CKEY_TYPE_N:
      if (file_->hasFindPattern())
        file_->findPrev(file_->getFindPattern());

      break;

    case CKEY_TYPE_K:
      error("Unimplemented");
      break;

    case CKEY_TYPE_C:
      setInsertMode(true);

      file_->deleteEOL();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case CKEY_TYPE_q:
      error("Unimplemented");
      break;

    case CKEY_TYPE_u: // undo last change
      file_->undo();

      break;

    case CKEY_TYPE_U: // undo line
      file_->undoLine();

      break;

    case CKEY_TYPE_a:
      setInsertMode(true);

      file_->cursorRight(1);

      break;
    case CKEY_TYPE_A:
      setInsertMode(true);

      file_->cursorToRight();

      break;

    case CKEY_TYPE_i:
      setInsertMode(true);

      break;
    case CKEY_TYPE_I:
      setInsertMode(true);

      file_->cursorToLeft();
      file_->cursorSkipSpace();

      break;

    case CKEY_TYPE_o:
      setInsertMode(true);

      file_->newLineBelow();

      break;
    case CKEY_TYPE_O:
      setInsertMode(true);

      file_->newLineAbove();

      break;

    case CKEY_TYPE_Backslash:
      error("Unimplemented");
      break;

    case CKEY_TYPE_s: // Synonym for 'cl'
      error("Unimplemented");
      break;

#if 0
    case CKEY_TYPE_g:
      error("Unimplemented");
      break;
#endif

    case CKEY_TYPE_Semicolon:
      if (findChar_ != '\0')
        doFindChar(findChar_, count_, findForward_, findTill_);

      break;
    case CKEY_TYPE_Comma:
      if (findChar_ != '\0') {
        bool saveFindForward = findForward_;

        doFindChar(findChar_, count_, ! findForward_, findTill_);

        findForward_ = saveFindForward;
      }

      break;

    case CKEY_TYPE_x:
    case CKEY_TYPE_DEL:
      file_->deleteChar();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;
    case CKEY_TYPE_X:
      file_->cursorLeft(1);
      file_->deleteChar();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case CKEY_TYPE_v: // char visual
    case CKEY_TYPE_V: // line visual
      file_->setVisual(! file_->getVisual());

      if (! file_->getVisual())
        file_->clearSelection();
      else
        file_->setSelectRange(file_->getPos(), file_->getSelectEnd());

      break;

    case CKEY_TYPE_Period: {
      lastCommand_.exec();

      break;
    }

    case CKEY_TYPE_TAB:
    case CKEY_TYPE_Tab:
    case CKEY_TYPE_KP_Tab: {
      file_->cursorRight(file_->getTabStop());

      break;
    }

    case CKEY_TYPE_Escape:
      break;

    default:
      error("Unsupported key " + CEvent::keyTypeName(key));
      goto done;
  }

 done:
  count_    = 0;
  lastKey_ = CKEY_TYPE_NUL;
  register_ = '\0';
}

void
CVEditVi::
processControlChar(const CKeyEvent &event)
{
  CKeyType key = event.getType();

  switch (key) {
    case CKEY_TYPE_b:
    case CKEY_TYPE_STX: {
      int num = file_->getPageLength();

      file_->cursorUp(num);

      file_->scrollBottom();

      break;
    }
    case CKEY_TYPE_d:
    case CKEY_TYPE_EOT: {
      int num = file_->getPageLength() / 2;

      file_->cursorDown(num);

      file_->scrollTop();

      break;
    }
    case CKEY_TYPE_e:
    case CKEY_TYPE_ENQ: {
      int row1 = file_->getPageTop();
      int row2 = file_->getRow();

      int num = row2 - row1 - 1;

      if (num > 0) {
        file_->cursorUp(num);

        file_->scrollTop();

        file_->cursorDown(num);
      }
      else {
        file_->cursorDown(1);

        file_->scrollTop();
      }

      break;
    }
    case CKEY_TYPE_f:
    case CKEY_TYPE_ACK: {
      int num = file_->getPageLength();

      file_->cursorDown(num);

      file_->scrollTop();

      break;
    }
    case CKEY_TYPE_g:
    case CKEY_TYPE_BEL: {
      std::cout << file_->getFileName() << " " << file_->getNumLines() << " lines" << std::endl;

      break;
    }
    case CKEY_TYPE_h:
    case CKEY_TYPE_BackSpace:
      file_->cursorLeft(1);

      break;
    case CKEY_TYPE_l:
    case CKEY_TYPE_FF: { // mark all lines changed ?
      file_->setIgnoreChanged(true);

      file_->updateSyntax();

      break;
    }
    case CKEY_TYPE_r:
    case CKEY_TYPE_DC2: {
      file_->redo();

      break;
    }
    case CKEY_TYPE_u:
    case CKEY_TYPE_Sys_Req: {
      int num = file_->getPageLength() / 2;

      file_->cursorUp(num);

      file_->scrollBottom();

      break;
    }
    case CKEY_TYPE_v: // block visual
      // TODO
      break;
    case CKEY_TYPE_y:
    case CKEY_TYPE_EM: {
      int row1 = file_->getPageBottom();
      int row2 = file_->getRow();

      int num = row1 - row2 - 1;

      if (num > 0) {
        file_->cursorDown(num);

        file_->scrollBottom();

        file_->cursorUp(num);
      }
      else {
        file_->cursorUp(1);

        file_->scrollBottom();
      }

      break;
    }
    default:
      break;
  }
}

void
CVEditVi::
processInsertChar(const CKeyEvent &event)
{
  CKeyType key = event.getType();

  lastCommand_.addKey(key);

  if (CEvent::keyTypeIsAlpha(key) ||
      CEvent::keyTypeIsDigit(key)) {
    normalInsertChar(event);

    return;
  }

  switch (key) {
    case CKEY_TYPE_QuoteLeft:
    case CKEY_TYPE_Minus:
    case CKEY_TYPE_Equal:
      normalInsertChar(event);

      break;

    case CKEY_TYPE_BackSpace:
      if (file_->getOverwriteMode()) {
        if      (file_->cursorLeft(1)) {
          // TODO: revert char
        }
        else if (file_->cursorUp(1)) {
          // TODO: revert char
        }
      }
      else {
        if      (file_->cursorLeft(1))
          file_->deleteChar();
        else if (file_->cursorUp(1)) {
          file_->cursorToRight();

          file_->joinLine();
        }
      }

      break;
    case CKEY_TYPE_DEL:
      if (file_->getOverwriteMode()) {
        if      (file_->cursorLeft(1)) {
          // TODO: revert char
        }
        else if (file_->cursorUp(1)) {
          // TODO: revert char
        }
      }
      else
        file_->deleteChar();

      break;

    case CKEY_TYPE_AsciiTilde:
    case CKEY_TYPE_Exclam:
    case CKEY_TYPE_At:
    case CKEY_TYPE_NumberSign:
    case CKEY_TYPE_Dollar:
    case CKEY_TYPE_Percent:
    case CKEY_TYPE_AsciiCircum:
    case CKEY_TYPE_Ampersand:
    case CKEY_TYPE_Asterisk:
    case CKEY_TYPE_ParenLeft:
    case CKEY_TYPE_ParenRight:
    case CKEY_TYPE_Underscore:
    case CKEY_TYPE_Plus:

    case CKEY_TYPE_BraceLeft:
    case CKEY_TYPE_BraceRight:
    case CKEY_TYPE_Bar:

    case CKEY_TYPE_Colon:
    case CKEY_TYPE_QuoteDbl:

    case CKEY_TYPE_Less:
    case CKEY_TYPE_Greater:
    case CKEY_TYPE_Question:

    case CKEY_TYPE_BracketLeft:
    case CKEY_TYPE_BracketRight:
    case CKEY_TYPE_Backslash:

    case CKEY_TYPE_Semicolon:
    case CKEY_TYPE_Apostrophe:

    case CKEY_TYPE_Comma:
    case CKEY_TYPE_Period:
    case CKEY_TYPE_Slash:
      normalInsertChar(event);

      break;

    case CKEY_TYPE_Escape:
      setInsertMode(false);

      file_->cursorLeft(1);

      break;

    case CKEY_TYPE_Space:
      normalInsertChar(event);

      break;

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

    case CKEY_TYPE_TAB:
    case CKEY_TYPE_Tab:
    case CKEY_TYPE_KP_Tab: {
      for (uint i = 0; i < file_->getTabStop(); ++i)
        file_->insertChar(' ');

      break;
    }

    case CKEY_TYPE_Insert: {
      file_->setOverwriteMode(! file_->getOverwriteMode());

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
      error("Unsupported key " + CEvent::keyTypeName(key));
      break;
  }

  count_    = 0;
  lastKey_ = CKEY_TYPE_NUL;
  register_ = '\0';
}

void
CVEditVi::
processCmdLineChar(const CKeyEvent &event)
{
  CKeyType key = event.getType();

  if (key == CKEY_TYPE_Escape) {
    setCmdLineMode(false, "");
    return;
  }

  cmdLine_->keyPress(event);

  std::string line = getCmdLineString();

  if (key == CKEY_TYPE_Return) {
    bool quit;

    if (line[0] == ':')
      file_->runEdCmd(line.substr(1), quit);
    else
      file_->runEdCmd(line, quit);

    if (quit)
      file_->quit();

    setCmdLineMode(false, "");
  }

  if (line.empty())
    setCmdLineMode(false, "");
}

void
CVEditVi::
normalInsertChar(const CKeyEvent &event)
{
  if (file_->getOverwriteMode())
    file_->replaceChar(event.getText()[0]);
  else
    file_->insertChar(event.getText()[0]);

  count_    = 0;
  lastKey_  = CKEY_TYPE_NUL;
  register_ = '\0';
}

bool
CVEditVi::
processMoveChar(const CKeyEvent &event, CIPoint2D &new_pos)
{
  CKeyType key = event.getType();

  uint x = new_pos.x;
  uint y = new_pos.y;

  bool rc = true;

  switch (key) {
    case CKEY_TYPE_b:
      file_->prevWord(&y, &x);
      break;
    case CKEY_TYPE_B:
      file_->prevWORD(&y, &x);
      break;
    case CKEY_TYPE_w:
      file_->nextWord(&y, &x);
      break;
    case CKEY_TYPE_W:
      file_->endWord(&y, &x);
      break;
    case CKEY_TYPE_e:
      file_->endWord(&y, &x);
      break;
    case CKEY_TYPE_E:
      file_->endWORD(&y, &x);
      break;
    case CKEY_TYPE_h:
    case CKEY_TYPE_Left:
    case CKEY_TYPE_BackSpace:
      rc = file_->cursorLeft (1, &y, &x);
      break;
    case CKEY_TYPE_j:
    case CKEY_TYPE_Down:
      rc = file_->cursorDown (1, &y, &x);
      break;
    case CKEY_TYPE_k:
    case CKEY_TYPE_Up:
      rc = file_->cursorUp   (1, &y, &x);
      break;
    case CKEY_TYPE_l:
    case CKEY_TYPE_Right:
    case CKEY_TYPE_Space:
      rc = file_->cursorRight(1, &y, &x);
      break;
    case CKEY_TYPE_0:
      file_->cursorToLeft(&y, &x);
      break;
    case CKEY_TYPE_Dollar:
      file_->cursorToRight(&y, &x);
      break;
    case CKEY_TYPE_Minus:
      file_->cursorFirstNonBlankUp(&y, &x);
      break;
    case CKEY_TYPE_Plus:
    case CKEY_TYPE_Return:
      file_->cursorFirstNonBlankDown(&y, &x);
      break;
    case CKEY_TYPE_Underscore:
    case CKEY_TYPE_AsciiCircum:
      file_->cursorFirstNonBlank(&y, &x);
      break;
    case CKEY_TYPE_BraceLeft:
      file_->prevParagraph(&y, &x);
      break;
    case CKEY_TYPE_BraceRight:
      file_->nextParagraph(&y, &x);
      break;
    default:
      rc = false;
      break;
  }

  if (rc)
    new_pos = CIPoint2D(x, y);

  return rc;
}

bool
CVEditVi::
doFindChar(char c, uint count, bool forward, bool till)
{
  findChar_    = c;
  findForward_ = forward;
  findTill_    = till;

  bool rc = true;

  if (forward) {
    for (uint i = 0; i < std::max(count, 1U); ++i)
      if (! (rc = file_->findNextChar(c, false)))
        break;

    if (till && rc)
      file_->cursorLeft(1);
  }
  else {
    bool rc = true;

    for (uint i = 0; i < std::max(count, 1U); ++i)
      if (! (rc = file_->findPrevChar(c, false)))
        break;

    if (till && rc)
      file_->cursorRight(1);
  }

  return rc;
}

void
CVEditVi::
setInsertMode(bool insertMode)
{
  if (insertMode == insertMode_)
    return;

  insertMode_ = insertMode;

  file_->setOverwriteMode(false);

  file_->stateChanged();

  if (insertMode) {
    file_->startGroup();

    file_->setExtraLineChar(true);
  }
  else {
    file_->endGroup();

    file_->setExtraLineChar(false);
  }
}

void
CVEditVi::
setCmdLineMode(bool cmdLineMode, const std::string &str)
{
  cmdLineMode_ = cmdLineMode;

  if (! cmdLine_)
    cmdLine_ = dynamic_cast<CVLineEdit *>(CEditMgrInst->createLineEdit(file_));

  if (cmdLineMode_)
    cmdLine_->setLine(str);
  else
    cmdLine_->setLine(str);

  cmdLine_->cursorEnd();
}

void
CVEditVi::
drawCmdLine(const CIBBox2D &bbox)
{
  LineEditRenderer r;

  cmdLine_->setFont(file_->getFont());

  cmdLine_->drawInside(&r, bbox);
}

void
CVEditVi::
error(const std::string &msg) const
{
  std::cerr << msg << std::endl;
}

//-------

CVEditLastCommand::
CVEditLastCommand(CVEditFile *file) :
 file_(file)
{
}

void
CVEditLastCommand::
clear()
{
  keys_.clear();
}

void
CVEditLastCommand::
addCount(uint n)
{
  if (n == 0) return;

  std::string str = CStrUtil::toString(n);

  uint len = str.size();

  for (uint i = 0; i < len; ++i) {
    CKeyType key = CEvent::charKeyType(str[i]);

    addKey(key);
  }
}

void
CVEditLastCommand::
addKey(CKeyType key)
{
  keys_.push_back(key);
}

void
CVEditLastCommand::
exec()
{
  static char text[2];

  CKeyEvent event;

  uint len = keys_.size();

  for (uint i = 0; i < len; ++i) {
    char c = CEvent::keyTypeChar(keys_[i]);

    text[0] = c;

    event.setType(keys_[i]);
    event.setText(text);

    file_->keyPress(event);
  }
}
