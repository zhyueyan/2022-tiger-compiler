#ifndef TIGER_FRAME_FRAME_H_
#define TIGER_FRAME_FRAME_H_

#include <list>
#include <vector>
#include <memory>
#include <string>

#include "tiger/frame/temp.h"
#include "tiger/translate/tree.h"
#include "tiger/codegen/assem.h"


namespace frame {

class RegManager {
public:
  RegManager() : temp_map_(temp::Map::Empty()) {}
  virtual ~RegManager() {}
  temp::Temp *GetRegister(int regno) { return regs_[regno]; }

  /**
   * Get general-purpose registers except RSI
   * NOTE: returned temp list should be in the order of calling convention
   * @return general-purpose registers
   */
  [[nodiscard]] virtual temp::TempList *Registers() = 0;

  /**
   * Get registers which can be used to hold arguments
   * NOTE: returned temp list must be in the order of calling convention
   * @return argument registers
   */
  [[nodiscard]] virtual temp::TempList *ArgRegs() = 0;

  /**
   * Get caller-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return caller-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CallerSaves() = 0;

  /**
   * Get callee-saved registers
   * NOTE: returned registers must be in the order of calling convention
   * @return callee-saved registers
   */
  [[nodiscard]] virtual temp::TempList *CalleeSaves() = 0;

  /**
   * Get return-sink registers
   * @return return-sink registers
   */
  [[nodiscard]] virtual temp::TempList *ReturnSink() = 0;

  /**
   * Get word size
   */
  [[nodiscard]] virtual int WordSize() = 0;

  [[nodiscard]] virtual temp::Temp *FramePointer() = 0;

  [[nodiscard]] virtual temp::Temp *StackPointer() = 0;

  [[nodiscard]] virtual temp::Temp *ReturnValue() = 0;


  temp::Map *temp_map_;
protected:
  std::vector<temp::Temp *> regs_;
};

class Access { //reg or stack
public:
  /* TODO: Put your lab5 code here */
  virtual tree::Exp *ToExp(tree::Exp *framePtr) const = 0;
  virtual ~Access() = default;
  
};

class Frame {
  /* TODO: Put your lab5 code here */
  public:
    std::list<frame::Access *> *formals_; //传进来的参数列表
    int s_offset;
    int max_args;
    temp::Label *frame_size_;
  protected:
    temp::Label* label;
    
    
  public:
    Frame(temp::Label* name, std::list<bool> escapes) : label(name) {};
    virtual Access* AllocLocal(bool escape) = 0; //创建函数体内局部变量
    virtual std::string GetLabel() const = 0;
    // virtual tree::Exp *FrameAddress() const = 0; //通过栈指针获得frame位置
    
};

/**
 * Fragments
 */

class Frag {
public:
  virtual ~Frag() = default;

  enum OutputPhase {
    Proc,
    String,
  };

  /**
   *Generate assembly for main program
   * @param out FILE object for output assembly file
   */
  virtual void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const = 0;
};

class StringFrag : public Frag {
public:
  temp::Label *label_;
  std::string str_;

  StringFrag(temp::Label *label, std::string str)
      : label_(label), str_(std::move(str)) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class ProcFrag : public Frag {
public:
  tree::Stm *body_;
  Frame *frame_;

  ProcFrag(tree::Stm *body, Frame *frame) : body_(body), frame_(frame) {}

  void OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const override;
};

class Frags {
public:
  Frags() = default;
  void PushBack(Frag *frag) { frags_.emplace_back(frag); }
  const std::list<Frag*> &GetList() { return frags_; }

private:
  std::list<Frag*> frags_;
};

/* TODO: Put your lab5 code here */
tree::Exp *ExternalCall(std::string s, tree::ExpList *args);

tree::Stm *procEntryExit1(Frame* frame, tree::Exp* body);
assem::InstrList *ProcEntryExit2(assem::InstrList *body);
assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body);
Frame *NewFrame(temp::Label *name, std::list<bool> formals);

} // namespace frame

#endif