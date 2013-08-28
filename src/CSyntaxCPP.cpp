#include <CSyntaxCPP.h>
#include <CFile.h>
#include <map>
#include <cstring>

static const char *
keywords[] = {
  "auto",
  "break",
  "case",
  "char",
  "class",
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
  "namespace",
  "register",
  "return",
  "short",
  "signed",
  "sizeof",
  "static",
  "struct",
  "switch",
  "template",
  "typedef",
  "typename",
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

static TokenHash token_hash[20];

CSyntaxCPP::
CSyntaxCPP()
{
  in_comment_ = false;
  continued_  = false;
  last_token_ = TOKEN_NONE;

  uint num_keywords = sizeof(keywords)/sizeof(char *);

  for (uint i = 0; i < num_keywords; ++i) {
    uint len = strlen(keywords[i]);

    token_hash[len][keywords[i]] = TOKEN_KEYWORD;
  }
}

CSyntaxCPP::
~CSyntaxCPP()
{
}

void
CSyntaxCPP::
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
CSyntaxCPP::
processLine(const std::string &line)
{
  bool continued = continued_;

  continued_ = false;

  uint i   = 0;
  uint len = line.size();

  const char *cstr = line.c_str();

  if (! in_comment_) {
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

    if (! in_comment_) {
      while (i < len && isspace(cstr[i]))
        ++i;
    }

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
    else if (in_comment_) {
      if (i < len - 1 && c == '*' && cstr[i + 1] == '/') {
        word += "*/";

        addToken(line_num_, word_start, word, TOKEN_COMMENT);

        in_comment_ = false;

        ++i;
      }
      else
        word += c;
    }
    else {
      if      (i < len - 1 && c == '/' && cstr[i + 1] == '/') {
        word_start = i;
        word       = line.substr(i);

        addToken(line_num_, word_start, word, TOKEN_COMMENT);

        break;
      }
      else if (i < len - 1 && c == '/' && cstr[i + 1] == '*') {
        word_start  = i;
        word        = "/*";
        in_comment_ = true;

        ++i;
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
    CSyntaxToken token = findWord(word);

    if (token != TOKEN_NONE)
      addToken(line_num_, word_start, word, token);
    else
      addText(line_num_, word_start, word);
  }
  else if (in_string)
    addToken(line_num_, word_start, word, TOKEN_STRING);
  else if (in_comment_)
    addToken(line_num_, word_start, word, TOKEN_COMMENT);
}

CSyntaxToken
CSyntaxCPP::
findWord(const std::string &word)
{
  const TokenHash &hash = token_hash[word.size()];

  TokenHash::const_iterator p = hash.find(word);

  if (p == hash.end())
    return TOKEN_NONE;

  return (*p).second;
}
