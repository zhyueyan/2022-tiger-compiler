#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;
namespace frame {
/* TODO: Put your lab5 code here */
temp::TempList *X64RegManager::Registers() {
  temp::TempList* templist = new temp::TempList();
  for(int i = 0; i < REG_COUNT; i++){
    templist->Append(regs_[i]);
  }
  return templist;
}
temp::TempList *X64RegManager::ArgRegs() {
  temp::TempList* templist = new temp::TempList({
    regs_[RDI],
    regs_[RSI],
    regs_[RDX],
    regs_[RCX],
    regs_[R8],
    regs_[R9],
  });
  return templist;
}
temp::TempList *X64RegManager::CallerSaves() {
  temp::TempList* templist = new temp::TempList({
    regs_[RAX],
    regs_[RDI],
    regs_[RSI],
    regs_[RDX],
    regs_[RCX],
    regs_[R8],
    regs_[R9],
    regs_[R10],
    regs_[R11],
  });
  return templist;
}
temp::TempList *X64RegManager::CalleeSaves() {
  temp::TempList* templist = new temp::TempList({
    regs_[RBX], 
    regs_[RBP], 
    regs_[R12],
    regs_[R13], 
    regs_[R14], 
    regs_[R15],
  });
  return templist;
}
temp::TempList *X64RegManager::ReturnSink() {
  temp::TempList* templist = CalleeSaves();
  templist->Append(regs_[RAX]);
  templist->Append(regs_[RSP]);
  return templist;
}
int X64RegManager::WordSize() {
  return WORD_SIZE;
}
temp::Temp *X64RegManager::FramePointer() {
  return regs_[RBP];
}
temp::Temp *X64RegManager::StackPointer() {
  return regs_[RSP];
}
temp::Temp *X64RegManager::ReturnValue() {
  return regs_[RAX];
}


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

/* TODO: Put your lab5 code here */
Access* X64Frame::AllocLocal(bool escape) {
  if(escape){
    Access *access = new InFrameAccess(s_offset);
    s_offset -= WORD_SIZE;
    return access;
  }
  else{
    Access *access = new InRegAccess(temp::TempFactory::NewTemp());
    return access;
  }
}
std::string X64Frame::GetLabel() const {
  return label->Name();
}

tree::Stm *procEntryExit1(Frame* frame, tree::Exp* body) {
  //保存caller callee寄存器？
  tree::Stm *stm = new tree::ExpStm(new tree::ConstExp(0));
  tree::Stm *restore = new tree::ExpStm(new tree::ConstExp(0));
  temp::TempList *callee_save = reg_manager->CalleeSaves();
  for(int i = 0 ; i < callee_save->GetList().size(); i++){
    if(i == 1) continue;
    temp::Temp *new_temp = temp::TempFactory::NewTemp();
    stm = new tree::SeqStm(stm,new tree::MoveStm(new tree::TempExp(new_temp),new tree::TempExp(callee_save->NthTemp(i))));
    restore = new tree::SeqStm(restore,new tree::MoveStm(new tree::TempExp(callee_save->NthTemp(i)),new tree::TempExp(new_temp)));
    
  }
  int i = 0;
  temp::TempList *t = reg_manager->ArgRegs();
  for(auto it = frame->formals_->begin(); it != frame->formals_->end(); it++, i++){
    if(i >= 6){
      stm = new tree::SeqStm(stm,new tree::MoveStm((*it)->ToExp(new tree::TempExp(reg_manager->FramePointer())),new tree::MemExp(new tree::BinopExp(tree::PLUS_OP,new tree::TempExp(reg_manager->FramePointer()),new tree::ConstExp((i-5)*WORD_SIZE)))));
    }
    else{
      temp::Temp *reg = t->NthTemp(i);
      stm = new tree::SeqStm(stm,new tree::MoveStm((*it)->ToExp(new tree::TempExp(reg_manager->FramePointer())),new tree::TempExp(reg)));
    }
    
  }
  // std::list<frame::Access *> *formal_list = frame->formals_;
  // tree::Stm *restore = new tree::ExpStm(new tree::ConstExp(0));
  // for(auto f: *formal_list){
  //   temp::Temp *new_temp = temp::TempFactory::NewTemp();
  //   tree::Exp *temp_exp =  f->ToExp(new tree::TempExp(reg_manager->FramePointer()));
  //   if(typeid(*temp_exp) == typeid(tree::TempExp)){
  //     stm = new tree::SeqStm(stm,new tree::MoveStm(new tree::TempExp(new_temp),temp_exp));
  //     restore = new tree::SeqStm(restore,new tree::MoveStm(temp_exp,new tree::TempExp(new_temp)));
      
  //   }
    
  // }
  tree::Stm *body_ret = new tree::MoveStm(new tree::TempExp(reg_manager->ReturnValue()),body);
  stm = new tree::SeqStm(stm,body_ret);
  return new tree::SeqStm(stm,restore);
}

assem::InstrList *ProcEntryExit2(assem::InstrList *body) {
  assem::Instr *return_sink = new assem::OperInstr("", nullptr, reg_manager->ReturnSink(), nullptr);
  body->Append(return_sink);
  return body;
}

assem::Proc *ProcEntryExit3(Frame *frame, assem::InstrList *body){
  std::string prolog = frame->GetLabel() + ":\n";
  std::string num = std::to_string(-frame->s_offset+frame->max_args*WORD_SIZE);
  std::string fs = ".set " + frame->frame_size_->Name() + ", " + num + "\n";
  prolog.append(fs);
  prolog.append("subq $"+num+", %rsp\n");
  std::string epilog = "addq $"+num+", %rsp\n";
  epilog.append("retq\n");
  return new assem::Proc(prolog,body,epilog);
}
Frame *NewFrame(temp::Label *name, std::list<bool> formals)
{
  return new X64Frame(name,formals);
}
tree::Exp *ExternalCall(std::string s, tree::ExpList *args) {
  return new tree::CallExp(new tree::NameExp(temp::LabelFactory::NamedLabel(s)),args);
}



} // namespace frame
