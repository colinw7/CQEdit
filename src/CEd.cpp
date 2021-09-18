#include <CEd.h>
#include <CEdit.h>
#include <COptVal.h>
#include <CFile.h>
#include <CRegExp.h>
#include <CStrUtil.h>
#include <CStrParse.h>
#include <CCommand.h>

CEd::
CEd(CEditFile *file) :
 file_(file)
{
}

CEd::
~CEd()
{
}

void
CEd::
init()
{
  // move to first character on last line
  setPos(CIPoint2D(0, std::max(0, (int) file_->getNumLines() - 1)));
}

bool
CEd::
execFile(const std::string &fileName)
{
  std::vector<std::string> lines;

  CFile file(fileName);

  if (! file.exists() || ! file.isRegular())
    return false;

  file.toLines(lines);

  std::vector<std::string>::const_iterator p1, p2;

  for (p1 = lines.begin(), p2 = lines.end(); p1 != p2; ++p1) {
    std::string line = CStrUtil::stripSpaces(*p1);

    if (line.empty() || line[0] == '#')
      continue;

    execCmd(line);
  }

  return true;
}

bool
CEd::
execCmd(const std::string &cmd)
{
  if (mode_ == INPUT) {
    if (cmd == ".") {
      file_->startGroup();

      int start;

      if (input_data_.isCmd('a') || input_data_.isCmd('i'))
        start = input_data_.getStartLine();
      else
        start = input_data_.getStartLine() - 1;

      if (input_data_.isCmd('c')) {
        for (int i = input_data_.getStartLine(); i <= input_data_.getEndLine(); ++i)
          deleteLine(input_data_.getStartLine() - 1);
      }

      int i = start;

      StringList::const_iterator p1, p2;

      for (p1 = input_data_.getLines().begin(), p2 = input_data_.getLines().end(); p1 != p2; ++p1)
        addLine(i++, *p1);

      file_->endGroup();

      mode_ = COMMAND;

      input_data_.clearLines();

      setPos(CIPoint2D(0, i));
    }
    else
      input_data_.addLine(cmd);

    return true;
  }

  //----------

  COptValT<int> line_num1, line_num2;
  COptValT<int> char_num1, char_num2;

  CStrParse parse(cmd);

  parse.skipSpace();

  // read range start
  if      (parse.isChar('+')) {
    parse.skipChar();

    int d = 0;

    if (parse.isDigit())
      parse.readInteger(&d);
    else {
      d = 1;

      while (parse.isChar('+')) {
        ++d;

        parse.skipChar();
      }
    }

    line_num1 = getRow() + d + 1;
  }
  else if (parse.isChar('-') || parse.isChar('^')) {
    parse.skipChar();

    int d = 0;

    if (parse.isDigit())
      parse.readInteger(&d);
    else {
      d = 1;

      while (parse.isChar('-')) {
        ++d;

        parse.skipChar();
      }
    }

    line_num1 = getRow() - d + 1;
  }
  else if (parse.isDigit()) {
    int i;

    parse.readInteger(&i);

    line_num1 = i;
  }
  else if (parse.isChar('.')) {
    parse.skipChar();

    line_num1 = getRow() + 1;

    // offset
    if (parse.isChar('+') || parse.isChar('-')) {
      int s = (parse.isChar('+') ? 1 : -1);

      parse.skipChar();

      int d = 0;

      if (parse.isDigit())
        parse.readInteger(&d);

      line_num1 = line_num1.getValue() + d*s;
    }
  }
  else if (parse.isChar('$')) {
    parse.skipChar();

    line_num1 = file_->getNumLines();

    // offset
    if (parse.isChar('+') || parse.isChar('-')) {
      int s = (parse.isChar('+') ? 1 : -1);

      parse.skipChar();

      int d = 0;

      if (parse.isDigit())
        parse.readInteger(&d);

      line_num1 = line_num1.getValue() + d*s;
    }
  }
  else if (parse.isChar('\'')) {
    parse.skipChar();

    char c;

    if (! parse.readChar(&c))
      return true;

    uint line_num, char_num;

    if (! file_->getMarkPos(std::string(&c, 1), &line_num, &char_num)) {
      error("Mark not set");
      return false;
    }

    line_num1 = line_num + 1;
  }
  else if (parse.isChar('%')) {
    parse.skipChar();

    line_num1 = 1;
    line_num2 = file_->getNumLines();
  }
  else if (parse.isChar('/')) {
    std::string str;

    parse.skipChar();

    while (! parse.eof() && ! parse.isChar('/')) {
      if (parse.isChar('\\'))
        parse.skipChar();

      char c;

      if (parse.readChar(&c))
        str += c;
    }

    if (parse.eof() && ! getEx()) {
      error("Missing terminating '/'");
      return false;
    }

    parse.skipChar();

    int fline_num, fchar_num;

    if (findNext(str, &fline_num, &fchar_num)) {
      line_num1 = fline_num;
      char_num1 = fchar_num;
    }
  }
  else if (parse.isChar('?')) {
    std::string str;

    parse.skipChar();

    while (! parse.eof() && ! parse.isChar('?')) {
      if (parse.isChar('\\'))
        parse.skipChar();

      char c;

      if (parse.readChar(&c))
        str += c;
    }

    if (parse.eof() && ! getEx()) {
      error("Missing terminating '?'");
      return false;
    }

    parse.skipChar();

    int fline_num, fchar_num;

    if (findPrev(str, &fline_num, &fchar_num)) {
      line_num1 = fline_num;
      char_num1 = fchar_num;
    }
  }

  parse.skipSpace();

  if (! line_num2.isValid()) {
    // read range end (can be more than one value)
    while (parse.isChar(',') || parse.isChar(';')) {
      char c = parse.getCharAt();

      if (line_num2.isValid())
        line_num1 = line_num2;

      parse.skipChar();

      if (! line_num1.isValid())
        line_num1 = 1;

      parse.skipSpace();

      // + offset
      if      (parse.isChar('+')) {
        parse.skipChar();

        if (parse.isDigit()) {
          int i;

          parse.readInteger(&i);

          if (c == ';')
            line_num2 = line_num1.getValue() + i;
          else
            line_num2 = getRow() + i + 1;
        }
      }
      // - offset
      else if (parse.isChar('-')) {
        parse.skipChar();

        if (parse.isDigit()) {
          int i;

          parse.readInteger(&i);

          if (c == ';')
            line_num2 = line_num1.getValue() - i;
          else
            line_num2 = getRow() - i + 1;
        }
      }
      // absolute value
      else if (parse.isDigit()) {
        int i;

        parse.readInteger(&i);

        line_num2 = i;
      }
      // current
      else if (parse.isChar('.')) {
        parse.skipChar();

        line_num2 = getRow() + 1;

        // offset
        if (parse.isChar('+') || parse.isChar('-')) {
          int s = (parse.isChar('+') ? 1 : -1);

          parse.skipChar();

          int d = 0;

          if (parse.isDigit())
            parse.readInteger(&d);

          line_num2 = line_num2.getValue() + d*s;
        }
      }
      // last line
      else if (parse.isChar('$')) {
        parse.skipChar();

        line_num2 = file_->getNumLines();

        // offset
        if (parse.isChar('+') || parse.isChar('-')) {
          int s = (parse.isChar('+') ? 1 : -1);

          parse.skipChar();

          int d = 0;

          if (parse.isDigit())
            parse.readInteger(&d);

          line_num2 = line_num2.getValue() + d*s;
        }
      }
      else if (parse.isChar('\'')) {
        parse.skipChar();

        char c1;

        if (parse.readChar(&c1)) {
          uint line_num, char_num;

          if (! file_->getMarkPos(std::string(&c1, 1), &line_num, &char_num)) {
            error("Mark not set");
            return false;
          }

          line_num2 = line_num + 1;
        }
      }
      else if (parse.isChar('/')) {
        std::string str;

        parse.skipChar();

        while (! parse.eof() && ! parse.isChar('/')) {
          if (parse.isChar('\\'))
            parse.skipChar();

          char c1;

          if (parse.readChar(&c1))
            str += c1;
        }

        if (parse.eof() && ! getEx()) {
          error("Missing terminating '/'");
          return false;
        }

        parse.skipChar();

        CRegExp regexp(str);

        regexp.setCaseSensitive(case_sensitive_);

        uint fline_num, fchar_num;

        if (c == ';') {
          if (file_->findNext(regexp, line_num1.getValue() - 1, 0, &fline_num, &fchar_num)) {
            line_num2 = fline_num + 1;
            char_num2 = fchar_num;
          }
        }
        else {
          if (file_->findNext(regexp, getRow(), 0, &fline_num, &fchar_num)) {
            line_num2 = fline_num + 1;
            char_num2 = fchar_num + 1;
          }
        }
      }
      else if (parse.isChar('?')) {
        std::string str;

        parse.skipChar();

        while (! parse.eof() && ! parse.isChar('?')) {
          if (parse.isChar('\\'))
            parse.skipChar();

          char c1;

          if (parse.readChar(&c1))
            str += c1;
        }

        if (parse.eof() && ! getEx()) {
          error("Missing terminating '?'");
          return false;
        }

        parse.skipChar();
      }

      if (! line_num2.isValid())
        line_num2 = file_->getNumLines();

      parse.skipSpace();
    }
  }

  // read command char
  char c = '\0';

  if (! parse.readChar(&c))
    c = '\0';

  if      (c == '/') {
    // no start range so use current
    if (! line_num1.isValid()) {
      line_num1 = getRow() + 1;
      char_num1 = 0;
    }

    // no end range so use last line
    if (! line_num2.isValid()) {
      line_num2 = file_->getNumLines();
      char_num2 = 0;
    }
  }
  else if (c == '?') {
    // no start range so use current
    if (! line_num1.isValid()) {
      line_num1 = getRow() - 1;
      char_num1 = 0;
    }

    // no end range so use last line
    if (! line_num2.isValid()) {
      line_num2 = 1;
      char_num2 = 0;
    }
  }
  else {
    // no start range so use current
    if (! line_num1.isValid()) {
      line_num1 = getRow() + 1;
      char_num1 = 0;
    }

    // no end range so use start
    if (! line_num2.isValid()) {
      line_num2 = line_num1;
      char_num2 = char_num1;
    }
  }

  // ensure valid range
  if (line_num1.getValue() < 1 || line_num1.getValue() > (int) file_->getNumLines() + 1 ||
      line_num2.getValue() < 1 || line_num2.getValue() > (int) file_->getNumLines() + 1) {
    error("Invalid range");
    return false;
  }

  switch (c) {
    case 'a':   // (.)a - add line and enter input mode
    case 'c':   // (.,.)c - change line and enter input mode
    case 'i': { // (.)i - insert line and enter input mode
      mode_ = INPUT;

      input_data_.setStartLine(line_num1.getValue());
      input_data_.setEndLine  (line_num2.getValue());

      input_data_.setCmd(c);

      break;
    }
    case 'd': { // (.,.)d - delete line
      doDelete(line_num1.getValue(), line_num2.getValue());

      break;
    }
    case 'e': { // e [<file>|!<command>] - edit file
      if (! parse.isSpace() && ! parse.eof()) {
        std::string str;

        parse.readString(str);

        error("Not an editor command: e" + str);

        return false;
      }

      parse.skipSpace();

      std::string fileName;

      parse.readNonSpace(fileName);

      if (fileName.empty())
        fileName = file_->getFileName();

      file_->startGroup();

      if (! fileName.empty() && fileName[0] == '!') {
        CCommand cmd1(fileName.substr(1));

        std::string dest;

        cmd1.addStringDest(dest);

        cmd1.start();

        std::vector<std::string> lines;

        file_->deleteAllLines();

        CStrUtil::addLines(dest, lines);

        int numLines = lines.size();

        for (int l = 0; l < numLines; ++l)
          addLine(l, lines[l]);
      }
      else {
        if (! fileName.empty())
          file_->loadLines(fileName);
      }

      setPos(CIPoint2D(0, file_->getNumLines() - 1));

      file_->endGroup();

      break;
    }
    case 'E': { // e [<file>|!<command>] - force edit file
      error("E: Unimplemented");
      break;
    }
    case 'f': { // f <file> - set default filename
      parse.skipSpace();

      std::string fileName;

      parse.readNonSpace(fileName);

      if (! fileName.empty())
        file_->setFileName(fileName);

      break;
    }
    case 'g': { // (.,.)g/<regexp>/<cmd>... - apply cmds to matching lines
      // read separator char
      char sep;

      if (! parse.readChar(&sep)) {
        error("Regular expression missing from global");
        return false;
      }

      // get regexp
      std::string find;

      while (! parse.eof() && ! parse.isChar(sep)) {
        char c1;

        parse.readChar(&c1);

        find += c1;
      }

      // use previous find if empty
      if (find.empty())
        find = file_->getFindPattern().getPattern();

      if (parse.isChar(sep))
        parse.skipChar();

      // get command
      std::string cmd1 = "p";

      while (! parse.eof()) {
        parse.readString(cmd1);

        parse.skipSpace();
      }

      if (! parse.eof()) {
        error("Trailing characters");
        return false;
      }

      doGlob(line_num1.getValue(), line_num2.getValue(), find, cmd1);

      break;
    }
    case 'G': { // (.,.)G/<regexp>/ - edit each matching line
      error("G: Unimplemented");
      break;
    }
    case 'H': { // H - toggle help
      error("H: Unimplemented");
      break;
    }
    case 'h': { // h - display help
      error("h: Unimplemented");
      break;
    }
    case 'j': { // (.,.)j - join lines
      doJoin(line_num1.getValue(), line_num2.getValue());

      break;
    }
    case 'k': { // (.)k<c> - mark line with char
      char c1;

      if (! parse.readChar(&c1))
        return false;

      doMark(line_num1.getValue(), c1);

      break;
    }
    case 'l': { // (.,.)l - print lines unambiguously ($ at end)
      doPrint(line_num1.getValue(), line_num2.getValue(), /*numbered*/false, /*eol*/true);

      break;
    }
    case 'm': { // (.,.)m(.) - move lines
      COptValT<int> line_num3;

      parse.skipSpace();

      // read dest
      if      (parse.isDigit()) {
        int i;

        parse.readInteger(&i);

        line_num3 = i;
      }
      else if (parse.isChar('.')) {
        parse.skipChar();

        line_num3 = getRow() + 1;
      }
      else if (parse.isChar('$')) {
        parse.skipChar();

        line_num3 = file_->getNumLines();
      }

      parse.skipSpace();

      if (! line_num3.isValid())
        line_num3 = getRow() + 1;

      doMove(line_num1.getValue(), line_num2.getValue(), line_num3.getValue());

      break;
    }
    case 'n': { // (.,.)n - print numbered lines
      doPrint(line_num1.getValue(), line_num2.getValue(), /*numbered*/true, /*eol*/false);

      break;
    }
    case 'p': { // (.,.)p - print lines
      doPrint(line_num1.getValue(), line_num2.getValue(), /*numbered*/false, /*eol*/false);

      setPos(CIPoint2D(0, line_num2.getValue() - 1));

      break;
    }
    case 'P': { // P - toggle prompt
      error("P: Unimplemented");
      break;
    }
    case 'q': { // q - quit
      edNotifyQuit(false);

      break;
    }
    case 'Q': { // Q - force quit
      edNotifyQuit(true);

      break;
    }
    case 'r': { // (.)r [<file>|!<command>] - read file after line
      parse.skipSpace();

      std::string fileName;

      parse.readNonSpace(fileName);

      if (fileName.empty())
        fileName = file_->getFileName();

      file_->startGroup();

      if (! fileName.empty() && fileName[0] == '!') {
        CCommand cmd1(fileName.substr(1));

        std::string dest;

        cmd1.addStringDest(dest);

        cmd1.start();

        std::vector<std::string> lines;

        CStrUtil::addLines(dest, lines);

        int numLines = lines.size();

        for (int l = 0; l < numLines; ++l)
          addLine(line_num1.getValue() + l, lines[l]);
      }
      else {
        if (! fileName.empty())
          file_->addFileLines(fileName, line_num1.getValue());
      }

      // TODO: fix line pos after read
      setPos(CIPoint2D(0, file_->getNumLines() - 1));

      file_->endGroup();

      break;
    }
    case 's': { // (.,.)s[/<regexp>/<replace>/[g|<n>]] - substitute
      // read separator char
      char sep;

      if (! parse.readChar(&sep))
        return false;

      // get regexp
      std::string find, replace;

      while (! parse.eof() && ! parse.isChar(sep)) {
        char c1;

        parse.readChar(&c1);

        find += c1;
      }

      // use previous find if empty
      if (find.empty())
        find = file_->getFindPattern().getPattern();

      if (parse.isChar(sep))
        parse.skipChar();

      // get replace string
      while (! parse.eof() && ! parse.isChar(sep)) {
        char c1;

        parse.readChar(&c1);

        replace += c1;
      }

      if (parse.isChar(sep))
        parse.skipChar();

      // get optional modifier char
      // TODO: support <n>
      char mod = '\0';

      if (! parse.eof()) {
        parse.readChar(&mod);

        if (mod != 'g')
          return false;
      }

      parse.skipSpace();

      if (! parse.eof())
        return false;

      doSubstitute(line_num1.getValue(), line_num2.getValue(), find, replace, mod);

      break;
    }
    case 't': { // (.,.)t(.) - copy lines
      COptValT<int> line_num3;

      parse.skipSpace();

      // read dest
      if      (parse.isDigit()) {
        int i;

        parse.readInteger(&i);

        line_num3 = i;
      }
      else if (parse.isChar('.')) {
        parse.skipChar();

        line_num3 = getRow() + 1;
      }
      else if (parse.isChar('$')) {
        parse.skipChar();

        line_num3 = file_->getNumLines();
      }

      parse.skipSpace();

      if (! line_num3.isValid())
        line_num3 = getRow() + 1;

      doCopy(line_num1.getValue(), line_num2.getValue(), line_num3.getValue());

      break;
    }
    case 'u': { // u - undo
      doUndo();

      break;
    }
    case 'v': { // (.,.)v/<regexp>/<cmd>... - apply cmds to non-matching lines
      error("v: Unimplemented");
      break;
    }
    case 'V': { // (.,.)V/<regexp>/ - edit each non-matching line
      error("V: Unimplemented");
      break;
    }
    case 'w': { // (.,.)w[q] [<file>|!<command>] - write to file
      bool quit = false;

      if (parse.isChar('q')) {
        parse.skipChar();

        quit = true;
      }

      parse.skipSpace();

      std::string fileName;

      parse.readNonSpace(fileName);

      if (fileName.empty())
        fileName = file_->getFileName();

      if (! fileName.empty() && fileName[0] == '!') {
        error("w!: Unimplemented");
        break;
      }
      else {
        if (! fileName.empty())
          file_->saveLines(fileName);
      }

      if (quit)
        edNotifyQuit(false);

      break;
    }
    case 'W': { // (.,.)W[q] [<file>|!<command>] - append to file
      error("W: Unimplemented");
      break;
    }
    case 'x': { // (.)x - paste cut buffer
      doPaste(line_num1.getValue());

      break;
    }
    case 'y': { // (.,.)y - copy to buffer
      doCopy(line_num1.getValue(), line_num2.getValue());

      break;
    }
    case 'z': { // (.)z[<n>] - scroll to line
      error("z: Unimplemented");
      break;
    }
    case '!': { // ![!]<command> - execute command
      std::string str = parse.getAt();

      doExecute(line_num1.getValue(), line_num2.getValue(), str);

      break;
    }
    case '#': { // (.,.)# - comment
      break;
    }
    case '=': { // (.)= - print line number
      output(CStrUtil::toString(line_num1.getValue()));

      break;
    }
    case '/': { // / - find next
      // get regexp
      std::string find;

      while (! parse.eof() && ! parse.isChar('/')) {
        char c1;

        parse.readChar(&c1);

        find += c1;
      }

      doFindNext(line_num1.getValue(), line_num2.getValue(), find);

      break;
    }
    case '?': { // / - find prev
      // get regexp
      std::string find;

      while (! parse.eof() && ! parse.isChar('/')) {
        char c1;

        parse.readChar(&c1);

        find += c1;
      }

      doFindPrev(line_num1.getValue(), line_num2.getValue(), find);

      break;
    }
    case '@': { // @ - output text
      std::string str = parse.getAt();

      output(str);

      break;
    }
    case '\0': { // move to line
      if (! getEx())
        doPrint(line_num1.getValue(), line_num2.getValue(), /*numbered*/false, /*eol*/false);

      if (! char_num2.isValid())
        char_num2.setValue(0);

      setPos(CIPoint2D(char_num2.getValue(), line_num2.getValue() - 1));

      break;
    }
    default:
      return false;
  }

  return true;
}

bool
CEd::
findNext(const std::string &str, int *line_num, int *char_num)
{
  CRegExp regexp(str);

  regexp.setCaseSensitive(case_sensitive_);

  uint fline_num, fchar_num;

  uint num_lines = file_->getNumLines();

  if (! getEx() && getRow() >= num_lines) {
    if (file_->findNext(regexp, 0, 0, num_lines - 1, -1, &fline_num, &fchar_num)) {
      *line_num = fline_num + 1;
      *char_num = fchar_num;
      return true;
    }

    return false;
  }

  int row1, col1, row2, col2;

  if (getEx()) {
    row1 = getRow();
    col1 = getCol() + 1;
    row2 = getRow();
    col2 = getCol() - 1;
  }
  else {
    row1 = getRow() + 1;
    col1 = 0;
    row2 = getRow();
    col2 = -1;
  }

  if (file_->findNext(regexp, row1, col1, &fline_num, &fchar_num) ||
      file_->findNext(regexp, 0, 0, row2, col2, &fline_num, &fchar_num)) {
    *line_num = fline_num + 1;
    *char_num = fchar_num;
    return true;
  }

  return false;
}

bool
CEd::
findPrev(const std::string &str, int *line_num, int *char_num)
{
  CRegExp regexp(str);

  regexp.setCaseSensitive(case_sensitive_);

  uint fline_num, fchar_num;

  uint num_lines = file_->getNumLines();

  if (! getEx() && getRow() == 0) {
    if (file_->findPrev(regexp, num_lines - 1, -1, 0, 0, &fline_num, &fchar_num)) {
      *line_num = fline_num + 1;
      *char_num = fchar_num;
      return true;
    }

    return false;
  }

  int row1, col1, row2, col2;

  if (getEx()) {
    row1 = getRow();
    col1 = getCol() - 1;
    row2 = getRow();
    col2 = getCol() + 1;
  }
  else {
    row1 = getRow() - 1;
    col1 = -1;
    row1 = getRow();
    col1 = 0;
  }

  if (file_->findPrev(regexp, row1, col1, &fline_num, &fchar_num) ||
      file_->findPrev(regexp, num_lines - 1, -1, row2, col2, &fline_num, &fchar_num)) {
    *line_num = fline_num + 1;
    *char_num = fchar_num;
    return true;
  }

  return false;
}

void
CEd::
doSubstitute(int line_num1, int line_num2, const std::string &find,
             const std::string &replace, char mod)
{
  bool global = (mod == 'g');

  file_->startGroup();

  CRegExp regexp(find);

  regexp.setCaseSensitive(case_sensitive_);

  for (int i = line_num1; i <= line_num2; ++i) {
    uint fline_num, fchar_num, len;

    if (file_->findNext(regexp, i - 1, 0, i - 1, -1, &fline_num, &fchar_num, &len)) {
      int spos = fchar_num;
      int epos = spos + len - 1;

      file_->replace(i - 1, spos, epos, replace);

      if (global) {
        while (file_->findNext(regexp, i - 1, epos + 1, i - 1, -1, &fline_num, &fchar_num, &len)) {
          int spos1 = fchar_num;
          int epos1 = spos + len - 1;

          file_->replace(i - 1, spos1, epos1, replace);
        }
      }
    }
  }

  file_->endGroup();
}

void
CEd::
doFindNext(int line_num1, int line_num2, const std::string &find)
{
  CRegExp regexp(find);

  regexp.setCaseSensitive(case_sensitive_);

  file_->findNext(regexp, line_num1 - 1, 0, line_num2 - 1, -1);
}

void
CEd::
doFindPrev(int line_num1, int line_num2, const std::string &find)
{
  CRegExp regexp(find);

  regexp.setCaseSensitive(case_sensitive_);

  file_->findPrev(regexp, line_num1 - 1, -1, line_num2 - 1, 0);
}

void
CEd::
doGlob(int line_num1, int line_num2, const std::string &find, const std::string &cmd)
{
  file_->startGroup();

  CRegExp regexp(find);

  regexp.setCaseSensitive(case_sensitive_);

  for (int i = line_num1; i <= line_num2; ++i) {
    const CEditLine *line = file_->getEditLine(i - 1);

    if (! line->findNext(regexp))
      continue;

    if      (cmd == "d")
      deleteLine(--i);
    else if (cmd == "p")
      output(line->getString());
    else {
      error("Not an editor command: " + cmd);
      break;
    }
  }

  file_->endGroup();
}

void
CEd::
doJoin(int line_num1, int line_num2)
{
  file_->startGroup();

  for (int i = line_num1; i < line_num2; ++i)
    file_->joinLine(line_num1 - 1);

  file_->endGroup();
}

void
CEd::
doMove(int line_num1, int line_num2, int line_num3)
{
  file_->startGroup();

  if      (line_num3 < line_num1) {
    for (int i = line_num2; i >= line_num1; --i)
      file_->moveLine(i - 1, line_num3 - 1);
  }
  else if (line_num3 > line_num2) {
    for (int i = line_num1; i <= line_num2; ++i)
      file_->moveLine(line_num1 - 1, line_num3 - 1);
  }

  file_->endGroup();
}

void
CEd::
doCopy(int line_num1, int line_num2, int line_num3)
{
  file_->startGroup();

  if      (line_num3 < line_num1) {
    for (int i = line_num2; i >= line_num1; --i)
      file_->copyLine(line_num2 - 1, line_num3 - 1);
  }
  else if (line_num3 > line_num2) {
    for (int i = line_num2; i >= line_num1; --i)
      file_->copyLine(i - 1, line_num3 - 1);
  }

  file_->endGroup();
}

void
CEd::
doDelete(int line_num1, int line_num2)
{
  file_->startGroup();

  for (int i = line_num1; i <= line_num2; ++i)
    deleteLine(line_num1 - 1);

  file_->endGroup();

  setPos(CIPoint2D(0, line_num1 - 1));
}

void
CEd::
doCopy(int /*line_num1*/, int /*line_num2*/)
{
}

void
CEd::
doPaste(int /*line_num1*/)
{
}

void
CEd::
doUndo()
{
  file_->undo();
}

void
CEd::
doMark(int i, char c)
{
  file_->setMarkPos(std::string(&c, 1), i - 1, 0);
}

void
CEd::
doExecute(int line_num1, int line_num2, const std::string &cmdStr)
{
  file_->startGroup();

  std::string src;

  for (int i = line_num1; i <= line_num2; ++i) {
    if (i > line_num1) src += "\n";

    src += file_->getLine(line_num1 - 1);

    deleteLine(line_num1 - 1);
  }

  CCommand cmd(cmdStr);

  std::string dest;

  cmd.addStringSrc (src);
  cmd.addStringDest(dest);

  cmd.start();

  std::vector<std::string> lines;

  CStrUtil::addLines(dest, lines);

  int numLines = lines.size();

  for (int l = 0; l < numLines; ++l)
    addLine(line_num1 + l - 1, lines[l]);

  file_->endGroup();
}

void
CEd::
doPrint(int line_num1, int line_num2, bool numbered, bool eol)
{
  for (int i = line_num1; i <= line_num2; ++i) {
    std::string line = file_->getLine(i - 1);

    std::string str;

    if (numbered) {
      str += CStrUtil::toString(i);
      str += "\t";
    }

    str += line;

    if (eol)
      str += "$";

    output(str);
  }
}

void
CEd::
output(const std::string &msg)
{
  std::cout << msg << std::endl;
}

void
CEd::
error(const std::string &msg)
{
  std::cerr << msg << std::endl;
}

void
CEd::
addLine(uint row, const std::string &line)
{
  file_->addLine(row, line);
}

void
CEd::
deleteLine(uint row)
{
  file_->deleteLine(row);
}

void
CEd::
setPos(const CIPoint2D &p)
{
  file_->setPos(p);
}

uint
CEd::
getRow() const
{
  return file_->getRow();
}

uint
CEd::
getCol() const
{
  return file_->getCol();
}

void
CEd::
edNotifyQuit(bool /*force*/)
{
  quit_ = true;
}
