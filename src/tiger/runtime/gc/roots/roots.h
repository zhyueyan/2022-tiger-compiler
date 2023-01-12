#ifndef TIGER_RUNTIME_GC_ROOTS_H
#define TIGER_RUNTIME_GC_ROOTS_H

#include <iostream>
#include <vector>

namespace gc {

const std::string GC_ROOTS = "GLOBAL_GC_ROOTS";

struct PointerMap {
  std::string label;  //本pointerMap的label, 为"L"+returnAddressLabel
  std::string returnAddressLabel;
  std::string nextPointerMapLabel = "0";
  std::string frameSize;
  std::string isMain = "0";
  std::string registerPointers;
  std::vector<int> offsets;
  std::string endLabel = "-1";
};
class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
  
};

}

#endif // TIGER_RUNTIME_GC_ROOTS_H