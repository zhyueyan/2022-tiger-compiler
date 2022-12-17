#include "tiger/regalloc/regalloc.h"

#include "tiger/output/logger.h"

extern frame::RegManager *reg_manager;

namespace ra {
/* TODO: Put your lab6 code here */
    #define K ((int)reg_manager->Registers()->GetList().size()-1)
    RegAllocator::RegAllocator(frame::Frame *frame, std::unique_ptr<cg::AssemInstr> assem_instr)
    {
        frame_ = frame;
        assem_instr_ = std::move(assem_instr);

        simplify_worklist_ = new live::INodeList();
        freeze_worklist_ = new live::INodeList();
        spill_worklist_ = new live::INodeList();
        precolored_ = new live::INodeList();

        spilled_nodes_ = new live::INodeList();
        coalesced_nodes_ = new live::INodeList();
        colored_nodes_ = new live::INodeList();

        select_stack_ = new live::INodeList();

        coalesced_moves_ = new live::MoveList();
        constrained_moves_ = new live::MoveList();
        frozen_moves_ = new live::MoveList();
        worklist_moves_ = new live::MoveList();
        active_moves_ = new live::MoveList();

    }

    void RegAllocator::RegAlloc()
    {   
        flow_graph_fac_ = new fg::FlowGraphFactory(assem_instr_.get()->GetInstrList());
        flow_graph_fac_->AssemFlowGraph(); //build the graph and store in the flowgraph_

        live_graph_fac_ = new live::LiveGraphFactory(flow_graph_fac_->GetFlowGraph());
        live_graph_fac_->Liveness(); // liveness analysis produce: movelist graph(IGraphPtr)

        /* INIT */
        worklist_moves_ = live_graph_fac_->GetLiveGraph().moves;
        for(auto reg_temp:reg_manager->Registers()->GetList()){
            precolored_->Append(live_graph_fac_->GetTempNodeMap()->Look(reg_temp));
        }
        // Init();
        MakeWorkList();

        while(!simplify_worklist_->GetList().empty() || !worklist_moves_->GetList().empty() || !freeze_worklist_->GetList().empty() || !spill_worklist_->GetList().empty()) {
            if(!simplify_worklist_->GetList().empty())
                Simplify();
            else if(!worklist_moves_->GetList().empty())
                Coalesce();
            else if(!freeze_worklist_->GetList().empty())
                Freeze();
            else if(!spill_worklist_->GetList().empty())
                SelectSpill();
        }

        AssignColors();

        if(!spilled_nodes_->GetList().empty()){
            RewriteProgram(); //put spilled nodes on stack
            Init();
            RegAlloc();
        }
        else{

        }
    }

    void RegAllocator::Init()
    {
        spilled_nodes_->Clear();
        colored_nodes_->Clear();
        coalesced_nodes_->Clear();
        precolored_->Clear();
        select_stack_->Clear();

        delete coalesced_moves_;
        coalesced_moves_ = new live::MoveList();
        delete constrained_moves_;
        constrained_moves_ = new live::MoveList();
        delete frozen_moves_;
        frozen_moves_ = new live::MoveList();
        delete worklist_moves_;
        worklist_moves_ = new live::MoveList();
        delete active_moves_;
        active_moves_ = new live::MoveList();

        color_.clear();

        delete flow_graph_fac_;
        delete live_graph_fac_;
    }

    void RegAllocator::Simplify()
    {
        
        live::INodePtr node = simplify_worklist_->GetList().front();
        printf("simplify temp %d\n",node->NodeInfo()->Int());
        simplify_worklist_->DeleteNode(node);
        select_stack_->Prepend(node);
        /* TODO: delete related edges and nodes, update degree */
        for(auto adj_node: Adjacent(node)->GetList()){
            DecrementDegree(adj_node);
        }
        // for(auto node: simplify_worklist_->GetList()){
        //     printf("simplify\n");
        //     if(!precolored_->Contain(node)){
        //         simplify_worklist_->DeleteNode(node);
        //         select_stack_->Prepend(node);
        //         /* TODO: delete related edges and nodes, update degree */
        //         for(auto adj_node: Adjacent(node)->GetList()){
        //             DecrementDegree(adj_node);
        //         }
        //         break;
        //     }
        // }
    }

    std::unique_ptr<Result> RegAllocator::TransferResult()
    {
        temp::Map *coloring_ = temp::Map::Empty();
        for(auto n:color_){
            temp::Temp *t = n.first->NodeInfo();
            printf("temp %d : %d",n.first->NodeInfo()->Int(), n.second);
            temp::Temp *reg = reg_manager->Registers()->NthTemp(n.second);
            coloring_->Enter(t,reg_manager->temp_map_->Look(reg));
        }
        std::unique_ptr<Result> res = std::make_unique<Result>(coloring_,assem_instr_.get()->GetInstrList());
        return std::move(res);
    }

    void RegAllocator::MakeWorkList()
    {
        // int reg_num = reg_manager->Registers()->GetList().size();
        for(auto n: live_graph_fac_->GetLiveGraph().interf_graph->Nodes()->GetList()){
            if(precolored_->Contain(n)) continue; //colored register?
            if(n->Degree() >= K){
                spill_worklist_->Append(n);
            }
            else if(MoveRelated(n)){
                freeze_worklist_->Append(n);
            }
            else {
                simplify_worklist_->Append(n);
            }
        }
    }

    bool RegAllocator::MoveRelated(live::INodePtr node)
    {
        live::MoveList *node_moves = NodeMoves(node);
        return !node_moves->GetList().empty();
    }

    live::MoveList* RegAllocator::NodeMoves(live::INodePtr node)
    {
        live::MoveList* movelist_n = live_graph_fac_->GetLiveGraph().moves->MoveList_n(node);
        return movelist_n->Intersect(worklist_moves_->Union(active_moves_));
    }

    live::INodeListPtr RegAllocator::Adjacent(live::INodePtr node)
    {
        return node->Adj()->Diff(select_stack_->Union(coalesced_nodes_));
    }

    void RegAllocator::DecrementDegree(live::INodePtr node)
    {
        int degree = live_graph_fac_->degree_->at(node)--; // RIGHT?
        if(degree == K){
            live::INodeListPtr list = Adjacent(node);
            list->Unique_Append(node);
            EnableMoves(list);
            spill_worklist_->DeleteNode(node);
            if(MoveRelated(node)){
                freeze_worklist_->Unique_Append(node);
            }
            else{
                simplify_worklist_->Unique_Append(node);
            }
        }
    }

    void RegAllocator::EnableMoves(live::INodeListPtr nodes)
    {
        for(auto node: nodes->GetList()){
            for(auto move: NodeMoves(node)->GetList()){
                if(active_moves_->Contain(move.first,move.second)){
                    active_moves_->Delete(move.first,move.second);
                    worklist_moves_->Unique_Append(move.first,move.second);
                }
            }
        }
    }

    live::INodePtr RegAllocator::GetAlias(live::INodePtr node)
    {
        if(coalesced_nodes_->Contain(node)){
            return GetAlias(live_graph_fac_->alias_->Look(node));
        }
        else return node;
    }

    void RegAllocator::Coalesce()
    {
        std::pair<live::INodePtr, live::INodePtr> move_para = worklist_moves_->GetList().front();
        live::INodePtr x;
        live::INodePtr y;
        // printf("Coalesce\n");
        if(precolored_->Contain(y)){
            x = GetAlias(move_para.first);
            y = GetAlias(move_para.second);
        }
        else {
            y = GetAlias(move_para.first);
            x = GetAlias(move_para.second);
        }
        printf("Coalesce temp %d temp %d\n",x->NodeInfo()->Int(),y->NodeInfo()->Int());
        worklist_moves_->Delete(move_para.first,move_para.second);
        if(x == y){
            coalesced_moves_->Unique_Append(move_para.first,move_para.second);
            AddWorklist(x);
        }
        else if(precolored_->Contain(y) || x->Adj(y)){ //TO CHANGE: adjset
            constrained_moves_->Unique_Append(move_para.first,move_para.second);
            AddWorklist(x);
            AddWorklist(y);
        }
        else if((precolored_->Contain(x) && George(x,y)) || Briggs(x,y)){
            coalesced_moves_->Unique_Append(move_para.first,move_para.second);
            Combine(x,y);
            AddWorklist(x);
        } 
        else {
            active_moves_->Unique_Append(move_para.first,move_para.second);
        }



    }

    void RegAllocator::AddWorklist(live::INodePtr node)
    {
        if(!precolored_->Contain(node) && !MoveRelated(node) && live_graph_fac_->degree_->at(node) < K){
            freeze_worklist_->DeleteNode(node);
            simplify_worklist_->Unique_Append(node);
        }
    }

    bool RegAllocator::George(live::INodePtr u, live::INodePtr v)
    {
        bool canCoalesce = true;
        for(auto t: Adjacent(v)->GetList()){
            if(live_graph_fac_->degree_->at(t) >= K && !precolored_->Contain(t) && !t->Adj(u))
                return false;
        }
        return true;
    }

    bool RegAllocator::Briggs(live::INodePtr u, live::INodePtr v)
    {
        int k = 0;
        for(auto t: Adjacent(u)->Union(Adjacent(v))->GetList()){
            if(live_graph_fac_->degree_->at(t) >= K)
                k++;
        }
        return (k < K);
    }

    void RegAllocator::Combine(live::INodePtr u, live::INodePtr v)
    {
        freeze_worklist_->DeleteNode(v);
        spill_worklist_->DeleteNode(v);
        coalesced_nodes_->Unique_Append(v);
        live_graph_fac_->alias_->Enter(v,u);

        live::INodeList * nodes = new live::INodeList();
        nodes->Append(v);
        EnableMoves(nodes);

        for(auto t: Adjacent(v)->Diff(Adjacent(u))->GetList()){
            live_graph_fac_->GetLiveGraph().interf_graph->AddEdge(t,u);
            DecrementDegree(v);
        }
        if(live_graph_fac_->degree_->at(u) >= K && freeze_worklist_->Contain(u)){
            freeze_worklist_->DeleteNode(u);
            spill_worklist_->Unique_Append(u);
        }
    }

    void RegAllocator::Freeze()
    {
        printf("freeze\n");
        live::INodePtr freeze_node = freeze_worklist_->GetList().front();
        freeze_worklist_->DeleteNode(freeze_node);
        simplify_worklist_->Unique_Append(freeze_node);
        FreezeMoves(freeze_node);
    }

    void RegAllocator::FreezeMoves(live::INodePtr node)
    {
        for(auto m: NodeMoves(node)->GetList()){
            live::INodePtr v = (GetAlias(m.second)==GetAlias(node)) ? GetAlias(m.first) : GetAlias(m.second);
            active_moves_->Delete(m.first,m.second);
            frozen_moves_->Unique_Append(m.first,m.second);
            if(NodeMoves(v)->GetList().empty() && live_graph_fac_->degree_->at(v) < K){
                freeze_worklist_->DeleteNode(v);
                simplify_worklist_->Unique_Append(v);
            }
        }
    }

    void RegAllocator::SelectSpill()
    {
        // printf("SelectSpill\n");
        live::INodePtr node = NULL;
        double max_p = 0;
        for(auto n: spill_worklist_->GetList()){
            double pri = (double)live_graph_fac_->GetLiveGraph().priority[n->NodeInfo()]/live_graph_fac_->degree_->at(n);
            if(pri > max_p){
                max_p = pri;
                node = n;
            }
        }
        printf("SelectSpill temp %d\n",node->NodeInfo()->Int());
        assert(node != NULL);
        spill_worklist_->DeleteNode(node);
        simplify_worklist_->Unique_Append(node);
        FreezeMoves(node);
    }

    void RegAllocator::AssignColors()
    {
        printf("=== assign color ===\n");
        color_.clear();
        int i = 0;
        for(auto node: precolored_->GetList()){
            color_[node] = i++;
            // printf("temp %d color: %d\n",node->NodeInfo()->Int(),i-1);
        }
        while(!select_stack_->GetList().empty()){
            live::INodePtr node = select_stack_->GetList().front();
            select_stack_->DeleteNode(node);
            bool *ok_map = new bool[K];
            int num = 0;
            for(int i = 0; i < K; i++) ok_map[i] = false;
            // if(node->NodeInfo()->Int() == 255){
            //     printf("here\n");
            //     for(auto w: node->Adj()->GetList()){
            //         printf("temp %d ",w->NodeInfo()->Int());
            //     }
            // }
            for(auto w: node->Adj()->GetList()){
                live::INodePtr n = GetAlias(w);
                if(colored_nodes_->Contain(n) || precolored_->Contain(n)){
                    ok_map[color_[n]] = true;
                }
            }
            bool find = false;
            for(int i = 0; i < K; i++){
                if(ok_map[i] == false){
                    if(!colored_nodes_->Contain(node))
                        printf("temp %d contained in colored\n",node->NodeInfo()->Int());
                    // assert(!colored_nodes_->Contain(node),stderr,"temp %d\n",node->NodeInfo()->Int());
                    colored_nodes_->Unique_Append(node);
                    color_[node] = i;
                    find = true;
                    break;
                }
            }
            if(find == false){
                
                assert(!spilled_nodes_->Contain(node));
                spilled_nodes_->Append(node);
            }
            // printf("\n");
            // if(num >= K-1){
            //     assert(!spilled_nodes_->Contain(node));
            //     spilled_nodes_->Append(node);
            // }
            // else{
            //     for(int i = 0; i < K-1; i++){
            //         if(ok_map[i] == false){
            //             assert(!colored_nodes_->Contain(node));
            //             colored_nodes_->Unique_Append(node);
            //             color_[node] = i;
            //             break;
            //         }
            //     }
            // }
            
            delete ok_map;
            
        }
        for(auto n:coalesced_nodes_->GetList()){
            color_[n] = color_[GetAlias(n)];
        }
    }

    void RegAllocator::RewriteProgram()
    {
        printf("rewrite\n");
        int fs = -frame_->s_offset;
        for(auto n: spilled_nodes_->GetList()){
            frame::Access *access = frame_->AllocLocal(true);
            auto it = assem_instr_.get()->GetInstrList()->GetList().cbegin();
            for(auto it = assem_instr_.get()->GetInstrList()->GetList().cbegin(); it != assem_instr_.get()->GetInstrList()->GetList().cend(); it++){
                assem::Instr *instr = *it;
                if(instr->Use()->Contain(n->NodeInfo())){
                    temp::Temp *new_temp = temp::TempFactory::NewTemp();
                    std::string num = std::to_string(-frame_->s_offset - WORD_SIZE);
                    std::string s = "movq (" + frame_->frame_size_->Name() + "-" + num + ")" + "(`s0), `d0";
                    assem::Instr *res = new assem::MoveInstr(s,new temp::TempList(new_temp),new temp::TempList(reg_manager->StackPointer()));
                    assem_instr_.get()->GetInstrList()->Insert(it,res);
                    instr->Use()->Repalce(n->NodeInfo(),new_temp);
                    it++;
                    if(it == assem_instr_.get()->GetInstrList()->GetList().cend()) break;
                }
                if(instr->Def()->Contain(n->NodeInfo())){
                    it++;
                    temp::Temp *new_temp = temp::TempFactory::NewTemp();
                    std::string num = std::to_string(-frame_->s_offset - WORD_SIZE);
                    std::string s = "movq `s0, (" + frame_->frame_size_->Name() + "-" + num + ")(%rsp)";
                    assem::Instr *res = new assem::MoveInstr(s,NULL,new temp::TempList({new_temp,reg_manager->StackPointer()}));
                    assem_instr_.get()->GetInstrList()->Insert(it,res);
                    instr->Def()->Repalce(n->NodeInfo(),new_temp);
                }
            }
        }
        printf("rewrite-end\n");
        // for(auto it = assem_instr_.get()->GetInstrList()->GetList().cbegin(); it != assem_instr_.get()->GetInstrList()->GetList().cend(); it++){
        //     if(typeid(*(*it)) == typeid(assem::MoveInstr)){

        //     }
        // }
    }


} // namespace ra