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
#include "synch.h"

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
// ts command, lab 1
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
ForkingOther(int which)    // for ThreadTest4 Priority, lab 2
{
    int currentPriority = currentThread->getPriority();
    printf("hello from thread %s\n", currentThread->getName());
    printf("%s is forking a thread of greater priority\n", currentThread->getName());
    Thread *t = new Thread("C", currentPriority - 1);
    t->Fork(DoNothing, 1);
    printf("goodbye from thread %s\n", currentThread->getName());
}*/

// WasteTime: re-enable interrupt to pass away the time
void WasteTime(int which)
{
    for (int i=0; i<30; ++i)
    {
        interrupt->SetLevel(IntOff);
        printf("%d from thread %s\n", i, currentThread->getName());
        interrupt->SetLevel(IntOn);
    }
}

//--------Lab 3---------
// using semaphore or condition var
// to solve producer-consumer problem
//
List *resources_buffer;
Semaphore *resources_empty;
Semaphore *resources_full;
Semaphore *resources_mutex;

Lock *condition_mutex;
Condition *condition_full;
Condition *condition_empty;
int empty_buffer, total_buffer;

void buffer_print_item(int item){
    printf("%d ", item);
}
void produce_semaphore(int which)
{
    int yieldOrNot = Random()%2;
    if (yieldOrNot){
        printf("%s wants to yield CPU\n", currentThread->getName());
        currentThread->Yield();
    }
    int resource = Random();
    //printf("producer %d produces item %d\n", which, resource);
    // is there empty buffer?
    resources_empty->P();
    resources_mutex->P();
    resources_buffer->Append((void *)resource);
    printf("producer %d puts item %d in buffer\ncurrent buffer: ", which, resource);
    resources_buffer->Mapcar(buffer_print_item);
    printf("\n");
    resources_mutex->V();
    resources_full->V();
}

void consume_semaphore(int which)
{
    int yieldOrNot = Random()%2;
    if (yieldOrNot){
        printf("%s wants to yield CPU\n", currentThread->getName());
        currentThread->Yield();
    }
    //printf("consumer %d wants to take an item\n", which);
    //printf("\n");
    // take a resource away
    resources_full->P();
    resources_mutex->P();
    int resource = (int)resources_buffer->Remove();
    printf("consumer %d takes item %d from buffer\ncurrent buffer: ", which, resource);
    resources_buffer->Mapcar(buffer_print_item);
    printf("\n");
    resources_mutex->V(); 
    resources_empty->V();
}
void produce_condition(int which)
{
    int yieldOrNot = Random()%2;
    if (yieldOrNot){
        //printf("%s wants to yield CPU\n", currentThread->getName());
        currentThread->Yield();
    }
    int resource = Random();
    //printf("%s produces item %d\n", currentThread->getName(), resource);
    condition_mutex->Acquire();
    while (empty_buffer == 0){
        condition_empty->Wait(condition_mutex);
    }
    resources_buffer->Append((void *)resource);
    empty_buffer --;
    printf("%s puts item %d in buffer\ncurrent buffer: ", currentThread->getName(), resource);
    resources_buffer->Mapcar(buffer_print_item);
    printf("\n");
    if (empty_buffer == total_buffer-1){
        condition_full->Broadcast(condition_mutex);
    }
    condition_mutex->Release();
}

void consume_condition(int which)
{
    int yieldOrNot = Random()%2;
    if (yieldOrNot){
        //printf("%s wants to yield CPU\n", currentThread->getName());
        currentThread->Yield();
    }
    //printf("%s wants to take an item\n", currentThread->getName());
    condition_mutex->Acquire();
    while (empty_buffer == total_buffer)
        condition_full->Wait(condition_mutex);
    int resource = (int)resources_buffer->Remove();
    empty_buffer ++;
    printf("%s takes item %d from buffer\ncurrent buffer: ", currentThread->getName(), resource);
    resources_buffer->Mapcar(buffer_print_item);
    printf("\n");
    if (empty_buffer == 1)
        condition_empty->Broadcast(condition_mutex);
    condition_mutex->Release();
}
//-------end Lab 3---------

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

//------------Lab 3------------
void
ThreadTest6()   // Producer Consumer semaphore
{
    resources_buffer = new List();
    resources_mutex = new Semaphore("mutex", 1);
    resources_full = new Semaphore("full", 0);
    resources_empty = new Semaphore("empty", 3);    // assume there are 3 buffers

    // create 10 producer and 10 consumers
    Thread *producers[10];
    Thread *consumers[10];
    for (int i=0; i<10; ++i){
        producers[i] = new Thread("producer");
        //printf("producer %d TID %d ready\n", i, producers[i]->getTID());
        producers[i]->Fork(produce_semaphore, i);
    }

    for (int i=0; i<10; ++i){
        consumers[i] = new Thread("consumer");
        //printf("consumer %d TID %d ready\n", i, consumers[i]->getTID());
        consumers[i]->Fork(consume_semaphore, i);
    }
}

void
ThreadTest7()   // Producer Consumer semaphore
{
    resources_buffer = new List();
    empty_buffer = total_buffer = 3;
    condition_mutex = new Lock("mutex");
    condition_empty = new Condition("empty");
    condition_full = new Condition("full");

    // create 10 producer and 10 consumers
    Thread *producers[10];
    Thread *consumers[10];
    char producerName[10][100];
    char consumerName[10][100];
    for (int i=0; i<10; ++i){
        sprintf(producerName[i], "producer %d\0", i);
        producers[i] = new Thread(producerName[i]);
        //printf("%s ready\n", producerName[i]);
        producers[i]->Fork(produce_condition, i);
        //producers[i]->Fork(WasteTime, i);
    }

    for (int i=0; i<10; ++i){
        sprintf(consumerName[i], "consumer %d\0", i);
        consumers[i] = new Thread(consumerName[i]);
        //printf("%s ready\n", consumerName[i]);
        consumers[i]->Fork(consume_condition, i);
        //consumers[i]->Fork(WasteTime, i);
    }
    printf("main finishes...\n");
}
//------------end Lab 3------------
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
    case 6:
    ThreadTest6();
    break;
    case 7:
    ThreadTest7();
    break;
    default:
	printf("No test specified.\n");
	break;
    }
}

