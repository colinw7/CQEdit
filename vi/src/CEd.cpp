#include <CEd.h>
#include <CVi.h>
#include <CFile.h>
#include <CRegExp.h>
#include <CStrUtil.h>
#include <CStrParse.h>
#include <CCommand.h>

#include <optional>
#include <iostream>

namespace CVi {

Ed::
Ed(CVi::App *app) :
 app_(app)
{
}

Ed::
~Ed()
{
}

void
Ed::
init()
{
  // move to first character on last line
  setPos(0, std::max(0, int(app_->getNumLines()) - 1));
}

bool
Ed::
execFile(const std::string &fileName)
{
  std::vector<std::string> lines;

  CFile file(fileName);

  if (! file.exists() || ! file.isRegular())
    return false;

  file.toLines(lines);

  std::vector<std::string>::const_iterator p1, p2;

  for (const auto &line : lines) {
    auto line1 = CStrUtil::stripSpaces(line);

    if (line1.empty() || line1[0] == '#')
      continue;

    execCmd(line1);
  }

  return true;
}

bool
Ed::
execCmd(const std::string &cmd)
{
  if (mode_ == INPUT) {
    if (cmd == ".") {
      app_->startGroup();

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

      for (const auto &line : input_data_.getLines())
        addLine(i++, line);

      app_->endGroup();

      mode_ = COMMAND;

      input_data_.clearLines();

      setPos(0, i);
    }
    else
      input_data_.addLine(cmd);

    return true;
  }

  //----------

  std::optional<int> line_num1, line_num2;
  std::optional<int> char_num1, char_num2;

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

      line_num1 = line_num1.value() + d*s;
    }
  }
  else if (parse.isChar('$')) {
    parse.skipChar();

    line_num1 = app_->getNumLines();

    // offset
    if (parse.isChar('+') || parse.isChar('-')) {
      int s = (parse.isChar('+') ? 1 : -1);

      parse.skipChar();

      int d = 0;

      if (parse.isDigit())
        parse.readInteger(&d);

      line_num1 = line_num1.value() + d*s;
    }
  }
  else if (parse.isChar('\'')) {
    parse.skipChar();

    char c;

    if (! parse.readChar(&c))
      return true;

    uint line_num, char_num;

    if (! app_->getMarkPos(std::string(&c, 1), &line_num, &char_num)) {
      error("Mark not set");
      return false;
    }

    line_num1 = line_num + 1;
  }
  else if (parse.isChar('%')) {
    parse.skipChar();

    line_num1 = 1;
    line_num2 = app_->getNumLines();
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

    if (str == "")
      str = app_->getFindPattern().getPattern();

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

    if (str == "")
      str = app_->getFindPattern().getPattern();

    int fline_num, fchar_num;

    if (findPrev(str, &fline_num, &fchar_num)) {
      line_num1 = fline_num;
      char_num1 = fchar_num;
    }
  }
  else if (parse.isAlpha()) {
    auto readWord = [&](std::string &word) {
      word = "";

      if (! parse.isAlpha())
        return false;

      while (! parse.eof() && (parse.isAlnum() || parse.isChar('_'))) {
        char c;

        if (parse.readChar(&c))
          word += c;
      }

      return true;
    };

    auto pos = parse.getPos();

    std::string cmd1;
    (void) readWord(cmd1);

    if      (cmd1 == "set") {
      parse.skipSpace();

      std::string name;
      (void) readWord(name);

      parse.skipSpace();

      std::string value;

      if      (parse.isChar('=')) {
        parse.skipChar();

        parse.skipSpace();

        while (! parse.eof()) {
          char c;

          if (parse.readChar(&c))
            value += c;
        }
      }
      else if (parse.isChar('?')) {
        error("'set option?' not implemented");
        return false;
      }
      else {
        (void) readWord(value);

        if (value == "all") {
          error("'set all' not implemented");
          return false;
        }

        if (value == "")
          value = "1";
      }

      app_->setNameValue(name, value);

      return true;
    }
    else if (cmd1 == "map" || cmd1 == "unmap") {
      error("Map not implemented");
      return false;
    }
    else if (cmd1 == "ab") {
      error("Abreviations not implemented");
      return false;
    }

    parse.setPos(pos);
  }

  parse.skipSpace();

  if (! line_num2) {
    // read range end (can be more than one value)
    while (parse.isChar(',') || parse.isChar(';')) {
      char c = parse.getCharAt();

      if (line_num2)
        line_num1 = line_num2;

      parse.skipChar();

      if (! line_num1)
        line_num1 = 1;

      parse.skipSpace();

      // + offset
      if      (parse.isChar('+')) {
        parse.skipChar();

        if (parse.isDigit()) {
          int i;

          parse.readInteger(&i);

          if (c == ';')
            line_num2 = line_num1.value() + i;
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
            line_num2 = line_num1.value() - i;
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

          line_num2 = line_num2.value() + d*s;
        }
      }
      // last line
      else if (parse.isChar('$')) {
        parse.skipChar();

        line_num2 = app_->getNumLines();

        // offset
        if (parse.isChar('+') || parse.isChar('-')) {
          int s = (parse.isChar('+') ? 1 : -1);

          parse.skipChar();

          int d = 0;

          if (parse.isDigit())
            parse.readInteger(&d);

          line_num2 = line_num2.value() + d*s;
        }
      }
      else if (parse.isChar('\'')) {
        parse.skipChar();

        char c1;

        if (parse.readChar(&c1)) {
          uint line_num, char_num;

          if (! app_->getMarkPos(std::string(&c1, 1), &line_num, &char_num)) {
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

        regexp.setCaseSensitive(getCaseSensitive());

        uint fline_num, fchar_num;

        if (c == ';') {
          if (app_->findNext(regexp, line_num1.value() - 1, 0, &fline_num, &fchar_num)) {
            line_num2 = fline_num + 1;
            char_num2 = fchar_num;
          }
        }
        else {
          if (app_->findNext(regexp, getRow(), 0, &fline_num, &fchar_num)) {
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

      if (! line_num2)
        line_num2 = app_->getNumLines();

      parse.skipSpace();
    }
  }

  // read command char
  char c = '\0';

  if (! parse.readChar(&c))
    c = '\0';

  if      (c == '/') {
    // no start range so use current
    if (! line_num1) {
      line_num1 = getRow() + 1;
      char_num1 = 0;
    }

    // no end range so use last line
    if (! line_num2) {
      line_num2 = app_->getNumLines();
      char_num2 = 0;
    }
  }
  else if (c == '?') {
    // no start range so use current
    if (! line_num1) {
      line_num1 = getRow() - 1;
      char_num1 = 0;
    }

    // no end range so use last line
    if (! line_num2) {
      line_num2 = 1;
      char_num2 = 0;
    }
  }
  else {
    // no start range so use current
    if (! line_num1) {
      line_num1 = getRow() + 1;
      char_num1 = 0;
    }

    // no end range so use start
    if (! line_num2) {
      line_num2 = line_num1;
      char_num2 = char_num1;
    }
  }

  // ensure valid range
  if (line_num1.value() < 1 || line_num1.value() > int(app_->getNumLines()) + 1 ||
      line_num2.value() < 1 || line_num2.value() > int(app_->getNumLines()) + 1) {
    error("Invalid range");
    return false;
  }

  switch (c) {
    case 'a':   // (.)a - add line and enter input mode
    case 'c':   // (.,.)c - change line and enter input mode
    case 'i': { // (.)i - insert line and enter input mode
      mode_ = INPUT;

      input_data_.setStartLine(line_num1.value());
      input_data_.setEndLine  (line_num2.value());

      input_data_.setCmd(c);

      break;
    }
    case 'd': { // (.,.)d - delete line
      doDelete(line_num1.value(), line_num2.value());

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
        fileName = app_->getFileName();

      app_->startGroup();

      if (! fileName.empty() && fileName[0] == '!') {
        CCommand cmd1(fileName.substr(1));

        std::string dest;

        cmd1.addStringDest(dest);

        cmd1.start();

        std::vector<std::string> lines;

        app_->deleteAllLines();

        CStrUtil::addLines(dest, lines);

        int numLines = int(lines.size());

        for (int l = 0; l < numLines; ++l)
          addLine(l, lines[l]);
      }
      else {
        if (! fileName.empty())
          app_->loadLines(fileName);
      }

      setPos(0, app_->getNumLines() - 1);

      app_->endGroup();

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
        app_->setFileName(fileName);

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
        find = app_->getFindPattern().getPattern();

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

      doGlob(line_num1.value(), line_num2.value(), find, cmd1);

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
      doJoin(line_num1.value(), line_num2.value());

      break;
    }
    case 'k': { // (.)k<c> - mark line with char
      char c1;

      if (! parse.readChar(&c1))
        return false;

      doMark(line_num1.value(), c1);

      break;
    }
    case 'l': { // (.,.)l - print lines unambiguously ($ at end)
      doPrint(line_num1.value(), line_num2.value(), /*numbered*/false, /*eol*/true);

      break;
    }
    case 'm': { // (.,.)m(.) - move lines
      std::optional<int> line_num3;

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

        line_num3 = app_->getNumLines();
      }

      parse.skipSpace();

      if (! line_num3)
        line_num3 = getRow() + 1;

      doMove(line_num1.value(), line_num2.value(), line_num3.value());

      break;
    }
    case 'n': { // (.,.)n - print numbered lines
      doPrint(line_num1.value(), line_num2.value(), /*numbered*/true, /*eol*/false);

      break;
    }
    case 'p': { // (.,.)p - print lines
      doPrint(line_num1.value(), line_num2.value(), /*numbered*/false, /*eol*/false);

      setPos(0, line_num2.value() - 1);

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
        fileName = app_->getFileName();

      app_->startGroup();

      if (! fileName.empty() && fileName[0] == '!') {
        CCommand cmd1(fileName.substr(1));

        std::string dest;

        cmd1.addStringDest(dest);

        cmd1.start();

        std::vector<std::string> lines;

        CStrUtil::addLines(dest, lines);

        int numLines = int(lines.size());

        for (int l = 0; l < numLines; ++l)
          addLine(line_num1.value() + l, lines[l]);
      }
      else {
        if (! fileName.empty())
          app_->addFileLines(fileName, line_num1.value());
      }

      // TODO: fix line pos after read
      setPos(0, app_->getNumLines() - 1);

      app_->endGroup();

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
        find = app_->getFindPattern().getPattern();

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

      doSubstitute(line_num1.value(), line_num2.value(), find, replace, mod);

      break;
    }
    case 't': { // (.,.)t(.) - copy lines
      std::optional<int> line_num3;

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

        line_num3 = app_->getNumLines();
      }

      parse.skipSpace();

      if (! line_num3)
        line_num3 = getRow() + 1;

      doCopy(line_num1.value(), line_num2.value(), line_num3.value());

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
        fileName = app_->getFileName();

      if (! fileName.empty() && fileName[0] == '!') {
        error("w!: Unimplemented");
        break;
      }
      else {
        if (! fileName.empty())
          app_->saveLines(fileName);
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
      doPaste(line_num1.value());

      break;
    }
    case 'y': { // (.,.)y - copy to buffer
      doCopy(line_num1.value(), line_num2.value());

      break;
    }
    case 'z': { // (.)z[<n>] - scroll to line
      error("z: Unimplemented");
      break;
    }
    case '!': { // ![!]<command> - execute command
      std::string str = parse.getAt();

      doExecute(line_num1.value(), line_num2.value(), str);

      break;
    }
    case '#': { // (.,.)# - comment
      break;
    }
    case '=': { // (.)= - print line number
      output(CStrUtil::toString(line_num1.value()));

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

      doFindNext(line_num1.value(), line_num2.value(), find);

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

      doFindPrev(line_num1.value(), line_num2.value(), find);

      break;
    }
    case '@': { // @ - output text
      std::string str = parse.getAt();

      output(str);

      break;
    }
    case '\0': { // move to line
      if (! getEx())
        doPrint(line_num1.value(), line_num2.value(), /*numbered*/false, /*eol*/false);

      if (! char_num2)
        char_num2 = 0;

      setPos(char_num2.value(), line_num2.value() - 1);

      break;
    }
    default:
      return false;
  }

  return true;
}

bool
Ed::
findNext(const std::string &str, int *line_num, int *char_num)
{
  CRegExp regexp(str);

  regexp.setCaseSensitive(getCaseSensitive());

  uint fline_num, fchar_num;

  uint num_lines = app_->getNumLines();

  if (! getEx() && getRow() >= num_lines) {
    if (app_->findNext(regexp, 0, 0, num_lines - 1, -1, &fline_num, &fchar_num)) {
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

  if (app_->findNext(regexp, row1, col1, &fline_num, &fchar_num) ||
      app_->findNext(regexp, 0, 0, row2, col2, &fline_num, &fchar_num)) {
    *line_num = fline_num + 1;
    *char_num = fchar_num;
    return true;
  }

  return false;
}

bool
Ed::
findPrev(const std::string &str, int *line_num, int *char_num)
{
  CRegExp regexp(str);

  regexp.setCaseSensitive(getCaseSensitive());

  uint fline_num, fchar_num;

  uint num_lines = app_->getNumLines();

  if (! getEx() && getRow() == 0) {
    if (app_->findPrev(regexp, num_lines - 1, -1, 0, 0, &fline_num, &fchar_num)) {
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
    row2 = getRow();
    col2 = 0;
  }

  if (app_->findPrev(regexp, row1, col1, &fline_num, &fchar_num) ||
      app_->findPrev(regexp, num_lines - 1, -1, row2, col2, &fline_num, &fchar_num)) {
    *line_num = fline_num + 1;
    *char_num = fchar_num;
    return true;
  }

  return false;
}

void
Ed::
doSubstitute(int line_num1, int line_num2, const std::string &find,
             const std::string &replace, char mod)
{
  bool global = (mod == 'g');

  app_->startGroup();

  CRegExp regexp(find);

  regexp.setCaseSensitive(getCaseSensitive());

  for (int i = line_num1; i <= line_num2; ++i) {
    uint fline_num, fchar_num, len;

    if (app_->findNext(regexp, i - 1, 0, i - 1, -1, &fline_num, &fchar_num, &len)) {
      int spos = fchar_num;
      int epos = spos + len - 1;

      app_->replace(i - 1, spos, epos, replace);

      if (global) {
        while (app_->findNext(regexp, i - 1, epos + 1, i - 1, -1, &fline_num, &fchar_num, &len)) {
          int spos1 = fchar_num;
          int epos1 = spos + len - 1;

          app_->replace(i - 1, spos1, epos1, replace);
        }
      }
    }
  }

  app_->endGroup();
}

void
Ed::
doFindNext(int line_num1, int line_num2, const std::string &find)
{
  CRegExp regexp(find);

  regexp.setCaseSensitive(getCaseSensitive());

  app_->findNext(regexp, line_num1 - 1, 0, line_num2 - 1, -1);
}

void
Ed::
doFindPrev(int line_num1, int line_num2, const std::string &find)
{
  CRegExp regexp(find);

  regexp.setCaseSensitive(getCaseSensitive());

  app_->findPrev(regexp, line_num1 - 1, -1, line_num2 - 1, 0);
}

void
Ed::
doGlob(int line_num1, int line_num2, const std::string &find, const std::string &cmd)
{
  app_->startGroup();

  CRegExp regexp(find);

  regexp.setCaseSensitive(getCaseSensitive());

  for (int i = line_num1; i <= line_num2; ++i) {
    auto *line = app_->getLine(i - 1);

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

  app_->endGroup();
}

void
Ed::
doJoin(int line_num1, int line_num2)
{
  app_->startGroup();

  for (int i = line_num1; i < line_num2; ++i)
    app_->joinLine(line_num1 - 1);

  app_->endGroup();
}

void
Ed::
doMove(int line_num1, int line_num2, int line_num3)
{
  app_->startGroup();

  if      (line_num3 < line_num1) {
    for (int i = line_num2; i >= line_num1; --i)
      app_->moveLine(i - 1, line_num3 - 1);
  }
  else if (line_num3 > line_num2) {
    for (int i = line_num1; i <= line_num2; ++i)
      app_->moveLine(line_num1 - 1, line_num3 - 1);
  }

  app_->endGroup();
}

void
Ed::
doCopy(int line_num1, int line_num2, int line_num3)
{
  app_->startGroup();

  if      (line_num3 < line_num1) {
    for (int i = line_num2; i >= line_num1; --i)
      app_->copyLine(line_num2 - 1, line_num3 - 1);
  }
  else if (line_num3 > line_num2) {
    for (int i = line_num2; i >= line_num1; --i)
      app_->copyLine(i - 1, line_num3 - 1);
  }

  app_->endGroup();
}

void
Ed::
doDelete(int line_num1, int line_num2)
{
  app_->startGroup();

  for (int i = line_num1; i <= line_num2; ++i)
    deleteLine(line_num1 - 1);

  app_->endGroup();

  setPos(0, line_num1 - 1);
}

void
Ed::
doCopy(int /*line_num1*/, int /*line_num2*/)
{
}

void
Ed::
doPaste(int /*line_num1*/)
{
}

void
Ed::
doUndo()
{
  app_->undo();
}

void
Ed::
doMark(int i, char c)
{
  app_->setMarkPos(std::string(&c, 1), i - 1, 0);
}

void
Ed::
doExecute(int line_num1, int line_num2, const std::string &cmdStr)
{
  app_->startGroup();

  std::string src;

  for (int i = line_num1; i <= line_num2; ++i) {
    src += app_->getLine(line_num1 - 1)->getString() + "\n";

    deleteLine(line_num1 - 1);
  }

  CCommand cmd(cmdStr);

  std::string dest;

  cmd.addStringSrc (src);
  cmd.addStringDest(dest);

  cmd.start();

  cmd.wait();

  std::vector<std::string> lines;

  CStrUtil::addLines(dest, lines);

  int numLines = int(lines.size());

  for (int l = 0; l < numLines; ++l)
    addLine(line_num1 + l - 1, lines[l]);

  app_->setPos(0, line_num1 + numLines - 1);

  app_->endGroup();
}

void
Ed::
doPrint(int line_num1, int line_num2, bool numbered, bool eol)
{
  for (int i = line_num1; i <= line_num2; ++i) {
    std::string line = app_->getLine(i - 1)->getString();

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
Ed::
output(const std::string &msg)
{
  std::cout << msg << std::endl;
}

void
Ed::
error(const std::string &msg)
{
  std::cerr << msg << std::endl;
}

void
Ed::
addLine(uint row, const std::string &line)
{
  app_->addLine(row, line);
}

void
Ed::
deleteLine(uint row)
{
  app_->deleteLine(row);
}

void
Ed::
setPos(uint x, uint y)
{
  app_->setPos(x, y);
}

uint
Ed::
getRow() const
{
  return app_->getRow();
}

uint
Ed::
getCol() const
{
  return app_->getCol();
}

void
Ed::
edNotifyQuit(bool /*force*/)
{
  quit_ = true;
}

}
