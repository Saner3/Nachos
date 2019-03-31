// progtest.cc 
//	Test routines for demonstrating that Nachos can load
//	a user program and execute it.  
//
//	Also, routines for testing the Console hardware device.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "console.h"
#include "addrspace.h"
#include "synch.h"

void Thread2Run(int which){
    printf("thread 2 running ... \n");
    machine->Run();
}
//----------------------------------------------------------------------
// StartProcess
// 	Run a user program.  Open the executable, load it into
//	memory, and jump to it.
//----------------------------------------------------------------------

void
StartProcess(char *filename)
{
    /*  this part might not run correctly because 
    OpenFile *executable = fileSystem->Open(filename);

#ifdef TWO_FILE_TEST
    OpenFile *executable2 = fileSystem->Open(filename);
    Thread *thread2 = new Thread("thread2");
    AddrSpace *space2;
#endif

    AddrSpace *space;
    
    if (executable == NULL) {
	printf("Unable to open file %s\n", filename);
	return;
    }
    printf("Allocate physical pages for thread 1\n");
    space = new AddrSpace(executable); 
#ifdef TWO_FILE_TEST
    printf("Allocate physical pages for thread 2\n");
    space2 = new AddrSpace(executable2);  
    thread2->space = space2; 
#endif 
    currentThread->space = space;

    delete executable;			// close file

#ifdef TWO_FILE_TEST
    delete executable2;
    space2->InitRegisters();
    space2->RestoreState();
    thread2->Fork(Thread2Run, 1);   // Yield in Fork
    printf("thread 1 running ...\n");
#endif
    */
    // --------- for Excercise 6/7 -------
    //clear the physical memory
    bzero(machine->mainMemory, sizeof(machine->mainMemory));
#ifdef REVERSE
    machine->InitRevPageTable();
#endif
    AddrSpace *space;

#ifdef TWO_FILE_TEST
    Thread *thread2 = new Thread("thread2");
#endif

    printf("Allocate physical pages for thread one\n");
    space = new AddrSpace(filename); 
    currentThread->space = space;

#ifdef TWO_FILE_TEST
    printf("Allocate physical pages for thread two\n");
    AddrSpace *space2 = new AddrSpace(filename);  
    thread2->space = space2; 
    space2->InitRegisters();
    space2->RestoreState();
    thread2->Fork(Thread2Run, 1);   // Yield in Fork
    printf("thread 1 running ...\n");
#endif

    space->InitRegisters();		// set the initial register values
    space->RestoreState();		// load page table register
    machine->Run();			// jump to the user progam
    ASSERT(FALSE);			// machine->Run never returns;
					// the address space exits
					// by doing the syscall "exit"
}

// Data structures needed for the console test.  Threads making
// I/O requests wait on a Semaphore to delay until the I/O completes.

//static Console *console;
static SynchConsole *console;
//static Semaphore *readAvail;
//static Semaphore *writeDone;

//----------------------------------------------------------------------
// ConsoleInterruptHandlers
// 	Wake up the thread that requested the I/O.
//----------------------------------------------------------------------

//static void ReadAvail(int arg) { readAvail->V(); }
//static void WriteDone(int arg) { writeDone->V(); }

//----------------------------------------------------------------------
// ConsoleTest
// 	Test the console by echoing characters typed at the input onto
//	the output.  Stop when the user types a 'q'.
//----------------------------------------------------------------------

void 
ConsoleTest (char *in, char *out)
{
    char ch;

    //console = new Console(in, out, ReadAvail, WriteDone, 0);
    console = new SynchConsole(in, out, 0);
    //readAvail = new Semaphore("read avail", 0);
    //writeDone = new Semaphore("write done", 0);
    
    for (;;) {
	//readAvail->P();		// wait for character to arrive
	ch = console->GetChar();
	console->PutChar(ch);	// echo it!
	//writeDone->P() ;        // wait for write to finish
	if (ch == 'q') return;  // if q, quit
    }
}
