#include <CSyntaxC.h>
#include <iostream>

CSyntax::
CSyntax() :
 line_num_(0), notifier_(NULL)
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
init()
{
  line_num_ = 1;
}

void
CSyntax::
term()
{
}

void
CSyntax::
addToken(uint line_num, uint word_start, const std::string &word, CSyntaxToken token)
{
  if (! notifier_)
    std::cerr << line_num << ":" << word_start << ">" << word << std::endl;
  else
    notifier_->addToken(line_num, word_start, word, token);
}

void
CSyntax::
addText(uint line_num, uint word_start, const std::string &word)
{
  if (notifier_)
    notifier_->addText(line_num, word_start, word);
}
