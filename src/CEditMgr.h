#ifndef CEDIT_MGR_H
#define CEDIT_MGR_H

class CEditFile;
class CEditCursor;
class CEditEd;
class CEditLine;
class CEditChar;
class CLineEdit;

class CEditFactory {
 public:
  virtual ~CEditFactory() { }

  virtual CEditFile   *createFile    ()                = 0;
  virtual CEditCursor *createCursor  (CEditFile *file) = 0;
  virtual CEditEd     *createEd      (CEditFile *file) = 0;
  virtual CEditLine   *createLine    (CEditFile *file) = 0;
  virtual CEditChar   *createChar    (CEditLine *line) = 0;
  virtual CLineEdit   *createLineEdit(CEditFile *file) = 0;
};

class CEditDefFactory : public CEditFactory {
 public:
  virtual ~CEditDefFactory() { }

  virtual CEditFile   *createFile    ();
  virtual CEditCursor *createCursor  (CEditFile *file);
  virtual CEditEd     *createEd      (CEditFile *file);
  virtual CEditLine   *createLine    (CEditFile *file);
  virtual CEditChar   *createChar    (CEditLine *line);
  virtual CLineEdit   *createLineEdit(CEditFile *file);
};

//---

#define CEditMgrInst CEditMgr::getInstance()

class CEditMgr {
 public:
  static CEditMgr *getInstance();

  void setFactory(CEditFactory *factory);

  CEditFactory *getFactory();

  CEditFile   *createFile();
  CEditCursor *createCursor(CEditFile *file);
  CEditEd     *createEd(CEditFile *file);
  CEditLine   *createLine(CEditFile *file);
  CEditChar   *createChar(CEditLine *line);
  CLineEdit   *createLineEdit(CEditFile *file);

 private:
  CEditMgr();
 ~CEditMgr();

 private:
  CEditMgr(const CEditMgr &rhs);
  CEditMgr &operator=(const CEditMgr &rhs);

 private:
  CEditFactory *factory_ { nullptr };
};

#endif
