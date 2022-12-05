#include "tiger/escape/escape.h"
#include "tiger/absyn/absyn.h"

namespace esc {
void EscFinder::FindEscape() { absyn_tree_->Traverse(env_.get()); }
} // namespace esc

namespace absyn {

void AbsynTree::Traverse(esc::EscEnvPtr env) {
  /* TODO: Put your lab5 code here */
  root_->Traverse(env,0);

}

void SimpleVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  esc::EscapeEntry *entry = env->Look(sym_);
  if(entry->depth_ < depth){
    *(entry->escape_) = true;
  }
  return;
}

void FieldVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
  return;
}

void SubscriptVar::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
}

void VarExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
}

void NilExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void IntExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void StringExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void CallExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> list = args_->GetList();
  for(auto it = list.begin(); it != list.end(); it++) {
      (*it)->Traverse(env,depth);
    }
}

void OpExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  left_->Traverse(env,depth);
  right_->Traverse(env,depth);
}

void RecordExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<EField *> list = fields_->GetList();
  for(auto it = list.begin(); it != list.end(); it++) {
      (*it)->exp_->Traverse(env,depth);
    }
}

void SeqExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Exp *> list = seq_->GetList();
  for(auto it = list.begin(); it != list.end(); it++) {
      (*it)->Traverse(env,depth);
    }
}

void AssignExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  var_->Traverse(env,depth);
  exp_->Traverse(env,depth);
}

void IfExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env,depth);
  then_->Traverse(env,depth);
  if(elsee_ != NULL) elsee_->Traverse(env,depth);
}

void WhileExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  test_->Traverse(env,depth);
  body_->Traverse(env,depth);
}

void ForExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  escape_ = false;
  esc::EscapeEntry *e = new esc::EscapeEntry(depth,&escape_);
  env->Enter(var_,e);
  lo_->Traverse(env,depth);
  hi_->Traverse(env,depth);
  body_->Traverse(env,depth);
}

void BreakExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void LetExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<Dec *> list = decs_->GetList();
  for(auto it = list.begin(); it != list.end(); it++) {
      (*it)->Traverse(env,depth);
    }
  body_->Traverse(env,depth);
}

void ArrayExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  size_->Traverse(env,depth);
  init_->Traverse(env,depth);
}

void VoidExp::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

void FunctionDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  std::list<FunDec *> list = functions_->GetList();
  for(auto it = list.begin(); it != list.end(); it++) {
      env->BeginScope();
      std::list<Field *> param_list = (*it)->params_->GetList();
      for(auto param = param_list.begin(); param != param_list.end(); param++) {
        (*param)->escape_ = false;
        esc::EscapeEntry *e = new esc::EscapeEntry(depth+1,&(*param)->escape_);
        env->Enter((*param)->name_,e);
      }
      (*it)->body_->Traverse(env,depth+1);
      env->EndScope();
    }
}

void VarDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  escape_ = false;
  esc::EscapeEntry *e = new esc::EscapeEntry(depth,&escape_);
  env->Enter(var_,e);
  init_->Traverse(env,depth);
}

void TypeDec::Traverse(esc::EscEnvPtr env, int depth) {
  /* TODO: Put your lab5 code here */
  return;
}

} // namespace absyn
