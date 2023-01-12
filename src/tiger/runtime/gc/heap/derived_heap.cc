#include "derived_heap.h"
#include <stdio.h>
#include <stack>
#include <cstring>

namespace gc {
// TODO(lab7): You need to implement those interfaces inherited from TigerHeap correctly according to your design.
    void DerivedHeap::Initialize(uint64_t size)
    {
        heap = new char[size];
        limit = size;
        used = 0;
        free_list = new HeapBlockList();
        used_list = new HeapBlockList();
        free_list->Insert(new HeapBlock(size,heap,NULL,NULL,NULL));
    }

    char *DerivedHeap::Allocate(bool *type, uint64_t size)
    {
        HeapBlock* cur_free = free_list->first;
        for(int i = 0; i < free_list->listSize; i++){
            if(cur_free->size > size){
                used += size;
                HeapBlock* b = new HeapBlock(size,cur_free->heapPtr,NULL,NULL,type);
                used_list->Insert(b);
                cur_free->heapPtr += size;
                cur_free->size -= size;

                return b->heapPtr;
            }
            else if(cur_free->size == size){
                used += size;
                HeapBlock* b = new HeapBlock(size,cur_free->heapPtr,NULL,NULL,type);
                used_list->Insert(b);
                free_list->Delete(b);
                return b->heapPtr;
            }
            cur_free = cur_free->next;
        }
        return NULL;
    }

    uint64_t DerivedHeap::Used() const
    {
        return used;
    }

    uint64_t DerivedHeap::MaxFree() const
    {
        int max = 0;
        HeapBlock* cur = free_list->first;
        for(int i = 0; i < free_list->listSize; i++){
            if(cur->size > max) max = cur->size;
            cur = cur->next;
        }
        return max;
    }

    void DerivedHeap::GC()
    {
        
    }
} // namespace gc

