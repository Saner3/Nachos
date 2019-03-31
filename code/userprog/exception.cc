// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"

//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);

    if ((which == SyscallException) && (type == SC_Halt)) {
	DEBUG('a', "Shutdown, initiated by user program.\n");

    // print TLB statistics
#ifdef USE_TLB
#ifdef TLB_RANDOM
    printf("using random tlb replace strategy\n");
#endif
#ifdef TLB_LRU
    printf("using LRU tlb replace strategy\n");
#endif
#ifdef TLB_FIFO
    printf("using FIFO tlb replace strategy\n");
#endif
    printf("tlb_miss: %d, tlb_hit: %d, tlb miss rate: %.4f\n", 
        machine->tlb_miss, machine->tlb_hit, (float)machine->tlb_miss/(machine->tlb_miss + machine->tlb_hit));
#endif
   	interrupt->Halt();
    } /*else if ((which == SyscallException) && (type == SC_Exit))
    {
        printf("%s exiting\n", currentThread->getName());
    // deallocate all physical memory
        for (int i=0; i<machine->pageTableSize; ++i){
#ifndef REVERSE
            if (machine->pageTable[i].valid){
                machine->pageTable[i].valid = FALSE;
                machine->DeallocPhyPage(machine->pageTable[i].physicalPage);
            }
#else
            if (machine->pageTable[i].valid && machine->pageTable[i].tid == currentThread->getTID()){
                machine->pageTable[i].valid = FALSE;
                machine->DeallocPhyPage(i);
            }
#endif
        }
        int NextPC = machine->ReadRegister(NextPCReg);
        machine->WriteRegister(PCReg, NextPC);
    } */else if (which == PageFaultException){
        int badvaddr = machine->ReadRegister(BadVAddrReg);
        unsigned int vpn = (unsigned) badvaddr/PageSize;
        if (machine->tlb != NULL)   // TLB miss
        {
            DEBUG('a', "PageFaultException at vaddr %d\n", badvaddr);
            int pos = -1;
            for (int i = 0; i < TLBSize; i++)
                if (machine->tlb[i].valid == FALSE) // find an empty entry
                {
                    pos = i;
                    break;
                }
            // replace
            if (pos == -1){
#ifdef TLB_RANDOM
                pos = Random() % TLBSize;
#endif
#ifdef TLB_LRU
                int maxitem = -1;
                for (int i = 0; i < TLBSize; i++){
                    if (machine->tlb_LRUqueue[i] > maxitem){
                        maxitem = machine->tlb_LRUqueue[i];
                        pos = i;
                    }
                } 
                for (int i = 0; i < TLBSize; i++)
                    machine->tlb_LRUqueue[i]++;
#endif
#ifdef TLB_FIFO
                pos = TLBSize - 1;
                for (int i = 0; i < TLBSize-1; i++){
                    machine->tlb[i] = machine->tlb[i+1];
                } 
#endif
            }
            // replace
            if (machine->pageTable[vpn].valid)
            {
                DEBUG('a', "Load from memory to tlb\n");
                machine->tlb[pos].valid = TRUE;
                machine->tlb[pos].virtualPage = vpn;
                machine->tlb[pos].physicalPage = machine->pageTable[vpn].physicalPage;
                machine->tlb[pos].use = FALSE;  // ?
                machine->tlb[pos].dirty = FALSE;   // ?
                machine->tlb[pos].readOnly = FALSE;   // ?
#ifdef TLB_LRU
                machine->tlb_LRUqueue[pos] = 0;
#endif
                return;
            }
            // else, no mapping of this page
        }
        // no mapping of this page
        char *swapfilename = currentThread->space->swapfilename;
        OpenFile *swapfile = fileSystem->Open(swapfilename);

        int pos = -1;   // physical page number
        int victim_paddr = -1;  // physical address
        if (currentThread->space->allocatedPages < maxPhyPages){
            // allocate a new page 
            pos = machine->AllocPhyPage();
            if (pos != -1) {
                currentThread->space->pageNumIncrease();
                victim_paddr = pos * PageSize;
                printf("load vpn %d to physical page %d\n", vpn, pos);
            }
        }
        if (pos == -1){
            // replace a physical page;
#ifndef REVERSE
            int j = Random() % currentThread->space->allocatedPages;
            int victim_vpn = -1;
            // we choose the j-th physical page as victim (randomly).
            for (int i=0; i<machine->pageTableSize; ++i){
                if (machine->pageTable[i].valid)
                {
                    if (j==0)//found
                    {
                        victim_vpn = i;
                        pos = machine->pageTable[victim_vpn].physicalPage;
                        break;
                    }
                    j--;
                }
            }
#else
            int j = Random() % currentThread->space->allocatedPages;
            int victim_vpn = -1;
            // we choose the j-th physical page as victim (randomly).
            for (int i=0; i<NumPhysPages; ++i){
                if (machine->pageTable[i].valid && machine->pageTable[i].tid == currentThread->getTID())
                {
                    if (j==0)//found
                    {
                        pos = i;
                        victim_vpn = machine->pageTable[pos].virtualPage;
                        break;
                    }
                    j--;
                }
            }
#endif
            if (pos == -1) ASSERT(FALSE);
            //printf("page of vpn %d replace to ppn %d, evict vpn %d\n", vpn, pos, victim_vpn);
            victim_paddr = pos * PageSize;
            // check if it is dirty
#ifndef REVERSE
            if (machine->pageTable[victim_vpn].dirty){
                //write back
                swapfile->WriteAt(&(machine->mainMemory[victim_paddr]), PageSize, victim_vpn * PageSize);
                machine->pageTable[victim_vpn].dirty = FALSE;
                //printf("vpn %d (ppn %d) is dirty, write back\n", victim_vpn, pos);
            }
            // modify pageTable
            machine->pageTable[victim_vpn].valid = FALSE;
        }
        // load content from file
        swapfile->ReadAt(&(machine->mainMemory[victim_paddr]), PageSize, vpn * PageSize);
        // modify pageTable
        machine->pageTable[vpn].valid = TRUE;
        machine->pageTable[vpn].virtualPage = vpn;
        machine->pageTable[vpn].physicalPage = pos;
        machine->pageTable[vpn].use = FALSE;
        machine->pageTable[vpn].dirty = FALSE;
        machine->pageTable[vpn].readOnly = FALSE;
#else
            if (machine->pageTable[pos].dirty){
                //write back
                swapfile->WriteAt(&(machine->mainMemory[victim_paddr]), PageSize, victim_vpn * PageSize);
                machine->pageTable[pos].dirty = FALSE;
                printf("vpn %d (ppn %d) is dirty, write back\n", victim_vpn, pos);
            }
        }
        // load content from file
        swapfile->ReadAt(&(machine->mainMemory[victim_paddr]), PageSize, vpn * PageSize);
        // modify pageTable
        machine->pageTable[pos].valid = TRUE;
        machine->pageTable[pos].virtualPage = vpn;
        machine->pageTable[pos].physicalPage = pos;
        machine->pageTable[pos].tid = currentThread->getTID();
        machine->pageTable[pos].use = FALSE;
        machine->pageTable[pos].dirty = FALSE;
        machine->pageTable[pos].readOnly = FALSE;
#endif
        delete swapfile;
        // do not need to increase PC
        // display pagetable
        for (int i=0; i<machine->pageTableSize; ++i){
            if (machine->pageTable[i].valid)
            printf("pagetable%d vpn: %d ppn: %d valid: %d\n", i, machine->pageTable[i].virtualPage, 
                machine->pageTable[i].physicalPage, machine->pageTable[i].valid);
        }
        printf("\n");
        
    }else {
	printf("Unexpected user mode exception %d %d\n", which, type);
	ASSERT(FALSE);
    }
}
