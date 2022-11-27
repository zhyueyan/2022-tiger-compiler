#include "tiger/translate/translate.h"

#include <tiger/absyn/absyn.h>
#include <set>

#include "tiger/env/env.h"
#include "tiger/errormsg/errormsg.h"
#include "tiger/frame/x64frame.h"
#include "tiger/frame/temp.h"
#include "tiger/frame/frame.h"

extern frame::Frags *frags;
extern frame::RegManager *reg_manager;

namespace tr {

Access *Access::AllocLocal(Level *level, bool escape) {
  /* TODO: Put your lab5 code here */
  frame::Frame *frame = level->frame_;
  frame::Access *acc = frame->AllocLocal(escape);
  Access* access = new Access(level,acc);
  return access;
}

class Cx {
public:
  PatchList trues_;
  PatchList falses_;
  tree::Stm *stm_;

  Cx(PatchList trues, PatchList falses, tree::Stm *stm)
      : trues_(trues), falses_(falses), stm_(stm) {}
};

class Exp {
public:
  [[nodiscard]] virtual tree::Exp *UnEx() = 0;
  [[nodiscard]] virtual tree::Stm *UnNx() = 0;
  [[nodiscard]] virtual Cx UnCx(err::ErrorMsg *errormsg) = 0;
};

class ExpAndTy {
public:
  tr::Exp *exp_;
  type::Ty *ty_;

  ExpAndTy(tr::Exp *exp, type::Ty *ty) : exp_(exp), ty_(ty) {}
};

class ExExp : public Exp {
public:
  tree::Exp *exp_;

  explicit ExExp(tree::Exp *exp) : exp_(exp) {}

  [[nodiscard]] tree::Exp *UnEx() override { 
    /* TODO: Put your lab5 code here */
    return exp_;
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return new tree::ExpStm(exp_);
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    tree::CjumpStm* stm = new tree::CjumpStm(tree::RelOp::NE_OP, exp_, new tree::ConstExp(0), NULL, NULL);
    std::list<temp::Label **> t;
    std::list<temp::Label **> trues_list({&stm->true_label_});
    tr::PatchList trues(trues_list);
    std::list<temp::Label **> falses_list({&stm->false_label_});
    tr::PatchList falses(falses_list);
    return Cx(trues, falses, stm);
  }
};

class NxExp : public Exp {
public:
  tree::Stm *stm_;

  explicit NxExp(tree::Stm *stm) : stm_(stm) {}

  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    return new tree::EseqExp(stm_,new tree::ConstExp(0));
  }
  [[nodiscard]] tree::Stm *UnNx() override { 
    /* TODO: Put your lab5 code here */
    return stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override {
    /* TODO: Put your lab5 code here */
    printf("Error: Nx cannot be a test exp.");
  }
};

class CxExp : public Exp {
public:
  Cx cx_;

  CxExp(PatchList trues, PatchList falses, tree::Stm *stm)
      : cx_(trues, falses, stm) {}

  CxExp(Cx cx)
      : cx_(cx) {}
  
  [[nodiscard]] tree::Exp *UnEx() override {
    /* TODO: Put your lab5 code here */
    temp::Temp *r = temp::TempFactory::NewTemp();
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    cx_.trues_.DoPatch(t);
    cx_.falses_.DoPatch(f);
    return new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(1)),
    new tree::EseqExp(cx_.stm_,new tree::EseqExp(new tree::LabelStm(f),
    new tree::EseqExp(new tree::MoveStm(new tree::TempExp(r),new tree::ConstExp(0)),
    new tree::EseqExp(new tree::LabelStm(t),new tree::TempExp(r))))));
  }
  [[nodiscard]] tree::Stm *UnNx() override {
    /* TODO: Put your lab5 code here */
    return cx_.stm_;
  }
  [[nodiscard]] Cx UnCx(err::ErrorMsg *errormsg) override { 
    /* TODO: Put your lab5 code here */
    return cx_;
  }
};

void ProgTr::Translate() {
  /* TODO: Put your lab5 code here */
  std::list<bool> e;
  main_level_.get()->frame_ = new frame::X64Frame(temp::LabelFactory::NamedLabel("tigermain"),e);
  main_level_.get()->parent_ = NULL;
  tr::ExpAndTy* et = absyn_tree_.get()->Translate(venv_.get(),tenv_.get(),main_level_.get(),temp::LabelFactory::NamedLabel("tigermain"),errormsg_.get());
  frame::ProcFrag *frag = new frame::ProcFrag(et->exp_->UnNx(), main_level_.get()->frame_);
  frags->PushBack(frag);
}

} // namespace tr

namespace absyn {

tree::Exp *calculateEscapeLevel(tr::Level *cur_level,tr::Level *access_level){
  tree::Exp *sl = new tree::TempExp(reg_manager->FramePointer());
  while(cur_level != access_level){
    frame::Access *parentLevel = (cur_level->frame_->formals_)->front();
    if(!parentLevel) printf("err");
    sl = parentLevel->ToExp(sl); //
    cur_level = cur_level->parent_;
  }
  return sl;
}

tr::ExpAndTy *AbsynTree::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  root_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *SimpleVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *x = venv->Look(sym_);
  if(!x || typeid(*x) != typeid(env::VarEntry)) {
    errormsg->Error(pos_,"undefined variable %s",sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
  }
  type::Ty *type = (static_cast<env::VarEntry *>(x))->ty_->ActualTy();
  
  tr::Access *access = (static_cast<env::VarEntry *>(x))->access_;
  tr::Level *cur_level = level;
  /* 判断是否为逃逸变量 */
  tree::Exp *exp;
  if(cur_level == access->level_){
    tree::Exp *sp = new tree::TempExp(reg_manager->FramePointer());
    exp = access->access_->ToExp(sp);
  }
  else{
    tree::Exp *sl = calculateEscapeLevel(cur_level,access->level_);
    exp = access->access_->ToExp(sl);
  }
  return new tr::ExpAndTy(new tr::ExExp(exp),type);
}

tr::ExpAndTy *FieldVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *et = var_->Translate(venv,tenv,level,label,errormsg);
  tr::Exp *e = et->exp_;
  type::Ty *t = et->ty_;
  if(typeid(*(t->ActualTy())) != typeid(type::RecordTy)) {
    errormsg->Error(pos_,"not a record type");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
  }
  else {
    std::list<type::Field *> list = (static_cast<type::RecordTy *>(t->ActualTy()))->fields_->GetList();
    int i = 0;
    for(auto it = list.begin(); it != list.end(); it++, i++) {
      if((*it)->name_ == sym_) {
        type::Ty *type = (*it)->ty_;
        tree::Exp *exp = new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,e->UnEx(),new tree::ConstExp(i*WORD_SIZE)));
        return new tr::ExpAndTy(new tr::ExExp(exp),type);
      }
    }
    errormsg->Error(pos_,"field %s doesn't exist",sym_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
  }
}

tr::ExpAndTy *SubscriptVar::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                      tr::Level *level, temp::Label *label,
                                      err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *et = var_->Translate(venv,tenv,level,label,errormsg);
  tr::Exp *e = et->exp_;
  type::Ty *t = et->ty_->ActualTy();
  if(typeid(*t) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_,"array type required");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
  }
  tr::ExpAndTy *s_et = subscript_->Translate(venv,tenv,level,label,errormsg);
  tr::Exp *s_e = s_et->exp_;
  type::Ty *s_t = s_et->ty_;
  if(typeid(*s_t) != typeid(type::IntTy)) {
    errormsg->Error(pos_,"subscript needs to be int");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
  }
  else {
    tree::Exp *exp = new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,e->UnEx(),new tree::BinopExp(tree::BinOp::MUL_OP, s_e->UnEx(), new tree::ConstExp(WORD_SIZE))));
    return new tr::ExpAndTy(new tr::ExExp(exp),(static_cast<type::ArrayTy *>(t))->ty_);
  }
}

tr::ExpAndTy *VarExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return var_->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *NilExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::NilTy::Instance());
}

tr::ExpAndTy *IntExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(val_)),type::IntTy::Instance());
}

tr::ExpAndTy *StringExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  temp::Label *str_label = temp::LabelFactory::NewLabel();
  frame::StringFrag *frag = new frame::StringFrag(str_label,str_);
  frags->PushBack(frag);
  tree::Exp *exp = new tree::NameExp(str_label);
  return new tr::ExpAndTy(new tr::ExExp(exp),type::StringTy::Instance());
}

tr::ExpAndTy *CallExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  env::EnvEntry *x = venv->Look(func_);
  tree::Exp *fun_name;
  if(!x || typeid(*x) != typeid(env::FunEntry)) {
    // fun_name = new tree::NameExp(func_);
    errormsg->Error(pos_,"undefined function %s",func_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  else {
    std::list<type::Ty *> formals_list = (static_cast<env::FunEntry *>(x))->formals_->GetList();
    std::list<Exp *> actuals_list = args_->GetList();
    auto actuals_it = actuals_list.begin();
    auto formals_it = formals_list.begin();
    tree::ExpList *exp_list = new tree::ExpList();
    /* static link */
    if((static_cast<env::FunEntry *>(x))->level_->parent_){
      tree::Exp *exp;
      if(level == (static_cast<env::FunEntry *>(x))->level_->parent_){
        tree::Exp *sp = new tree::TempExp(reg_manager->FramePointer());//要算栈大小偏移吗
        exp = sp;
      }
      else if(level->parent_ == (static_cast<env::FunEntry *>(x))->level_->parent_){
        /* 考虑调用和函数定义共享parent情况 */
        tree::Exp *sl = calculateEscapeLevel(level,level->parent_);
        exp = sl;
      }
      else{
        tree::Exp *sl = calculateEscapeLevel(level,(static_cast<env::FunEntry *>(x))->level_->parent_);
        exp = sl;
      }
      exp_list->Append(exp);
    }
    
    /* formals*/
    for(; actuals_it != actuals_list.end() && formals_it != formals_list.end(); actuals_it++,formals_it++) {
      tr::ExpAndTy *actual_et = (*actuals_it)->Translate(venv,tenv,level,label,errormsg);
      type::Ty *actual_type = actual_et->ty_;
      if(!(*formals_it)->ActualTy()->IsSameType(actual_type)) {
        errormsg->Error((*actuals_it)->pos_, "para type mismatch");
      }
      exp_list->Append(actual_et->exp_->UnEx());
    }
    if(actuals_it != actuals_list.end()) {
      errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
    }
    if(formals_it != formals_list.end()) {
      errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
    }
    type::Ty *type = (static_cast<env::FunEntry *>(x))->result_->ActualTy();
    tree::Exp *res;
    if((static_cast<env::FunEntry *>(x))->level_->parent_){
      fun_name = new tree::NameExp((static_cast<env::FunEntry *>(x))->label_);
      res = new tree::CallExp(fun_name,exp_list);
    }
    else {
      res = frame::ExternalCall(func_->Name(),exp_list);
    }
    if(actuals_list.size()+1 > reg_manager->ArgRegs()->GetList().size()){
      int num = actuals_list.size()+1-reg_manager->ArgRegs()->GetList().size();
      level->frame_->max_args = (level->frame_->max_args>num)?level->frame_->max_args:num;
    }
    
    return new tr::ExpAndTy(new tr::ExExp(res),type);
  }

}

tr::ExpAndTy *OpExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *left = left_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *l_t = left->ty_;
  tr::Exp *l_e = left->exp_;
  tr::ExpAndTy *right = right_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *r_t = right->ty_;
  tr::Exp *r_e = right->exp_;
  switch(oper_) {
    case absyn::MINUS_OP:case absyn::PLUS_OP:case absyn::TIMES_OP:case absyn::DIVIDE_OP:
    {
      if(!l_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(left_->pos_,"integer required");
      }
      if(!r_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(right_->pos_,"integer required");
      }
      tree::Exp *exp = new tree::BinopExp(tree::BinOp(oper_-2),l_e->UnEx(),r_e->UnEx());
      return new tr::ExpAndTy(new tr::ExExp(exp),type::IntTy::Instance());
    }
    case absyn::EQ_OP: case absyn::NEQ_OP: case absyn::LT_OP: 
    case absyn::LE_OP: case absyn::GT_OP: case absyn::GE_OP:
    {
      if(!l_t->IsSameType(r_t)) {
        errormsg->Error(left_->pos_, "same type required");
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
      }
      /* compare string */
      if(typeid(*l_t) == typeid(type::StringTy)){
        std::string s = "string_equal";
        tree::ExpList *args = new tree::ExpList();
        args->Append(l_e->UnEx());
        args->Append(r_e->UnEx());
        tree::Exp *exp = frame::ExternalCall(s,args);
        if(oper_ == EQ_OP){
          tr::Exp *exp_ex = new tr::ExExp(exp);
          return new tr::ExpAndTy(new tr::CxExp(exp_ex->UnCx(errormsg)),type::IntTy::Instance());
        }
        else{
          tr::Exp *exp_ex = new tr::ExExp(new tree::BinopExp(tree::MINUS_OP, new tree::ConstExp(1), exp));
          return new tr::ExpAndTy(new tr::CxExp(exp_ex->UnCx(errormsg)),type::IntTy::Instance());
        }
      }
      /* compare int */
      else{
        tree::CjumpStm *stm = new tree::CjumpStm(tree::RelOp(oper_-6),l_e->UnEx(),r_e->UnEx(),NULL,NULL);
        tr::PatchList trues(std::list<temp::Label **>({&stm->true_label_}));
        tr::PatchList falses(std::list<temp::Label **>({&stm->false_label_}));
        return new tr::ExpAndTy(new tr::CxExp(trues,falses,stm),type::IntTy::Instance());
      }
    }
    case absyn::AND_OP:
    {
      if(!l_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(left_->pos_,"integer required");
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
      }
      if(!r_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(right_->pos_,"integer required");
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
      }
      temp::Label *z = temp::LabelFactory::NewLabel();
      tr::PatchList trues = r_e->UnCx(errormsg).trues_;
      tr::PatchList falses = tr::PatchList::JoinPatch(l_e->UnCx(errormsg).falses_,r_e->UnCx(errormsg).falses_);
      tr::PatchList l_trues = l_e->UnCx(errormsg).trues_;
      std::list<temp::Label **> list = l_trues.GetList();
      for(auto it: list){
        *it = z;
      }
      tree::Stm *stm = new tree::SeqStm(l_e->UnCx(errormsg).stm_,new tree::SeqStm(new tree::LabelStm(z),r_e->UnCx(errormsg).stm_));
      return new tr::ExpAndTy(new tr::CxExp(trues,falses,stm),type::IntTy::Instance());
    }
    case absyn::OR_OP: 
    {
      if(!l_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(left_->pos_,"integer required");
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
      }
      if(!r_t->IsSameType(type::IntTy::Instance())){
        errormsg->Error(right_->pos_,"integer required");
        return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::IntTy::Instance());
      }
      temp::Label *z = temp::LabelFactory::NewLabel();
      tr::PatchList falses = r_e->UnCx(errormsg).falses_;
      tr::PatchList trues = tr::PatchList::JoinPatch(l_e->UnCx(errormsg).trues_,r_e->UnCx(errormsg).trues_);
      tr::PatchList l_falses = l_e->UnCx(errormsg).falses_;
      std::list<temp::Label **> list = l_falses.GetList();
      for(auto it: list){
        *it = z;
      }
      tree::Stm *stm = new tree::SeqStm(l_e->UnCx(errormsg).stm_,new tree::SeqStm(new tree::LabelStm(z),r_e->UnCx(errormsg).stm_));
      return new tr::ExpAndTy(new tr::CxExp(trues,falses,stm),type::IntTy::Instance());
    }
  }

}

tr::ExpAndTy *RecordExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,      
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *recTy = tenv->Look(typ_);
  /* type checking */
  if(!recTy) {
    errormsg->Error(pos_, "undefined type %s",typ_->Name().data());
    new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  recTy = recTy->ActualTy();
  if(typeid(*recTy) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "undefined type %s",typ_->Name().data());
    new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  
  
  std::list<EField *> list = fields_->GetList();
  std::list<type::Field *> formal_list = (static_cast<type::RecordTy *>(recTy))->fields_->GetList();
  if (list.size() != formal_list.size()) {
    errormsg->Error(pos_, "fields mismatch");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
  }
  /* alloc exp */
  tree::Stm *alloc;
  temp::Temp *t = temp::TempFactory::NewTemp();
  tree::Exp *temp_exp = new tree::TempExp(t);
  std::string call_name = "alloc_record";
  tree::ExpList *args = new tree::ExpList({new tree::ConstExp(list.size() * WORD_SIZE)});
  alloc = new tree::MoveStm(temp_exp,frame::ExternalCall(call_name,args));
  
  auto it = list.rbegin();
  auto formal_it = formal_list.rbegin();
  int i = list.size()-1;
  tree::Stm *stm = NULL;
  for(; it != list.rend() && formal_it != formal_list.rend(); it++,formal_it++,i--) {
    tr::ExpAndTy *te = (*it)->exp_->Translate(venv,tenv,level,label,errormsg);
    type::Ty *t = te->ty_;
    tr::Exp *e = te->exp_;
    type::Ty *formalTy = (*formal_it)->ty_->ActualTy();
    if(!t->IsSameType(formalTy)) {
      errormsg->Error((*it)->exp_->pos_, "record type mismatch");
      new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
    }
    if((*it)->name_->Name() != (*formal_it)->name_->Name()) {
      errormsg->Error(pos_ , "record name mismatch");
      new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
    }
    /* create exp */
    if(!stm){
      stm = new tree::MoveStm(new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,temp_exp,new tree::ConstExp(i*WORD_SIZE))),e->UnEx());
    }
    else{
      tree::Stm *move = new tree::MoveStm(new tree::MemExp(new tree::BinopExp(tree::BinOp::PLUS_OP,temp_exp,new tree::ConstExp(i*WORD_SIZE))),e->UnEx());
      stm = new tree::SeqStm(move,stm);
    }
  }
  stm = new tree::SeqStm(alloc,stm);
  tree::Exp *exp = new tree::EseqExp(stm,temp_exp);
  return new tr::ExpAndTy(new tr::ExExp(exp),recTy);
}

tr::ExpAndTy *SeqExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> list = seq_->GetList();
  auto it = list.rbegin();
  tree::Exp *exp = NULL;
  tr::ExpAndTy *et;
  type::Ty *type;
  for(; it != list.rend(); it++) {
      et = (*it)->Translate(venv,tenv,level,label,errormsg);
      tr::Exp *e = et->exp_;
      if(!exp){
        exp = e->UnEx();
        type = et->ty_;
      }
      else{
        exp = new tree::EseqExp(e->UnNx(),exp);
      } 
    }
  return new tr::ExpAndTy(new tr::ExExp(exp),type);
}

tr::ExpAndTy *AssignExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   tr::Level *level, temp::Label *label,                       
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *v_et = var_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *v_t = v_et->ty_->ActualTy();
  tr::Exp *v_e = v_et->exp_;
  // if(typeid(*exp_) == typeid(tree::MemExp)){

  // }
  tr::ExpAndTy *e_et = exp_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *e_t = e_et->ty_;
  tr::Exp *e_e = e_et->exp_;
  if(!v_t->IsSameType(e_t)){
    errormsg->Error(exp_->pos_, "unmatched assign exp");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  if(typeid(*var_) == typeid(SimpleVar)){
    env::EnvEntry *entry = venv->Look(static_cast<SimpleVar *>(var_)->sym_);
    if(entry->readonly_ == true){
      errormsg->Error(var_->pos_, "loop variable can't be assigned");
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
    }
  }
  tree::Stm *stm = new tree::MoveStm(v_e->UnEx(),e_e->UnEx());
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
}

tr::ExpAndTy *IfExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                               tr::Level *level, temp::Label *label,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  tr::ExpAndTy *test_et = test_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *test_t = test_et->ty_->ActualTy();
  tr::Cx test_e = test_et->exp_->UnCx(errormsg);
  if(typeid(*test_t) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "integer required for if test condition");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  
  if(elsee_ == NULL){
    tr::ExpAndTy *then_et = then_->Translate(venv,tenv,level,label,errormsg);
    type::Ty *then_t = then_et->ty_->ActualTy();
    if(typeid(*then_t) != typeid(type::VoidTy)){
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
    }
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    test_e.falses_.DoPatch(f);
    test_e.trues_.DoPatch(t);
    tree::Stm* stm = new tree::SeqStm(test_e.stm_, 
                        new tree::SeqStm(new tree::LabelStm(t), 
                          new tree::SeqStm(then_et->exp_->UnNx(), 
                            new tree::LabelStm(f))));
    return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());
  }
  else{
    tr::ExpAndTy *then_et = then_->Translate(venv,tenv,level,label,errormsg);
    tr::ExpAndTy *elsee_et = elsee_->Translate(venv,tenv,level,label,errormsg);
    if(!then_et->ty_->IsSameType(elsee_et->ty_)){
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
      return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
    }
    temp::Label *t = temp::LabelFactory::NewLabel();
    temp::Label *f = temp::LabelFactory::NewLabel();
    temp::Label *joint = temp::LabelFactory::NewLabel();
    test_e.falses_.DoPatch(f);
    test_e.trues_.DoPatch(t);
    std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>{joint};
    if(typeid(*then_et->ty_) == typeid(type::VoidTy)){
      tree::Stm*  stm = new tree::SeqStm(test_e.stm_, 
                          new tree::SeqStm(new tree::LabelStm(t), 
                            new tree::SeqStm(then_et->exp_->UnNx(), 
                              new tree::SeqStm(new tree::JumpStm(new tree::NameExp(joint),jumps),
                                new tree::SeqStm(new tree::LabelStm(f),
                                  new tree::SeqStm(elsee_et->exp_->UnNx(),
                                    new tree::LabelStm(joint)))))));
      return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());                          
    }
    else{
      tree::TempExp *temp = new tree::TempExp(temp::TempFactory::NewTemp());
      tree::Exp* exp = new tree::EseqExp(test_e.stm_, 
                        new tree::EseqExp(new tree::LabelStm(t), 
                          new tree::EseqExp(new tree::MoveStm(temp,then_et->exp_->UnEx()), 
                            new tree::EseqExp(new tree::JumpStm(new tree::NameExp(joint),jumps),
                              new tree::EseqExp(new tree::LabelStm(f),
                                new tree::EseqExp(new tree::MoveStm(temp,elsee_et->exp_->UnEx()),
                                  new tree::EseqExp(new tree::LabelStm(joint),
                                    temp)))))));
      return new tr::ExpAndTy(new tr::ExExp(exp), then_et->ty_);                              
    }
                            
  }


}

tr::ExpAndTy *WhileExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,            
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  tr::ExpAndTy *test_et = test_->Translate(venv,tenv,level,label,errormsg);
  tr::Cx test_e = test_et->exp_->UnCx(errormsg);
  if(typeid(*(test_et->ty_)) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "integer required for while test condition");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
  }
  temp::Label *test = temp::LabelFactory::NewLabel();
  temp::Label *body = temp::LabelFactory::NewLabel();
  temp::Label *done = temp::LabelFactory::NewLabel();
  tr::ExpAndTy *body_et = body_->Translate(venv,tenv,level,done,errormsg);
  if(typeid(*(body_et->ty_)) != typeid(type::VoidTy)){
    errormsg->Error(body_->pos_, "while body must produce no value");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)), type::VoidTy::Instance());
  }
  test_e.falses_.DoPatch(done);
  test_e.trues_.DoPatch(body);
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>{test};
  tree::Stm*  stm = new tree::SeqStm(new tree::LabelStm(test), 
                          new tree::SeqStm(test_e.stm_, 
                            new tree::SeqStm(new tree::LabelStm(body), 
                              new tree::SeqStm(body_et->exp_->UnNx(),
                                new tree::SeqStm(new tree::JumpStm(new tree::NameExp(test),jumps),
                                  new tree::LabelStm(done))))));
  venv->EndScope();
  tenv->EndScope();
  return new tr::ExpAndTy(new tr::NxExp(stm), type::VoidTy::Instance());

}

tr::ExpAndTy *ForExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  sym::Symbol *limit = sym::Symbol::UniqueSymbol("limit");
  DecList list;
  VarDec *h = new VarDec(hi_->pos_, limit, nullptr, hi_);
  h->escape_ = false;
  VarDec *i = new VarDec(lo_->pos_, var_, nullptr, lo_);
  i->escape_ = escape_;
  list.Prepend(h);
  list.Prepend(i);
  SimpleVar *i_var = new SimpleVar(pos_,var_);
  VarExp *i_exp = new VarExp(pos_,i_var);
  OpExp *test = new OpExp(pos_,LE_OP,i_exp,new VarExp(pos_,new SimpleVar(pos_,limit)));
  ExpList exp_list;
  exp_list.Prepend(new AssignExp(pos_, i_var, new OpExp(pos_, PLUS_OP, i_exp, new IntExp(pos_, 1))));
  exp_list.Prepend(body_);
  WhileExp *while_exp = new WhileExp(body_->pos_,test,new SeqExp(pos_,&exp_list));
  LetExp *let_exp = new LetExp(pos_,&list,while_exp);
  return let_exp->Translate(venv,tenv,level,label,errormsg);
}

tr::ExpAndTy *BreakExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::vector<temp::Label *> *jumps = new std::vector<temp::Label *>{label};
  tree::Stm *jump_stm = new tree::JumpStm(new tree::NameExp(label), jumps);
  return new tr::ExpAndTy(new tr::NxExp(jump_stm), type::VoidTy::Instance());
}

tr::ExpAndTy *LetExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  venv->BeginScope();
  tenv->BeginScope();
  std::list<Dec *> dec_list = decs_->GetList();
  tree::Stm *dec = NULL;
  for(auto it = dec_list.begin(); it != dec_list.end(); it++) {
      tree::Stm *stm = (*it)->Translate(venv,tenv,level,label,errormsg)->UnNx();
      if(dec == NULL) {
        dec = stm;
      }
      else{
        dec = new tree::SeqStm(dec,stm);
      }
  }
  tr::ExpAndTy *b_et = body_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *type = b_et->ty_->ActualTy();
  tree::Exp *exp;
  if(dec == NULL){
    exp = b_et->exp_->UnEx();
  }
  else{
    exp = new tree::EseqExp(dec,b_et->exp_->UnEx());
  }
  venv->EndScope();
  tenv->EndScope();
  return new tr::ExpAndTy(new tr::ExExp(exp),type);
}

tr::ExpAndTy *ArrayExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                  tr::Level *level, temp::Label *label,                    
                                  err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::Ty *type = tenv->Look(typ_)->ActualTy();
  if(!type || typeid(*type) != typeid(type::ArrayTy)){
    errormsg->Error(pos_,"undefined array type %s",typ_->Name().data());
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
  }
  tr::ExpAndTy *s_et = size_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *size_type = s_et->ty_->ActualTy();
  if(!size_type->IsSameType(type::IntTy::Instance())){
    errormsg->Error(size_->pos_,"int type needed in array size");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type);
  }
  tr::ExpAndTy *i_et = init_->Translate(venv,tenv,level,label,errormsg);
  type::Ty *init_type = i_et->ty_->ActualTy();
  type::Ty *def_type = static_cast<type::ArrayTy *>(type)->ty_;
  if(!def_type->IsSameType(init_type)){
    errormsg->Error(init_->pos_,"array type mismatch");
    return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type);
  }

  tree::ExpList *args = new tree::ExpList({s_et->exp_->UnEx(), i_et->exp_->UnEx()});
  temp::Temp *t = temp::TempFactory::NewTemp();
  tree::Exp *temp_exp = new tree::TempExp(t);
  tree::Stm *init_stm = new tree::MoveStm(temp_exp, frame::ExternalCall("init_array", args));
  tree::Exp *exp = new tree::EseqExp(init_stm, temp_exp);
  
  return new tr::ExpAndTy(new tr::ExExp(exp),type);
}

tr::ExpAndTy *VoidExp::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                 tr::Level *level, temp::Label *label,
                                 err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new tr::ExpAndTy(new tr::ExExp(new tree::ConstExp(0)),type::VoidTy::Instance());
}

tr::Exp *FunctionDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                                tr::Level *level, temp::Label *label,
                                err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> list = functions_->GetList();
  auto it = list.begin();
  std::set<std::string> record;
  for(; it != list.end() && it != list.end(); it++) {
    if(record.find((*it)->name_->Name()) != record.end()){
      errormsg->Error((*it)->pos_, "two functions have the same name");
      continue;
    }
    record.insert((*it)->name_->Name());
    type::Ty *returnType;
    if((*it)->result_ == NULL) {
      returnType = type::VoidTy::Instance();
    }
    else {
      returnType = tenv->Look((*it)->result_);
    }
    type::TyList *formals_tylist = (*it)->params_->MakeFormalTyList(tenv,errormsg);
    std::list<bool> formals;
    formals.push_back(true);
    for(auto i:(*it)->params_->GetList()){
      // printf("%d\n",i->escape_);
      formals.push_back(i->escape_);
    }
    tr::Level *new_level = new tr::Level((*it)->name_,formals,level);
    env::FunEntry *newFunc = new env::FunEntry(new_level,(*it)->name_,formals_tylist,returnType);
    venv->Enter((*it)->name_,newFunc);
  }
  for(it = list.begin(); it != list.end() && it != list.end(); it++) {
    venv->BeginScope();
    env::FunEntry* fun_entry = (env::FunEntry *)venv->Look((*it)->name_);
    std::list<type::Field *> formals_fieldlist = (*it)->params_->MakeFieldList(tenv,errormsg)->GetList();
    auto a = fun_entry->level_->frame_->formals_->begin();
    a++;
    for(auto f = formals_fieldlist.begin(); f != formals_fieldlist.end(); f++, a++) {
      venv->Enter((*f)->name_,new env::VarEntry(new tr::Access(fun_entry->level_,(*a)),(*f)->ty_));
    }
    tr::ExpAndTy *b_et = (*it)->body_->Translate(venv,tenv,fun_entry->level_,fun_entry->label_,errormsg);
    type::Ty *actualType = b_et->ty_;
    type::Ty *returnType = (*it)->result_ ? tenv->Look((*it)->result_) : type::VoidTy::Instance();
    if((*it)->result_ == NULL && !actualType->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(pos_, "procedure returns value");
    }
    else if(!actualType->IsSameType(returnType)) {
      if(typeid(*actualType) != typeid(type::IntTy)){
        printf("actualwrong");
      }
      if(typeid(*returnType) != typeid(type::IntTy)){
        printf("definewrong");
      }
      errormsg->Error(pos_, "wrong function return type");
    }
    venv->EndScope();
    frame::ProcFrag *proc = new frame::ProcFrag(frame::procEntryExit1(fun_entry->level_->frame_,b_et->exp_->UnEx()),fun_entry->level_->frame_);
    frags->PushBack(proc);
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

tr::Exp *VarDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                           tr::Level *level, temp::Label *label,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  // printf("%s\n",var_->Name().data());
  tr::ExpAndTy *i_et = init_->Translate(venv,tenv,level,label,errormsg);
  tr::Exp *i_e = i_et->exp_;
  type::Ty *i_t = i_et->ty_;
  if(typ_ == NULL) {
    if(typeid(*i_t) == typeid(type::NilTy)){
      errormsg->Error(init_->pos_, "init should not be nil without type specified");
      return new tr::NxExp(i_e->UnNx());
    }
  }
  else {
    type::Ty *def_type = tenv->Look(typ_);
    if (!def_type) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return new tr::NxExp(i_e->UnNx());
    }
    if(!i_t->IsSameType(def_type)){
      errormsg->Error(init_->pos_, "type mismatch");
      return new tr::NxExp(i_e->UnNx());
    }
  }
  tr::Access *new_a = new tr::Access(level,level->frame_->AllocLocal(escape_));
  env::VarEntry *newVar = new env::VarEntry(new_a,i_t);
  venv->Enter(var_,newVar);
  // printf("enter var %s\n",var_->Name().c_str());
  tree::Exp *sp = new tree::TempExp(reg_manager->FramePointer());
  tree::Exp *dst = new_a->access_->ToExp(sp);
  tree::Stm *stm = new tree::MoveStm(dst,i_e->UnEx());
  return new tr::NxExp(stm);
}

tr::Exp *TypeDec::Translate(env::VEnvPtr venv, env::TEnvPtr tenv,
                            tr::Level *level, temp::Label *label,
                            err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  std::list<NameAndTy *> list = types_->GetList();
  std::set<std::string> record;
  for(auto it = list.begin(); it != list.end(); it++) {
      /* save the header */
      if(record.find((*it)->name_->Name()) != record.end()){
        errormsg->Error(pos_, "two types have the same name");
        return new tr::ExExp(new tree::ConstExp(0));
      }
      record.insert((*it)->name_->Name());
      tenv->Enter((*it)->name_,new type::NameTy((*it)->name_,NULL));
  }
  for(auto it = list.begin(); it != list.end(); it++) {
      /* save the body */
      type::NameTy* tenvTy = static_cast<type::NameTy*>(tenv->Look((*it)->name_));
      type::Ty *bodyType = (*it)->ty_->Translate(tenv,errormsg);
      tenvTy->ty_ = bodyType;
      tenv->Set((*it)->name_,tenvTy);
  }
  for(auto it = list.begin(); it != list.end(); it++) {
      /* check the circle */
      type::Ty *p = static_cast<type::NameTy*>(tenv->Look((*it)->name_))->ty_;
      while(typeid(*p) == typeid(type::NameTy)) {
        type::NameTy* pp = static_cast<type::NameTy*>(p);
        if(pp->sym_ == (*it)->name_) {
          errormsg->Error((*it)->ty_->pos_, "illegal type cycle");
          return new tr::ExExp(new tree::ConstExp(0));
        }
        p = pp->ty_;
      }
  }
  return new tr::ExExp(new tree::ConstExp(0));
}

type::Ty *NameTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return tenv->Look(name_);
}

type::Ty *RecordTy::Translate(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  type::FieldList *list = record_->MakeFieldList(tenv,errormsg);
  return new type::RecordTy(list);
}

type::Ty *ArrayTy::Translate(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab5 code here */
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn
