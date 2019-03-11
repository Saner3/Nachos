// threadtest.cc 
//	Simple test case for the threads assignment.
//
//	Create two threads, and have them context switch
//	back and forth between themselves by calling Thread::Yield, 
//	to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"

// testnum is set in main.cc
int testnum = 1;
//----------------------------------------------------------------------
// SimpleThread
// 	Loop 5 times, yielding the CPU to another ready thread 
//	each iteration.
//
//	"which" is simply a number identifying the thread, for debugging
//	purposes.
//----------------------------------------------------------------------

void
SimpleThread(int which)
{
    int num;
    for (num = 0; num < 5; num++) {
        printf("*** thread %d UID %d TID %d looped %d times\n", which, currentThread->getUID(), currentThread->getTID(), num);
        currentThread->Yield();
    }
}
//----------------------------------------------------------------------
// DoNothing
//  Do nothing except printing info.
//----------------------------------------------------------------------

void
DoNothing(int which)
{
    printf("*** thread %s UID %d TID %d\n", currentThread->getName(), currentThread->getUID(), currentThread->getTID());
}

//-----------
// ts command
//-----------
void 
ThreadStatus(int which)
{
    printf(" Thread %d execute ts command.\n", which);
    printf(" PID  UID        state name\n");
    for (int i=0; i<MaxThreadsNum; ++i)
    {
        if (allThreads[i] != NULL)
        {
            char status_string[20];
            switch(allThreads[i]->getStatus())
            {
            case JUST_CREATED:
                sprintf(status_string, "JUST CREATED");
                break;
            case RUNNING:
                sprintf(status_string, "RUNNING");
                break;
            case READY:
                sprintf(status_string, "READY");
                break;
            case BLOCKED:
                sprintf(status_string, "BLOCKED");
                break;
            default:
                break;
            };
            printf("%4d %4d %12s %s\n", allThreads[i]->getTID(), allThreads[i]->getUID(), status_string, allThreads[i]->getName());
        }
    }
}

/*
void
ForkingOther1(int which)    // for ThreadTest4 Priority
{
    int currentPriority = currentThread->getPriority();
    printf("hello from thread %s\n", currentThread->getName());
    printf("%s is forking a thread of greater priority\n", currentThread->getName());
    Thread *t = new Thread("C", currentPriority - 1);
    t->Fork(DoNothing, 1);
    printf("goodbye from thread %s\n", currentThread->getName());
}*/

void
ForkingOther2(int which)    // for ThreadTest5 RoundRobin
{
    int currentPriority = currentThread->getPriority();
    printf("hello from thread %s\n", currentThread->getName());
    printf("%s is forking a thread of 3\n", currentThread->getName());
    Thread *t = new Thread("C", currentPriority, 3);
    t->Fork(DoNothing, 1);
    printf("goodbye from thread %s\n", currentThread->getName());
}

void WasteTime(int which)
{
    for (int i=0; i<30; ++i)
    {
        interrupt->SetLevel(IntOff);
        printf("%d from thread %s\n", i, currentThread->getName());
        interrupt->SetLevel(IntOn);
    }
}
//----------------------------------------------------------------------
// ThreadTest1
// 	Set up a ping-pong between two threads, by forking a thread 
//	to call SimpleThread, and then calling SimpleThread ourselves.
//----------------------------------------------------------------------

void
ThreadTest1()
{
    DEBUG('t', "Entering ThreadTest1");

    Thread *t = new Thread("forked thread");
    t->Fork(SimpleThread, 1);
    SimpleThread(0);
}

//--------Lab 1-------------//

//----------------------------------------------------------------------
// ThreadTest2
//  Thread number limits
//----------------------------------------------------------------------

void
ThreadTest2()
{
    DEBUG('t', "Entering ThreadTest2");
    
    Thread *t[129];
    for (int i=1; i<=128; ++i)
    {
        t[i] = new Thread("forked thread");
        t[i]->Print();
        t[i]->Fork(SimpleThread, i);
    }
    
    SimpleThread(0);
}
//----------------------------------------------------------------------
// ThreadTest3
//  Print Thread Status (ts command)
//----------------------------------------------------------------------

void
ThreadTest3()
{
    DEBUG('t', "Entering ThreadTest3");
    Thread *t1 = new Thread("forked thread 1");
    t1->Fork(ThreadStatus, 1);

    Thread *t2 = new Thread("forked thread 2");
    t2->Fork(SimpleThread, 2);

    Thread *t3 = new Thread("forked thread 3");
    t3->Fork(SimpleThread, 3);
}

//-------end Lab 1-------------

//---------Lab 2-------------

/*  // Priority
void
ThreadTest4()
{
    DEBUG('t', "Entering ThreadTest4");
    int currentPriority = currentThread->getPriority();
    printf("hello from thread %s\n", currentThread->getName());
    printf("%s is forking a thread of greater priority\n", currentThread->getName());
    Thread *t = new Thread("B", currentPriority - 1);
    t->Fork(ForkingOther, 1);
    printf("goodbye from thread %s\n", currentThread->getName());
}
*/

void
ThreadTest5()   // RoundRobin
{
    DEBUG('t', "Entering ThreadTest5");
    int currentPriority = currentThread->getPriority();
    printf("hello from thread %s\n", currentThread->getName());
    printf("%s is forking a thread of 3 timeslice\n", currentThread->getName());
    Thread *t = new Thread("B", currentPriority-1, 3);
    t->Fork(WasteTime, 1);
    printf("goodbye from thread %s\n", currentThread->getName());
}
//-------end Lab 2-------------

//----------------------------------------------------------------------
// ThreadTest
// 	Invoke a test routine.
//----------------------------------------------------------------------

void
ThreadTest()
{
    switch (testnum) {
    case 1:
	ThreadTest1();
    break;
    case 2:
    ThreadTest2();
	break;
    case 3:
    ThreadTest3();
    break;
    /*
    case 4:
    ThreadTest4();
    break;*/
    case 5:
    ThreadTest5();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

