#include <CSyntaxC.h>
#include <CFile.h>
#include <map>
#include <cstring>

static const char *
keywords[] = {
  "auto",
  "break",
  "case",
  "char",
  "const",
  "continue",
  "default",
  "do",
  "double",
  "else",
  "enum",
  "extern",
  "float",
  "for",
  "goto",
  "if",
  "int",
  "long",
  "register",
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "struct",
  "switch",
  "typedef",
  "union",
  "unsigned",
  "void",
  "volatile",
  "while",
};

#if 0
static char *
storage_class_specifiers[] = {
  "auto",
  "extern",
  "register",
  "static",
  "typedef",
};

static char *
type_specifiers[] = {
  "char",
  "double",
  "enum",
  "float",
  "int",
  "long",
  "short",
  "signed",
  "struct",
  "union",
  "unsigned",
  "void",
};

static char *
operators[] = {
  ";",
  ":",
  ",",
  "=",
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  "+",
  "-",
  "*",
  "/",
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
  "==",
  "!=",
  "<",
  ">",
  "<=",
  ">=",
  "<<",
  ">>",
  "%",
  "++",
  "--",
  "~",
  "!",
  ".",
  "->",
};

static char *
pre_pros[] = {
  "if",
  "ifdef",
  "ifndef",
  "error",
};
#endif

typedef std::map<std::string,CSyntaxToken> TokenHash;

#define MAX_TOKEN_LEN 64

static TokenHash token_hash[MAX_TOKEN_LEN];

CSyntaxC::
CSyntaxC()
{
  continued_  = false;
  last_token_ = TOKEN_NONE;

  uint num_keywords = sizeof(keywords)/sizeof(char *);

  for (uint i = 0; i < num_keywords; ++i) {
    uint len = strlen(keywords[i]);

    token_hash[len][keywords[i]] = TOKEN_KEYWORD;
  }
}

CSyntaxC::
~CSyntaxC()
{
}

void
CSyntaxC::
parseFile(CFile *file)
{
  init();

  line_num_ = 0;

  std::string line;

  while (file->readLine(line)) {
    processLine(line);

    ++line_num_;
  }

  term();
}

void
CSyntaxC::
processLine(const std::string &line)
{
  bool continued = continued_;

  continued_ = false;

  uint len = line.size();

  if (continued && last_token_ == TOKEN_PREPRO) {
    addToken(line_num_, 0, line, TOKEN_PREPRO);

    if (len > 0 && line[len - 1] == '\\') {
      continued_  = true;
      last_token_ = TOKEN_PREPRO;
    }

    return;
  }

  //------

  // skip leading space

  const char *cstr = line.c_str();

  uint i = 0;

  while (i < len && isspace(cstr[i]))
    ++i;

  //------

  // preprocessor line

  if (i < len && cstr[i] == '#') {
    addToken(line_num_, i, line.substr(i), TOKEN_PREPRO);

    if (len > 0 && line[len - 1] == '\\') {
      continued_  = true;
      last_token_ = TOKEN_PREPRO;
    }

    return;
  }

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
        CSyntaxToken token = findWord(word);

        if (token != TOKEN_NONE) {
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

        addToken(line_num_, word_start, word, TOKEN_STRING);

        in_string = false;
      }
      else
        word += c;
    }
    else {
      if      (isalpha(c) || c == '_') {
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
    CSyntaxToken token = findWord(word);

    if (token != TOKEN_NONE)
      addToken(line_num_, word_start, word, token);
    else
      addText(line_num_, word_start, word);
  }
  else if (in_string)
    addToken(line_num_, word_start, word, TOKEN_STRING);
}

CSyntaxToken
CSyntaxC::
findWord(const std::string &word)
{
  uint len = word.size();

  if (len >= MAX_TOKEN_LEN)
    return TOKEN_NONE;

  const TokenHash &hash = token_hash[len];

  TokenHash::const_iterator p = hash.find(word);

  if (p == hash.end())
    return TOKEN_NONE;

  return (*p).second;
}
