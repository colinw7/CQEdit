#include <CSyntaxPython.h>
#include <CFile.h>
#include <map>
#include <cstring>

static const char *
keywords[] = {
  "and",
  "as",
  "assert",
  "async",
  "await",
  "break",
  "class",
  "continue",
  "def",
  "del",
  "elif",
  "else",
  "except",
  "False",
  "finally",
  "for",
  "from",
  "global",
  "if",
  "import",
  "in",
  "is",
  "lambda",
  "None",
  "nonlocal",
  "not",
  "or",
  "pass",
  "raise",
  "return",
  "True",
  "try",
  "while",
  "with",
  "yield",
};

// "match", "case" and "_" are 'soft keywords

#if 0
static char *
operators[] = {
  "+",
  "-",
  "*",
  "/",
  "//",
  "%",
  "**",
  "==",
  "!=",
  "<",
  ">",
  "<=",
  ">=",
  "is",
  "is not",
  "in",
  "not in",
  "[",
  "]",
  "(",
  ")",
  "{",
  "}",

  ";",
  ":",
  ",",
  "=",
  "...",
  "*=",
  "/=",
  "%=",
  "+=",
  "-=",
  "<<=",
  ">>=",
  "&=",
  "^=",
  "|=",
  "?",
  "||",
  "&&",
  "|",
  "^",
  "&",
  "<<",
  ">>",
  "++",
  "--",
  "~",
  "!",
  ".",
  "->",
};
#endif

using TokenHash = std::map<std::string, CSyntaxToken>;

static TokenHash token_hash[20];

CSyntaxPython::
CSyntaxPython()
{
  continued_  = false;
  last_token_ = CSyntaxToken::NONE;

  auto num_keywords = sizeof(keywords)/sizeof(char *);

  for (uint i = 0; i < num_keywords; ++i) {
    auto len = strlen(keywords[i]);

    token_hash[len][keywords[i]] = CSyntaxToken::KEYWORD;
  }
}

CSyntaxPython::
~CSyntaxPython()
{
}

void
CSyntaxPython::
processLine(const std::string &line)
{
  //bool continued = continued_;

  continued_ = false;

  uint i   = 0;
  auto len = line.size();

  const char *cstr = line.c_str();

  // skip leading space
  while (i < len && isspace(cstr[i]))
    ++i;

  //uint lead = i;

  //------

  std::string word;
  bool        in_word    = false;
  bool        in_string  = false;
  uint        word_start = 0;

  for ( ; i < len; ++i) {
    char c = cstr[i];

    if      (in_word) {
      if (isalnum(c) || c == '_')
        word += c;
      else {
        auto token = findWord(word);

        if (token != CSyntaxToken::NONE) {
          addToken(line_num_, word_start, word, token);

          word = c;

          addText(line_num_, word_start, word);
        }
        else {
          word += c;

          addText(line_num_, word_start, word);
        }

        in_word = false;
      }
    }
    else if (in_string) {
      if (c == word[0]) {
        word += c;

        addToken(line_num_, word_start, word, CSyntaxToken::STRING);

        in_string = false;
      }
      else
        word += c;
    }
    else {
      if      (c == '#') {
        word_start = i;
        word       = line.substr(i);

        addToken(line_num_, word_start, word, CSyntaxToken::COMMENT);

        break;
      }
      else if (isalpha(c) || c == '_') {
        word_start = i;
        word       = c;
        in_word    = true;
      }
      else if (c == '\'' || c == '\"') {
        word_start = i;
        word       = c;
        in_string  = true;
      }
      else {
        word = c;

        addText(line_num_, i, word);
      }
    }
  }

  if      (in_word) {
    auto token = findWord(word);

    if (token != CSyntaxToken::NONE)
      addToken(line_num_, word_start, word, token);
    else
      addText(line_num_, word_start, word);
  }
  else if (in_string)
    addToken(line_num_, word_start, word, CSyntaxToken::STRING);
}

CSyntaxToken
CSyntaxPython::
findWord(const std::string &word)
{
  const auto &hash = token_hash[word.size()];

  auto p = hash.find(word);

  if (p == hash.end())
    return CSyntaxToken::NONE;

  return (*p).second;
}
