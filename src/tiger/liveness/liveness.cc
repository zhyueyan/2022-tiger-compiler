#include "tiger/liveness/liveness.h"

extern frame::RegManager *reg_manager;

namespace live {

bool MoveList::Contain(INodePtr src, INodePtr dst) {
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [src, dst](std::pair<INodePtr, INodePtr> move) {
                       return move.first == src && move.second == dst;
                     });
}

bool MoveList::Contain_Single(INodePtr reg)
{ 
  return std::any_of(move_list_.cbegin(), move_list_.cend(),
                     [reg](std::pair<INodePtr, INodePtr> move) {
                       return move.first == reg || move.second == reg;
                     });
}

void MoveList::Delete(INodePtr src, INodePtr dst) {
  assert(src && dst);
  auto move_it = move_list_.begin();
  for (; move_it != move_list_.end(); move_it++) {
    if (move_it->first == src && move_it->second == dst) {
      break;
    }
  }
  move_list_.erase(move_it);
}

MoveList *MoveList::Union(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : move_list_) {
    res->move_list_.push_back(move);
  }
  for (auto move : list->GetList()) {
    if (!res->Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::Intersect(MoveList *list) {
  auto *res = new MoveList();
  for (auto move : list->GetList()) {
    if (Contain(move.first, move.second))
      res->move_list_.push_back(move);
  }
  return res;
}

MoveList *MoveList::MoveList_n(INodePtr reg) {
  MoveList* movelist = new MoveList();
  for(auto pair: move_list_){
    if(pair.first == reg || pair.second == reg){
      movelist->Append(pair.first,pair.second);
    }
  }
  return movelist;
}

void LiveGraphFactory::LiveMap() {
  /* TODO: Put your lab6 code here */

  // initialize
  for(auto node: flowgraph_->Nodes()->GetList()){
    in_.get()->Enter(node,new temp::TempList());
    out_.get()->Enter(node,new temp::TempList());
  }
  bool changed = false;
  do{
    changed = false;
    auto node = flowgraph_->Nodes()->GetList().rbegin();
    for(; node != flowgraph_->Nodes()->GetList().rend(); node++){
      temp::TempList *node_in = in_.get()->Look(*node);
      temp::TempList *node_out = out_.get()->Look(*node);
      temp::TempList *node_in_new = (*node)->NodeInfo()->Use()->Union(node_out->Differ((*node)->NodeInfo()->Def())); //new 什么时候release？
      // printf("node_in_new:");
      // for(auto t:node_in_new->GetList()){
      //   printf("temp %d // ",t->Int());
      // }
      // printf("\n");
      // printf("%s\n ",(*node)->NodeInfo()->to_string().c_str());
      temp::TempList *node_out_new = new temp::TempList();
      // printf("succ nodes:");
      for(auto succ_node: (*node)->Succ()->GetList()){
        printf("%s ",succ_node->NodeInfo()->to_string().c_str());
        node_out_new = node_out_new->Union(in_.get()->Look(succ_node));
      }
      // printf("\n");
      // printf("node_out_new:");
      // for(auto t:node_out_new->GetList()){
      //   printf("temp %d // ",t->Int());
      // }
      // printf("\n");
      if(node_out_new->Differ(node_out)->GetList().size() != 0 || node_in_new->Differ(node_in)->GetList().size() != 0){
        changed = true;
      }
      // *node_in. = *node_in_new;
      // *node_out = *node_out_new;
      in_.get()->Enter(*node,node_in_new);
      out_.get()->Enter(*node,node_out_new);
    }
  } while(changed);
  auto node = flowgraph_->Nodes()->GetList().begin();
  for(; node != flowgraph_->Nodes()->GetList().end(); node++){
      printf("%s\n ",(*node)->NodeInfo()->to_string().c_str());
      printf("out_temp:");
      for(auto t:out_.get()->Look(*node)->GetList()){
        printf("temp %d // ",t->Int());
      }
      printf("\n");
  }
}

void LiveGraphFactory::InterfGraph() {
  /* TODO: Put your lab6 code here */
  // intialize the node in interf graph
  for(auto flow_node: flowgraph_->Nodes()->GetList()){
    for(auto def_temp: flow_node->NodeInfo()->Def()->GetList()){
      temp_node_map_->Enter(def_temp,live_graph_.interf_graph->NewNode(def_temp));
    }
    for(auto use_temp: flow_node->NodeInfo()->Use()->GetList()){
      temp_node_map_->Enter(use_temp,live_graph_.interf_graph->NewNode(use_temp));
    }
  }
  for(auto reg_temp: reg_manager->Registers()->GetList()){
    INodePtr node = live_graph_.interf_graph->NewNode(reg_temp);
    temp_node_map_->Enter(reg_temp,node);
  }

  // build the interference graph
  for(auto node: flowgraph_->Nodes()->GetList()){
    temp::TempList *live = out_.get()->Look(node);
    // printf("%s\n out_live:",node->NodeInfo()->to_string().c_str());
    if(typeid(*(node->NodeInfo())) == typeid(assem::MoveInstr)) {
      live = live->Differ(node->NodeInfo()->Use());
      //movelist[n]?
      temp::TempList *dst = node->NodeInfo()->Def();
      temp::TempList *src = node->NodeInfo()->Use();
      if(dst->GetList().size() == 1 && src->GetList().size() == 1)
        live_graph_.moves->Append(temp_node_map_->Look(src->GetList().front()),temp_node_map_->Look(dst->GetList().front())); //temp_node_map什么时候enter的
    }
    live = live->Union(node->NodeInfo()->Def());
    // for(auto t:live->GetList()){
    //   printf("temp %d // ",t->Int());
    // }
    // printf("\n");
    for(auto def_temp: node->NodeInfo()->Def()->GetList())
      for(auto live_temp:live->GetList()) {
        INodePtr def_node = temp_node_map_->Look(def_temp);
        INodePtr live_node = temp_node_map_->Look(live_temp);
        live_graph_.interf_graph->AddEdge(def_node,live_node);
        printf("temp %d ====> temp %d\n",def_temp->Int(),live_temp->Int());
      }
  }
  
  // init the degree_ map
  for(auto node: live_graph_.interf_graph->Nodes()->GetList()){
    degree_->insert(std::pair<live::INodePtr, int>(node,node->Degree()));
  }

  // init the priority 
  // printf("=== priority ===\n");
  for (auto p : flowgraph_->Nodes()->GetList()) {
    temp::TempList* def = p->NodeInfo()->Def();
    temp::TempList* use = p->NodeInfo()->Use();
    for (auto q : def->GetList()) {
      if (live_graph_.priority.find(q) == live_graph_.priority.end()) live_graph_.priority[q] = 0;
      live_graph_.priority[q]++;
      // printf("%d",live_graph_.priority[q]);
    };
    for (auto q : use->GetList()) {
      if (live_graph_.priority.find(q) == live_graph_.priority.end()) live_graph_.priority[q] = 0;
      live_graph_.priority[q]++;
      // printf("%d",live_graph_.priority[q]);
    };
  };
 

}

void LiveGraphFactory::Liveness() {
  LiveMap();
  InterfGraph();
}

} // namespace live
