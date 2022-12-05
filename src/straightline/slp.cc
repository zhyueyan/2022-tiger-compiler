#include "straightline/slp.h"

#include <iostream>

namespace A {
  const int max(int a, int b) {
      if(a >= b) return a;
      else return b;
  }
int A::CompoundStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  int max1 = stm1->MaxArgs();
  int max2 = stm2->MaxArgs(); 
  return A::max(max1,max2);
}

Table *A::CompoundStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  
  Table *temp = stm1->Interp(t);
  return stm2->Interp(temp);
}

int A::AssignStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exp->MaxArgs();
}

Table *A::AssignStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  IntAndTable *res = exp->Interp(t);
  return res->t->Update(id,res->i);

}

int A::PrintStm::MaxArgs() const {
  // TODO: put your code here (lab1).
  return exps->MaxArgs();
}

Table *A::PrintStm::Interp(Table *t) const {
  // TODO: put your code here (lab1).
  return exps->Interp(t)->t;
}

int A::IdExp::MaxArgs() const {
  return 0;
}

IntAndTable* A::IdExp::Interp(Table *t) const {
    int value = t->Lookup(id);
    return new IntAndTable(value,t);
}

int A::NumExp::MaxArgs() const {
  return 0;
}

IntAndTable* A::NumExp::Interp(Table *t) const {
  return new IntAndTable(num,t);
}

int A::OpExp::MaxArgs() const {
  return 0;
}

IntAndTable* A::OpExp::Interp(Table *t) const {
  IntAndTable *res1 = left->Interp(t);
  IntAndTable *res2 = right->Interp(res1->t);
  int value = 0;
  switch (oper)
  {
  case PLUS:
    value = res1->i + res2->i;
    break;
  case MINUS:
    value = res1->i - res2->i;
    break;
  case TIMES:
    value = res1->i * res2->i;
    break;
  case DIV:
    value = res1->i / res2->i;
    break;
  
  default:
    break;
  }
  return new IntAndTable(value,res2->t);
}

int A::EseqExp::MaxArgs() const {
  return max(stm->MaxArgs(),exp->MaxArgs());
}

IntAndTable* A::EseqExp::Interp(Table *t) const {
  Table *res1 = stm->Interp(t);
  return exp->Interp(res1);
}

int A::PairExpList::MaxArgs() const {
  return max(exp->MaxArgs(),1+tail->MaxArgs());
}

IntAndTable* A::PairExpList::Interp(Table *t) const {
  IntAndTable *res1 = exp->Interp(t);
  printf("%d ",res1->i);
  return tail->Interp(res1->t);
}

int A::LastExpList::MaxArgs() const {
  return max(exp->MaxArgs(),1);
}

IntAndTable* A::LastExpList::Interp(Table *t) const {
  IntAndTable *res = exp->Interp(t);
  printf("%d\n",res->i);
  return res;
}


int Table::Lookup(const std::string &key) const {
  if (id == key) {
    return value;
  } else if (tail != nullptr) {
    return tail->Lookup(key);
  } else {
    assert(false);
  }
}

Table *Table::Update(const std::string &key, int val) const {
  return new Table(key, val, this);
}
}  // namespace A
