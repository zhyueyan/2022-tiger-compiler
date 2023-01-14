#include "tiger/codegen/codegen.h"

#include <cassert>
#include <sstream>
#include <string_view>

extern frame::RegManager *reg_manager;

namespace {

constexpr int maxlen = 1024;


} // namespace

namespace cg {

void CodeGen::Codegen() {
  /* TODO: Put your lab5 code here */
  // tree::StmList *stm_list = traces_.get()->GetStmList();
  assem::InstrList *instr_list = new assem::InstrList();
  std::list<tree::Stm *> stm_list = traces_.get()->GetStmList()->GetList();
  for(auto stm:stm_list){
    stm->Munch(*instr_list,fs_);
  }
  instr_list = frame::ProcEntryExit2(instr_list);
  assem_instr_ = std::make_unique<AssemInstr>(instr_list);
}

void AssemInstr::Print(FILE *out, temp::Map *map) const {
  for (auto instr : instr_list_->GetList())
    instr->Print(out, map);
  fprintf(out, "\n");
}
} // namespace cg

namespace tree {
/* TODO: Put your lab5 code here */

void SeqStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  left_->Munch(instr_list,fs);
  right_->Munch(instr_list,fs);
}

void LabelStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  instr_list.Append(new assem::LabelInstr(label_->Name(),label_));
}

void JumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  std::string str = "jmp " + exp_->name_->Name();
  instr_list.Append(new assem::OperInstr(str,NULL,NULL,new assem::Targets(jumps_)));
}

void CjumpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  /* 要不要判断const情况？*/
  temp::Temp *left_temp = left_->Munch(instr_list,fs);
  temp::Temp *right_temp = right_->Munch(instr_list,fs);
  std::string cmp_str = "cmpq `s1, `s0";
  temp::TempList *src = new temp::TempList();
  src->Append(left_temp);
  src->Append(right_temp);
  instr_list.Append(new assem::OperInstr(cmp_str, NULL, src, NULL));
  
  std::string str = "";
  switch(op_){
    case EQ_OP: str = "je"; break;
    case NE_OP: str = "jne"; break;
    case LT_OP: str = "jl"; break;
    case LE_OP: str = "jle"; break;
    case GT_OP: str = "jg"; break;
    case GE_OP: str = "jge"; break;
  }
  str = str + " " + true_label_->Name();
  instr_list.Append(new assem::OperInstr(str,NULL,NULL,new assem::Targets(new std::vector<temp::Label *>{true_label_,false_label_})));

}

void MoveStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(typeid(*dst_)==typeid(MemExp)){
    temp::Temp *dst_temp = (static_cast<MemExp *>(dst_))->exp_->Munch(instr_list,fs);
    temp::Temp *src_temp = src_->Munch(instr_list,fs);
    std::string str = "movq `s0, (`s1)";
    temp::TempList *src = new temp::TempList();
    src->Append(src_temp);
    src->Append(dst_temp);
    instr_list.Append(new assem::MoveInstr(str,NULL,src));
  }
  else{
    temp::Temp *dst_temp = dst_->Munch(instr_list,fs);
    temp::Temp *src_temp = src_->Munch(instr_list,fs);
    std::string str = "movq `s0, `d0";
    temp::TempList *src = new temp::TempList();
    temp::TempList *dst = new temp::TempList();
    src->Append(src_temp);
    dst->Append(dst_temp);
    /* For GC: convey the pointer */
    // dst_temp->is_pointer = src_temp->is_pointer;
    instr_list.Append(new assem::MoveInstr(str,dst,src));
  }
  
  
}

void ExpStm::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  exp_->Munch(instr_list,fs);
}

temp::Temp *BinopExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *left_temp = left_->Munch(instr_list,fs);
  temp::Temp *right_temp = right_->Munch(instr_list,fs);
  /* For GC: convey the pointer */
  bool r_pointer = left_temp->is_pointer | right_temp->is_pointer;
  temp::Temp *r = temp::TempFactory::NewTemp(r_pointer);
  if(op_ == tree::BinOp::PLUS_OP || op_ == tree::BinOp::MINUS_OP){
    std::string str = "";
    switch(op_){
      case tree::BinOp::PLUS_OP: str = "addq `s1, `d0"; break;
      case tree::BinOp::MINUS_OP: str = "subq `s1, `d0"; break;
      
    }
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0",new temp::TempList({r}),new temp::TempList({left_temp})));
    temp::TempList *src = new temp::TempList();
    src->Append(r);
    src->Append(right_temp);
    temp::TempList *dst = new temp::TempList();
    dst->Append(r);
    instr_list.Append(new assem::OperInstr(str,dst,src,NULL));
    return r;
  }
  else if(op_ == tree::BinOp::MUL_OP){
    temp::Temp *rdx = reg_manager->ArgRegs()->NthTemp(2);
    temp::TempList *src = new temp::TempList(); 
    temp::TempList *dst = new temp::TempList();
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), new temp::TempList(left_temp)));
    src->Append(reg_manager->ReturnValue());
    src->Append(right_temp);
    instr_list.Append(new assem::OperInstr("imulq `s1", new temp::TempList({reg_manager->ReturnValue(),rdx}), src, NULL));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(r), new temp::TempList(reg_manager->ReturnValue())));
    return r;
  }
  else if(op_ == tree::BinOp::DIV_OP){
    temp::Temp *rdx = reg_manager->ArgRegs()->NthTemp(2);
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(reg_manager->ReturnValue()), new temp::TempList(left_temp)));
    instr_list.Append(new assem::OperInstr("cqto", new temp::TempList({rdx, reg_manager->ReturnValue()}), new temp::TempList(reg_manager->ReturnValue()), NULL));
    instr_list.Append(new assem::OperInstr("idivq `s1", new temp::TempList({rdx, reg_manager->ReturnValue()}), new temp::TempList({reg_manager->ReturnValue(),right_temp,rdx}), NULL));
    instr_list.Append(new assem::MoveInstr("movq `s0, `d0", new temp::TempList(r), new temp::TempList(reg_manager->ReturnValue())));
    return r;
  }
  
}

temp::Temp *MemExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *t = temp::TempFactory::NewTemp(false);
  temp::Temp *addr = exp_->Munch(instr_list,fs);
  std::string str = "movq (`s0), `d0";
  temp::TempList *src = new temp::TempList();
  temp::TempList *dst = new temp::TempList();
  src->Append(addr);
  dst->Append(t);
  instr_list.Append(new assem::OperInstr(str,dst,src,NULL));
  return t;
}

temp::Temp *TempExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  if(temp_ == reg_manager->FramePointer()){
    temp::Temp *t = temp::TempFactory::NewTemp(false);
    std::string fs_(fs);
    std::string str = "leaq " + fs_ + "(`s0), `d0";
    
    temp::TempList *src = new temp::TempList();
    temp::TempList *dst = new temp::TempList();
    src->Append(reg_manager->StackPointer());
    dst->Append(t);
    instr_list.Append(new assem::OperInstr(str,dst,src,NULL));
    return t;
  }
  else{
    return temp_;
  }
}

temp::Temp *EseqExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  stm_->Munch(instr_list,fs);
  return exp_->Munch(instr_list,fs);

}

temp::Temp *NameExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *t = temp::TempFactory::NewTemp(false);
  std::string str = "leaq " + name_->Name() + "(%rip), `d0"; //name只能存相对位置
  temp::TempList *dst = new temp::TempList();
  dst->Append(t);
  instr_list.Append(new assem::OperInstr(str,dst,NULL,NULL));
  return t;
}

temp::Temp *ConstExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::Temp *t = temp::TempFactory::NewTemp(false);
  std::string str = "movq $" + std::to_string(consti_) + ", `d0";
  temp::TempList *dst = new temp::TempList();
  dst->Append(t);
  instr_list.Append(new assem::OperInstr(str,dst,NULL,NULL));
  return t;
}

temp::Temp *CallExp::Munch(assem::InstrList &instr_list, std::string_view fs) {
  /* TODO: Put your lab5 code here */
  temp::TempList *used_list = args_->MunchArgs(instr_list,fs);
  std::string str = "callq " + static_cast<NameExp *>(fun_)->name_->Name();
  temp::TempList *def_list = reg_manager->CallerSaves();
  def_list->Append(reg_manager->ReturnValue());
  instr_list.Append(new assem::OperInstr(str,def_list,used_list,NULL));

  /* For GC: ret address */
  temp::Label *ret_addr = temp::LabelFactory::NewLabel();
  instr_list.Append(new assem::LabelInstr(ret_addr->Name(),ret_addr));

  /* ret value */
  temp::Temp *t = temp::TempFactory::NewTemp(false);
  temp::TempList *dst = new temp::TempList();
  dst->Append(t);
  temp::TempList *src = new temp::TempList();
  src->Append(reg_manager->ReturnValue());
  instr_list.Append(new assem::MoveInstr("movq `s0, `d0",dst,src));
  return t;
  
}

temp::TempList *ExpList::MunchArgs(assem::InstrList &instr_list, std::string_view fs) { //转换视角前
  /* TODO: Put your lab5 code here */
  temp::TempList *list = reg_manager->ArgRegs();
  temp::TempList *used_list = new temp::TempList();
  int i = 0;
  for(auto arg:exp_list_){
    if(i < list->GetList().size()){
      temp::Temp *t = arg->Munch(instr_list,fs);
      temp::TempList *dst = new temp::TempList();
      dst->Append(list->NthTemp(i));
      temp::TempList *src = new temp::TempList();
      src->Append(t);
      instr_list.Append(new assem::MoveInstr("movq `s0, `d0",dst,src));
      used_list->Append(list->NthTemp(i));
    }
    else{
      temp::Temp *t = arg->Munch(instr_list,fs);
      temp::TempList *src = new temp::TempList();
      src->Append(t);
      src->Append(reg_manager->StackPointer());
      int num = (i - list->GetList().size()) * WORD_SIZE;
      std::string str = "movq `s0, "+ std::to_string(num) + "(`s1)";
      instr_list.Append(new assem::OperInstr(str,NULL,src,NULL));
    }
    i++;
  }
  return used_list;
  
}

} // namespace tree
