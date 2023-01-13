#pragma once

#include "heap.h"
#include "../roots/roots.h"
#include <vector>

namespace gc {

class HeapBlock {
  public:
   size_t size;
   int mark;
   bool *pointermap;//show the pointer field: Null for Array
   char* heapPtr;
   HeapBlock* next;
   HeapBlock* prev;
   HeapBlock(size_t s, char* ptr, HeapBlock* n, HeapBlock* p, bool *map):
     size(s),heapPtr(ptr),next(n),prev(p),pointermap(map)
   {
     mark = 0;
   }
};
class HeapBlockList {
  public:
   int listSize;
   HeapBlock* first;
   HeapBlockList(){listSize = 0; first = NULL;}
   void Insert(HeapBlock* b){
     b->next = first;
     if(first)
      first->prev = b;
     first = b;
     listSize++;
   }
   void Delete(HeapBlock* b){
     if(b->prev){
       b->prev->next = b->next;
     }
     if(b->next){
       b->next->prev = b->prev;
     }
     listSize--;
   }
};

class DerivedHeap : public TigerHeap {
  // TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
  private:
   char *heap;
   int limit;
   int used;
   HeapBlockList* free_list;
   HeapBlockList* used_list;
   std::vector<ReadedPointerMap> pointer_maps;
   void Mark(HeapBlock *block);
  public:
   DerivedHeap(){
    getPointerMap();
   }
   char *Allocate(bool *type, uint64_t size);
   uint64_t Used() const;
   uint64_t MaxFree() const;
   void Initialize(uint64_t size);
   void GC();
   void getPointerMap();

};


} // namespace gc

