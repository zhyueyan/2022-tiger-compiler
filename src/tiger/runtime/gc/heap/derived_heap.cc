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
                // HeapBlock* b = new HeapBlock(size,cur_free->heapPtr,NULL,NULL,type);
                used_list->Insert(cur_free);
                free_list->Delete(cur_free);
                return cur_free->heapPtr;
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

    void DerivedHeap::Mark(HeapBlock *block)
    {
        block->mark = 1;
        if(block->pointermap != NULL){
            for(int i = 0; i < block->size/WORD_SIZE; i++){
                if(block->pointermap[i] == 1){
                    uint64_t *address = (uint64_t *)block->heapPtr + i;
                    HeapBlock *used_block = used_list->first;
                    for(int i = 0; i < used_list->listSize; i++){    
                        if(used_block->heapPtr == (char *)*address){
                            if(used_block->mark != 1){
                                Mark(used_block);
                                break;
                            }
                        }
                        used_block = used_block->next;
                    }
                }
            }
        }
    }
    void DerivedHeap::GC()
    {
        uint64_t *sp;
        GET_TIGER_STACK(sp);
        // printf("sp: %#x ret %#x\n",sp,(*sp));
        // printf("size: %d",used);
        std::vector<HeapBlock *> roots;
        ReadedPointerMap start;
        /* find the start map */
        for(int i = 0; i < pointer_maps.size(); i++){
            if(pointer_maps[i].returnAddress == (*sp)){
                start = pointer_maps[i];
            }
        }
        /* get all readed pointer maps */
        std::vector<ReadedPointerMap> maps;
        uint64_t *cur_sp = sp;
        while(true){
            maps.push_back(start);
            cur_sp = cur_sp + start.frameSize/WORD_SIZE + 1;
            // printf("sp: %#x ret: %#x\n",cur_sp,(*cur_sp));
            for(int i = 0; i < pointer_maps.size(); i++){
                if(pointer_maps[i].returnAddress == (*cur_sp)){
                    start = pointer_maps[i];
                    continue;
                }
            }
            break;
        }
        // exit(0);
        /* get roots */
        HeapBlock *used_block = used_list->first;
        for(int i = 0; i < used_list->listSize; i++){
            cur_sp = sp;
            for(int k = 0; k < maps.size(); k++){
                ReadedPointerMap map = maps[k];
                
                for(int j = 0; j < map.offsets.size(); j++){
                    cur_sp = cur_sp + map.frameSize/WORD_SIZE + 1;
                    // printf("cur sp: %#x\n",cur_sp);
                    // printf("stack pointer: %#x\n",cur_sp+map.offsets[j]/WORD_SIZE);
                    // printf("stack value: %d\n",*(cur_sp+map.offsets[j]/WORD_SIZE));
                    // printf("heap value: %d\n",(uint64_t)used_block->heapPtr);
                    if(*(cur_sp+map.offsets[j]/WORD_SIZE) == (uint64_t)used_block->heapPtr){
                            roots.push_back(used_block);
                        }
                }
            }
            used_block = used_block->next;
        }
        /* start mark */
        for(int i = 0; i < roots.size(); i++){
            Mark(roots[i]);
        }

        /* start sweep */
        HeapBlock *block = used_list->first;
        for(int i = 0; i < used_list->listSize; i++){
            HeapBlock *temp = block->next;
            if(block->mark == 0){
                used_list->Delete(block);
                free_list->Insert(block);
                used -= block->size;
            }
            block = temp;
        }
        
        
    }

    void DerivedHeap::getPointerMap()
    {
        uint64_t *start = &GLOBAL_GC_ROOTS;
        uint64_t *pos = start;
        while(true){
            uint64_t nextAddress = *pos;
            pos++;
            uint64_t returnAddress = *pos;
            pos++;
            uint64_t frameSize = *pos;
            pos++;
            uint64_t reg = *pos;
            bool* registerPointers = new bool[6];

            for(int i = 5; i >= 0; i--){
                registerPointers[i] = reg%2;
                reg = reg/2;
            }
            pos++;
            int64_t *offset = (int64_t *)pos;
            std::vector<int64_t> offsets;
            while(*offset != -1){
                offsets.push_back(*offset);
                offset++;
                pos++;
            }
            pos++;
            ReadedPointerMap new_map;
            new_map.nextAddress = nextAddress;
            new_map.returnAddress = returnAddress;
            new_map.frameSize = frameSize;
            new_map.registerPointers = registerPointers;
            new_map.offsets = offsets;
            pointer_maps.push_back(new_map);
            // printf("nextAddress %d, returnAddress %d, frameSize %d\n",nextAddress,returnAddress,frameSize);
            // printf("registerPointers\n");
            // for(int i = 0; i < 6; i++){
            //     printf("%d ",registerPointers[i]);
            // }
            // printf("\n");
            // printf("offsets\n");
            // for(int i = 0; i < offsets.size(); i++){
            //     printf("%d ",offsets[i]);
            // }
            // printf("\n");
            if(nextAddress == 0) break;
        }
    }
} // namespace gc

