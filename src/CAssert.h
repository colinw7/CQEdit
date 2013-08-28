#ifndef CAssert_H
#define CAssert_H

#include <cassert>

class CAssert {
 public:
  static bool exec(bool e, const char *m) {
    if (! e) {
      std::cerr << m << std::endl;

      assert(false);
    }

    return e;
  }
};

#define CASSERT(e, m) CAssert::exec(e, m)

#endif
