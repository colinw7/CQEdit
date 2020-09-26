#include <CSyntaxC.h>
#include <CFile.h>
#include <iostream>

CSyntax::
CSyntax()
{
}

void
CSyntax::
setNotifier(CSyntaxNotifier *notifier)
{
  notifier_ = notifier;
}

void
CSyntax::
parseFile(CFile *file)
{
  init();

  line_num_ = 0;

  std::string line;

  while (file->readLine(line)) {
    preProcessLine(line);

    processLine(line);

    ++line_num_;

    postProcessLine(line);
  }

  term();
}

void
CSyntax::
init()
{
  line_num_ = 1;

  if (notifier_)
    notifier_->init();
}

void
CSyntax::
term()
{

  if (notifier_)
    notifier_->term();
}

void
CSyntax::
preProcessLine(const std::string &line)
{
  if (notifier_)
    notifier_->preProcessLine(line);
}

void
CSyntax::
postProcessLine(const std::string &line)
{
  if (notifier_)
    notifier_->postProcessLine(line);
}

void
CSyntax::
addToken(uint line_num, uint word_start, const std::string &word, CSyntaxToken token)
{
  if (notifier_)
    notifier_->addToken(line_num, word_start, word, token);
  else
    std::cerr << line_num << ":" << word_start << ">" << word << std::endl;
}

void
CSyntax::
addText(uint line_num, uint word_start, const std::string &word)
{
  if (notifier_)
    notifier_->addText(line_num, word_start, word);
}
