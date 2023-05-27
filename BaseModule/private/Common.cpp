
#include "Common.hpp"

namespace tp {
  ualni next2pow(ualni v) {
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v |= v >> 32;
    return v + 1;
  }

  uhalni next2pow(uhalni v) {
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    return v + 1;
  }

  ufalni next2pow(ufalni v) {
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    return v + 1;
  }

  ualni hash(const char* bytes) {
    unsigned long hash = 5381;
    int c;
    while ((c = *bytes++)) {
      hash = ((hash << 5) + hash) + c;
    }
    return hash;
  }

  ualni hash(alni bytes) { return abs(bytes); }
  ualni hash(alnf bytes) { return (alni)(abs(bytes)); 	}
  ualni hash(halni bytes) { return hash(alni(bytes)); }
  ualni hash(uhalni bytes) { return hash(alni(bytes)); }
  ualni hash(ualni bytes) { return hash(alni(bytes)); }
}