#ifndef TIGER_REGALLOC_REGALLOC_H_
#define TIGER_REGALLOC_REGALLOC_H_

#include "tiger/codegen/assem.h"
#include "tiger/codegen/codegen.h"
#include "tiger/frame/frame.h"
#include "tiger/frame/temp.h"
#include "tiger/liveness/liveness.h"
#include "tiger/regalloc/color.h"
#include "tiger/util/graph.h"

#include <map>

namespace ra {

class Result {
public:
  temp::Map *coloring_;
  assem::InstrList *il_;

  Result() : coloring_(nullptr), il_(nullptr) {}
  Result(temp::Map *coloring, assem::InstrList *il)
      : coloring_(coloring), il_(il) {}
  Result(const Result &result) = delete;
  Result(Result &&result) = delete;
  Result &operator=(const Result &result) = delete;
  Result &operator=(Result &&result) = delete;
  ~Result(){
    delete coloring_;
    delete il_;
  }
};

class RegAllocator {
  /* TODO: Put your lab6 code here */
  public:
    RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr);
    void RegAlloc();
    std::unique_ptr<Result> TransferResult();
  private:
    frame::Frame *frame_;
    std::unique_ptr<cg::AssemInstr> assem_instr_;

    live::LiveGraphFactory *live_graph_fac_; //表示指令与指令之间的执行顺序关系
    fg::FlowGraphFactory *flow_graph_fac_; //表示变量的同时活跃

    live::INodeList *precolored_; //TO INIT INIT in RegAlloc

    live::INodeList *simplify_worklist_;
    live::INodeList *freeze_worklist_;
    live::INodeList *spill_worklist_;
    
    live::INodeList *spilled_nodes_;
    live::INodeList *coalesced_nodes_;
    live::INodeList *colored_nodes_;

    live::INodeList *select_stack_;

    live::MoveList *coalesced_moves_;
    live::MoveList *constrained_moves_;
    live::MoveList *frozen_moves_;
    live::MoveList *worklist_moves_;
    live::MoveList *active_moves_;



    std::map<live::INode*, int> color_;// result


    void MakeWorkList();
    // void Init(); //
    bool MoveRelated(live::INodePtr node);
    live::MoveList* NodeMoves(live::INodePtr node);

    live::INodeListPtr Adjacent(live::INodePtr node);
    void DecrementDegree(live::INodePtr node);
    void EnableMoves(live::INodeListPtr nodes);

    live::INodePtr GetAlias(live::INodePtr node);
    void AddWorklist(live::INodePtr node);
    void FreezeMoves(live::INodePtr node);
    
    void Simplify();
    void Coalesce();
    void Freeze();
    void SelectSpill();

    void AssignColors();
    void RewriteProgram();

    bool George(live::INodePtr u, live::INodePtr v);// v to u
    bool Briggs(live::INodePtr u, live::INodePtr v);
    void Combine(live::INodePtr u, live::INodePtr v);

    void Init();

};

} // namespace ra

#endif