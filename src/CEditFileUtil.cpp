#include <CEditFileUtil.h>
#include <CEditFile.h>
#include <CEditLine.h>

CEditFileUtil::
CEditFileUtil(CEditFile *file) :
 file_(file)
{
}

void
CEditFileUtil::
deleteWord()
{
  deleteWord(file_->getRow(), file_->getCol());
}

void
CEditFileUtil::
deleteWord(uint line_num, uint char_num)
{
  const auto *line = file_->getEditLine(line_num);

  if (line->isEmpty() || char_num >= line->getLength() - 1)
    return;

  uint num = 0;

  if (file_->isWordChar(line->getChar(char_num))) {
    while (int(char_num + num) < (int) line->getLength() - 1 &&
           file_->isWordChar(line->getChar(char_num + num)))
      ++num;
  }
  else {
    while (int(char_num + num) < (int) line->getLength() - 1 &&
           ! file_->isWordChar(line->getChar(char_num + num)))
      ++num;
  }

  if (num > 0)
    file_->deleteChars(line_num, char_num, num);
}

void
CEditFileUtil::
deleteEOL()
{
  deleteEOL(file_->getRow(), file_->getCol());
}

void
CEditFileUtil::
deleteEOL(uint line_num, uint char_num)
{
  const auto *line = file_->getEditLine(line_num);

  uint num = std::max(int(line->getLength()) - int(char_num), 0);

  if (num > 0)
    file_->deleteChars(line_num, char_num, num);
}

void
CEditFileUtil::
shiftLeft(uint line_num1, uint line_num2)
{
  if (line_num1 > line_num2)
    std::swap(line_num1, line_num2);

  uint n = file_->getOptions().shiftwidth;

  file_->startGroup();

  for (uint line_num = line_num1; line_num <= line_num2; ++line_num) {
    const auto *line = file_->getEditLine(line_num);

    uint len = line->getLength();

    uint n1 = 0;

    for (uint j = 0; j < n && j < len; ++j) {
      char c = line->getChar(j);

      if (! isspace(c)) break;

      ++n1;
    }

    file_->deleteChars(line_num, 0, n1);
  }

  file_->endGroup();
}

void
CEditFileUtil::
shiftRight(uint line_num1, uint line_num2)
{
  if (line_num1 > line_num2)
    std::swap(line_num1, line_num2);

  std::string chars;

  uint n = file_->getOptions().shiftwidth;

  for (uint j = 0; j < n; ++j)
    chars += " ";

  file_->startGroup();

  for (uint line_num = line_num1; line_num <= line_num2; ++line_num) {
    const auto *line = file_->getEditLine(line_num);

    if (line->isEmpty()) continue;

    file_->addChars(line_num, 0, chars);
  }

  file_->endGroup();
}

void
CEditFileUtil::
nextWord()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  nextWord(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

// a word a series of alphanumeric or _ characters OR
// a series of non-blank characters
void
CEditFileUtil::
nextWord(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  // skip current word
  bool found = false;

  if (*char_num < line->getLength()) {
    if      (file_->isWordChar(line->getChar(*char_num))) {
      while (*char_num < line->getLength() &&
             file_->isWordChar(line->getChar(*char_num)))
        ++(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num < line->getLength() &&
             ! file_->isWordChar(line->getChar(*char_num)) &&
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
    if (*line_num >= file_->getNumLines() - 1)
      break;

    ++(*line_num);

    line = file_->getEditLine(*line_num);

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
CEditFileUtil::
nextWORD()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  nextWORD(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

// a WORD is a series of non-blank characters
void
CEditFileUtil::
nextWORD(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

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
    if (*line_num >= file_->getNumLines() - 1)
      break;

    ++(*line_num);

    line = file_->getEditLine(*line_num);

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
CEditFileUtil::
prevWord()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  prevWord(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
prevWord(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  // skip previous character
  if (*char_num > 0)
    --(*char_num);

  // skip spaces
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  // if at start of line and more lines go back a line
  while (*char_num == 0 && *line_num > 0) {
    --(*line_num);

    line = file_->getEditLine(*line_num);

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
    if      (file_->isWordChar(line->getChar(*char_num))) {
      while (*char_num > 0 && file_->isWordChar(line->getChar(*char_num - 1)))
        --(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num > 0 && ! file_->isWordChar(line->getChar(*char_num - 1)) &&
             ! isspace(line->getChar(*char_num - 1)))
        --(*char_num);
    }
  }
}

void
CEditFileUtil::
prevWORD()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  prevWORD(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
prevWORD(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  // skip previous character
  if (*char_num > 0)
    --(*char_num);

  // skip spaces
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  // if at start of line and more lines go back a line
  while (*char_num == 0 && *line_num > 0) {
    --(*line_num);

    line = file_->getEditLine(*line_num);

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
CEditFileUtil::
endWord()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  endWord(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
endWord(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  // if already at word end, increment to next char
  if      (*char_num < line->getLength() - 1) {
    if      (  file_->isWordChar(line->getChar(*char_num)) &&
             ! file_->isWordChar(line->getChar(*char_num + 1))) {
      ++(*char_num);

      // start of new word so we're done
      if (! isspace(line->getChar(*char_num)))
        return;
    }
    else if (! isspace(line->getChar(*char_num)) &&
             (isspace   (line->getChar(*char_num + 1)) ||
              file_->isWordChar(line->getChar(*char_num + 1)))) {
      ++(*char_num);
    }
  }
  else if (*char_num == line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num))) {
      // new line
      if (*line_num < file_->getNumLines() - 1) {
        ++(*line_num);

        line = file_->getEditLine(*line_num);

        *char_num = 0;
      }
    }
  }
  else {
    // new line
    if (*line_num < file_->getNumLines() - 1) {
      ++(*line_num);

      line = file_->getEditLine(*line_num);

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
      if (*line_num < file_->getNumLines() - 1) {
        ++(*line_num);

        line = file_->getEditLine(*line_num);

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
    if      (file_->isWordChar(line->getChar(*char_num))) {
      while (*char_num < line->getLength() - 1 &&
             file_->isWordChar(line->getChar(*char_num + 1)))
        ++(*char_num);
    }
    else if (! isspace(line->getChar(*char_num))) {
      while (*char_num < line->getLength() - 1 &&
             ! isspace   (line->getChar(*char_num + 1)) &&
             ! file_->isWordChar(line->getChar(*char_num + 1)))
        ++(*char_num);
    }
  }
}

void
CEditFileUtil::
endWORD()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  endWORD(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
endWORD(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  // if already at word end, increment to next char
  if      (*char_num < line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num)) &&
          isspace(line->getChar(*char_num + 1)))
      ++(*char_num);
  }
  else if (*char_num == line->getLength() - 1) {
    if (! isspace(line->getChar(*char_num))) {
      // new line
      if (*line_num < file_->getNumLines() - 1) {
        ++(*line_num);

        line = file_->getEditLine(*line_num);

        *char_num = 0;
      }
    }
  }
  else {
    // new line
    if (*line_num < file_->getNumLines() - 1) {
      ++(*line_num);

      line = file_->getEditLine(*line_num);

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
      if (*line_num < file_->getNumLines() - 1) {
        ++(*line_num);

        line = file_->getEditLine(*line_num);

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
CEditFileUtil::
getWord(std::string &word)
{
  const auto &pos = file_->getPos();

  return getWord(pos.y, pos.x, word);
}

bool
CEditFileUtil::
getWord(uint line_num, uint char_num, std::string &word)
{
  const auto *line = file_->getEditLine(line_num);

  if (! file_->isWordChar(line->getChar(char_num)))
    return false;

  int char_num1 = char_num;

  // find start of word
  while (char_num1 > 0 &&
         file_->isWordChar(line->getChar(char_num1)) &&
         file_->isWordChar(line->getChar(char_num1 - 1)))
    --char_num1;

  int char_num2 = char_num1;

  // find end of word
  while (char_num2 < (int) line->getLength() - 1 &&
         file_->isWordChar(line->getChar(char_num2)) &&
         file_->isWordChar(line->getChar(char_num2 + 1)))
    ++char_num2;

  for (int i = char_num1; i <= char_num2; ++i)
    word += line->getChar(i);

  return true;
}

void
CEditFileUtil::
nextSentence()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  nextSentence(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

// a sentence is the first non-blank after a blank line
// of the first non-blank after a '. ' '? ' '! '
void
CEditFileUtil::
nextSentence(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  uint num = 0;

  // empty line - go to next line
  if      (line->isEmpty()) {
    if (! file_->nextLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }
  // blank line - go to first non-blank
  else if (line->isBlank()) {
    ;
  }
  // sentence end - skip to sentence start
  else if (line->isSentenceEnd(*char_num, &num)) {
    *char_num += num;

    if (*char_num >= line->getLength()) {
      if (! file_->nextLine(line_num, char_num))
        return;

      line = file_->getEditLine(*line_num);
    }
  }
  // in sentence - skip sentence and skip to next sentence start
  else {
    while (true) {
      while (*char_num < line->getLength() &&
             ! line->isSentenceEnd(*char_num, &num))
        ++(*char_num);

      if (*char_num < line->getLength() &&
          line->isSentenceEnd(*char_num, &num)) {
        *char_num += num;

        if (*char_num >= line->getLength()) {
          if (! file_->nextLine(line_num, char_num))
            return;

          line = file_->getEditLine(*line_num);
        }

        break;
      }

      // next line
      if (*char_num >= line->getLength()) {
        if (! file_->nextLine(line_num, char_num))
          return;

        line = file_->getEditLine(*line_num);

        if (line->isBlank())
          break;
      }
    }
  }

  while (! line->isEmpty() && line->isBlank()) {
    if (! file_->nextLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }

  while (*char_num < line->getLength() &&
         isspace(line->getChar(*char_num)))
    ++(*char_num);
}

void
CEditFileUtil::
prevSentence()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  prevSentence(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
prevSentence(uint *line_num, uint *char_num)
{
  uint num = 0;

  const auto *line = file_->getEditLine(*line_num);

  bool sentence = false;

  // blank line is a sentence
  if (line->isEmpty()) {
    // skip empty lines
    while (line->isEmpty()) {
      // go to previous line
      if (! file_->prevLine(line_num, char_num))
        return;

      line = file_->getEditLine(*line_num);
    }

    sentence = true;
  }

  // skip blank lines
  while (! line->isEmpty() && line->isBlank()) {
    // go to previous line
    if (! file_->prevLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }

  // empty line is a sentence so we are done
  if (line->isEmpty())
    return;

  // go to last non-blank on non-blank line
  while (*char_num > 0 && isspace(line->getChar(*char_num)))
    --(*char_num);

  if (*char_num > 0 && line->isSentenceEnd(*char_num, &num)) {
    sentence = true;

    --(*char_num);
  }

  while (true) {
    // find previous sentence end or start of line
    while (*char_num > 0 && ! line->isSentenceEnd(*char_num, &num))
      --(*char_num);

    // if find previous sentence end then done
    if (*char_num > 0 && line->isSentenceEnd(*char_num, &num)) {
      if (sentence) {
        nextSentence(line_num, char_num);
        return;
      }

      sentence = true;

      --(*char_num);

      continue;
    }

    // go to previous line
    if (! file_->prevLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);

    if (line->isEmpty()) {
      if (sentence)
        nextSentence(line_num, char_num);

      return;
    }
  }
}

void
CEditFileUtil::
nextParagraph()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  nextParagraph(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
nextParagraph(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  while (line->isEmpty()) {
    if (! file_->nextLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }

  while (! line->isEmpty()) {
    if (! file_->nextLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }
}

void
CEditFileUtil::
prevParagraph()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  prevParagraph(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
prevParagraph(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  while (line->isEmpty()) {
    if (! file_->prevLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }

  while (! line->isEmpty()) {
    if (! file_->prevLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }
}

void
CEditFileUtil::
nextSection()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  nextSection(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
nextSection(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  uint pos;

  if (! file_->nextLine(line_num, char_num))
    return;

  line = file_->getEditLine(*line_num);

  while (! line->isSection(*char_num, &pos)) {
    if (! file_->nextLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }
}

void
CEditFileUtil::
prevSection()
{
  const auto &pos = file_->getPos();

  uint x = pos.x;
  uint y = pos.y;

  prevSection(&y, &x);

  file_->setPos(CIPoint2D(x, y));
}

void
CEditFileUtil::
prevSection(uint *line_num, uint *char_num)
{
  const auto *line = file_->getEditLine(*line_num);

  uint pos;

  if (! file_->prevLine(line_num, char_num))
    return;

  line = file_->getEditLine(*line_num);

  while (! line->isSection(*char_num, &pos)) {
    if (! file_->prevLine(line_num, char_num))
      return;

    line = file_->getEditLine(*line_num);
  }
}
