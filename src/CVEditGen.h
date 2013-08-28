#ifndef CVEDIT_GEN_H
#define CVEDIT_GEN_H

class CVEditFile;
class CKeyEvent;

class CVEditGen {
 public:
  CVEditGen(CVEditFile *file);

  void processChar(const CKeyEvent &event);

 private:
  CVEditFile *file_;
};

#endif
