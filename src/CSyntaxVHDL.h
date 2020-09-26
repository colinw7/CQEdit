#ifndef CSYNTAX_VHDL_H
#define CSYNTAX_VHDL_H

#include <CSyntax.h>
#include <map>
#include <vector>

class CFile;

class CSyntaxVHDL : public CSyntax {
 public:
  enum class BlockType {
    NONE,
    ENTITY,
    ARCHITECTURE,
    PROCESS,
    BEGIN,
    IF,
    WHILE,
    END
  };

 public:
  CSyntaxVHDL();

  virtual ~CSyntaxVHDL();

  void init() override;
  void term() override;

  void processLine(const std::string &line) override;

 private:
  enum { MAX_TOKEN_LEN = 64 };

  using TokenHash = std::map<std::string, CSyntaxToken>;

  struct BlockDef {
    BlockType   type;
    std::string name;
    bool        hasName { false };
    std::string extraKeyword;
  };

  using NameBlockType = std::map<std::string, BlockType>;
  using BlockTypeName = std::map<BlockType, BlockDef>;

  struct BlockData {
    BlockType   blockType   { BlockType::NONE };
    std::string blockName;
    int         startLine   { -1 };
    int         endLine     { -1 };
  };

  using BlockStack = std::vector<BlockData>;

 private:
  CSyntaxToken findWord(const std::string &str);

  void printBlock(const BlockData &blockData) const;
  void printBlockName(const BlockData &blockData) const;

 private:
  TokenHash     tokenHash_[MAX_TOKEN_LEN];
  NameBlockType nameBlockType_;
  BlockTypeName blockTypeName_;
  BlockStack    blockStack_;
  BlockData     blockData_;
  bool          inBlockName_ { false };
  std::string   extraKeyword_;
};

#endif
