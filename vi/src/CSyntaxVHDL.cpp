#include <CSyntaxVHDL.h>
#include <CFile.h>
#include <map>
#include <cstring>
#include <iostream>

static const char *
keywords[] = {
  "abs",
  "access",
  "after",
  "alias",
  "all",
  "and",
  "architecture",
  "array",
  "assert",
  "attribute",
  "begin",
  "block",
  "body",
  "buffer",
  "bus",
  "case",
  "component",
  "configuration",
  "constant",
  "disconnect",
  "downto",
  "else",
  "elsif",
  "end",
  "entity",
  "exit",
  "file",
  "for",
  "function",
  "generate",
  "generic",
  "guarded",
  "if",
  "impure  ",
  "in",
  "inertial",
  "inout",
  "is",
  "label",
  "library",
  "linkage",
  "literal",
  "loop",
  "map",
  "mod",
  "nand",
  "new",
  "next",
  "nor",
  "not",
  "null",
  "of",
  "on",
  "open",
  "or",
  "others",
  "out",
  "package",
  "port",
  "postponed",
  "procedure",
  "process",
  "pure",
  "range",
  "record",
  "register",
  "reject",
  "rem",
  "report",
  "return",
  "rol",
  "ror",
  "select",
  "severity",
  "shared",
  "signal",
  "sla  ",
  "sll",
  "sra",
  "srl",
  "subtype",
  "then",
  "to",
  "transport",
  "type",
  "unaffected",
  "units",
  "until",
  "use",
  "variable",
  "wait",
  "when",
  "while",
  "with",
  "xnor",
  "xor"
};

CSyntaxVHDL::
CSyntaxVHDL()
{
  auto num_keywords = sizeof(keywords)/sizeof(char *);

  for (uint i = 0; i < num_keywords; ++i) {
    auto len = strlen(keywords[i]);

    tokenHash_[len][keywords[i]] = CSyntaxToken::KEYWORD;
  }

  auto addBlock = [&](const std::string &name, BlockType type,
                      bool hasName, const std::string &extraKeyword) {
    BlockDef blockDef;

    blockDef.type         = type;
    blockDef.name         = name;
    blockDef.hasName      = hasName;
    blockDef.extraKeyword = extraKeyword;

    nameBlockType_[name] = type;
    blockTypeName_[type] = blockDef;
  };

  addBlock("architecture", BlockType::ARCHITECTURE, true , "of");
  addBlock("begin"       , BlockType::BEGIN       , false, "");
  addBlock("entity"      , BlockType::ENTITY      , true , "is");
  addBlock("if"          , BlockType::IF          , false, "");
  addBlock("process"     , BlockType::PROCESS     , false, "");
}

CSyntaxVHDL::
~CSyntaxVHDL()
{
}

void
CSyntaxVHDL::
init()
{
}

void
CSyntaxVHDL::
term()
{
  while (blockData_.blockType != BlockType::NONE) {
    printBlock(blockData_);

    if (! blockStack_.empty())
      blockData_ = blockStack_.back();
    else
      blockData_ = BlockData();

    blockStack_.pop_back();
  }
}

void
CSyntaxVHDL::
processLine(const std::string &line)
{
  auto len = line.size();

  //------

  // skip leading space
  auto *cstr = line.c_str();

  //------

#if 0
  auto endBlockName = [&]() {
    if (inBlockName_) {
      auto pn = blockTypeName_.find(blockData_.blockType);

      if      (pn != blockTypeName_.end()) {
        const auto &blockDef = (*pn).second;

        std::cerr << blockDef.name;
      }
      else
        std::cerr << "<block>";

      std::cerr << " " << blockData_.blockName << "\n";

      inBlockName_ = false;
    }
  };
#endif

  auto nameBlockType = [&](const std::string &name) {
    auto pb = nameBlockType_.find(name);
    if (pb == nameBlockType_.end()) return BlockType::NONE;

    return (*pb).second;
  };

  auto startKeyword = [&](const std::string &word) {
    //endBlockName();

    auto blockType = nameBlockType(word);
    if (blockType == BlockType::NONE) return;

    blockStack_.push_back(blockData_);

    blockData_.blockType = blockType;
    blockData_.blockName = "";
    blockData_.startLine = int(line_num_);

    auto pn = blockTypeName_.find(blockData_.blockType);

    if (pn != blockTypeName_.end()) {
      const auto &blockDef = (*pn).second;

      inBlockName_  = blockDef.hasName;
      extraKeyword_ = blockDef.extraKeyword;
    }
    else {
      inBlockName_  = false;
      extraKeyword_ = "";
    }

    std::cerr << "Start: "; printBlockName(blockData_);
  };

  auto endKeyword = [&](BlockType blockType) {
    //endBlockName();

    if (blockType != BlockType::NONE) {
      while (blockData_.blockType != BlockType::NONE &&
             blockData_.blockType != blockType) {
        blockData_.endLine = int(line_num_);

        if (blockData_.blockType != BlockType::NONE) {
          std::cerr << "End: "; printBlock(blockData_);
        }

        //---

        if (! blockStack_.empty()) {
          blockData_ = blockStack_.back();

          blockStack_.pop_back();
        }
        else
          blockData_ = BlockData();
      }
    }

    //---

    blockData_.endLine = int(line_num_);

    if (blockData_.blockType != BlockType::NONE) {
      std::cerr << "End: "; printBlock(blockData_);
    }

    //---

    if (! blockStack_.empty()) {
      blockData_ = blockStack_.back();

      blockStack_.pop_back();
    }
    else
      blockData_ = BlockData();
  };

  //------

  auto readWord = [&](uint &i) {
    std::string word;

    while (i < len) {
      char c = cstr[i];

      if (isalnum(c) || c == '_')
        word += c;
      else
        break;

      ++i;
    }

    return word;
  };

  auto readString = [&](char startChar, uint &i) {
    std::string word;

    while (i < len) {
      char c = cstr[i++];

      if (c == startChar) {
        word += c;
        break;
      }
      else
        word += c;
    }

    return word;
  };

  auto skipSpace = [&](uint &i) {
    while (i < len && isspace(cstr[i]))
      ++i;
  };

  //------

  uint i = 0;

  skipSpace(i);

  while (i < len) {
    char c = cstr[i];

    // -- comment to end of line
    if      (i < len - 1 && c == '-' && cstr[i + 1] == '-') {
      uint wordStart = i;

      std::string word = line.substr(i);

      addToken(line_num_, wordStart, word, CSyntaxToken::COMMENT);

      break;
    }
    // word
    else if (isalpha(c) || c == '_') {
      uint wordStart = i;

      std::string word = readWord(i);

      //---

      CSyntaxToken token = findWord(word);

      if (token != CSyntaxToken::NONE) {
        if (word == "end") {
          skipSpace(i);

          uint wordStart1 = i;

          std::string word1 = readWord(i);

          CSyntaxToken token1 = findWord(word1);

          if (token1 != CSyntaxToken::NONE) {
            auto blockType1 = nameBlockType(word1);

            endKeyword(blockType1);

            addToken(line_num_, wordStart, word , token );
            addToken(line_num_, wordStart, word1, token1);
          }
          else {
            endKeyword(BlockType::NONE);

            addToken(line_num_, wordStart, word, token);

            if (word1 == blockData_.blockName)
              endKeyword(BlockType::NONE);

            addText(line_num_, wordStart1, word1);
          }
        }
        else {
          startKeyword(word);

          addToken(line_num_, wordStart, word, token);

          if (inBlockName_) {
            skipSpace(i);

            uint wordStart1 = i;

            std::string word1 = readWord(i);

            blockData_.blockName = word1;

            addText(line_num_, wordStart1, word1);

            if (extraKeyword_ != "") {
              skipSpace(i);

              uint wordStart2 = i;

              std::string word2 = readWord(i);

              CSyntaxToken token2 = findWord(word2);

              if (token2 != CSyntaxToken::NONE) {
                if (word2 != extraKeyword_)
                  endKeyword(BlockType::NONE);

                addToken(line_num_, wordStart2, word2, token2);
              }
              else {
                endKeyword(BlockType::NONE);

                addText(line_num_, wordStart2, word2);
              }
            }

            inBlockName_ = false;
          }
        }
      }
      else {
        if (inBlockName_)
          blockData_.blockName = word;

        addText(line_num_, wordStart, word);
      }
    }
    // start string
    else if (c == '\'' || c == '\"') {
      uint wordStart = i;

      std::string word = readString(c, i);

      addToken(line_num_, wordStart, word, CSyntaxToken::STRING);
    }
    else {
      std::string word = std::string(&c, 1);

      addText(line_num_, i, word);

      ++i;
    }

    skipSpace(i);
  }
}

CSyntaxToken
CSyntaxVHDL::
findWord(const std::string &word)
{
  auto len = word.size();

  if (len >= MAX_TOKEN_LEN)
    return CSyntaxToken::NONE;

  const auto &hash = tokenHash_[len];

  auto p = hash.find(word);

  if (p == hash.end())
    return CSyntaxToken::NONE;

  return (*p).second;
}

void
CSyntaxVHDL::
printBlock(const BlockData &blockData) const
{
  auto pn = blockTypeName_.find(blockData.blockType);

  if (pn != blockTypeName_.end()) {
    const auto &blockDef = (*pn).second;

    std::cerr << blockDef.name;
  }
  else
    std::cerr << "<none>";

  std::cerr << " " << blockData.blockName;
  std::cerr << " " << blockData.startLine;
  std::cerr << " " << blockData.endLine;
  std::cerr << "\n";
}

void
CSyntaxVHDL::
printBlockName(const BlockData &blockData) const
{
  auto pn = blockTypeName_.find(blockData.blockType);

  if (pn != blockTypeName_.end()) {
    const auto &blockDef = (*pn).second;

    std::cerr << blockDef.name << "\n";
  }
  else
    std::cerr << "<none>\n";
}
