#include <CVi.h>
#include <CEd.h>
#include <CSyntaxC.h>
#include <CSyntaxCPP.h>
#include <CFile.h>
#include <CStrUtil.h>

#include <cstring>
#include <cmath>
#include <iostream>

static bool my_assert(const char *m, bool ret) {
  std::cerr << m << "\n";
  return ret;
}

#define CASSERT(b,m) (! (b) ? my_assert(m, false) : true)

//---

namespace CVi {

App::
App()
{
  iface_ = new Interface;
}

App::
~App()
{
  resetUndo();

  delete ed_;
}

void
App::
setInterface(Interface *iface)
{
  delete iface_;

  iface_ = iface;
}

void
App::
init()
{
  addLine("");

  //---

  ed_ = new Ed(this);

  ed_->init();

  ed_->setEx(true);

  //---

  cmdLine_ = CmdLineP(createCmdLine());
}

CmdLine *
App::
createCmdLine() const
{
  return new CmdLine(const_cast<App *>(this));
}

void
App::
setFileName(const std::string &filename)
{
  filename_ = filename;

  auto &buffer = getBuffer('%');

  buffer.clear();

  buffer.addLine(filename_, false);
}

//---

bool
App::
loadLines(const std::string &filename)
{
  subDeleteAllLines();

  if (filename != "") {
    setFileName(filename);

    CFile file(filename);

    if (file.exists() && file.isRegular())
      addFileLines(filename, 0);
  }

  if (getNumLines() == 0)
    addLine("");

  resetUndo();

  setUnsaved(false);

  return true;
}

bool
App::
addFileLines(const std::string &filename)
{
  return addFileLines(filename, getRow());
}

bool
App::
addFileLines(const std::string &filename, uint line_num)
{
  CFile file(filename);

  if (! file.exists() || ! file.isRegular())
    return false;

  startGroup();

  std::string line;

  while (file.readLine(line)) {
    uint len = uint(line.size());

    if (len > 0 && line[len - 1] == '\r')
      line = line.substr(0, len - 1);

    addLine(line_num, line);

    ++line_num;
  }

  endGroup();

  return true;
}

bool
App::
saveLines(const std::string &filename)
{
  CFile file(filename);

  if (file.exists() && ! file.isRegular())
    return false;

  setFileName(filename);

  for (auto *line : lines_) {
    file.write(line->getCString());

    file.putC('\n');
  }

  setUnsaved(true);

  return true;
}

//---

void
App::
processChar(char key)
{
  KeyData keyData;

  keyData.key = key;

  processChar(keyData);
}

void
App::
processChar(const KeyData &keyData)
{
  if      (getInsertMode())
    processInsertChar(keyData);
  else if (getCmdLineMode())
    processCmdLineChar(keyData);
  else if (getVisualMode() != VisualMode::NONE)
    processVisualChar(keyData);
  else
    processCommandChar(keyData);
}

void
App::
processCommandChar(const KeyData &keyData)
{
  char key = char(keyData.key);

  switch (keyData.key) {
    case int(KeyData::KeyCode::SHIFT):
    case int(KeyData::KeyCode::CONTROL):
    case int(KeyData::KeyCode::CAPS_LOCK):
    case int(KeyData::KeyCode::META):
    case int(KeyData::KeyCode::ALT):
    case int(KeyData::KeyCode::SUPER):
    case int(KeyData::KeyCode::HYPER):
      return;
    default:
      break;
  }

  //------

  if (lastKey_ != '\0') {
    switch (lastKey_) {
      case '`': { // goto mark line and char
        uint line_num, char_num;

        if (getMarkPos(std::string(&key, 1), &line_num, &char_num)) {
          markReturn();

          cursorTo(line_num, char_num);
        }

        goto done;
      }
      case '"': { // set register
        if      (key == '\t') {
          iface_->displayRegisters();
        }
        else if (isalnum(key) || strchr("#/", key))
          register_ = key;
        else
          error("Invalid register name '" + std::string(&key, 1) + "'");

        goto done;
      }
      case '\'': { // goto mark line
        if (key == '\t')
          iface_->displayMarks();
        else {
          uint line_num, char_num;

          if (getMarkPos(std::string(&key, 1), &line_num, &char_num)) {
            markReturn();

            cursorTo(line_num, 0);
          }
        }

        goto done;
      }
      case 'c': { // change
        if      (key == 'c') {
          setInsertMode(true);

          cursorToLeft();
          deleteEOL();
        }
        else if (key == 'w') {
          setInsertMode(true);

          deleteWord();
        }
        else if (key == 'l') {
          setInsertMode(true);

          for (uint i = 0; i < std::max(count_, 1U); ++i)
            deleteChar();
        }
        else {
          uint start_x, start_y;
          getPos(&start_x, &start_y);

          int end_x = start_x;
          int end_y = start_y;

          bool rc = processMoveChar(keyData, end_x, end_y);

          if (rc) {
            setInsertMode(true);

            deleteTo(start_y, start_x, end_y, end_x);
          }
        }

        lastCommand_.clear();
        lastCommand_.addCount(count_);
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case 'd': { // delete
        if      (key == 'd')
          deleteLine();
        else if (key == 'w')
          deleteWord();
        else if (key == 'l')
          deleteChar();
        else {
          uint start_x, start_y;
          getPos(&start_x, &start_y);

          int end_x = start_x;
          int end_y = start_y;

          bool rc = processMoveChar(keyData, end_x, end_y);

          if (rc)
            deleteTo(start_y, start_x, end_y, end_x);
        }

        lastCommand_.clear();
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case 'f': { // find next char
        doFindChar(key, std::max(count_, 1U), true, false);

        break;
      }
      case 'F': { // find prev char
        doFindChar(key, std::max(count_, 1U), false, false);

        break;
      }
      case 'g': {
        uint x1, y1;
        getPos(&x1, &y1);

        setSelectRange(y1, x1, y1, x1);

        // last visual mode
        if      (key == 'v') {
          selection_ = lastSelection_;
        }
        // select next match
        else if (key == 'n') {
          if (hasFindPattern())
            findNext(getFindPattern());
        }
        // select prev match
        else if (key == 'N') {
          if (hasFindPattern())
            findPrev(getFindPattern());
        }
        // test code !!!
        else {
          int x2 = x1, y2 = y1;
          bool rc = processMoveChar(keyData, x2, y2);

          if (rc)
            setPos(x2, y2);
        }

        setVisualMode(VisualMode::CHAR);

        updateSelectRange();

        break;
      }
      case 'm': { // mark
        setMarkPos(std::string(&key, 1));

        goto done;
      }
      case 'r': { // replace char
        replaceChar(key);

        cursorLeft(1);

        lastCommand_.clear();
        lastCommand_.addKey(lastKey_);
        lastCommand_.addKey(key);

        break;
      }
      case 't': { // till next char
        doFindChar(key, std::max(count_, 1U), true, true);

        break;
      }
      case 'T': { // till prev char
        doFindChar(key, std::max(count_, 1U), false, true);

        break;
      }
      case 'y': { // yank
        if      (key == 'y')
          yankLines(register_, std::max(count_, 1U));
        else if (key == 'w')
          yankWords(register_, std::max(count_, 1U));
        else {
          uint start_x, start_y;
          getPos(&start_x, &start_y);

          int end_x = start_x;
          int end_y = start_y;

          bool rc = processMoveChar(keyData, end_x, end_y);

          if (rc)
            yankTo(register_, start_y, start_x, end_y, end_x, false);
        }

        break;
      }
      case 'z': { // scroll to
        if (keyData.key == int(KeyData::KeyCode::ENTER) ||
            keyData.key == int(KeyData::KeyCode::RETURN) ||
            key == '\r' || key == '+')
          iface_->scrollTop();
        else if (key == '.' || key == 'z')
          iface_->scrollMiddle();
        else if (key == 'b' || key == '-')
          iface_->scrollBottom();
        else if (key == 't')
          iface_->scrollTop();

        break;
      }
      case 'Z': {
        if      (key == 'Q')
          iface_->quit();
        else if (key == 'Z') {
          saveLines(getFileName());

          iface_->quit();
        }

        break;
      }
      case '!': {
        std::string str;

        if      (key == '!')
          str = ":.!";
        else {
          uint pos_x1, pos_y1;
          getPos(&pos_x1, &pos_y1);

          setSelectRange(pos_y1, pos_x1, pos_y1, pos_x1);

          int pos_x2 = pos_x1, pos_y2 = pos_y1;
          bool rc = processMoveChar(keyData, pos_x2, pos_y2);

          if (rc) {
            int d = std::abs(pos_y2 - int(pos_y1));

            if (d != 0) {
              cursorTo(pos_y2, pos_x2);

              str = ":.,+" + CStrUtil::toString(d) + "!";
            }
            else
              str = ":.!";
          }
        }

        setCmdLineMode(true, str);

        break;
      }

      case '>': {
        uint start_x, start_y;
        getPos(&start_x, &start_y);

        int end_x = start_x;
        int end_y = start_y;

        if      (key == '>')
          shiftRight(start_y, end_y);
        else {
          bool rc = processMoveChar(keyData, end_x, end_y);

          if (rc)
            shiftRight(start_y, end_y);
        }

        break;
      }
      case '<': {
        uint start_x, start_y;
        getPos(&start_x, &start_y);

        int end_x = start_x;
        int end_y = start_y;

        if      (key == '<')
          shiftLeft(start_y, end_y);
        else {
          bool rc = processMoveChar(keyData, end_x, end_y);

          if (rc)
            shiftRight(start_y, end_y);
        }

        break;
      }

      case '[': {
        if (key == '[')
          prevSection();

        break;
      }
      case ']': {
        if (key == ']')
          nextSection();

        break;
      }
      default:
        break;
    }

    goto done;
  }

  //------

  if      (key == '0') {
    if (count_ == 0) {
      cursorToLeft();

      goto done;
    }
    else {
      count_ = count_*10 + (key - '0');

      //count_str_ += CEvent::keyTypeChar(key);
      iface_->stateChanged();

      return;
    }
  }
  else if (key >= '1' && key <= '9') {
    count_ = count_*10 + (key - '0');

    //count_str_ += CEvent::keyTypeChar(key);
    iface_->stateChanged();

    return;
  }

  //------

  if (keyData.is_control || keyData.is_meta)
    processControlChar(keyData);
  else
    processNormalChar(keyData);

  return;

 done:
  count_   = 0;
  lastKey_ = '\0';
}

void
App::
processNormalChar(const KeyData &keyData)
{
  char key = char(keyData.key);

  switch (keyData.key) {
    // cursor movement
    case 'h':
    case '\b':
    case int(KeyData::KeyCode::BACKSPACE): {
      cursorLeft(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::LEFT): {
      if (keyData.is_shift) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          prevWord();
      }
      else
        cursorLeft(std::max(count_, 1U));

      break;
    }
    case 'j': {
      cursorDown(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::DOWN): {
      if (keyData.is_shift) {
        int num = iface_->getPageLength();

        cursorDown(num);

        iface_->scrollTop();
      }
      else
        cursorDown(std::max(count_, 1U));

      break;
    }
    case 'k': {
      cursorUp(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::UP): {
      if (keyData.is_shift) {
        int num = iface_->getPageLength();

        cursorUp(num);

        iface_->scrollBottom();
      }
      else
        cursorUp(std::max(count_, 1U));

      break;
    }
    case 'l':
    case ' ':
    case '\f': {
      cursorRight(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::RIGHT): {
      if (keyData.is_shift) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          nextWord();
      }
      else
        cursorRight(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::INSERT): {
      setOverwriteMode(! getOverwriteMode());

      break;
    }
    case int(KeyData::KeyCode::HOME): {
      cursorTo(0, 0);

      break;
    }
    case int(KeyData::KeyCode::END): {
      cursorTo(getNumLines() - 1, 0);

      break;
    }
    case 'b': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        prevWord();

      break;
    }
    case 'B': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        prevWORD();

      break;
    }
    case 'w': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        nextWord();

      break;
    }
    case 'W': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        nextWORD();

      break;
    }
    case 'e': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        endWord();

      break;
    }
    case 'E': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        endWORD();

      break;
    }

    //----------

    // scroll
    case int(KeyData::KeyCode::PAGE_DOWN):
    case '\006': /*ACK*/ {
      int num = iface_->getPageLength();

      cursorDown(num);

      iface_->scrollTop();

      break;
    }
    case int(KeyData::KeyCode::PAGE_UP):
    case 0x02 /*STX*/: {
      int num = iface_->getPageLength();

      cursorUp(num);

      iface_->scrollBottom();

      break;
    }
    case int(KeyData::KeyCode::SYS_REQ): {
      int num = iface_->getPageLength() / 2;

      cursorUp(num);

      iface_->scrollBottom();

      break;
    }
    case 0x04 /*EOT*/: {
      int num = iface_->getPageLength() / 2;

      cursorDown(num);

      iface_->scrollTop();

      break;
    }
    case 0x05 /*ENQ*/: {
      int row1 = iface_->getPageTop();
      int row2 = getRow();

      int num = row2 - row1 - 1;

      if (num > 0) {
        cursorUp(num);

        iface_->scrollTop();

        cursorDown(num);
      }
      else {
        cursorDown(1);

        iface_->scrollTop();
      }

      break;
    }
    case 0x19 /*EM*/: {
      int row1 = iface_->getPageBottom();
      int row2 = getRow();

      int num = row1 - row2 - 1;

      if (num > 0) {
        cursorDown(num);

        iface_->scrollBottom();

        cursorUp(num);
      }
      else {
        cursorUp(1);

        iface_->scrollBottom();
      }

      break;
    }

    //----------

    case '': {
      std::cout << getFileName() << " " << getNumLines() << " lines\n";

      break;
    }
    case 0x12 /*DC2*/: {
      redo();

      break;
    }

    //----------

    // Multi-char commands
    case 'c':
    case 'd':
    case 'f':
    case 'g':
    case 'm':
    case 'r':
    case 't':
    case 'y':
    case 'z':
    case 'F':
    case 'T':
    case 'Z':
    case '`':
    case '"':
    case '\'':
    case '<':
    case '>':
    case '[':
    case ']':
    case '!': {
      lastKey_ = key;
      return;
    }

    //----------

    case '-': // first non-blank up
      cursorFirstNonBlankUp();
      break;

    case int(KeyData::KeyCode::ENTER):
    case int(KeyData::KeyCode::RETURN):
    case '+': // first non-blank down
    case '\r':
      cursorFirstNonBlankDown();
      break;

    case '_': // first non-blank
    case '^':
      cursorFirstNonBlank();
      break;

    case '~':
      swapChar();

      cursorRight(1);

      break;

    case '#': { // find string under cursor backward
      std::string word;

      if (getWord(word))
        findPrev(word);

      break;
    }

    case '*': { // find string under cursor forward
      std::string word;

      if (getWord(word))
        findNext(word);

      break;
    }

    case '$': {
      uint num = std::max(count_, 1U) - 1;

      cursorDown(num);

      cursorToRight();

      break;
    }

    case '%': { // percent of file (count)
                // find matching pair (no count)
      if (count_ > 0) {
        if (count_ <= 100) {
          int pos = (count_*(getNumLines() - 1))/100;

          cursorTo(pos, 0);
        }
      }
      else {
        char c = getChar();

        if (! strchr("([{}])", c)) {
          if (findNextChar("([{}])", false))
            c = getChar();
          else
            break;
        }

        if      (c == '(')
          findNextChar(')', true);
        else if (c == '[')
          findNextChar(']', true);
        else if (c == '{')
          findNextChar('}', true);
        else if (c == '}')
          findPrevChar('{', true);
        else if (c == ']')
          findPrevChar('[', true);
        else if (c == ')')
          findPrevChar('(', true);
      }

      break;
    }

    case '&': // repeat last search/replace
      error("Unimplemented");
      break;

    case '(': // sentence backward
      prevSentence();
      break;
    case ')': // sentence forward
      nextSentence();
      break;

    case '{': // paragraph backward
      prevParagraph();
      break;
    case '}': // paragraph forward
      nextParagraph();
      break;

    case '@': // execute command in buffer
      error("Unimplemented");
      break;

    case '=': // filter through format command
      error("Unimplemented");
      break;

    case 'p': {
      startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i)
        pasteAfter(register_);

      endGroup();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;
    }
    case 'P': {
      startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i)
        pasteBefore(register_);

      endGroup();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;
    }

    case 'Q': // ex mode
      error("Unimplemented");
      break;

    case 'R': // replace mode
      setInsertMode(true);

      setOverwriteMode(true);

      break;

    case 'Y':
      yankLines(register_, count_);

      break;

    case '|': { // to column
      cursorToLeft();

      int num = count_ - 1;

      cursorRight(num);

      break;
    }

    case 'S': // 'cc' - TODO: count
      setInsertMode(true);

      deleteLine();

      newLineAbove();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case 'D':
      deleteEOL();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case 'G':
      markReturn();

      // goto line
      if (count_ == 0) {
        cursorTo(getNumLines() - 1, 0);

        cursorToRight();
      }
      else
        cursorTo(count_ - 1, 0);

      break;
    case 'H': {
      int row = iface_->getPageTop();

      if (count_ == 0)
        cursorTo(row, 0);
      else
        cursorTo(row + count_ - 1, 0);

      break;
    }
    case 'M': {
      int row = (iface_->getPageBottom() + iface_->getPageTop())/2;

      cursorTo(row, 0);

      break;
    }
    case 'L': {
      int row = iface_->getPageBottom();

      if (count_ == 0)
        cursorTo(row, 0);
      else
        cursorTo(row - count_ + 1, 0);

      break;
    }

    case 'J':
      joinLine();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case ':':
      setCmdLineMode(true, ":");

      break;

    case '/':
      setCmdLineMode(true, "/");

      break;
    case '?':
      setCmdLineMode(true, "?");

      break;
    case 'n':
      if (hasFindPattern())
        findNext(getFindPattern());

      break;
    case 'N':
      if (hasFindPattern())
        findPrev(getFindPattern());

      break;

    case 'K':
      error("Unimplemented");
      break;

    case 'C':
      setInsertMode(true);

      deleteEOL();

      lastCommand_.clear();
      lastCommand_.addKey(key);

      break;

    case 'q':
      error("Unimplemented");
      break;

    case 'u': // undo last change
      undo();

      break;

    case 'U': // undo line
      undoLine();

      break;

    case 'a':
      setInsertMode(true);

      cursorRight(1);

      break;
    case 'A':
      setInsertMode(true);

      cursorToRight();

      break;

    case 'i':
      setInsertMode(true);

      break;
    case 'I':
      setInsertMode(true);

      cursorToLeft();
      cursorSkipSpace();

      break;

    case 'o':
      setInsertMode(true);

      newLineBelow();

      break;
    case 'O':
      setInsertMode(true);

      newLineAbove();

      break;

    case '\\':
      error("Unimplemented");
      break;

    case 's': // Synonym for 'cl'
      error("Unimplemented");
      break;

#if 0
    case 'g':
      error("Unimplemented");
      break;
#endif

    case ';':
      if (findChar_ != '\0')
        doFindChar(findChar_, count_, findForward_, findTill_);

      break;
    case ',':
      if (findChar_ != '\0') {
        bool saveFindForward = findForward_;

        doFindChar(findChar_, count_, ! findForward_, findTill_);

        findForward_ = saveFindForward;
      }

      break;

    case 'x':
    case 0x7f /*DEL*/: {
      startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i)
        deleteChar();

      endGroup();

      lastCommand_.clear();
      lastCommand_.addCount(count_);
      lastCommand_.addKey(key);

      break;
    }
    case 'X':
      startGroup();

      for (uint i = 0; i < std::max(count_, 1U); ++i) {
        cursorLeft(1);
        deleteChar();
      }

      endGroup();

      lastCommand_.clear();
      lastCommand_.addCount(count_);
      lastCommand_.addKey(key);

      break;

    case 'V': // line visual
      if (getVisualMode() != VisualMode::LINE)
        setVisualMode(VisualMode::LINE);
      else
        setVisualMode(VisualMode::NONE);

      if (getVisualMode() == VisualMode::LINE) {
        uint pos_x, pos_y;
        getPos(&pos_x, &pos_y);

        int n = std::max(count_, 1U);

        setSelectRange(pos_y, 0, pos_y + n - 1, 999);

        cursorDown(n - 1);
      }
      else
        clearSelection();

      break;
    case 'v': // char visual
      if (getVisualMode() != VisualMode::CHAR)
        setVisualMode(VisualMode::CHAR);
      else
        setVisualMode(VisualMode::NONE);

      if (getVisualMode() == VisualMode::CHAR) {
        uint pos_x, pos_y;
        getPos(&pos_x, &pos_y);

        int n = std::max(count_, 1U);

        setSelectRange(pos_y, pos_x, pos_y, pos_x + n - 1);

        cursorRight(n - 1);
      }
      else
        clearSelection();

      break;

    case '.': {
      lastCommand_.exec(this);

      break;
    }

    case '\t': {
      cursorRight(iface_->getTabStop());

      break;
    }

    case int(KeyData::KeyCode::ESCAPE):
    case '\033':
      setVisualMode(VisualMode::NONE);
      break;

    default:
      error(std::string("Unsupported key ") + key);
      goto done;
  }

 done:
  count_    = 0;
  lastKey_  = '\0';
  register_ = '\0';
}

void
App::
processControlChar(const KeyData &keyData)
{
  //char key = char(keyData.key);

  switch (keyData.key) {
    case 'b':
    case 0x02 /*STX*/: {
      int num = iface_->getPageLength();

      cursorUp(num);

      iface_->scrollBottom();

      break;
    }
    case 'd':
    case 0x04 /*EOT*/: {
      int num = iface_->getPageLength() / 2;

      cursorDown(num);

      iface_->scrollTop();

      break;
    }
    case 'e':
    case 0x05 /*ENQ*/: {
      int row1 = iface_->getPageTop();
      int row2 = getRow();

      int num = row2 - row1 - 1;

      if (num > 0) {
        cursorUp(num);

        iface_->scrollTop();

        cursorDown(num);
      }
      else {
        cursorDown(1);

        iface_->scrollTop();
      }

      break;
    }
    case 'f':
    case 0x06 /*ACK*/: {
      int num = iface_->getPageLength();

      cursorDown(num);

      iface_->scrollTop();

      break;
    }
    case 'g':
    case '\a': {
      std::cout << getFileName() << " " << getNumLines() << " lines\n";

      break;
    }
    case 'h':
    case '\b':
    case int(KeyData::KeyCode::BACKSPACE): {
      cursorLeft(1);

      break;
    }
    case 'l':
    case '\f': { // mark all lines changed ?
      iface_->setIgnoreChanged(true);

      iface_->updateSyntax();

      iface_->setIgnoreChanged(false);

      break;
    }
    case 'r':
    case 0x12 /*DC2*/: {
      redo();

      break;
    }
    case 'u':
    case int(KeyData::KeyCode::SYS_REQ): {
      int num = iface_->getPageLength() / 2;

      cursorUp(num);

      iface_->scrollBottom();

      break;
    }
    case 'v': // block visual
      if (getVisualMode() != VisualMode::BLOCK)
        setVisualMode(VisualMode::BLOCK);
      else
        setVisualMode(VisualMode::NONE);

      if (getVisualMode() == VisualMode::BLOCK) {
        uint pos_x, pos_y;
        getPos(&pos_x, &pos_y);

        int n = std::max(count_, 1U);

        setSelectRange(pos_y, pos_x, pos_y, pos_x + n - 1);

        cursorRight(n - 1);
      }
      else
        clearSelection();

      break;

    case 'y':
    case 0x19 /*EM*/: {
      int row1 = iface_->getPageBottom();
      int row2 = getRow();

      int num = row1 - row2 - 1;

      if (num > 0) {
        cursorDown(num);

        iface_->scrollBottom();

        cursorUp(num);
      }
      else {
        cursorUp(1);

        iface_->scrollBottom();
      }

      break;
    }
    default:
      break;
  }
}

void
App::
processInsertChar(const KeyData &keyData)
{
  char key = char(keyData.key);

  lastCommand_.addKey(key);

  if (isalpha(key) || isdigit(key)) {
    processNormalInsertChar(key);

    return;
  }

  switch (keyData.key) {
    case '`':
    case '-':
    case '=':
      processNormalInsertChar(key);

      break;

    case '\b':
    case int(KeyData::KeyCode::BACKSPACE): {
      if (getOverwriteMode()) {
        if      (cursorLeft(1)) {
          // TODO: revert char
        }
        else if (cursorUp(1)) {
          // TODO: revert char
        }
      }
      else {
        if      (cursorLeft(1))
          deleteChar();
        else if (cursorUp(1)) {
          cursorToRight();

          joinLine();
        }
      }

      break;
    }
    case 0x7f /*DEL*/:
      if (getOverwriteMode()) {
        if      (cursorLeft(1)) {
          // TODO: revert char
        }
        else if (cursorUp(1)) {
          // TODO: revert char
        }
      }
      else
        deleteChar();

      break;

    case '~':
    case '!':
    case '@':
    case '#':
    case '$':
    case '%':
    case '^':
    case '&':
    case '*':
    case '(':
    case ')':
    case '_':
    case '+':

    case '{':
    case '}':
    case '|':

    case ':':
    case '"':

    case '<':
    case '>':
    case '?':

    case '[':
    case ']':
    case '\\':

    case ';':
    case '\'':

    case ',':
    case '.':
    case '/':
      processNormalInsertChar(key);

      break;

    case int(KeyData::KeyCode::ESCAPE):
    case '\033':
      setInsertMode(false);
      setVisualMode(VisualMode::NONE);

      cursorLeft(1);

      break;

    case ' ':
      processNormalInsertChar(key);

      break;

    case int(KeyData::KeyCode::ENTER):
    case int(KeyData::KeyCode::RETURN):
    case '\r':
      splitLine();
      break;

    case int(KeyData::KeyCode::LEFT):
      cursorLeft(1);
      break;
    case int(KeyData::KeyCode::UP):
      cursorUp(1);
      break;
    case int(KeyData::KeyCode::RIGHT):
      cursorRight(1);
      break;
    case int(KeyData::KeyCode::DOWN):
      cursorDown(1);
      break;

    case '\t': {
      for (uint i = 0; i < iface_->getTabStop(); ++i)
        insertChar(' ');

      break;
    }

    case int(KeyData::KeyCode::INSERT): {
      setOverwriteMode(! getOverwriteMode());

      break;
    }

    case int(KeyData::KeyCode::SHIFT):
    case int(KeyData::KeyCode::CONTROL):
    case int(KeyData::KeyCode::CAPS_LOCK):
    case int(KeyData::KeyCode::META):
    case int(KeyData::KeyCode::ALT):
    case int(KeyData::KeyCode::SUPER):
    case int(KeyData::KeyCode::HYPER):
      break;

    default:
      error(std::string("Unsupported key ") + key);
      break;
  }

  count_    = 0;
  lastKey_  = '\0';
  register_ = '\0';
}

void
App::
processVisualChar(const KeyData &keyData)
{
  char key = char(keyData.key);

  if      (lastKey_ != '\0') {
    switch (lastKey_) {
      case 'a': {
        switch (keyData.key) {
          case 'w':
            nextWord();
            break;
          case 'W':
            nextWORD();
            break;
          case 's':
            nextSentence();
            break;
          case 'p':
            nextParagraph();
            break;

          default:
            error(std::string("Unsupported key ") + key);
            break;
        }
        break;
      }
      case 'i': {
        switch (keyData.key) {
          case 'w':
          case 'W':
          case 's':
          case 'p':
          default:
            error(std::string("Unsupported key ") + key);
            break;
        }
        break;
      }
      case 'g': {
        auto selection = getSelection();

        // last visual mode
        if      (key == 'v') {
        }
        // select next match
        else if (key == 'n') {
          if (hasFindPattern())
            findNext(getFindPattern());
        }
        // select prev match
        else if (key == 'N') {
          if (hasFindPattern())
            findPrev(getFindPattern());
        }
        // format lines to 'textwidth' length
        else if (key == 'q') {
          error("Unimplemented");
          break;
        }
        // test code !!!
        else {
          int x2 = selection.col2, y2 = selection.row2;
          bool rc = processMoveChar(keyData, x2, y2);

          if (rc)
            setPos(x2, y2);
        }

        updateSelectRange();

        break;
      }
      default:
        error(std::string("Unsupported key ") + key);
        break;
    }

    lastKey_ = '\0';

    return;
  }

  //---

  switch (keyData.key) {
    // cursor movement
    case 'h':
    case '\b':
    case int(KeyData::KeyCode::BACKSPACE): {
      cursorLeft(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::LEFT): {
      if (keyData.is_shift) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          prevWord();
      }
      else
        cursorLeft(std::max(count_, 1U));

      break;
    }
    case 'j': {
      cursorDown(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::DOWN): {
      if (keyData.is_shift) {
        int num = iface_->getPageLength();

        cursorDown(num);

        iface_->scrollTop();
      }
      else
        cursorDown(std::max(count_, 1U));

      break;
    }
    case 'k': {
      cursorUp(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::UP): {
      if (keyData.is_shift) {
        int num = iface_->getPageLength();

        cursorUp(num);

        iface_->scrollBottom();
      }
      else
        cursorUp(std::max(count_, 1U));

      break;
    }
    case 'l':
    case ' ':
    case '\f': {
      cursorRight(std::max(count_, 1U));

      break;
    }
    case int(KeyData::KeyCode::RIGHT): {
      if (keyData.is_shift) {
        for (uint i = 0; i < std::max(count_, 1U); ++i)
          nextWord();
      }
      else
        cursorRight(std::max(count_, 1U));

      break;
    }

    case 'b': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        prevWord();

      break;
    }
    case 'B': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        prevWORD();

      break;
    }
    case 'w': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        nextWord();

      break;
    }
    case 'W': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        nextWORD();

      break;
    }
    case 'e': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        endWord();

      break;
    }
    case 'E': {
      for (uint i = 0; i < std::max(count_, 1U); ++i)
        endWORD();

      break;
    }

    // Other visual point
    case 'o':
    case 'O': {
      int selRow1, selCol1, selRow2, selCol2;
      getSelectStart(&selRow1, &selCol1);
      getSelectEnd  (&selRow2, &selCol2);

      uint x, y;
      getPos(&x, &y);

      if      (getVisualMode() == VisualMode::LINE) {
        if      (int(y) == selRow1)
          setPos(0, selRow2);
        else if (int(y) == selRow2)
          setPos(0, selRow1);
      }
      else if (getVisualMode() == VisualMode::CHAR) {
        if      (int(y) == selRow1)
          setPos(selCol2, selRow2);
        else if (int(y) == selRow2)
          setPos(selCol1, selRow1);
      }
      else {
        if (keyData.key == 'o') {
          if      (int(y) == selRow1 && int(x) == selCol1)
            setPos(selCol2, selRow2);
          else if (int(y) == selRow2 && int(x) == selCol2)
            setPos(selCol1, selRow1);
        }
        else {
          if      (int(x) == selCol1)
            setPos(selCol2, y);
          else if (int(x) == selCol2)
            setPos(selCol1, y);
        }
      }

      break;
    }

    // switch case
    case '~':
      processSelection(SelectionOp::SWAP_CASE);
      break;
    // delete selection
    case 'd':
      deleteSelection();
      break;
    // change selection
    case 'c':
      changeSelection();
      break;
    // yank selection
    case 'y':
      yankSelection();
      break;
    // shift right selection
    case '>':
      rshiftSelection();
      break;
    // shift left selection
    case '<':
      lshiftSelection();
      break;
    // filter though external command
    case '!': {
      std::string str = "'<,'>!";
      setCmdLineMode(true, str);
      break;
    }
    case '=': { // filter through format command
      error("Unimplemented");
      break;
    }

    // start ex command for selection
    case ':': {
      std::string str = "'<,'>";
      setCmdLineMode(true, str);
      break;
    }
    // single char replace
    case 'r': {
      error("Unimplemented");
      break;
    }
    // insert replace
    case 's': {
      error("Unimplemented");
      break;
    }
    // change selection lines
    case 'C': {
      error("Unimplemented");
      break;
    }
    // substitute selection lines
    case 'S': {
      error("Unimplemented");
      break;
    }
    // replace selection lines
    case 'R': {
      error("Unimplemented");
      break;
    }
    case 'x': // delete
      deleteSelection();
      break;
    case 'D': // delete to end of line
      deleteSelection();
      break;
    case 'X': // delete
      deleteSelection();
      break;
    case 'Y': // yank
      yankSelection();
      break;
    case 'p': // put
    case 'P': {
      error("Unimplemented");
      break;
    }
    case 'J': {
      error("Join");
      break;
    }
    case 'U': {
      processSelection(SelectionOp::TO_UPPER);
      break;
    }
    case 'u': {
      processSelection(SelectionOp::TO_LOWER);
      break;
    }
    case 'I': {
      setCursorList(/*left*/true);
      setInsertMode(true);
      break;
    }
    case 'A': {
      setCursorList(/*left*/false);
      setInsertMode(true);
      break;
    }

    case 'a': // add
    case 'i': // insert ?
    case 'g': { // goto
      lastKey_ = key;
      break;
    }

    case 'V': // line visual
      if (getVisualMode() != VisualMode::LINE)
        setVisualMode(VisualMode::LINE);
      else
        setVisualMode(VisualMode::NONE);

      break;
    case 'v': // char visual
      if (keyData.is_control) {
        if (getVisualMode() != VisualMode::BLOCK)
          setVisualMode(VisualMode::BLOCK);
        else
          setVisualMode(VisualMode::NONE);
      }
      else {
        if (getVisualMode() != VisualMode::CHAR)
          setVisualMode(VisualMode::CHAR);
        else
          setVisualMode(VisualMode::NONE);
      }

      break;

    case int(KeyData::KeyCode::ESCAPE):
    case '\033':
      setVisualMode(VisualMode::NONE);
      break;

    case int(KeyData::KeyCode::SHIFT):
    case int(KeyData::KeyCode::CONTROL):
    case int(KeyData::KeyCode::CAPS_LOCK):
    case int(KeyData::KeyCode::META):
    case int(KeyData::KeyCode::ALT):
    case int(KeyData::KeyCode::SUPER):
    case int(KeyData::KeyCode::HYPER):
      break;

    default:
      error(std::string("Unsupported key ") + key);
      break;
  }
}

void
App::
processCmdLineChar(const KeyData &keyData)
{
  char key = char(keyData.key);

  if (keyData.key == int(KeyData::KeyCode::ESCAPE) || key == '\033') {
    setCmdLineMode(false, "");
    return;
  }

  if      (keyData.key == int(KeyData::KeyCode::BACKSPACE))
    cmdLine_->backspace();
  else if (keyData.key <= 127)
    cmdLine_->keyPress(key);

  auto line = getCmdLineString();

  if (keyData.key == int(KeyData::KeyCode::ENTER) ||
      keyData.key == int(KeyData::KeyCode::RETURN) || key == '\r') {
    bool quitted;

    if (line[0] == ':')
      runEdCmd(line.substr(1), quitted);
    else
      runEdCmd(line, quitted);

    if (quitted)
      iface_->quit();

    setCmdLineMode(false, "");
  }

  if (line.empty())
    setCmdLineMode(false, "");
}

void
App::
processNormalInsertChar(char key)
{
  if (getOverwriteMode())
    replaceChar(key);
  else
    insertChar(key);

  for (auto &pos :  extraCursorPosList_) {
    insertChar(pos.row, pos.col, key);

    ++pos.col;
  }

  count_    = 0;
  lastKey_  = '\0';
  register_ = '\0';
}

bool
App::
processMoveChar(const KeyData &keyData, int &new_pos_x, int &new_pos_y)
{
  uint x = new_pos_x;
  uint y = new_pos_y;

  bool rc = true;

  switch (keyData.key) {
    case 'b':
      prevWord(&y, &x);
      break;
    case 'B':
      prevWORD(&y, &x);
      break;
    case 'w':
      nextWord(&y, &x);
      break;
    case 'W':
      endWord(&y, &x);
      break;
    case 'e':
      endWord(&y, &x);
      break;
    case 'E':
      endWORD(&y, &x);
      break;
    case 'h':
    case '\b':
    case int(KeyData::KeyCode::BACKSPACE):
    case int(KeyData::KeyCode::LEFT): {
      rc = cursorLeft(1, &y, &x);
      break;
    }
    case 'j':
    case int(KeyData::KeyCode::DOWN):
      rc = cursorDown(1, &y, &x);
      break;
    case 'k':
    case int(KeyData::KeyCode::UP):
      rc = cursorUp(1, &y, &x);
      break;
    case 'l':
    case int(KeyData::KeyCode::RIGHT):
    case ' ':
      rc = cursorRight(1, &y, &x);
      break;
    case '0':
      cursorToLeft(&y, &x);
      break;
    case '$':
      cursorToRight(&y, &x);
      break;
    case '-':
      cursorFirstNonBlankUp(&y, &x);
      break;
    case '+':
    case '\r':
      cursorFirstNonBlankDown(&y, &x);
      break;
    case '_':
    case '^':
      cursorFirstNonBlank(&y, &x);
      break;
    case '{':
      prevParagraph(&y, &x);
      break;
    case '}':
      nextParagraph(&y, &x);
      break;
    default:
      rc = false;
      break;
  }

  if (rc) {
    new_pos_x = x;
    new_pos_y = y;
  }

  return rc;
}

//---

void
App::
updateSelectRange()
{
  int select_start_row, select_start_col;
  getSelectStart(&select_start_row, &select_start_col);

  uint x, y;
  getPos(&x, &y);

  rangeSelect(select_start_row, select_start_col, y, x, true);
}

//---

bool
App::
doFindChar(char c, uint count, bool forward, bool till)
{
  findChar_    = c;
  findForward_ = forward;
  findTill_    = till;

  bool rc = true;

  if (forward) {
    for (uint i = 0; i < std::max(count, 1U); ++i)
      if (! (rc = findNextChar(c, false)))
        break;

    if (till && rc)
      cursorLeft(1);
  }
  else {
    bool rc1 = true;

    for (uint i = 0; i < std::max(count, 1U); ++i)
      if (! (rc1 = findPrevChar(c, false)))
        break;

    if (till && rc1)
      cursorRight(1);
  }

  return rc;
}

void
App::
setInsertMode(bool insertMode)
{
  if (insertMode == insertMode_)
    return;

  insertMode_ = insertMode;

  iface_->stateChanged();

  setOverwriteMode(false);

  if (insertMode) {
    startGroup();

    setExtraLineChar(true);
  }
  else {
    endGroup();

    setExtraLineChar(false);
  }

  if (! insertMode_)
    extraCursorPosList_.clear();
}

void
App::
setOverwriteMode(bool value)
{
  if (overwriteMode_ == value)
    return;

  overwriteMode_ = value;

  iface_->stateChanged();
}

void
App::
setVisualMode(VisualMode mode)
{
  if (visual_ == mode)
    return;

  visual_ = mode;

  iface_->stateChanged();
}


void
App::
changeSelection()
{
  insertRows_.clear();

  // TODO: repeat edits on first line to all lines
  if (getVisualMode() == App::VisualMode::BLOCK) {
    auto selection = getSelection();

    for (uint r = selection.row1; r <= selection.row2; ++r)
      insertRows_.push_back(r);
  }

  setInsertMode(true);

  deleteSelection();
}

void
App::
deleteSelection()
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  if      (visualMode == App::VisualMode::CHAR) {
    startGroup();

    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    setPos(selRow1, selCol1);

    subDeleteChars(selRow1, selCol1, getLineLength(selRow1) - selCol1);

    for (int r = selRow1 + 1; r < selRow2; ++r) {
      setPos(r, 0);

      subDeleteChars(r, 0, getLineLength(r));
    }

    setPos(selRow2, 0);

    subDeleteChars(selRow2, 0, selCol2 + 1);

    endGroup();
  }
  else if (visualMode == App::VisualMode::LINE) {
    startGroup();

    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    for (int r = selRow1; r <= selRow2; ++r)
      subDeleteLine(r);

    endGroup();
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    startGroup();

    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    for (int r = selRow2; r >= selRow1; --r) {
      setPos(r, selCol1);

      subDeleteChars(r, selCol1, selCol2 - selCol1 + 1);
    }

    endGroup();
  }

  clearSelection();

  setVisualMode(VisualMode::NONE);
}

void
App::
yankSelection()
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  std::vector<std::string> lines;

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    int r = selRow1;

    std::string line;

    for (int c = selCol1; c <= getLineLength(r); ++c)
      line += getChar(r, c);

    lines.push_back(line);

    for (r = selRow1 + 1; r < selRow2; ++r) {
      line = "";

      for (int c = 0; c < getLineLength(r); ++c)
        line += getChar(r, c);

      lines.push_back(line);
    }

    line = "";

    for (int c = 0; c <= selCol2; ++c)
      line += getChar(r, c);

    lines.push_back(line);
  }
  else if (visualMode == App::VisualMode::LINE) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    for (int r = selRow1; r <= selRow2; ++r) {
      std::string line;

      for (int c = 0; c <= getLineLength(r); ++c)
        line += getChar(r, c);

      lines.push_back(line);
    }
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    for (int r = selRow1; r <= selRow2; ++r) {
      std::string line;

      for (int c = selCol1; c <= selCol2; ++c)
        line += getChar(r, c);

      lines.push_back(line);
    }
  }

  auto &buffer = getBuffer('\0');

  buffer.clear();

  for (auto &line : lines)
    buffer.addLine(line, /*new_line*/true);

  clearSelection();

  setVisualMode(VisualMode::NONE);
}

void
App::
lshiftSelection()
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  int line1 { 0 }, line2 { 0 };

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    line1 = selRow1;
    line2 = selRow2;
  }
  else if (visualMode == App::VisualMode::LINE) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    line1 = selRow1;
    line2 = selRow2;
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    line1 = selRow1;
    line2 = selRow2;
  }

  shiftLeft(line1, line2);

  clearSelection();

  setVisualMode(VisualMode::NONE);
}

void
App::
rshiftSelection()
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  int line1 { 0 }, line2 { 0 };

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    line1 = selRow1;
    line2 = selRow2;
  }
  else if (visualMode == App::VisualMode::LINE) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    line1 = selRow1;
    line2 = selRow2;
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    line1 = selRow1;
    line2 = selRow2;
  }

  shiftRight(line1, line2);

  clearSelection();

  setVisualMode(VisualMode::NONE);
}

void
App::
processSelection(SelectionOp op)
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  auto processChar = [&](int r, int c) {
    auto *line = getLine(r); assert(line);
    auto c1 = line->getChar(c);
    if      (op == SelectionOp::SWAP_CASE) {
      if      (std::islower(c1)) c1 = char(std::toupper(int(c1)));
      else if (std::isupper(c1)) c1 = char(std::tolower(int(c1)));
    }
    else if (op == SelectionOp::TO_UPPER) {
      if (std::islower(c1)) c1 = char(std::toupper(int(c1)));
    }
    else if (op == SelectionOp::TO_LOWER) {
      if (std::isupper(c1)) c1 = char(std::tolower(int(c1)));
    }
    line->setChar(c, c1);
  };

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    int r = selRow1;

    for (int c = selCol1; c <= getLineLength(r); ++c)
      processChar(r, c);

    for (r = selRow1 + 1; r < selRow2; ++r) {
      for (int c = 0; c < getLineLength(r); ++c)
        processChar(r, c);
    }

    for (int c = 0; c <= selCol2; ++c)
      processChar(r, c);
  }
  else if (visualMode == App::VisualMode::LINE) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    for (int r = selRow1; r <= selRow2; ++r)
      for (int c = 0; c <= getLineLength(r); ++c)
        processChar(r, c);
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    for (int r = selRow1; r <= selRow2; ++r)
      for (int c = selCol1; c <= selCol2; ++c)
        processChar(r, c);
  }
}

void
App::
setCursorList(bool left)
{
  auto visualMode = this->getVisualMode();

  int selRow1, selCol1, selRow2, selCol2;
  getSelectStart(&selRow1, &selCol1);
  getSelectEnd  (&selRow2, &selCol2);

  CursorPosList cursorPosList;

  if      (visualMode == App::VisualMode::CHAR) {
    if      (selRow1 > selRow2) {
      std::swap(selRow1, selRow2);
      std::swap(selCol1, selCol2);
    }
    else if (selRow1 == selRow2 && selCol1 > selCol2) {
      std::swap(selCol1, selCol2);
    }

    int r = selRow1;

    cursorPosList.push_back(CursorPos(selRow1, left ? selCol1 : selCol2 + 1));

    for (r = selRow1 + 1; r < selRow2; ++r)
      cursorPosList.push_back(CursorPos(r, left ? 0 : getLineLength(r)));

    cursorPosList.push_back(CursorPos(r, left ? 0 : getLineLength(r)));
  }
  else if (visualMode == App::VisualMode::LINE) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);

    for (int r = selRow1; r <= selRow2; ++r)
      cursorPosList.push_back(CursorPos(r, left ? 0 : getLineLength(r)));
  }
  else if (visualMode == App::VisualMode::BLOCK) {
    if (selRow1 > selRow2)
      std::swap(selRow1, selRow2);
    if (selCol1 > selCol2)
      std::swap(selCol1, selCol2);

    for (int r = selRow1; r <= selRow2; ++r)
      cursorPosList.push_back(CursorPos(r, left ? selCol1 : selCol2 + 1));
  }

  cursorPos_ = cursorPosList[0];

  extraCursorPosList_.clear();

  for (size_t i = 1; i < cursorPosList.size(); ++i)
    extraCursorPosList_.push_back(cursorPosList[i]);
}

int
App::
getLineLength(int r) const
{
  auto *line = getLine(r);
  if (! line) return 0;
  return int(line->getLength());
}

void
App::
setCmdLineMode(bool cmdLineMode, const std::string &str)
{
  cmdLineMode_ = cmdLineMode;

  cmdLine_->setVisible(cmdLineMode_);

  if (cmdLineMode_)
    cmdLine_->setLine(str);
  else
    cmdLine_->setLine("");

  cmdLine_->cursorEnd();

  if (! cmdLineMode_) {
    setVisualMode(VisualMode::NONE);

    clearSelection();
  }
}

bool
App::
getCaseSensitive() const
{
  return ed_->getCaseSensitive();
}

void
App::
setCaseSensitive(bool value)
{
  ed_->setCaseSensitive(value);
}

std::string
App::
getCmdLineString() const
{
  return cmdLine_->getLine();
}

//---

void
App::
error(const std::string &msg) const
{
  std::cerr << "Error: " << msg << "\n";
}

//---

void
App::
getPos(uint *x, uint *y) const
{
  *y = cursorPos_.row;
  *x = cursorPos_.col;
}

void
App::
setPos(uint x, uint y)
{
  if (y != cursorPos_.row || x != cursorPos_.col) {
    cursorPos_.row = y;
    cursorPos_.col = x;

    iface_->positionChanged();
  }
}

uint
App::
getRow() const
{
  uint x, y;
  getPos(&x, &y);

  return y;
}

uint
App::
getCol() const
{
  uint x, y;
  getPos(&x, &y);

  return x;
}

void
App::
fixPos()
{
  bool changed = false;

  uint x, y;
  getPos(&x, &y);

  if (y >= getNumLines()) {
    int y1 = getNumLines() - 1;

    y = uint(y1 >= 0 ? y1 : 0);

    changed = true;
  }

  if (x > getLineEnd(y)) {
    x = getLineEnd(y);

    changed = true;
  }

  if (changed)
    setPos(x, y);
}

uint
App::
getNumLines() const
{
  return uint(lines_.size());
}

bool
App::
isLinesEmpty() const
{
  return lines_.empty();
}

void
App::
setExtraLineChar(bool extraLineChar)
{
  extraLineChar_ = extraLineChar;
}

void
App::
setChanged(bool changed)
{
  changed_ = changed;
}

void
App::
setUnsaved(bool unsaved)
{
  unsaved_ = unsaved;
}

//---

char
App::
getChar() const
{
  uint x, y;
  getPos(&x, &y);

  return getChar(y, x);
}

char
App::
getChar(uint line_num, uint char_num) const
{
  if (line_num >= getNumLines())
    return '\0';

  return getLine(line_num)->getChar(char_num);
}

//---

void
App::
addLine(const std::string &str)
{
  addLine(getRow(), str);
}

void
App::
addLine(uint line_num, const std::string &str)
{
  CASSERT(line_num <= getNumLines(), "Invalid Line Num");

  auto *line = new Line;

  line->addChars(0, str);

  subAddLine(line_num, line);
}

void
App::
subAddLine(uint line_num, Line *line)
{
  lines_.addLine(line_num, line);

  addUndo(new DeleteLineUndoCmd(this, line_num));

  setChanged(true);
  setUnsaved(true);
}

void
App::
addChars(uint line_num, uint char_num, const std::string &chars)
{
  CASSERT(line_num <= getNumLines(), "Invalid Line Num");

  subAddChars(line_num, char_num, chars);
}

void
App::
subAddChars(uint line_num, uint char_num, const std::string &chars)
{
  lines_.addLineChars(line_num, char_num, chars);

  addUndo(new DeleteCharsUndoCmd(this, line_num, char_num, int(chars.size())));

  setChanged(true);
  setUnsaved(true);
}

//---

void
App::
moveLine(uint line_num1, int line_num2)
{
  CASSERT(line_num1 < getNumLines(), "Invalid Line Num");

  CASSERT(line_num2 >= -1 && line_num2 < int(getNumLines()), "Invalid Line Num");

  subMoveLine(line_num1, line_num2);
}

void
App::
subMoveLine(uint line_num1, int line_num2)
{
  lines_.moveLine(line_num1, line_num2);

  addUndo(new MoveLineUndoCmd(this, line_num2, line_num1 - 1));

  setChanged(true);
  setUnsaved(true);
}

void
App::
copyLine(uint line_num1, uint line_num2)
{
  CASSERT(line_num1 < getNumLines(), "Invalid Line Num");

  auto *line = getLine(line_num1)->dup();

  subAddLine(line_num2, line);
}

//---

void
App::
deleteAllLines()
{
  subDeleteAllLines();

  addLine("");
}

void
App::
subDeleteAllLines()
{
  uint numLines = getNumLines();

  for (int i = numLines - 1; i >= 0; --i)
    subDeleteLine(i);
}

void
App::
deleteLine()
{
  deleteLine(getRow());
}

void
App::
deleteLine(uint line_num)
{
  if (! CASSERT(line_num < getNumLines(), "Invalid Line Num")) return;

  yankLines('\0', line_num, 1);

  subDeleteLine(line_num);

  if (isLinesEmpty())
    addLine("");
}

void
App::
subDeleteLine(uint line_num)
{
  auto str = getLine(line_num)->getString();

  lines_.deleteLine(line_num);

  addUndo(new AddLineUndoCmd(this, line_num, str));

  setChanged(true);
  setUnsaved(true);
}

void
App::
deleteWord()
{
  deleteWord(getRow(), getCol());
}

void
App::
deleteWord(uint line_num, uint char_num)
{
  auto *line = getLine(line_num);

  if (line->isEmpty() || char_num >= line->getLength() - 1)
    return;

  uint num = 0;

  if (isWordChar(line->getChar(char_num))) {
    while (int(char_num + num) < int(line->getLength()) - 1 &&
           isWordChar(line->getChar(char_num + num)))
      ++num;
  }
  else {
    while (int(char_num + num) < int(line->getLength()) - 1 &&
           ! isWordChar(line->getChar(char_num + num)))
      ++num;
  }

  if (num > 0)
    deleteChars(line_num, char_num, num);
}

void
App::
deleteEOL()
{
  deleteEOL(getRow(), getCol());
}

void
App::
deleteEOL(uint line_num, uint char_num)
{
  auto *line = getLine(line_num);

  uint num = std::max(int(line->getLength()) - int(char_num), 0);

  if (num > 0)
    deleteChars(line_num, char_num, num);
}

void
App::
deleteTo(uint line_num, uint char_num)
{
  deleteTo(getRow(), getCol(), line_num, char_num);
}

void
App::
deleteTo(uint line_num1, uint char_num1, uint line_num2, uint char_num2)
{
  startGroup();

  if      (line_num1 < line_num2) {
    auto *line = getLine(line_num1);

    int num = line->getLength() - char_num1;

    deleteChars(line_num1, char_num1, num);

    for (uint i = 0; i < line_num2 - line_num1 - 1; ++i)
      deleteLine(line_num1 + i + 1);

    cursorDown(1);

    line_num2 = line_num1 + 1;

    deleteChars(line_num2, 0, char_num2);
  }
  else if (line_num2 < line_num1) {
    deleteChars(line_num1, 0, char_num1);

    for (uint i = 0; i < line_num1 - line_num2 - 1; ++i)
      deleteLine(line_num2 + i + 1);

    cursorUp(1);

    line_num2 = line_num1 - 1;

    auto *line = getLine(line_num2);

    int num = line->getLength() - char_num2;

    deleteChars(line_num2, char_num2, num);
  }
  else {
    if (char_num1 < char_num2) {
      int num = char_num2 - char_num1 + 1;

      deleteChars(line_num1, char_num1, num);
    }
    else {
      int num = char_num1 - char_num2 + 1;

      deleteChars(line_num1, char_num2, num);
    }
  }

  endGroup();
}

void
App::
deleteChar()
{
  deleteChar(getRow());
}

void
App::
deleteChar(uint line_num)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  auto *line = getLine(line_num);

  if (line->isEmpty())
    return;

  deleteChars(line_num, getCol(), 1);
}

void
App::
deleteChars(uint n)
{
  deleteChars(getRow(), getCol(), n);
}

void
App::
deleteChars(uint line_num, uint char_num, uint n)
{
  startGroup();

  subDeleteChars(line_num, char_num, n);

  endGroup();
}

void
App::
subDeleteChars(uint line_num, uint char_num, uint n)
{
  const auto *line = getLine(line_num);

  CASSERT(char_num + n <= line->getLength(), "Invalid Number of Chars");

  for (uint i = 0; i < n; ++i) {
    char c = line->getChar(char_num + i);

    addUndo(new InsertCharUndoCmd(this, line_num, char_num, c));
  }

  lines_.deleteLineChars(line_num, char_num, n);

  //fixPos();

  setChanged(true);
  setUnsaved(true);
}

//---

void
App::
shiftLeft(uint line_num1, uint line_num2)
{
  if (line_num1 > line_num2)
    std::swap(line_num1, line_num2);

  uint n = getOptions().getShiftWidth();

  startGroup();

  for (uint line_num = line_num1; line_num <= line_num2; ++line_num) {
    auto *line = getLine(line_num);

    uint len = line->getLength();

    uint n1 = 0;

    for (uint j = 0; j < n && j < len; ++j) {
      char c = line->getChar(j);

      if (! isspace(c)) break;

      ++n1;
    }

    deleteChars(line_num, 0, n1);
  }

  endGroup();
}

void
App::
shiftRight(uint line_num1, uint line_num2)
{
  if (line_num1 > line_num2)
    std::swap(line_num1, line_num2);

  std::string chars;

  uint n = getOptions().getShiftWidth();

  for (uint j = 0; j < n; ++j)
    chars += " ";

  startGroup();

  for (uint line_num = line_num1; line_num <= line_num2; ++line_num) {
    auto *line = getLine(line_num);

    if (line->isEmpty()) continue;

    addChars(line_num, 0, chars);
  }

  endGroup();
}

//---

void
App::
yankLines(char c, uint n)
{
  yankLines(c, getRow(), n);
}

void
App::
yankLines(char id, uint line_num, uint n)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  yankClear(id);

  for (uint i = 0; i < n; ++i) {
    uint line_num1 = line_num + i;

    auto *line = getLine(line_num1);

    uint len = line->getLength();

    subYankTo(id, line_num1, 0, line_num1, std::max(int(len) - 1, 0), true);
  }
}

void
App::
yankWords(char id, uint n)
{
  yankWords(id, getRow(), getCol(), n);
}

void
App::
yankWords(char id, uint line_num, uint char_num, uint n)
{
  yankClear(id);

  uint line_num1 = line_num;
  uint char_num1 = char_num;

  for (uint i = 0; i < n; ++i)
    endWord(&line_num1, &char_num1);

  subYankTo(id, line_num, char_num, line_num1, char_num1, false);
}

void
App::
yankChar(char id)
{
  yankChar(id, getRow(), getCol());
}

void
App::
yankChar(char id, uint line_num, uint char_num)
{
  yankClear(id);

  subYankTo(id, line_num, char_num, line_num, char_num, false);
}

void
App::
yankTo(char id, uint line_num, uint char_num, bool is_line)
{
  yankClear(id);

  subYankTo(id, getRow(), getCol(), line_num, char_num, is_line);
}

void
App::
yankTo(char id, uint line_num1, uint char_num1, uint line_num2, uint char_num2, bool is_line)
{
  yankClear(id);

  subYankTo(id, line_num1, char_num1, line_num2, char_num2, is_line);
}

void
App::
yankClear(char id)
{
  if (! inGroup()) {
    auto &buffer = getBuffer(id);

    buffer.clear();
  }
}

void
App::
subYankTo(char id, uint line_num1, uint char_num1, uint line_num2, uint char_num2, bool is_line)
{
  CASSERT(line_num1 < getNumLines() && line_num2 < getNumLines(), "Invalid Line Num");

  std::vector<BufferLine> bufferLines;

  if      (line_num1 < line_num2) {
    const auto *line1 = getLine(line_num1);

    const auto &str1 = line1->getString();

    bufferLines.push_back(BufferLine(str1.substr(char_num1), is_line));

    for (uint i = line_num1 + 1; i < line_num2; ++i) {
      const auto *line = getLine(i);

      bufferLines.push_back(BufferLine(line->getString(), true));
    }

    const auto *line2 = getLine(line_num2);

    const auto &str2 = line2->getString();

    bufferLines.push_back(BufferLine(str2.substr(0, char_num2), is_line));
  }
  else if (line_num2 < line_num1) {
    const auto *line2 = getLine(line_num2);

    const auto &str2 = line2->getString();

    bufferLines.push_back(BufferLine(str2.substr(char_num2), is_line));

    for (uint i = line_num2 + 1; i < line_num1; ++i) {
      const auto *line = getLine(i);

      bufferLines.push_back(BufferLine(line->getString(), true));
    }

    const auto *line1 = getLine(line_num1);

    const std::string &str1 = line1->getString();

    bufferLines.push_back(BufferLine(str1.substr(0, char_num1), is_line));
  }
   else {
    const auto *line1 = getLine(line_num1);

    const auto &str1 = line1->getString();

    std::string str2;

    if (char_num1 < char_num2)
      str2 = str1.substr(char_num1, char_num2 - char_num1 + 1);
    else
      str2 = str1.substr(char_num2, char_num1 - char_num2 + 1);

    bufferLines.push_back(BufferLine(str2, is_line));
  }

  if (inGroup()) {
    auto *group = groupList_.back();

    for (const auto &bufferLine : bufferLines)
      group->addLine(bufferLine.getLine(), bufferLine.hasNewLine());
  }
  else {
    auto &buffer = getBuffer(id);

    for (const auto &bufferLine : bufferLines)
      buffer.addLine(bufferLine.getLine(), bufferLine.hasNewLine());
  }
}

void
App::
pasteAfter(char id)
{
  pasteAfter(id, getRow(), getCol());
}

void
App::
pasteBefore(char id)
{
  pasteBefore(id, getRow(), getCol());
}

void
App::
pasteAfter(char id, uint line_num, uint char_num)
{
  auto &buffer = getBuffer(id);

  uint num_lines = buffer.getNumLines();

  if (num_lines == 0)
    return;

  auto *sline = buffer.getLine(0);
  auto *eline = static_cast<BufferLine *>(nullptr);

  if (num_lines > 1)
    eline = buffer.getLine(num_lines - 1);

  startGroup();

  if (! sline->hasNewLine()) {
    if (eline)
      splitLine(line_num, char_num);

    auto *line = getLine(line_num);

    if (char_num < line->getLength())
      addChars(line_num, char_num + 1, sline->getLine());
    else
      addChars(line_num, char_num, sline->getLine());
  }
  else {
    ++line_num;

    addLine(line_num, sline->getLine());

    cursorDown  (1);
    cursorToLeft();

    ++line_num;
  }

  for (uint i = 1; i < num_lines - 1; ++i) {
    auto *mline = buffer.getLine(i);

    addLine(line_num, mline->getLine());

    cursorDown  (1);
    cursorToLeft();

    ++line_num;
  }

  if (eline) {
    if (! eline->hasNewLine()) {
      cursorDown  (1);
      cursorToLeft();

      ++line_num;

      addChars(line_num, 0, eline->getLine());
    }
    else {
      addLine(line_num, eline->getLine());

      cursorDown  (1);
      cursorToLeft();
    }
  }

  endGroup();
}

void
App::
pasteBefore(char id, uint line_num, uint char_num)
{
  auto &buffer = getBuffer(id);

  uint num_lines = buffer.getNumLines();

  if (num_lines == 0)
    return;

  auto *sline = buffer.getLine(0);
  auto *eline = static_cast<BufferLine *>(nullptr);

  if (num_lines > 1)
    eline = buffer.getLine(num_lines - 1);

  startGroup();

  if (! sline->hasNewLine()) {
    if (eline)
      splitLine(line_num, char_num);

    addChars(line_num, char_num, sline->getLine());
  }
  else
    addLine(line_num, sline->getLine());

  for (uint i = 1; i < num_lines - 1; ++i) {
    auto *mline = buffer.getLine(i);

    addLine(line_num + i, mline->getLine());
  }

  if (eline) {
    if (! eline->hasNewLine())
      addChars(line_num + num_lines - 1, 0, eline->getLine());
    else
      addLine(line_num + num_lines - 1, eline->getLine());
  }

  endGroup();
}

//---

void
App::
insertChar(char c)
{
  uint x, y;
  getPos(&x, &y);

  insertChar(y, x, c);

  cursorRight(1);
}

void
App::
insertChar(uint line_num, uint char_num, char c)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  subInsertChar(line_num, char_num, c);

  endGroup();
}

void
App::
subInsertChar(uint line_num, uint char_num, char c)
{
  if (isLinesEmpty()) {
    auto *line = new Line;

    subAddLine(0, line);

    line_num = 0;
  }

  lines_.addLineChar(line_num, char_num, c);

  addUndo(new DeleteCharsUndoCmd(this, line_num, char_num, 1));

  setChanged(true);
  setUnsaved(true);
}

//---

void
App::
replaceChar(char c)
{
  uint x, y;
  getPos(&x, &y);

  replaceChar(y, x, c);

  cursorRight(1);
}

void
App::
replaceChar(uint line_num, uint char_num, char c)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  subReplaceChar(line_num, char_num, c);

  endGroup();
}

void
App::
subReplaceChar(uint line_num, uint char_num, char c)
{
  if (isLinesEmpty()) {
    auto *line = new Line;

    subAddLine(0, line);
  }

  const auto *line = getLine(line_num);

  char c1 = line->getChar(char_num);

  lines_.replaceLineChar(line_num, char_num, c);

  addUndo(new ReplaceCharUndoCmd(this, line_num, char_num, c1));

  setChanged(true);
  setUnsaved(true);
}

//---

void
App::
splitLine()
{
  splitLine(getRow(), getCol());
}

void
App::
splitLine(uint line_num, uint char_num)
{
  startGroup();

  subSplitLine(line_num, char_num);

  endGroup();
}

void
App::
subSplitLine(uint line_num, uint char_num)
{
  auto *line = new Line;

  lines_.addLine(line_num + 1, line);

  lines_.splitLine(line_num, char_num);

  addUndo(new JoinLineUndoCmd(this, line_num));

  setChanged(true);
  setUnsaved(true);

  cursorDown  (1);
  cursorToLeft();
}

//---

void
App::
joinLine()
{
  joinLine(getRow());
}

void
App::
joinLine(uint line_num)
{
  CASSERT(line_num < getNumLines() - 1, "Invalid Line Num");

  startGroup();

  subJoinLine(line_num);

  endGroup();
}

void
App::
subJoinLine(uint line_num)
{
  const auto *line1 = getLine(line_num);

  uint len1 = line1->getLength();

  lines_.joinLine(line_num);

  addUndo(new SplitLineUndoCmd(this, line_num, len1));

  subDeleteLine(line_num + 1);
}

//---

void
App::
newLineBelow()
{
  uint line_num = getRow();

  addLine(line_num + 1, "");

  cursorDown(1);

  cursorToLeft();
}

void
App::
newLineAbove()
{
  addLine(getRow(), "");

  cursorToLeft();
}

//---

bool
App::
cursorLeft(uint n)
{
  uint x, y;
  getPos(&x, &y);

  bool rc = cursorLeft(n, &y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();

  return rc;
}

bool
App::
cursorLeft(uint n, uint *, uint *char_num)
{
  for (uint i = 0; i < n; ++i) {
    if (*char_num <= 0)
      return false;

    --(*char_num);
  }

  return true;
}

bool
App::
cursorRight(uint n)
{
  uint x, y;
  getPos(&x, &y);

  bool rc = cursorRight(n, &y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();

  return rc;
}

bool
App::
cursorRight(uint n, uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint line_end = line->getEnd(isExtraLineChar());

  for (uint i = 0; i < n; ++i) {
    if (*char_num >= line_end)
      return false;

    ++(*char_num);
  }

  return true;
}

bool
App::
cursorUp(uint n)
{
  uint x, y;
  getPos(&x, &y);

  bool rc = cursorUp(n, &y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();

  return rc;
}

bool
App::
cursorUp(uint n, uint *line_num, uint *char_num)
{
  bool rc = true;

  for (uint i = 0; i < n; ++i) {
    if (*line_num <= 0) {
      rc = false;
      break;
    }

    --(*line_num);
  }

  auto *line = getLine(*line_num);

  uint line_end = line->getEnd(isExtraLineChar());

  if (*char_num >= line_end)
    *char_num = line_end;

  return rc;
}

bool
App::
cursorDown(uint n)
{
  uint x, y;
  getPos(&x, &y);

  bool rc = cursorDown(n, &y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();

  return rc;
}

bool
App::
cursorDown(uint n, uint *line_num, uint *char_num)
{
  if (n > 0 && isLinesEmpty())
    return false;

  bool rc = true;

  for (uint i = 0; i < n; ++i) {
    if (*line_num >= getNumLines() - 1) {
      rc = false;
      break;
    }

    ++(*line_num);
  }

  auto *line = getLine(*line_num);

  uint line_end = line->getEnd(isExtraLineChar());

  if (*char_num >= line_end)
    *char_num = line_end;

  return rc;
}

void
App::
cursorToLeft()
{
  uint x, y;
  getPos(&x, &y);

  cursorToLeft(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

void
App::
cursorToLeft(uint *, uint *char_num)
{
  *char_num = 0;
}

void
App::
cursorToRight()
{
  uint x, y;
  getPos(&x, &y);

  cursorToRight(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

void
App::
cursorToRight(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint line_end = line->getEnd(isExtraLineChar());

  *char_num = line_end;
}

void
App::
cursorSkipSpace()
{
  uint x, y;
  getPos(&x, &y);

  cursorSkipSpace(&y, &x);

  setPos(x, y);
}

void
App::
cursorSkipSpace(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint len = line->getLength();

  while (*char_num < len - 1 && isspace(line->getChar(*char_num)))
    ++(*char_num);
}

void
App::
cursorFirstNonBlankUp()
{
  uint x, y;
  getPos(&x, &y);

  cursorFirstNonBlankUp(&y, &x);

  setPos(x, y);
}

void
App::
cursorFirstNonBlankUp(uint *line_num, uint *char_num)
{
  cursorUp(1, line_num, char_num);

  cursorFirstNonBlank(line_num, char_num);
}

void
App::
cursorFirstNonBlankDown()
{
  uint x, y;
  getPos(&x, &y);

  cursorFirstNonBlankDown(&y, &x);

  setPos(x, y);
}

void
App::
cursorFirstNonBlankDown(uint *line_num, uint *char_num)
{
  cursorDown(1, line_num, char_num);

  cursorFirstNonBlank(line_num, char_num);
}

void
App::
cursorFirstNonBlank()
{
  uint x, y;
  getPos(&x, &y);

  cursorFirstNonBlank(&y, &x);

  setPos(x, y);
}

void
App::
cursorFirstNonBlank(uint *line_num, uint *char_num)
{
  cursorToLeft(line_num, char_num);

  cursorSkipSpace(line_num, char_num);
}

void
App::
cursorTo(uint line_num, uint char_num)
{
  setPos(char_num, line_num);
}

//---

void
App::
nextWord()
{
  uint x, y;
  getPos(&x, &y);

  nextWord(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

// a word a series of alphanumeric or _ characters OR
// a series of non-blank characters
void
App::
nextWord(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // skip current word
  bool found = false;

  if (*char_num < line->getLength()) {
    if      (isWordChar(line->getChar(*char_num))) {
      while (*char_num < line->getLength() &&
             isWordChar(line->getChar(*char_num)))
        ++(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num < line->getLength() &&
             ! isWordChar(line->getChar(*char_num)) &&
             ! isspace(line->getChar(*char_num)))
        ++(*char_num);
    }

    // skip space
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    if (*char_num < line->getLength() &&
        ! isspace(line->getChar(*char_num)))
      found = true;
  }

  // if no next word found then skip to next line
  while (! found) {
    if (*line_num >= getNumLines() - 1)
      break;

    ++(*line_num);

    line = getLine(*line_num);

    *char_num = 0;

    // empty line is a word
    if (line->isEmpty()) {
      found = true;
      break;
    }

    // skip space
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    // if not at end of line we have found a word
    if (*char_num < line->getLength()) {
      found = true;
      break;
    }
  }
}

void
App::
nextWORD()
{
  uint x, y;
  getPos(&x, &y);

  nextWORD(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

// a WORD is a series of non-blank characters
void
App::
nextWORD(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // skip current word
  bool found = false;

  if (*char_num < line->getLength()) {
    while (*char_num < line->getLength() &&
           ! isspace(line->getChar(*char_num)))
      ++(*char_num);

    // skip space
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    if (*char_num < line->getLength() &&
        ! isspace(line->getChar(*char_num)))
      found = true;
  }

  // if no next word found then skip to next line
  while (! found) {
    if (*line_num >= getNumLines() - 1)
      break;

    ++(*line_num);

    line = getLine(*line_num);

    *char_num = 0;

    // empty line is a word
    if (line->isEmpty()) {
      found = true;
      break;
    }

    // skip space
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    // if not at end of line we have found a word
    if (*char_num < line->getLength()) {
      found = true;
      break;
    }
  }
}

void
App::
prevWord()
{
  uint x, y;
  getPos(&x, &y);

  prevWord(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

void
App::
prevWord(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // skip previous character
  if (*char_num > 0)
    --(*char_num);

  // skip spaces
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  // if at start of line and more lines go back a line
  while (*char_num == 0 && *line_num > 0) {
    --(*line_num);

    line = getLine(*line_num);

    // blank line is a word so we're done
    if (line->isEmpty())
      return;

    *char_num = line->getLength() - 1;

    // skip spaces
    while (*char_num > 0 && isspace(line->getChar(*char_num)))
      --(*char_num);

    if (! isspace(line->getChar(*char_num)))
      break;
  }

  // skip to start of word
  if (*char_num > 0) {
    if      (isWordChar(line->getChar(*char_num))) {
      while (*char_num > 0 && isWordChar(line->getChar(*char_num - 1)))
        --(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num > 0 && ! isWordChar(line->getChar(*char_num - 1)) &&
             ! isspace(line->getChar(*char_num - 1)))
        --(*char_num);
    }
  }
}

void
App::
prevWORD()
{
  uint x, y;
  getPos(&x, &y);

  prevWORD(&y, &x);

  setPos(x, y);

  if (getVisualMode() != VisualMode::NONE)
    updateSelectRange();
}

void
App::
prevWORD(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // skip previous character
  if (*char_num > 0)
    --(*char_num);

  // skip spaces
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  // if at start of line and more lines go back a line
  while (*char_num == 0 && *line_num > 0) {
    --(*line_num);

    line = getLine(*line_num);

    // blank line is a word so we're done
    if (line->isEmpty())
      return;

    *char_num = line->getLength() - 1;

    // skip spaces
    while (*char_num > 0 && isspace(line->getChar(*char_num)))
      --(*char_num);

    if (! isspace(line->getChar(*char_num)))
      break;
  }

  // skip to start of WORD
  if (*char_num > 0) {
    while (*char_num > 0 && ! isspace(line->getChar(*char_num - 1)))
      --(*char_num);
  }
}

void
App::
endWord()
{
  uint x, y;
  getPos(&x, &y);

  endWord(&y, &x);

  setPos(x, y);
}

void
App::
endWord(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // if already at word end, increment to next char
  if      (*char_num < line->getLength() - 1) {
    if      (  isWordChar(line->getChar(*char_num)) &&
             ! isWordChar(line->getChar(*char_num + 1))) {
      ++(*char_num);

      // start of new word so we're done
      if (! isspace(line->getChar(*char_num)))
        return;
    }
    else if (! isspace(line->getChar(*char_num)) &&
             (isspace   (line->getChar(*char_num + 1)) ||
              isWordChar(line->getChar(*char_num + 1)))) {
      ++(*char_num);
    }
  }
  else if (*char_num == line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num))) {
      // new line
      if (*line_num < getNumLines() - 1) {
        ++(*line_num);

        line = getLine(*line_num);

        *char_num = 0;
      }
    }
  }
  else {
    // new line
    if (*line_num < getNumLines() - 1) {
      ++(*line_num);

      line = getLine(*line_num);

      *char_num = 0;
    }
  }

  // skip to next line with non-space
  if ((*char_num < line->getLength() &&
       isspace(line->getChar(*char_num))) || line->isEmpty()) {
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    // if end of line goto next line and skip non-word again
    while (*char_num >= line->getLength()) {
      // new line
      if (*line_num < getNumLines() - 1) {
        ++(*line_num);

        line = getLine(*line_num);

        *char_num = 0;
      }
      else
        break;

      // skip spaces
      while (*char_num < line->getLength() &&
             isspace(line->getChar(*char_num)))
        ++(*char_num);
    }
  }

  // skip to end of word
  if (*char_num < line->getLength()) {
    if      (isWordChar(line->getChar(*char_num))) {
      while (*char_num < line->getLength() - 1 &&
             isWordChar(line->getChar(*char_num + 1)))
        ++(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num < line->getLength() - 1 &&
             ! isspace   (line->getChar(*char_num + 1)) &&
             ! isWordChar(line->getChar(*char_num + 1)))
        ++(*char_num);
    }
  }
}

void
App::
endWORD()
{
  uint x, y;
  getPos(&x, &y);

  endWORD(&y, &x);

  setPos(x, y);
}

void
App::
endWORD(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  // if already at word end, increment to next char
  if      (*char_num < line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num)) &&
          isspace(line->getChar(*char_num + 1)))
      ++(*char_num);
  }
  else if (*char_num == line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num))) {
      // new line
      if (*line_num < getNumLines() - 1) {
        ++(*line_num);

        line = getLine(*line_num);

        *char_num = 0;
      }
    }
  }
  else {
    // new line
    if (*line_num < getNumLines() - 1) {
      ++(*line_num);

      line = getLine(*line_num);

      *char_num = 0;
    }
  }

  // skip to next line with non-space
  if ((*char_num < line->getLength() &&
       isspace(line->getChar(*char_num))) || line->isEmpty()) {
    while (*char_num < line->getLength() &&
           isspace(line->getChar(*char_num)))
      ++(*char_num);

    // if end of line goto next line and skip non-word again
    while (*char_num >= line->getLength()) {
      // new line
      if (*line_num < getNumLines() - 1) {
        ++(*line_num);

        line = getLine(*line_num);

        *char_num = 0;
      }

      // skip spaces
      while (*char_num < line->getLength() &&
             isspace(line->getChar(*char_num)))
        ++(*char_num);
    }
  }

  // skip to end of word
  if (*char_num < line->getLength()) {
    while (*char_num < line->getLength() - 1 &&
           ! isspace(line->getChar(*char_num + 1)))
      ++(*char_num);
  }
}

bool
App::
getWord(std::string &word)
{
  uint x, y;
  getPos(&x, &y);

  return getWord(y, x, word);
}

bool
App::
getWord(uint line_num, uint char_num, std::string &word)
{
  auto *line = getLine(line_num);

  if (! isWordChar(line->getChar(char_num)))
    return false;

  int char_num1 = char_num;

  // find start of word
  while (char_num1 > 0 &&
         isWordChar(line->getChar(char_num1)) &&
         isWordChar(line->getChar(char_num1 - 1)))
    --char_num1;

  int char_num2 = char_num1;

  // find end of word
  while (char_num2 < int(line->getLength()) - 1 &&
         isWordChar(line->getChar(char_num2)) &&
         isWordChar(line->getChar(char_num2 + 1)))
    ++char_num2;

  for (int i = char_num1; i <= char_num2; ++i)
    word += line->getChar(i);

  return true;
}

void
App::
nextSentence()
{
  uint x, y;
  getPos(&x, &y);

  nextSentence(&y, &x);

  setPos(x, y);
}

// a sentence is the first non-blank after a blank line
// of the first non-blank after a '. ' '? ' '! '
void
App::
nextSentence(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint num = 0;

  // empty line - go to next line
  if      (line->isEmpty()) {
    if (! nextLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }
  // blank line - go to first non-blank
  else if (isBlank(line)) {
    ;
  }
  // sentence end - skip to sentence start
  else if (isSentenceEnd(line, *char_num, &num)) {
    *char_num += num;

    if (*char_num >= line->getLength()) {
      if (! nextLine(line_num, char_num))
        return;

      line = getLine(*line_num);
    }
  }
  // in sentence - skip sentence and skip to next sentence start
  else {
    while (true) {
      while (*char_num < line->getLength() &&
             ! isSentenceEnd(line, *char_num, &num))
        ++(*char_num);

      if (*char_num < line->getLength() &&
          isSentenceEnd(line, *char_num, &num)) {
        *char_num += num;

        if (*char_num >= line->getLength()) {
          if (! nextLine(line_num, char_num))
            return;

          line = getLine(*line_num);
        }

        break;
      }

      // next line
      if (*char_num >= line->getLength()) {
        if (! nextLine(line_num, char_num))
          return;

        line = getLine(*line_num);

        if (isBlank(line))
          break;
      }
    }
  }

  while (! line->isEmpty() && isBlank(line)) {
    if (! nextLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }

  while (*char_num < line->getLength() &&
         isspace(line->getChar(*char_num)))
    ++(*char_num);
}

void
App::
prevSentence()
{
  uint x, y;
  getPos(&x, &y);

  prevSentence(&y, &x);

  setPos(x, y);
}

void
App::
prevSentence(uint *line_num, uint *char_num)
{
  uint num = 0;

  auto *line = getLine(*line_num);

  bool sentence = false;

  // blank line is a sentence
  if (line->isEmpty()) {
    // skip empty lines
    while (line->isEmpty()) {
      // go to previous line
      if (! prevLine(line_num, char_num))
        return;

      line = getLine(*line_num);
    }

    sentence = true;
  }

  // skip blank lines
  while (! line->isEmpty() && isBlank(line)) {
    // go to previous line
    if (! prevLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }

  // empty line is a sentence so we are done
  if (line->isEmpty())
    return;

  // go to last non-blank on non-blank line
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  if (*char_num > 0 && isSentenceEnd(line, *char_num, &num)) {
    sentence = true;

    --(*char_num);
  }

  while (true) {
    // find previous sentence end or start of line
    while (*char_num > 0 && ! isSentenceEnd(line, *char_num, &num))
      --(*char_num);

    // if find previous sentence end then done
    if (*char_num > 0 && isSentenceEnd(line, *char_num, &num)) {
      if (sentence) {
        nextSentence(line_num, char_num);
        return;
      }

      sentence = true;

      --(*char_num);

      continue;
    }

    // go to previous line
    if (! prevLine(line_num, char_num))
      return;

    line = getLine(*line_num);

    if (line->isEmpty()) {
      if (sentence)
        nextSentence(line_num, char_num);

      return;
    }
  }
}

void
App::
nextParagraph()
{
  uint x, y;
  getPos(&x, &y);

  nextParagraph(&y, &x);

  setPos(x, y);
}

void
App::
nextParagraph(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  while (line->isEmpty()) {
    if (! nextLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }

  while (! line->isEmpty()) {
    if (! nextLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }
}

void
App::
prevParagraph()
{
  uint x, y;
  getPos(&x, &y);

  prevParagraph(&y, &x);

  setPos(x, y);
}

void
App::
prevParagraph(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  while (line->isEmpty()) {
    if (! prevLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }

  while (! line->isEmpty()) {
    if (! prevLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }
}

void
App::
nextSection()
{
  uint x, y;
  getPos(&x, &y);

  nextSection(&y, &x);

  setPos(x, y);
}

void
App::
nextSection(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint pos;

  if (! nextLine(line_num, char_num))
    return;

  line = getLine(*line_num);

  while (! isSection(line, *char_num, &pos)) {
    if (! nextLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }
}

void
App::
prevSection()
{
  uint x, y;
  getPos(&x, &y);

  prevSection(&y, &x);

  setPos(x, y);
}

void
App::
prevSection(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  uint pos;

  if (! prevLine(line_num, char_num))
    return;

  line = getLine(*line_num);

  while (! isSection(line, *char_num, &pos)) {
    if (! prevLine(line_num, char_num))
      return;

    line = getLine(*line_num);
  }
}

bool
App::
nextLine(uint *line_num, uint *char_num)
{
  auto *line = getLine(*line_num);

  *char_num = line->getLength() - 1;

  if (*line_num >= getNumLines() - 1)
    return false;

  ++(*line_num);

  *char_num = 0;

  return true;
}

bool
App::
prevLine(uint *line_num, uint *char_num)
{
  *char_num = 0;

  if (*line_num <= 0)
    return false;

  --(*line_num);

  auto *line = getLine(*line_num);

  *char_num = std::max(int(line->getLength()) - 1, 0);

  return true;
}

void
App::
swapChar()
{
  swapChar(getRow(), getCol());
}

void
App::
swapChar(uint line_num, uint char_num)
{
  auto *line = getLine(line_num);

  char c = line->getChar(char_num);

  if      (islower(c)) c = char(toupper(c));
  else if (isupper(c)) c = char(tolower(c));

  replace(line_num, char_num, c);
}

bool
App::
findNext(const std::string &pattern)
{
  return findNext(pattern, getRow(), getCol() + 1);
}

bool
App::
findNext(const std::string &pattern, uint *fline_num, uint *fchar_num)
{
  return findNext(pattern, getRow(), getCol() + 1, fline_num, fchar_num);
}

bool
App::
findNext(const std::string &pattern, uint line_num, int char_num)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1);
}

bool
App::
findNext(const std::string &pattern, uint line_num, int char_num, uint *fline_num, uint *fchar_num)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1, fline_num, fchar_num);
}

bool
App::
findNext(const std::string &pattern, uint line_num1, int char_num1, int line_num2, int char_num2)
{
  uint fline_num, fchar_num;

  if (! findNext(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
App::
findNext(const std::string &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num)
{
  setFindPattern(CRegExp(pattern));

  if (findNext(getLine(line_num1), pattern, char_num1, -1, fchar_num)) {
    *fline_num = line_num1;
    return true;
  }

  for (int i = line_num1 + 1; i <= line_num2 - 1; ++i) {
    if (findNext(getLine(i), pattern, 0, -1, fchar_num)) {
      *fline_num = i;
      return true;
    }
  }

  if (findNext(getLine(line_num2), pattern, 0, char_num2, fchar_num)) {
    *fline_num = line_num2;
    return true;
  }

  return false;
}

bool
App::
findNext(const CRegExp &pattern, uint *len)
{
  return findNext(pattern, getRow(), getCol() + 1, len);
}

bool
App::
findNext(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len)
{
  return findNext(pattern, getRow(), getCol() + 1, fline_num, fchar_num, len);
}

bool
App::
findNext(const CRegExp &pattern, uint line_num, int char_num, uint *len)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1, len);
}

bool
App::
findNext(const CRegExp &pattern, uint line_num, int char_num,
         uint *fline_num, uint *fchar_num, uint *len)
{
  return findNext(pattern, line_num, char_num, getNumLines() - 1, -1, fline_num, fchar_num, len);
}

bool
App::
findNext(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *len)
{
  uint fline_num, fchar_num;

  if (! findNext(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num, len))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
App::
findNext(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len)
{
  setFindPattern(pattern);

  uint spos, epos;

  if (findNext(getLine(line_num1), pattern, char_num1, -1, &spos, &epos)) {
    *fline_num = line_num1;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  for (int i = int(line_num1) + 1; i <= line_num2 - 1; ++i) {
    if (findNext(getLine(i), pattern, 0, -1, &spos, &epos)) {
      *fline_num = i;
      *fchar_num = spos;
      if (len) *len = epos - spos + 1;
      return true;
    }
  }

  if (findNext(getLine(line_num2), pattern, 0, char_num2, &spos, &epos)) {
    *fline_num = line_num2;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  return false;
}

bool
App::
findPrev(const std::string &pattern)
{
  return findPrev(pattern, getRow(), getCol() - 1);
}

bool
App::
findPrev(const std::string &pattern, uint *fline_num, uint *fchar_num)
{
  return findPrev(pattern, getRow(), getCol() - 1, fline_num, fchar_num);
}

bool
App::
findPrev(const std::string &pattern, uint line_num, int char_num)
{
  return findPrev(pattern, line_num, char_num, 0, 0);
}

bool
App::
findPrev(const std::string &pattern, uint line_num, int char_num, uint *fline_num, uint *fchar_num)
{
  return findPrev(pattern, line_num, char_num, 0, 0, fline_num, fchar_num);
}

bool
App::
findPrev(const std::string &pattern, uint line_num1, int char_num1, int line_num2, int char_num2)
{
  uint fline_num, fchar_num;

  if (! findPrev(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
App::
findPrev(const std::string &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num)
{
  setFindPattern(CRegExp(pattern));

  if (findPrev(getLine(line_num1), pattern, char_num1, 0, fchar_num)) {
    *fline_num = line_num1;
    return true;
  }

  for (int i = line_num1 - 1; i >= line_num2 + 1; --i) {
    if (findPrev(getLine(i), pattern, -1, 0, fchar_num)) {
      *fline_num = i;
      return true;
    }
  }

  if (findPrev(getLine(line_num2), pattern, -1, char_num2, fchar_num)) {
    *fline_num = line_num2;
    return true;
  }

  return false;
}

bool
App::
findPrev(const CRegExp &pattern, uint *len)
{
  return findPrev(pattern, getRow(), getCol() - 1, len);
}

bool
App::
findPrev(const CRegExp &pattern, uint *fline_num, uint *fchar_num, uint *len)
{
  return findPrev(pattern, getRow(), getCol() - 1, fline_num, fchar_num, len);
}

bool
App::
findPrev(const CRegExp &pattern, uint line_num, int char_num, uint *len)
{
  return findPrev(pattern, line_num, char_num, 0, 0, len);
}

bool
App::
findPrev(const CRegExp &pattern, uint line_num, int char_num,
         uint *fline_num, uint *fchar_num, uint *len)
{
  return findPrev(pattern, line_num, char_num, 0, 0, fline_num, fchar_num, len);
}

bool
App::
findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *len)
{
  uint fline_num, fchar_num;

  if (! findPrev(pattern, line_num1, char_num1, line_num2, char_num2, &fline_num, &fchar_num, len))
    return false;

  cursorTo(fline_num, fchar_num);

  return true;
}

bool
App::
findPrev(const CRegExp &pattern, uint line_num1, int char_num1,
         int line_num2, int char_num2, uint *fline_num, uint *fchar_num, uint *len)
{
  setFindPattern(pattern);

  uint spos, epos;

  if (findPrev(getLine(line_num1), pattern, char_num1, 0, &spos, &epos)) {
    *fline_num = line_num1;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  for (int i = line_num1 - 1; i >= line_num2 + 1; --i) {
    if (findPrev(getLine(i), pattern, -1, 0, &spos, &epos)) {
      *fline_num = i;
      *fchar_num = spos;
      if (len) *len = epos - spos + 1;
      return true;
    }
  }

  if (findPrev(getLine(line_num2), pattern, -1, char_num2, &spos, &epos)) {
    *fline_num = line_num2;
    *fchar_num = spos;
    if (len) *len = epos - spos + 1;
    return true;
  }

  return false;
}

bool
App::
findNextChar(char c, bool multiline)
{
  return findNextChar(getRow(), getCol(), c, multiline);
}

bool
App::
findNextChar(const std::string &str, bool multiline)
{
  return findNextChar(getRow(), getCol(), str, multiline);
}

bool
App::
findNextChar(uint line_num, int char_num, char c, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  ++char_num;

  while (true) {
    auto *line = getLine(line_num);

    for (uint i = char_num; i < line->getLength(); ++i) {
      if (line->getChar(i) == c) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == getNumLines() - 1)
      break;

    ++line_num;

    char_num = 0;
  }

  return false;
}

bool
App::
findNextChar(uint line_num, int char_num, const std::string &str, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  ++char_num;

  while (true) {
    auto *line = getLine(line_num);

    const char *str1 = str.c_str();

    for (uint i = char_num; i < line->getLength(); ++i) {
      if (strchr(str1, line->getChar(i))) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == getNumLines() - 1)
      break;

    ++line_num;

    char_num = 0;
  }

  return false;
}

bool
App::
findPrevChar(char c, bool multiline)
{
  return findPrevChar(getRow(), getCol(), c, multiline);
}

bool
App::
findPrevChar(const std::string &str, bool multiline)
{
  return findPrevChar(getRow(), getCol(), str, multiline);
}

bool
App::
findPrevChar(uint line_num, int char_num, char c, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  --char_num;

  auto *line = getLine(line_num);

  while (true) {
    for (int i = char_num; i >= 0; --i) {
      if (line->getChar(i) == c) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == 0)
      break;

    --line_num;

    line = getLine(line_num);

    char_num = line->getLength() - 1;
  }

  return false;
}

bool
App::
findPrevChar(uint line_num, int char_num, const std::string &str, bool multiline)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  --char_num;

  auto *line = getLine(line_num);

  while (true) {
    const char *str1 = str.c_str();

    for (int i = char_num; i >= 0; --i) {
      if (strchr(str1, line->getChar(i))) {
        cursorTo(line_num, i);
        return true;
      }
    }

    if (! multiline)
      break;

    if (line_num == 0)
      break;

    --line_num;

    line = getLine(line_num);

    char_num = line->getLength() - 1;
  }

  return false;
}

//---

bool
App::
replace(uint line_num, uint char_num, char c)
{
  return replace(line_num, char_num, char_num, std::string(&c, 1));
}

bool
App::
replace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr)
{
  CASSERT(line_num < getNumLines(), "Invalid Line Num");

  startGroup();

  bool rc = subReplace(line_num, char_num1, char_num2, replaceStr);

  endGroup();

  return rc;
}

bool
App::
subReplace(uint line_num, uint char_num1, uint char_num2, const std::string &replaceStr)
{
  const auto *line = getLine(line_num);

  auto old = line->getSubString(char_num1, char_num2);

  lines_.replaceLineChars(line_num, char_num1, char_num2, replaceStr);

  char_num2 = char_num1 + uint(replaceStr.size()) - 1;

  addUndo(new ReplaceUndoCmd(this, line_num, char_num1, char_num2, old));

  setChanged(true);
  setUnsaved(true);

  return true;
}

//---

bool
App::
getMarkPos(const std::string &name, uint *row, uint *col)
{
  if (name == "<") {
    int row1, col1;
    if (! getSelectStart(&row1, &col1))
      return false;
    *row = row1;
    *col = col1;
    return true;
  }

  if (name == ">") {
    int row1, col1;
    if (! getSelectEnd(&row1, &col1))
      return false;
    *row = row1;
    *col = col1;
    return true;
  }

  //---

  auto p = markPosMap_.find(name);
  if (p == markPosMap_.end()) return false;

  auto pos = (*p).second;

  *row = pos.row;
  *col = pos.col;

  return pos.set;
}

void
App::
setMarkPos(const std::string &name)
{
  uint x, y;
  getPos(&x, &y);

  setMarkPos(name, y, x);
}

void
App::
setMarkPos(const std::string &name, uint row, uint col)
{
  markPosMap_[name] = MarkPos(row, col, true);
}

void
App::
markReturn()
{
  setMarkPos("'");
}

//---

void
App::
startGroup()
{
  auto *group = new Group;

  groupList_.push_back(group);

  undo_.startGroup();

  addUndo(new MoveToUndoCmd(this, getRow(), getCol()));
}

void
App::
endGroup()
{
  undo_.endGroup();

  if (! inGroup())
    return;

  auto *group = groupList_.back();

  groupList_.pop_back();

  auto p1 = group->lines.begin();
  auto p2 = group->lines.end  ();

  auto &buffer = getBuffer('\0');

  for ( ; p1 != p2; ++p1)
    buffer.addLine(*p1);

  delete group;
}

bool
App::
inGroup() const
{
  return ! groupList_.empty();
}

void
App::
addUndo(UndoCmd *cmd)
{
  if (! undo_.locked())
    undo_.addUndo(cmd);
  else
    delete cmd;
}

void
App::
undo()
{
  undo_.undo();

  fixPos();
}

void
App::
redo()
{
  undo_.redo();

  fixPos();
}

bool
App::
canUndo() const
{
  return undo_.canUndo();
}

bool
App::
canRedo() const
{
  return undo_.canRedo();
}

void
App::
undoLine()
{
#if 0
  const auto &last_line = cursor_->getLastLine();

  if (last_line.set) {
    auto text = last_line.text;

    cursor_->updateLastLine();

    lines_.replaceLineChars(last_line.row, text);
  }
#endif
}

void
App::
resetUndo()
{
  undo_.clear();
}

//---

Buffer &
App::
getBuffer(char c)
{
  return buffers_[c];
}

//---

bool
App::
isWordChar(char c) const
{
  return (isalnum(c) || c == '_');
}

uint
App::
getLineEnd(uint line_num) const
{
  if (line_num >= getNumLines())
    return 0;

  const auto *line = getLine(line_num);

  uint len = line->getLength();

  if (len > 0 && ! isExtraLineChar())
    --len;

  return len;
}

//---

bool
App::
isSentenceEnd(const Line *line, uint pos, uint *n) const
{
  *n = 0;

  uint len = line->getLength();

  if (pos + *n >= len)
    return false;

  char c = line->getChar(pos + *n);

  // check for . ! ?
  if (strchr(".?!", c) == nullptr)
    return false;

  (*n)++;

  // skip (, ], " and ' after . ! ?
  while (pos + *n < len - 1) {
    char ec = line->getChar(pos + *n);

    if (strchr(")]\"\'", ec) == nullptr)
      break;

    (*n)++;
  }

  // EOL is ok
  if (pos + *n >= len)
    return true;

  c = line->getChar(pos + *n);

  // space is ok
  if (isspace(c))
    return true;

  return false;
}

bool
App::
isSection(const Line *line, uint, uint *n) const
{
  uint len = line->getLength();

  if (len > 0 && line->getChar(0) == '{') {
    *n = 0;
    return true;
  }

  return false;
}

bool
App::
isBlank(const Line *line) const
{
  uint len = line->getLength();

  for (uint i = 0; i < len; ++i) {
    char c = line->getChar(i);

    if (! isspace(c))
      return false;
  }

  return true;
}

bool
App::
findNext(const Line *line, const std::string &pattern, int char_num1, int char_num2,
         uint *char_num) const
{
  if (line->isEmpty())
    return false;

  uint num_chars = line->getLength();

  if (char_num1 >= int(num_chars))
    return false;

  if (char_num2 < 0)
    char_num2 = num_chars - 1;

  const char *cline = line->getCString();

  const char *pattern1 = pattern.c_str();

  char *p = CStrUtil::strstr(&cline[char_num1], &cline[char_num2], pattern1);

  if (p == nullptr)
    return false;

  if (char_num)
    *char_num = uint(p - cline);

  return true;
}

bool
App::
findNext(const Line *line, const CRegExp &pattern, int char_num1, int char_num2,
         uint *spos, uint *epos) const
{
  if (line->isEmpty())
    return false;

  uint num_chars = line->getLength();

  if (char_num1 >= int(num_chars))
    return false;

  if (char_num2 < 0)
    char_num2 = num_chars - 1;

  std::string cline = line->getString().substr(char_num1, char_num2 - char_num1 + 1);

  if (! pattern.find(cline))
    return false;

  int spos1, epos1;

  if (! pattern.getMatchRange(&spos1, &epos1))
    return false;

  if (spos) *spos = spos1 + char_num1;
  if (epos) *epos = epos1 + char_num1;

  return true;
}

bool
App::
findPrev(const Line *line, const std::string &pattern, int char_num1, int char_num2,
         uint *char_num) const
{
  if (line->isEmpty())
    return false;

  uint num_chars = line->getLength();

  if (char_num1 < 0)
    char_num1 = num_chars - 1;

  if (char_num2 >= int(num_chars))
    return false;

  const char *cline = line->getCString();

  const char *pattern1 = pattern.c_str();

  char *p = CStrUtil::strrstr(&cline[char_num1], &cline[char_num2], pattern1);

  if (p == nullptr)
    return false;

  if (char_num)
    *char_num = uint(p - cline);

  return true;
}

bool
App::
findPrev(const Line *line, const CRegExp &pattern, int char_num1, int char_num2,
         uint *spos, uint *epos) const
{
  if (line->isEmpty())
    return false;

  uint num_chars = line->getLength();

  if (char_num1 < 0)
    char_num1 = num_chars - 1;

  if (char_num2 >= int(num_chars))
    return false;

  std::string cline = line->getString().substr(char_num2, char_num1 - char_num2 + 1);

  if (! pattern.find(cline))
    return false;

  int spos1, epos1;

  if (! pattern.getMatchRange(&spos1, &epos1))
    return false;

  if (spos) *spos = spos1 + char_num2;
  if (epos) *epos = epos1 + char_num2;

  return true;
}

bool
App::
getSelectStart(int *row, int *col) const
{
  if (selection_.set) {
    *row = selection_.row1;
    *col = selection_.col1;
  }
  else {
    *row = -1;
    *col = -1;
  }

  return selection_.set;
}

bool
App::
getSelectEnd(int *row, int *col) const
{
  if (selection_.set) {
    *row = selection_.row2;
    *col = selection_.col2;
  }
  else {
    *row = -1;
    *col = -1;
  }

  return selection_.set;
}

void
App::
setSelectRange(int row1, int col1, int row2, int col2)
{
  selection_.set = true;

  selection_.row1 = row1;
  selection_.col1 = col1;
  selection_.row2 = row2;
  selection_.col2 = col2;

  iface_->selectionChanged();
}

void
App::
clearSelection()
{
  lastSelection_ = selection_;

  selection_.set = false;

  iface_->selectionChanged();
}

void
App::
rangeSelect(int row1, int col1, int row2, int col2, bool select)
{
  selection_.set = select;

  selection_.row1 = row1;
  selection_.col1 = col1;
  selection_.row2 = row2;
  selection_.col2 = col2;

  iface_->selectionChanged();
}

//---

bool
App::
runEdCmd(const std::string &str, bool &quitted)
{
  //std::cerr << "Run Ed Cmd '" << str << "'\n";

  quitted = false;

  bool rc = true;

  std::vector<std::string> words;

  CStrUtil::toWords(str, words);

  uint num_words = uint(words.size());

  auto cmd = (num_words > 0 ? words[0] : std::string());

  if (cmd == "exit" || cmd == "quit") {
    quitted = true;
  }
  else {
    rc = ed_->execCmd(str);

    if (rc)
      quitted = ed_->isQuit();
  }

  return rc;
}

void
App::
setNameValue(const std::string &name, const std::string &value)
{
  if      (name == "list") {
    if (value == "1")
      setListMode(true);
  }
  else if (name == "nolist") {
    if (value == "1")
      setListMode(false);
  }
  else if (name == "number") {
    if (value == "1")
      setNumberMode(true);
  }
  else if (name == "nonumber") {
    if (value == "1")
      setNumberMode(false);
  }
  else if (name == "ignorecase") {
    if (value == "1")
      setCaseSensitive(false);
  }
  else if (name == "noignorecase") {
    if (value == "1")
      setCaseSensitive(true);
  }
  else if (name == "syntax") {
    if      (value == "c")
      setSyntax(new CSyntaxC);
    else if (value == "cpp")
      setSyntax(new CSyntaxCPP);
    else
      setSyntax(nullptr);
  }

  nameValues_[name] = value;

  iface_->stateChanged();
}

void
App::
setSyntax(CSyntax *syntax)
{
  delete syntax_;

  syntax_ = syntax;

  if (syntax_) {
    class Notifier : public CSyntaxNotifier {
     public:
      Notifier(App *app) :
       app_(app) {
      }

      App *app() const { return app_; }

      void setLine(Line *line) {
        line_ = line;
      }

      void addToken(uint, uint word_start, const std::string &word, CSyntaxToken token) override {
        line_->addAnnotation(word_start, int(word_start + word.size() - 1), token);
      }

     private:
      App*  app_  { nullptr };
      Line* line_ { nullptr };
    };

    //---

    Notifier notifier(this);

    syntax_->init();

    syntax_->setNotifier(&notifier);

    for (auto *line : lines_) {
      notifier.setLine(line);

      line->clearAnnotations();

      syntax_->processLine(line->chars());
    }

    syntax_->term();
  }
  else {
    for (auto *line : lines_)
      line->clearAnnotations();
  }
}

//------

Lines::
Lines()
{
}

Lines::
~Lines()
{
  clear();
}

void
Lines::
clear()
{
  auto p1 = begin();
  auto p2 = end  ();

  for ( ; p1 != p2; ++p1)
    delete *p1;

  lines_.clear();
}

const Line *
Lines::
getLine(uint line_num) const
{
  return lines_[line_num];
}

Line *
Lines::
getLine(uint line_num)
{
  return lines_[line_num];
}

void
Lines::
addLine(uint line_num, Line *line)
{
  if (line_num == lines_.size()) {
    lines_.push_back(line);

    line->setChanged(true);
  }
  else {
    lines_.push_back(lines_[lines_.size() - 1]);

    for (int i = int(lines_.size()) - 2; i >= int(line_num); --i)
      lines_[i + 1] = lines_[i];

    lines_[line_num] = line;

    for (uint i = line_num; i < lines_.size(); ++i)
      lines_[i]->setChanged(true);
  }
}

void
Lines::
addLineChar(uint line_num, uint char_num, char c)
{
  auto *line = lines_[line_num];

  line->insertChar(char_num, c);

  line->setChanged(true);
}

void
Lines::
addLineChars(uint line_num, uint char_num, const std::string &chars)
{
  auto *line = lines_[line_num];

  line->addChars(char_num, chars);

  line->setChanged(true);
}

void
Lines::
setLineChar(uint line_num, uint char_num, char c)
{
  auto *line = lines_[line_num];

  line->setChar(char_num, c);

  line->setChanged(true);
}

void
Lines::
replaceLineChar(uint line_num, uint char_num, char c)
{
  auto *line = lines_[line_num];

  line->replaceChar(char_num, c);

  line->setChanged(true);
}

void
Lines::
replaceLineChars(uint line_num, const std::string &str)
{
  auto *line = lines_[line_num];

  line->replace(str);

  line->setChanged(true);
}

void
Lines::
replaceLineChars(uint line_num, uint char_num1, uint char_num2, const std::string &str)
{
  auto *line = lines_[line_num];

  line->replace(char_num1, char_num2, str);

  line->setChanged(true);
}

void
Lines::
moveLine(uint line_num1, int line_num2)
{
  auto *line = lines_[line_num1];

  if      (line_num2 > int(line_num1)) {
    for (int i = line_num1 + 1; i <= line_num2; ++i)
      lines_[i - 1] = lines_[i];

    lines_[line_num2] = line;

    for (int i = line_num1; i <= line_num2; ++i)
      lines_[i]->setChanged(true);
  }
  else if (line_num2 < int(line_num1)) {
    for (int i = line_num1 - 1; i > line_num2; --i)
      lines_[i + 1] = lines_[i];

    lines_[line_num2 + 1] = line;

    for (int i = line_num2; i <= int(line_num1); ++i)
      lines_[i]->setChanged(true);
  }
}

void
Lines::
splitLine(uint line_num, uint char_num)
{
  auto *line1 = lines_[line_num    ];
  auto *line2 = lines_[line_num + 1];

  line1->split(line2, char_num);
}

void
Lines::
joinLine(uint line_num)
{
  auto *line1 = lines_[line_num    ];
  auto *line2 = lines_[line_num + 1];

  line1->join(line2);
}

void
Lines::
deleteLine(uint line_num)
{
  auto *line = lines_[line_num];

  uint num_lines = uint(lines_.size());

  for (uint i = line_num + 1; i < num_lines; ++i)
    lines_[i - 1] = lines_[i];

  lines_.pop_back();

  delete line;

  num_lines = uint(lines_.size());

  for (uint i = line_num; i < num_lines; ++i)
    lines_[i]->setChanged(true);
}

void
Lines::
deleteLineChars(uint line_num, uint char_num, uint n)
{
  auto *line = lines_[line_num];

  for (uint i = 0; i < n; ++i)
    line->deleteChar(char_num);

  line->setChanged(true);
}

//------

Line::
Line()
{
}

Line::
Line(const Line &line) :
 chars_(line.chars_)
{
}

Line::
~Line()
{
  clear();
}

Line &
Line::
operator=(const Line &line)
{
  chars_   = line.chars_;
  changed_ = true;

  return *this;
}

Line *
Line::
dup() const
{
  return new Line(*this);
}

void
Line::
addChars(uint pos, const std::string &chars)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  uint num = uint(chars.size());
  if (num == 0) return;

  uint old_len = uint(chars_.size());

  for (uint i = 0; i < num; ++i)
    chars_.push_back('\0');

  uint new_len = old_len + num;

  for (int i = int(old_len) - 1, j = int(new_len) - 1; i >= int(pos); --i, --j)
    std::swap(chars_[j], chars_[i]);

  for (uint i = 0; i < num; ++i)
    chars_[pos + i] = chars[i];

  //for (uint i = pos; i < new_len; ++i)
  //  chars_[i]->setChanged(true);

  setChanged(true);
}

void
Line::
addChar(uint pos, char c)
{
  addChars(pos, std::string(&c, 1));
}

void
Line::
addChars(uint pos, uint num, char c)
{
  if (num <= 0) return;

  std::string str;

  str.resize(num, c);

  addChars(pos, str);
}

uint
Line::
getLength() const
{
  return uint(chars_.size());
}

bool
Line::
isEmpty() const
{
  return chars_.empty();
}

Line::const_char_iterator
Line::
beginChar() const
{
  return chars_.begin();
}

Line::const_char_iterator
Line::
endChar() const
{
  return chars_.end();
}

void
Line::
clear()
{
  chars_.clear();

  setChanged(true);
}

char
Line::
getChar(uint pos) const
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return '\0';

  if (pos == getLength())
    return '\0';

  return chars_[pos];
}

void
Line::
setChar(uint pos, char c)
{
  if (! CASSERT(pos < getLength(), "Invalid Char Num")) return;

  chars_[pos] = c;

  setChanged(true);
}

void
Line::
insertChar(uint pos, char c)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  if (pos == chars_.size())
    chars_.push_back(c);
  else {
    chars_.push_back(chars_[chars_.size() - 1]);

    for (int i = int(chars_.size()) - 2; i >= int(pos); --i)
      chars_[i + 1] = chars_[i];

    chars_[pos] = c;
  }

  setChanged(true);
}

void
Line::
replaceChar(uint pos, char c)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  if (pos < getLength())
    setChar(pos, c);
  else
    insertChar(pos, c);
}

void
Line::
deleteChars(uint pos, uint num)
{
  if (! CASSERT(pos < getLength() + num - 1, "Invalid Char Num")) return;

  for (uint i = 0; i < num; ++i)
    deleteChar(pos);
}

void
Line::
deleteChar(uint pos)
{
  if (! CASSERT(pos < getLength(), "Invalid Char Num")) return;

  //char c1 = chars_[pos];

  if (pos > 0)
    chars_ = chars_.substr(0, pos) + chars_.substr(pos + 1);
  else
    chars_ = chars_.substr(pos + 1);
}

bool
Line::
findNext(const std::string & /*pattern*/, int /*char_num1*/, int /*char_num2*/,
         uint* /*char_num*/) const
{
  //return util_.findNext(pattern, char_num1, char_num2, char_num);
  return false;
}

bool
Line::
findNext(const CRegExp & /*pattern*/, int /*char_num1*/, int /*char_num2*/,
         uint* /*spos*/, uint* /*epos*/) const
{
  //return util_.findNext(pattern, char_num1, char_num2, spos, epos);
  return false;
}

bool
Line::
findPrev(const std::string & /*pattern*/, int /*char_num1*/, int /*char_num2*/,
         uint* /*char_num*/) const
{
  //return util_.findPrev(pattern, char_num1, char_num2, char_num);
  return false;
}

bool
Line::
findPrev(const CRegExp & /*pattern*/, int /*char_num1*/, int /*char_num2*/,
         uint* /*spos*/, uint* /*epos*/) const
{
  //return util_.findPrev(pattern, char_num1, char_num2, spos, epos);
  return false;
}

void
Line::
replace(const std::string &str)
{
  replace(0, getLength() - 1, str);
}

void
Line::
replace(int spos, int epos, const std::string &str)
{
  if (epos < 0)
    epos = getLength() - 1;

  if (! CASSERT(spos >= 0 && spos < int(getLength()), "Invalid Char Num"))
    return;

  if (! CASSERT(epos >= 0 && epos < int(getLength()), "Invalid Char Num"))
    return;

  if (! CASSERT(spos <= epos, "Invalid Range"))
    return;

  int len1 = epos - spos + 1;
  int len2 = int(str.size());

  if      (len1 > len2)
    deleteChars(spos, len1 - len2);
  else if (len2 > len1)
    addChars(spos + len1 - 1, len2 - len1, '\0');

  for (int i = 0; i < len2; ++i)
    setChar(spos + i, str[i]);

  setChanged(true);
}

void
Line::
split(Line *line, uint pos)
{
  if (! CASSERT(pos <= getLength(), "Invalid Char Num")) return;

  line->chars_ = chars_.substr(pos);

  chars_ = chars_.substr(0, pos);

  setChanged(true);

  line->setChanged(true);
}

void
Line::
join(Line *line)
{
  chars_ += line->chars_;

  line->chars_.clear();

  setChanged(true);
}

const std::string &
Line::
getString() const
{
  return chars_;
}

std::string
Line::
getSubString(int spos, int epos) const
{
  if (epos < 0)
    epos = getLength() - 1;

  return chars_.substr(spos, epos - spos + 1);
}

const char *
Line::
getCString() const
{
  return getString().c_str();
}

const char *
Line::
getSubCString(int spos, int epos) const
{
  static std::string buffer = getSubString(spos, epos);

  return buffer.c_str();
}

uint
Line::
getEnd(bool extraLineChar) const
{
  uint len = getLength();

  if (len > 0 && ! extraLineChar)
    --len;

  return len;
}

void
Line::
setChanged(bool changed)
{
  changed_ = changed;
}

void
Line::
addAnnotation(uint start, uint end, const CSyntaxToken &token)
{
  for (uint i = start; i <= end; ++i)
    charStyle_[i] = Style(token);
}

void
Line::
clearAnnotations()
{
  charStyle_.clear();
}

bool
Line::
getCharStyle(int i, Style &style) const
{
  auto p = charStyle_.find(i);
  if (p == charStyle_.end()) return false;

  style = (*p).second;

  return true;
}

void
Line::
print(std::ostream &os) const
{
  os << chars_;
}

std::ostream &
operator<<(std::ostream &os, const Line &line)
{
  line.print(os);

  return os;
}

//-------

LastCommand::
LastCommand()
{
}

void
LastCommand::
clear()
{
  keys_.clear();
}

void
LastCommand::
addCount(uint n)
{
  if (n == 0) return;

  auto str = CStrUtil::toString(n);

  uint len = uint(str.size());

  for (uint i = 0; i < len; ++i) {
    char key = str[i];

    addKey(key);
  }
}

void
LastCommand::
addKey(char key)
{
  keys_.push_back(key);
}

void
LastCommand::
exec(App *vi)
{
  uint len = uint(keys_.size());

  for (uint i = 0; i < len; ++i)
    vi->processChar(keys_[i]);
}

//------

void
CmdLine::
setLine(const std::string &line)
{
  line_ = line;

  updateLine();
}

void
CmdLine::
keyPress(char c)
{
  line_ += c;

  updateLine();
}

void
CmdLine::
backspace()
{
  line_ = line_.substr(0, line_.size() - 1);

  updateLine();
}

//------

AddLineUndoCmd::
AddLineUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0)
{
}

AddLineUndoCmd::
AddLineUndoCmd(App *vi, int line_num, const std::string &line) :
 UndoCmd(vi), line_num_(line_num), line_(line)
{
  if (vi_->getDebug())
    std::cerr << "Add: Add Line " << line_num << " '" << line << "'\n";
}

bool
AddLineUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int         pos  = int(CStrUtil::toInteger(argList[0]));
  const auto &line = argList[1];

  vi_->addLine(pos, line);

  return true;
}

bool
AddLineUndoCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (vi_->getDebug())
      std::cerr << "Exec: Add Line " << line_num_ << " '" << line_ << "'\n";

    vi_->addLine(line_num_, line_);
  }
  else {
    if (vi_->getDebug())
      std::cerr << "Exec: Delete Line " << line_num_ << "\n";

    vi_->deleteLine(line_num_);
  }

  return true;
}

//------

DeleteLineUndoCmd::
DeleteLineUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0)
{
}

DeleteLineUndoCmd::
DeleteLineUndoCmd(App *vi, int line_num) :
 UndoCmd(vi), line_num_(line_num)
{
  if (vi_->getDebug())
    std::cerr << "Add: Delete Line " << line_num_ << "\n";

  chars_ = vi_->getLine(line_num_)->getString();
}

bool
DeleteLineUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 1);

  int pos = int(CStrUtil::toInteger(argList[0]));

  vi_->deleteLine(pos);

  return true;
}

bool
DeleteLineUndoCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (vi_->getDebug())
      std::cerr << "Exec: Delete Line " << line_num_ << "\n";

    vi_->deleteLine(line_num_);
  }
  else {
    if (vi_->getDebug())
      std::cerr << "Exec: Add Line " << line_num_ << " '" << chars_ << "'\n";

    vi_->addLine(line_num_, chars_);
  }

  return true;
}

//------

MoveLineUndoCmd::
MoveLineUndoCmd(App *vi) :
 UndoCmd(vi), line_num1_(0), line_num2_(0)
{
}

MoveLineUndoCmd::
MoveLineUndoCmd(App *vi, int line_num1, int line_num2) :
 UndoCmd(vi), line_num1_(line_num1), line_num2_(line_num2)
{
}

bool
MoveLineUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int pos1 = int(CStrUtil::toInteger(argList[0]));
  int pos2 = int(CStrUtil::toInteger(argList[1]));

  vi_->moveLine(pos1, pos2);

  return true;
}

bool
MoveLineUndoCmd::
exec()
{
  if (getState() == UNDO_STATE)
    vi_->moveLine(line_num1_, line_num2_);
  else
    vi_->moveLine(line_num2_, line_num1_);

  return true;
}

//------

ReplaceUndoCmd::
ReplaceUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0), char_num1_(0), char_num2_(0)
{
}

ReplaceUndoCmd::
ReplaceUndoCmd(App *vi, int line_num, int char_num1, int char_num2, const std::string &str) :
 UndoCmd(vi), line_num_(line_num), char_num1_(char_num1), char_num2_(char_num2), str_(str)
{
}

bool
ReplaceUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 4);

  int  line_num  = int(CStrUtil::toInteger(argList[0]));
  int  char_num1 = int(CStrUtil::toInteger(argList[1]));
  int  char_num2 = int(CStrUtil::toInteger(argList[2]));
  auto str       = argList[3];

  vi_->replace(line_num, char_num1, char_num2, str);

  return true;
}

bool
ReplaceUndoCmd::
exec()
{
  auto str = vi_->getLine(line_num_)->getSubString(char_num1_, char_num2_);

  vi_->replace(line_num_, char_num1_, char_num2_, str_);

  str_ = str;

  return true;
}

//------

InsertCharUndoCmd::
InsertCharUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0), char_num_(0), c_(0)
{
}

InsertCharUndoCmd::
InsertCharUndoCmd(App *vi, int line_num, int char_num, char c) :
 UndoCmd(vi), line_num_(line_num), char_num_(char_num), c_(c)
{
  if (vi_->getDebug())
    std::cerr << "Add: Insert Char " << line_num_ << " " << char_num_ << " '" << c_ << "'\n";
}

bool
InsertCharUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int  line_num = int(CStrUtil::toInteger(argList[0]));
  int  char_num = int(CStrUtil::toInteger(argList[1]));
  auto str      = argList[2];

  vi_->insertChar(line_num, char_num, str[0]);

  return true;
}

bool
InsertCharUndoCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (vi_->getDebug())
      std::cerr << "Exec: Insert Char " << line_num_ << " " << char_num_ << " '" << c_ << "'\n";

    vi_->insertChar(line_num_, char_num_, c_);
  }
  else {
    if (vi_->getDebug())
      std::cerr << "Exec: Delete Char " << line_num_ << " " << char_num_ << " " << "\n";

    vi_->deleteChars(line_num_, char_num_, 1);
  }

  return true;
}

//------

ReplaceCharUndoCmd::
ReplaceCharUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0), char_num_(0), c_(0)
{
}

ReplaceCharUndoCmd::
ReplaceCharUndoCmd(App *vi, int line_num, int char_num, char c) :
 UndoCmd(vi), line_num_(line_num), char_num_(char_num), c_(c)
{
}

bool
ReplaceCharUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int  line_num = int(CStrUtil::toInteger(argList[0]));
  int  char_num = int(CStrUtil::toInteger(argList[1]));
  auto str      = argList[2];

  vi_->replaceChar(line_num, char_num, str[0]);

  return true;
}

bool
ReplaceCharUndoCmd::
exec()
{
  auto str = vi_->getLine(line_num_)->getSubString(char_num_, char_num_);

  vi_->replaceChar(line_num_, char_num_, c_);

  c_ = str[0];

  return true;
}

//------

DeleteCharsUndoCmd::
DeleteCharsUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0), char_num_(0)
{
}

DeleteCharsUndoCmd::
DeleteCharsUndoCmd(App *vi, int line_num, int char_num, int num_chars) :
 UndoCmd(vi), line_num_(line_num), char_num_(char_num)
{
  chars_ = vi_->getLine(line_num_)->getSubString(char_num_, char_num_ + num_chars - 1);

  if (vi_->getDebug())
      std::cerr << "Add: Delete Chars " << line_num_ << " " << char_num_ <<
                   " '" << chars_ << "'\n";
}

bool
DeleteCharsUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 3);

  int line_num  = int(CStrUtil::toInteger(argList[0]));
  int char_num  = int(CStrUtil::toInteger(argList[1]));
  int num_chars = int(CStrUtil::toInteger(argList[2]));

  vi_->deleteChars(line_num, char_num, num_chars);

  return true;
}

bool
DeleteCharsUndoCmd::
exec()
{
  if (getState() == UNDO_STATE) {
    if (vi_->getDebug())
      std::cerr << "Exec: Delete Chars " << line_num_ << " " << char_num_ <<
                   " '" << chars_ << "'\n";

    vi_->deleteChars(line_num_, char_num_, int(chars_.size()));
  }
  else {
    if (vi_->getDebug())
      std::cerr << "Exec: Add Chars " << line_num_ << " " << char_num_ << " '" << chars_ << "'\n";

    vi_->addChars(line_num_, char_num_, chars_);
  }

  return true;
}

//------

SplitLineUndoCmd::
SplitLineUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0), char_num_(0)
{
}

SplitLineUndoCmd::
SplitLineUndoCmd(App *vi, int line_num, int char_num) :
 UndoCmd(vi), line_num_(line_num), char_num_(char_num)
{
}

bool
SplitLineUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int line_num = int(CStrUtil::toInteger(argList[0]));
  int char_num = int(CStrUtil::toInteger(argList[1]));

  vi_->splitLine(line_num, char_num);

  return true;
}

bool
SplitLineUndoCmd::
exec()
{
  if (getState() == UNDO_STATE)
    vi_->splitLine(line_num_, char_num_);
  else
    vi_->joinLine(line_num_);

  return true;
}

//------

JoinLineUndoCmd::
JoinLineUndoCmd(App *vi) :
 UndoCmd(vi), line_num_(0)
{
}

JoinLineUndoCmd::
JoinLineUndoCmd(App *vi, int line_num) :
 UndoCmd(vi), line_num_(line_num)
{
  char_num_ = vi_->getCol();
}

bool
JoinLineUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 1);

  int line_num = int(CStrUtil::toInteger(argList[0]));

  vi_->joinLine(line_num);

  return true;
}

bool
JoinLineUndoCmd::
exec()
{
  if (getState() == UNDO_STATE)
    vi_->joinLine(line_num_);
  else
    vi_->splitLine(line_num_, char_num_);

  return true;
}

//------

MoveToUndoCmd::
MoveToUndoCmd(App *vi) :
 UndoCmd(vi)
{
}

MoveToUndoCmd::
MoveToUndoCmd(App *vi, int line_num, int char_num) :
 UndoCmd(vi), line_num_(line_num), char_num_(char_num)
{
  if (vi_->getDebug())
    std::cerr << "Add: Move To " << line_num << " " << char_num << "\n";
}

bool
MoveToUndoCmd::
exec(const std::vector<std::string> &argList)
{
  assert(argList.size() == 2);

  int line_num = int(CStrUtil::toInteger(argList[0]));
  int char_num = int(CStrUtil::toInteger(argList[1]));

  vi_->cursorTo(line_num, char_num);

  return true;
}

bool
MoveToUndoCmd::
exec()
{
  uint line_num = vi_->getRow();
  uint char_num = vi_->getCol();

  if (vi_->getDebug())
    std::cerr << "Exec: Move To " << line_num_ << " " << char_num_ << "\n";

  vi_->cursorTo(line_num_, char_num_);

  line_num_ = line_num;
  char_num_ = char_num;

  return true;
}

//------

UndoCmd::
UndoCmd(App *vi) :
 vi_(vi)
{
}

}
