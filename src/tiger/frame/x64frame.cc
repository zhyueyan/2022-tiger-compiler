#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;

namespace frame {
/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers() {
  temp::TempList* templist = new temp::TempList();
  return templist;
}
temp::TempList *X64RegManager::ArgRegs() {}
temp::TempList *X64RegManager::CallerSaves() {}
temp::TempList *X64RegManager::CalleeSaves() {}
temp::TempList *X64RegManager::ReturnSink() {}
int X64RegManager::WordSize() {}
temp::Temp *X64RegManager::FramePointer() {}
temp::Temp *X64RegManager::StackPointer() {}
temp::Temp *X64RegManager::ReturnValue() {}

class InFrameAccess : public Access {
public:
  int offset;
  explicit InFrameAccess(int offset) : offset(offset) {}
  tree::Exp *ToExp(tree::Exp *framePtr) const {
    tree::Exp *pos = new tree::BinopExp(tree::BinOp::PLUS_OP, framePtr, new tree::ConstExp(offset));
    return new tree::MemExp(pos);
  }
  
  /* TODO: Put your lab5 code here */
};


class InRegAccess : public Access {
public:
  temp::Temp *reg;

  explicit InRegAccess(temp::Temp *reg) : reg(reg) {}
  tree::Exp *ToExp(tree::Exp *framePtr) const {
    return new tree::TempExp(reg);
   }
  /* TODO: Put your lab5 code here */
};

class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
};
/* TODO: Put your lab5 code here */

} // namespace frame