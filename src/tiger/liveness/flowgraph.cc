#include "tiger/liveness/flowgraph.h"

namespace fg {

void FlowGraphFactory::AssemFlowGraph() {
  /* TODO: Put your lab6 code here */
  for(auto instr: instr_list_->GetList()){
    fg::FNodePtr node = flowgraph_->NewNode(instr);
    if(typeid(*instr) == typeid(assem::LabelInstr)){
      assem::LabelInstr *label_instr = static_cast<assem::LabelInstr *>(instr);
      label_map_.get()->Enter(label_instr->label_,node);
    }
  }
  // flowgraph_->Nodes()->GetList();
  for(auto n = flowgraph_->Nodes()->GetList().begin(); n != flowgraph_->Nodes()->GetList().end(); n++){
    FNodePtr node = (*n);
    if(typeid(*(node->NodeInfo())) == typeid(assem::OperInstr)){
      assem::OperInstr *instr = static_cast<assem::OperInstr *>(node->NodeInfo());
      if(instr->jumps_ != NULL){
        for(auto jump_label: *instr->jumps_->labels_){
          fg::FNodePtr to_node = label_map_.get()->Look(jump_label);
          flowgraph_->AddEdge(node,to_node);
        }
        if(instr->assem_.find("jmp")){ // not cjump
          continue;
        }
        
      }

    }
    if(++n != flowgraph_->Nodes()->GetList().end()){
      fg::FNodePtr next_node = *(n);
      flowgraph_->AddEdge(node,next_node);
    }
    n--;
      
    
  }
  
}

} // namespace fg

namespace assem {

temp::TempList *LabelInstr::Def() const {
  /* TODO: Put your lab6 code here */
  static temp::TempList *list = new temp::TempList();
  return list;

}

temp::TempList *MoveInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if(dst_ == NULL){
    static temp::TempList *list = new temp::TempList();
    return list;
  }
  else return dst_;
}

temp::TempList *OperInstr::Def() const {
  /* TODO: Put your lab6 code here */
  if(dst_ == NULL){
    static temp::TempList *list = new temp::TempList();
    return list;
  }
  else return dst_;
}

temp::TempList *LabelInstr::Use() const {
  /* TODO: Put your lab6 code here */
  static temp::TempList *list = new temp::TempList();
  return list;
}

temp::TempList *MoveInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if(src_ == NULL){
    static temp::TempList *list = new temp::TempList();
    return list;
  }
  else return src_;
}

temp::TempList *OperInstr::Use() const {
  /* TODO: Put your lab6 code here */
  if(src_ == NULL){
    static temp::TempList *list = new temp::TempList();
    return list;
  }
  else return src_;
}
} // namespace assem
