//
// Created by wzl on 2021/10/12.
//

#ifndef TIGER_COMPILER_X64FRAME_H
#define TIGER_COMPILER_X64FRAME_H

#include "tiger/frame/frame.h"

namespace frame {
#define WORD_SIZE 8
class X64RegManager : public RegManager {
  /* TODO: Put your lab5 code here */
  public:
    X64RegManager() {
      for(int i = 0; i < REG_COUNT; i++){
        temp::Temp *t = temp::TempFactory::NewTemp();
        temp_map_->Enter(t,&(reg_str[i]));
        regs_.push_back(t);
      }
      
    }
    ~X64RegManager() {}
    temp::TempList *Registers();
    temp::TempList *ArgRegs();
    temp::TempList *CallerSaves();
    temp::TempList *CalleeSaves();
    temp::TempList *ReturnSink();
    int WordSize();
    temp::Temp *FramePointer();
    temp::Temp *StackPointer();
    temp::Temp *ReturnValue();

  private:
    enum Register {
      RAX, RBX, RCX, RDX, RSI, RDI, RBP, R8, R9, R10, R11, R12, R13, R14, R15, RSP, REG_COUNT
    };
    std::string reg_str[16] = {"%rax","%rbx","%rcx","%rdx","%rsi","%rdi","%rbp","%r8","%r9","%r10","%r11","%r12","%r13","%r14","%r15","%rsp"};
};
class X64Frame : public Frame {
  /* TODO: Put your lab5 code here */
public:
  X64Frame(temp::Label* name, std::list<bool> escapes) : Frame(name, escapes) {
    frame_size_ = temp::LabelFactory::NamedLabel(name->Name() + "_framesize");
    this->s_offset = -WORD_SIZE;
    this->formals_ = new std::list<frame::Access *>;
    for (auto ele : escapes) {
      this->formals_->push_back(AllocLocal(ele));
    }
    max_args = 0;
  };
  Access* AllocLocal(bool escape);
  std::string GetLabel() const;
  
  
};


} // namespace frame
#endif // TIGER_COMPILER_X64FRAME_H
