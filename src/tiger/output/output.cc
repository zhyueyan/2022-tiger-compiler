#include "tiger/output/output.h"

#include <cstdio>

#include "tiger/output/logger.h"
#include "tiger/runtime/gc/roots/roots.h"
#include "tiger/frame/x64frame.h"

extern frame::RegManager *reg_manager;
extern frame::Frags *frags;
std::vector<gc::PointerMap> global_map;

namespace output {
void AssemGen::GenAssem(bool need_ra) {
  frame::Frag::OutputPhase phase;

  // Output proc
  phase = frame::Frag::Proc;
  fprintf(out_, ".text\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  // Output string
  phase = frame::Frag::String;
  fprintf(out_, ".section .rodata\n");
  for (auto &&frag : frags->GetList())
    frag->OutputAssem(out_, phase, need_ra);

  // GC: output pointer_map 
  fprintf(out_, ".global GLOBAL_GC_ROOTS\n");
  fprintf(out_, ".data\n");
  fprintf(out_, "GLOBAL_GC_ROOTS:\n");
  for(auto pt: global_map){
    fprintf(out_, "%s:\n",pt.label.data());
    fprintf(out_, ".quad %s\n",pt.nextPointerMapLabel.data());
    fprintf(out_,".quad %s\n",pt.returnAddressLabel.data());
    fprintf(out_,".quad %s\n",pt.frameSize.data());
    fprintf(out_,".quad %s\n",pt.registerPointers.data());
    for(auto off: pt.offsets){
      fprintf(out_,".quad %d\n",off);
    }
    fprintf(out_,".quad %s\n",pt.endLabel.data());
  }
}

} // namespace output

namespace frame {
void generatePointerMap(Frame *frame_, std::list<fg::FNodePtr> instr_list, tab::Table<graph::Node<assem::Instr>, temp::TempList> *in_, std::vector<int> offsets_, temp::Map *color_)
{
  bool next = false;
  std::vector<int> spill;
  std::vector<temp::Temp *> temps;
  std::string reg[6] = {"%rbx","%rbp","%r12","%r13","%r14","%r15"};
  for(auto v : *(frame_->locals_)){
    if(typeid(*v) == typeid(frame::InFrameAccess)){
      if(v->is_pointer){
        spill.push_back(static_cast<frame::InFrameAccess *>(v)->offset);
      }
    }
    if(typeid(*v) == typeid(frame::InRegAccess)){
      if(v->is_pointer){
        temps.push_back(static_cast<frame::InRegAccess *>(v)->reg);
      }
    }
  }
  printf("stack pointer\n");
  for(int i = 0; i < spill.size(); i++){
    printf("%d ",spill[i]);
  }
  printf("\n");
  for(auto it:instr_list){
    auto instr = it->NodeInfo();
    if(next){
      next = false;
      std::string ret_label = static_cast<assem::LabelInstr *>(instr)->label_->Name();
      gc::PointerMap map_;
      map_.label = "L" + ret_label;
      map_.returnAddressLabel = ret_label;
      map_.frameSize = frame_->frame_size_->Name();

      map_.offsets = offsets_;
      temp::TempList active_temp = *(in_->Look(it));
      bool reg_pointer[6] = {0};
      for(int i = 0; i < temps.size(); i++){
        if(active_temp.Contain(temps[i])){
          bool change = 0;
          for(int j = 0; j < 6; j++){
            if(color_->Look(temps[i])->compare(reg[i]) == 0){
              reg_pointer[j] = 1; 
              change = 1;
            }
          }
          assert(change == 1);
        }
      }
      for(int j = 0; j < 6; j++) printf("%d",reg_pointer[j]);
      printf("\n");
      map_.registerPointers = (char*)reg_pointer;
      if(global_map.empty()){
        global_map.push_back(map_);
      }
      else{
        auto it = --global_map.end();
        printf("%s\n",(*it).nextPointerMapLabel.data());
        (*it).nextPointerMapLabel = map_.label;
        global_map.push_back(map_);
      }
    }
    if(typeid(*instr) == typeid(assem::OperInstr)){
      std::string str = static_cast<assem::OperInstr *>(instr)->assem_;
      if(str.find("call") != str.npos){
        next = true;
        continue;
      }
    }
    if(typeid(*instr) == typeid(assem::MoveInstr)){
      std::string str = static_cast<assem::MoveInstr *>(instr)->assem_;
      std::string cmp = "(" + frame_->frame_size_->Name() + "-";
      if(str.find(cmp) != str.npos){
        auto start = str.find("-");
        auto end = str.find(")");
        int offset = -std::stoi(str.substr(start+1, end-start));
        for(int i = 0; i < spill.size(); i++){
          if(spill[i] == offset){
              bool has = false;
              for(int k = 0; k < offsets_.size(); k++){
                if(offsets_[k] == offset) {
                  has = true;
                  break;
                }
              }
              if(has == false)
                offsets_.push_back(offset);
          }
           
        }
      }
    }
    
  }
}

void ProcFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  std::unique_ptr<canon::Traces> traces;
  std::unique_ptr<cg::AssemInstr> assem_instr;
  std::unique_ptr<ra::Result> allocation;

  // When generating proc fragment, do not output string assembly
  if (phase != Proc)
    return;
  TigerLog(frame_->GetLabel()+"\n");
  TigerLog("-------====IR tree=====-----\n");
  TigerLog(body_);

  {
    // Canonicalize
    TigerLog("-------====Canonicalize=====-----\n");
    canon::Canon canon(body_);

    // Linearize to generate canonical trees
    TigerLog("-------====Linearlize=====-----\n");
    tree::StmList *stm_linearized = canon.Linearize();
    TigerLog(stm_linearized);

    // Group list into basic blocks
    TigerLog("------====Basic block_=====-------\n");
    canon::StmListList *stm_lists = canon.BasicBlocks();
    TigerLog(stm_lists);

    // Order basic blocks into traces_
    TigerLog("-------====Trace=====-----\n");
    tree::StmList *stm_traces = canon.TraceSchedule();
    TigerLog(stm_traces);

    traces = canon.TransferTraces();
  }

  temp::Map *color = temp::Map::LayerMap(reg_manager->temp_map_, temp::Map::Name());
  {
    // Lab 5: code generation
    TigerLog("-------====Code generate=====-----\n");
    cg::CodeGen code_gen(frame_, std::move(traces));
    code_gen.Codegen();
    assem_instr = code_gen.TransferAssemInstr();
    TigerLog(assem_instr.get(), color);
  }

  assem::InstrList *il = assem_instr.get()->GetInstrList();

  /* for GC: save the locals in frame */
  std::vector<int> offsets_;
  for(auto v : *(frame_->locals_)){
    if(typeid(*v) == typeid(frame::InFrameAccess)){
      if(v->is_pointer){
        offsets_.push_back(static_cast<frame::InFrameAccess *>(v)->offset);
      }
    }
  }
  
  // Lab 6: register allocation
  TigerLog("----====Register allocate====-----\n");
  ra::RegAllocator reg_allocator(frame_, std::move(assem_instr));
  reg_allocator.RegAlloc();
  allocation = reg_allocator.TransferResult();
  il = allocation->il_;
  color = temp::Map::LayerMap(reg_manager->temp_map_, allocation->coloring_);
  
  /* for GC: generate the pointer map in current frame */
  generatePointerMap(frame_,reg_allocator.flow_graph_fac_->GetFlowGraph()->Nodes()->GetList(),reg_allocator.live_graph_fac_->in_.get(),offsets_,color);

  TigerLog("-------====Output assembly for %s=====-----\n",
           frame_->GetLabel());

  assem::Proc *proc = frame::ProcEntryExit3(frame_, il);
  
  std::string proc_name = frame_->GetLabel();

  fprintf(out, ".globl %s\n", proc_name.data());
  fprintf(out, ".type %s, @function\n", proc_name.data());
  // prologue
  fprintf(out, "%s", proc->prolog_.data());
  // body
  proc->body_->Print(out, color);
  // epilog_
  fprintf(out, "%s", proc->epilog_.data());
  fprintf(out, ".size %s, .-%s\n", proc_name.data(), proc_name.data());
}

void StringFrag::OutputAssem(FILE *out, OutputPhase phase, bool need_ra) const {
  // When generating string fragment, do not output proc assembly
  if (phase != String)
    return;

  fprintf(out, "%s:\n", label_->Name().data());
  int length = static_cast<int>(str_.size());
  // It may contain zeros in the middle of string. To keep this work, we need
  // to print all the charactors instead of using fprintf(str)
  fprintf(out, ".long %d\n", length);
  fprintf(out, ".string \"");
  for (int i = 0; i < length; i++) {
    if (str_[i] == '\n') {
      fprintf(out, "\\n");
    } else if (str_[i] == '\t') {
      fprintf(out, "\\t");
    } else if (str_[i] == '\"') {
      fprintf(out, "\\\"");
    } else {
      fprintf(out, "%c", str_[i]);
    }
  }
  fprintf(out, "\"\n");
}
} // namespace frame
