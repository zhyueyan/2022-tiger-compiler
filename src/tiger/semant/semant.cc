#include "tiger/absyn/absyn.h"
#include "tiger/semant/semant.h"
#include <set>

namespace absyn {

void AbsynTree::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                           err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  root_->SemAnalyze(venv,tenv,0,errormsg);
}

type::Ty *SimpleVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *x = venv->Look(sym_);
  if(!x || typeid(*x) != typeid(env::VarEntry)) {
    errormsg->Error(pos_,"undefined variable %s",sym_->Name().data());
    return type::IntTy::Instance();
  }
  else {
    return (static_cast<env::VarEntry *>(x))->ty_->ActualTy();
  }
}

type::Ty *FieldVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  /* First to check the type of var, then find the type of the symbol*/
  type::Ty *t = var_->SemAnalyze(venv,tenv,labelcount,errormsg);
  
  if(typeid(*(t->ActualTy())) != typeid(type::RecordTy)) {
    errormsg->Error(pos_,"not a record type");
    return type::IntTy::Instance();
  }
  else {
    std::list<type::Field *> list = (static_cast<type::RecordTy *>(t->ActualTy()))->fields_->GetList();
    for(auto it = list.begin(); it != list.end(); it++) {
      if((*it)->name_ == sym_) {
        return (*it)->ty_;
      }
    }
    errormsg->Error(pos_,"field %s doesn't exist",sym_->Name().data());
    return type::IntTy::Instance();
  }
}

type::Ty *SubscriptVar::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                   int labelcount,
                                   err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *t = var_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  if(typeid(*t) != typeid(type::ArrayTy)) {
    errormsg->Error(pos_,"array type required");
    return type::IntTy::Instance();
  }
  type::Ty *st = subscript_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(typeid(*st) != typeid(type::IntTy)) {
    errormsg->Error(pos_,"subscript needs to be int");
    return type::IntTy::Instance();
  }
  else {
    return (static_cast<type::ArrayTy *>(t))->ty_;
  }
}

type::Ty *VarExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *ty = var_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
}

type::Ty *NilExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::NilTy::Instance();
}

type::Ty *IntExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::IntTy::Instance();
}

type::Ty *StringExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::StringTy::Instance();
}

type::Ty *CallExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  env::EnvEntry *x = venv->Look(func_);
  if(!x || typeid(*x) != typeid(env::FunEntry)) {
    errormsg->Error(pos_,"undefined function %s",func_->Name().data());
    return type::IntTy::Instance();
  }
  else {
    std::list<type::Ty *> formals_list = (static_cast<env::FunEntry *>(x))->formals_->GetList();
    std::list<Exp *> actuals_list = args_->GetList();
    auto actuals_it = actuals_list.begin();
    auto formals_it = formals_list.begin();
    for(; actuals_it != actuals_list.end() && formals_it != formals_list.end(); actuals_it++,formals_it++) {
      type::Ty *actual_type = (*actuals_it)->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
      if(!(*formals_it)->ActualTy()->IsSameType(actual_type)) {
        errormsg->Error((*actuals_it)->pos_, "para type mismatch");
      }
    }
    if(actuals_it != actuals_list.end()) {
      errormsg->Error(pos_, "too many params in function %s", func_->Name().data());
    }
    if(formals_it != formals_list.end()) {
      errormsg->Error(pos_, "too few params in function %s", func_->Name().data());
    }
    return (static_cast<env::FunEntry *>(x))->result_->ActualTy();
  }
}

type::Ty *OpExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *left = left_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty *right = right_->SemAnalyze(venv,tenv,labelcount,errormsg);
  switch(oper_) {
    case absyn::MINUS_OP:case absyn::PLUS_OP:case absyn::TIMES_OP:case absyn::DIVIDE_OP:
      if(!left->IsSameType(type::IntTy::Instance())){
        errormsg->Error(left_->pos_,"integer required");
      }
      if(!right->IsSameType(type::IntTy::Instance())){
        errormsg->Error(right_->pos_,"integer required");
      }
      return type::IntTy::Instance();
    default:
      if(!left->IsSameType(right)) {
        errormsg->Error(left_->pos_, "same type required");
        return type::IntTy::Instance();
      }
      return type::IntTy::Instance();
  }
}

type::Ty *RecordExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *recTy = tenv->Look(typ_);
  if(!recTy) {
    errormsg->Error(pos_, "undefined type %s",typ_->Name().data());
    return type::VoidTy::Instance();
  }
  recTy = recTy->ActualTy();
  if(typeid(*recTy) != typeid(type::RecordTy)){
    errormsg->Error(pos_, "undefined type %s",typ_->Name().data());
    return type::VoidTy::Instance();
  }
  std::list<EField *> list = fields_->GetList();
  std::list<type::Field *> formal_list = (static_cast<type::RecordTy *>(recTy))->fields_->GetList();
  auto it = list.begin();
  auto formal_it = formal_list.begin();
  for(; it != list.end() && formal_it != formal_list.end(); it++,formal_it++) {
    type::Ty *expTy = (*it)->exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty *formalTy = (*formal_it)->ty_->ActualTy();
    if(!expTy->IsSameType(formalTy)) {
      errormsg->Error((*it)->exp_->pos_, "record type mismatch");
      return type::VoidTy::Instance();
    }
    if((*it)->name_->Name() != (*formal_it)->name_->Name()) {
      errormsg->Error(pos_ , "record name mismatch");
      return type::VoidTy::Instance();
    }
  }
  return recTy;
}

type::Ty *SeqExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<Exp *> list = seq_->GetList();
  auto it = list.begin();
  type::Ty *cur_type;
  for(; it != list.end(); it++) {
      cur_type = (*it)->SemAnalyze(venv,tenv,labelcount,errormsg);
    }
  return cur_type;

}

type::Ty *AssignExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                                int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *varType = var_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  type::Ty *expType = exp_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(!varType->IsSameType(expType)){
    errormsg->Error(exp_->pos_, "unmatched assign exp");
    return type::VoidTy::Instance();
  }
  if(typeid(*var_) == typeid(SimpleVar)){
    env::EnvEntry *entry = venv->Look(static_cast<SimpleVar *>(var_)->sym_);
    if(entry->readonly_ == true){
      errormsg->Error(var_->pos_, "loop variable can't be assigned");
    }
  }
  return type::VoidTy::Instance();
}

type::Ty *IfExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                            int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *test_type = test_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  if(typeid(*test_type) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "integer required for if test condition");
    return type::VoidTy::Instance();
  }
  if(elsee_ == NULL){
    type::Ty *then_type = then_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(typeid(*then_type) != typeid(type::VoidTy)){
      errormsg->Error(then_->pos_, "if-then exp's body must produce no value");
    }
    return type::VoidTy::Instance();
  }
  else{
    type::Ty *then_type = then_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty *elsee_type = elsee_->SemAnalyze(venv,tenv,labelcount,errormsg);
    if(!then_type->IsSameType(elsee_type)){
      errormsg->Error(elsee_->pos_, "then exp and else exp type mismatch");
      return type::VoidTy::Instance();
    }
    return then_type->ActualTy();
  }
}

type::Ty *WhileExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  type::Ty *test_type = test_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(typeid(*test_type) != typeid(type::IntTy)){
    errormsg->Error(test_->pos_, "integer required for while test condition");
    return type::VoidTy::Instance();
  }
  type::Ty *body_type = body_->SemAnalyze(venv,tenv,labelcount+1,errormsg);
  if(typeid(*body_type) != typeid(type::VoidTy)){
    errormsg->Error(body_->pos_, "while body must produce no value");
    return type::VoidTy::Instance();
  }
  venv->EndScope();
  tenv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *ForExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  env::EnvEntry *newEntry = new env::VarEntry(type::IntTy::Instance(),true);
  venv->Enter(var_,newEntry);
  type::Ty *low_type = lo_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty *high_type = hi_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(typeid(*low_type) != typeid(type::IntTy)){
    errormsg->Error(lo_->pos_,"for exp's range type is not integer");
  }
  if(typeid(*high_type) != typeid(type::IntTy)){
    errormsg->Error(hi_->pos_,"for exp's range type is not integer");
  }
  type::Ty *body_type = body_->SemAnalyze(venv,tenv,labelcount+1,errormsg)->ActualTy();
  if(typeid(*body_type) != typeid(type::VoidTy)){
    errormsg->Error(body_->pos_,"none value required for for body");
  }
  venv->EndScope();
  tenv->EndScope();
  return type::VoidTy::Instance();
}

type::Ty *BreakExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  if(labelcount <= 0){
    errormsg->Error(pos_,"break is not inside any loop");
  } 
  return type::VoidTy::Instance();
}

type::Ty *LetExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  venv->BeginScope();
  tenv->BeginScope();
  std::list<Dec *> dec_list = decs_->GetList();
  for(auto it = dec_list.begin(); it != dec_list.end(); it++) {
      (*it)->SemAnalyze(venv,tenv,labelcount,errormsg);
    }
  type::Ty *type = body_->SemAnalyze(venv,tenv,labelcount,errormsg)->ActualTy();
  venv->EndScope();
  tenv->EndScope();
  return type;
}

type::Ty *ArrayExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                               int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *type = tenv->Look(typ_)->ActualTy();
  if(!type || typeid(*type) != typeid(type::ArrayTy)){
    errormsg->Error(pos_,"undefined array type %s",typ_->Name().data());
    return type::VoidTy::Instance();
  }
  type::Ty *size_type = size_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(!size_type->IsSameType(type::IntTy::Instance())){
    errormsg->Error(size_->pos_,"int type needed in array size");
    return type;
  }
  type::Ty *init_type = init_->SemAnalyze(venv,tenv,labelcount,errormsg);
  type::Ty *def_type = static_cast<type::ArrayTy *>(type)->ty_;
  if(!def_type->IsSameType(init_type)){
    errormsg->Error(init_->pos_,"array type mismatch");
    return type;
  }
  return type;
}

type::Ty *VoidExp::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                              int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return type::VoidTy::Instance();
}

void FunctionDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv,
                             int labelcount, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<FunDec *> list = functions_->GetList();
  auto it = list.begin();
  std::set<std::string> record;
  for(; it != list.end() && it != list.end(); it++) {
    if(record.find((*it)->name_->Name()) != record.end()){
      errormsg->Error((*it)->pos_, "two functions have the same name");
      return;
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
    env::FunEntry *newFunc = new env::FunEntry(formals_tylist,returnType);
    venv->Enter((*it)->name_,newFunc);
  }
  for(it = list.begin(); it != list.end() && it != list.end(); it++) {
    venv->BeginScope();
    std::list<type::Field *> formals_fieldlist = (*it)->params_->MakeFieldList(tenv,errormsg)->GetList();
    for(auto f = formals_fieldlist.begin(); f != formals_fieldlist.end(); f++) {
      venv->Enter((*f)->name_,new env::VarEntry((*f)->ty_));
    }
    type::Ty *actualType = (*it)->body_->SemAnalyze(venv,tenv,labelcount,errormsg);
    type::Ty *returnType = (*it)->result_ ? tenv->Look((*it)->result_) : type::VoidTy::Instance();
    if((*it)->result_ == NULL && !actualType->IsSameType(type::VoidTy::Instance())) {
      errormsg->Error(pos_, "procedure returns value");
    }
    else if(!actualType->IsSameType(returnType)) {
      errormsg->Error(pos_, "wrong function return type");
    }
    venv->EndScope();
  }
}

void VarDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                        err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::Ty *exp_type = init_->SemAnalyze(venv,tenv,labelcount,errormsg);
  if(typ_ == NULL) {
    if(typeid(*exp_type) == typeid(type::NilTy)){
      errormsg->Error(init_->pos_, "init should not be nil without type specified");
      return;
    }
    else {
      env::VarEntry *newVar = new env::VarEntry(exp_type);
      venv->Enter(var_,newVar);
    }
  }
  else {
    type::Ty *def_type = tenv->Look(typ_);
    if (!def_type) {
      errormsg->Error(pos_, "undefined type %s", typ_->Name().data());
      return;
    }
    if(!exp_type->IsSameType(def_type)){
      errormsg->Error(init_->pos_, "type mismatch");
      return;
    }
    env::VarEntry *newVar = new env::VarEntry(exp_type);
    venv->Enter(var_,newVar);
  }

}

void TypeDec::SemAnalyze(env::VEnvPtr venv, env::TEnvPtr tenv, int labelcount,
                         err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  std::list<NameAndTy *> list = types_->GetList();
  std::set<std::string> record;
  for(auto it = list.begin(); it != list.end(); it++) {
      /* save the header */
      if(record.find((*it)->name_->Name()) != record.end()){
        errormsg->Error(pos_, "two types have the same name");
        return;
      }
      record.insert((*it)->name_->Name());
      tenv->Enter((*it)->name_,new type::NameTy((*it)->name_,NULL));
  }
  for(auto it = list.begin(); it != list.end(); it++) {
      /* save the body */
      type::NameTy* tenvTy = static_cast<type::NameTy*>(tenv->Look((*it)->name_));
      type::Ty *bodyType = (*it)->ty_->SemAnalyze(tenv,errormsg);
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
          return;
        }
        p = pp->ty_;
      }
  }
}

type::Ty *NameTy::SemAnalyze(env::TEnvPtr tenv, err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return tenv->Look(name_);
}

type::Ty *RecordTy::SemAnalyze(env::TEnvPtr tenv,
                               err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  type::FieldList *list = record_->MakeFieldList(tenv,errormsg);
  return new type::RecordTy(list);

}

type::Ty *ArrayTy::SemAnalyze(env::TEnvPtr tenv,
                              err::ErrorMsg *errormsg) const {
  /* TODO: Put your lab4 code here */
  return new type::ArrayTy(tenv->Look(array_));
}

} // namespace absyn

namespace sem {

void ProgSem::SemAnalyze() {
  FillBaseVEnv();
  FillBaseTEnv();
  absyn_tree_->SemAnalyze(venv_.get(), tenv_.get(), errormsg_.get());
}

} // namespace tr
