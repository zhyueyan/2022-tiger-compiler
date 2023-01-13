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
  uint64_t registerPointers;
  std::vector<int> offsets;
  std::string endLabel = "-1";
};

struct ReadedPointerMap {
  // std::string label;  //本pointerMap的label, 为"L"+returnAddressLabel
  uint64_t returnAddress;
  uint64_t nextAddress;
  uint64_t frameSize;
  uint64_t isMain;
  bool *registerPointers;
  std::vector<int64_t> offsets;
};
class Roots {
  // Todo(lab7): define some member and methods here to keep track of gc roots;
  
};

}

#endif // TIGER_RUNTIME_GC_ROOTS_H