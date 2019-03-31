// fstest.cc 
//	Simple test routines for the file system.  
//
//	We implement:
//	   Copy -- copy a file from UNIX to Nachos
//	   Print -- cat the contents of a Nachos file 
//	   Perftest -- a stress test for the Nachos file system
//		read and write a really large file in tiny chunks
//		(won't work on baseline system!)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "utility.h"
#include "filesys.h"
#include "system.h"
#include "thread.h"
#include "disk.h"
#include "stats.h"

#define TransferSize 	10 	// make it small, just to be difficult

//----------------------------------------------------------------------
// Copy
// 	Copy the contents of the UNIX file "from" to the Nachos file "to"
//----------------------------------------------------------------------

void
Copy(char *from, char *to)
{
    FILE *fp;
    OpenFile* openFile;
    int amountRead, fileLength;
    char *buffer;

// Open UNIX file
    if ((fp = fopen(from, "r")) == NULL) {	 
	printf("Copy: couldn't open input file %s\n", from);
	return;
    }

// Figure out length of UNIX file
    fseek(fp, 0, 2);		
    fileLength = ftell(fp);
    fseek(fp, 0, 0);

// Create a Nachos file of the same length
    DEBUG('f', "Copying file %s, size %d, to file %s\n", from, fileLength, to);
    if (!fileSystem->Create(to, fileLength)) {	 // Create Nachos file
	printf("Copy: couldn't create output file %s\n", to);
	fclose(fp);
	return;
    }
    
    openFile = fileSystem->Open(to);
    ASSERT(openFile != NULL);
    
// Copy the data in TransferSize chunks
    buffer = new char[TransferSize];
    while ((amountRead = fread(buffer, sizeof(char), TransferSize, fp)) > 0)
	openFile->Write(buffer, amountRead);	
    delete [] buffer;

// Close the UNIX and the Nachos files
    delete openFile;
    fclose(fp);
}

//----------------------------------------------------------------------
// Print
// 	Print the contents of the Nachos file "name".
//----------------------------------------------------------------------

void
Print(char *name)
{
    OpenFile *openFile;    
    int i, amountRead;
    char *buffer;

    if ((openFile = fileSystem->Open(name)) == NULL) {
	printf("Print: unable to open file %s\n", name);
	return;
    }
    
    buffer = new char[TransferSize];
    while ((amountRead = openFile->Read(buffer, TransferSize)) > 0)
	for (i = 0; i < amountRead; i++)
	    printf("%c", buffer[i]);
    delete [] buffer;

    delete openFile;		// close the Nachos file
    return;
}

//----------------------------------------------------------------------
// PerformanceTest
// 	Stress the Nachos file system by creating a large file, writing
//	it out a bit at a time, reading it back a bit at a time, and then
//	deleting the file.
//
//	Implemented as three separate routines:
//	  FileWrite -- write the file
//	  FileRead -- read the file
//	  PerformanceTest -- overall control, and print out performance #'s
//----------------------------------------------------------------------

#define FileName 	"TestFile" //"TestFileVeryVeryVeryVeryVeryLongName"
#define Contents 	"1234567890"
#define ContentSize 	strlen(Contents)
#define FileSize 	((int)(ContentSize * 40))


#define FileName2    "Document/Self-introduction"
#define Contents2    "abcdefghij"
#define ContentSize2     strlen(Contents2)
#define FileSize2    ((int)(ContentSize2 *13))

static void 
FileWrite()
{
    OpenFile *openFile;    
    int i, numBytes;

    //printf("Sequential write of %d byte file, in %d byte chunks\n", 
	//FileSize, ContentSize);
    /*  test recursive directory

    if (!fileSystem->Create("Document", 0, TRUE)) {
      printf("Perf test: can't create Document\n");
      return;
    }
    else printf("create dir Document\n");

    if (!fileSystem->Create(FileName2, FileSize2)) {
      printf("Perf test: can't create %s\n", FileName2);
      return;
    } 
    else printf("create %s\n", FileName2);

    if (!fileSystem->Create("Document/Photos", 0, TRUE)) {
      printf("Perf test: can't create Document/Photos\n");
      return;
    } 
    else printf("create dir Document/Photos\n");
    */
    if (!fileSystem->Create(FileName, 0)) {
      printf("Perf test: can't create %s\n", FileName);
      return;
    }
    else printf("create %s\n", FileName);
    
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
	printf("Perf test: unable to open %s\n", FileName);
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        //printf("Write %s to file by %s (%d Bytes)\n", Contents, currentThread->getName(), ContentSize);
        numBytes = openFile->Write(Contents, ContentSize);
    	if (numBytes < 10) {
    	    printf("Perf test: unable to write %s\n", FileName);
    	    delete openFile;
    	    return;
    	}
        //else{
        //    printf("Write 10 Bytes to file\n");
        //}
    }
    delete openFile;	// close file
    /*
    openFile = fileSystem->Open(FileName2);
    if (openFile == NULL) {
    printf("Perf test: unable to open %s\n", FileName2);
    return;
    }
    for (i = 0; i < FileSize2; i += ContentSize2) {
        numBytes = openFile->Write(Contents2, ContentSize2);
        if (numBytes < 10) {
            printf("Perf test: unable to write %s\n", FileName2);
            delete openFile;
            return;
        }
    }
    delete openFile;    // close file*/
}

static void 
FileRead()
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize];
    int i, numBytes;

    printf("Sequential read of %d byte file, in %d byte chunks\n", 
	FileSize, ContentSize);

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
	printf("Perf test: unable to open file %s\n", FileName);
	delete [] buffer;
	return;
    }
    for (i = 0; i < FileSize; i += ContentSize) {
        numBytes = openFile->Read(buffer, ContentSize);
    /*
	if ((numBytes < 10) || strncmp(buffer, Contents, ContentSize)) {
	    printf("Perf test: unable to read %s\n", FileName);
	    delete openFile;
	    delete [] buffer;
	    return;
	}*/
    if (numBytes < 10) {
        printf("Perf test: unable to read %s\n", FileName);
        delete openFile;
        delete [] buffer;
        return;
    }
    //buffer[numBytes] = '\0';
    //printf("Content read by %s (%d Bytes): %s\n", currentThread->getName(), numBytes, buffer);
    }
    delete [] buffer;
    delete openFile;	// close file
}

void 
TestWrite(int which){
    OpenFile *openFile;    
    int i, numBytes;
    
    openFile = fileSystem->Open(FileName);
    if (openFile == NULL) {
    printf("Perf test: unable to open %s\n", FileName);
    return;
    }
    
    for (i = 0; i < FileSize2; i += ContentSize2) {
        char ContentsDIY[11];
        sprintf(ContentsDIY, "$%04d$%04d", which, i);
        //printf("Write %s to file by %s (%d Bytes)\n", Contents2, currentThread->getName(), ContentSize2);
        numBytes = openFile->Write(ContentsDIY, ContentSize2);
        if (numBytes < 10) {
            printf("Perf test: unable to write %s\n", FileName);
            delete openFile;
            return;
        }
    }
    delete openFile;    // close file
    fileSystem->Remove(FileName);
}

void 
TestRead(int which)
{
    OpenFile *openFile;    
    char *buffer = new char[ContentSize2];
    int i, numBytes;

    if ((openFile = fileSystem->Open(FileName)) == NULL) {
    printf("Perf test: unable to open file %s\n", FileName);
    delete [] buffer;
    return;
    }
    for (i = 0; i < FileSize2; i += ContentSize2) {
        numBytes = openFile->Read(buffer, ContentSize2);
        if (numBytes < 10) {
            printf("Perf test: unable to read %s\n", FileName);
            delete openFile;
            delete [] buffer;
            return;
        }
        //buffer[numBytes] = '\0';
        //printf("Content read by %s (%d Bytes): %s\n", currentThread->getName(), numBytes, buffer);
    }
    delete [] buffer;
    delete openFile;    // close file
    fileSystem->Remove(FileName);
}
void
PerformanceTest()
{
    printf("Starting file system performance test:\n");
    stats->Print();
    FileWrite();
    /*
    Thread *t2 = new Thread("writer1");
    t2->Fork(TestWrite, 2);
    Thread *t1 = new Thread("reader1");
    t1->Fork(TestRead, 1);
    Thread *t4 = new Thread("writer2");
    t4->Fork(TestWrite, 4);
    Thread *t3 = new Thread("reader2");
    t3->Fork(TestRead, 3);
    */
    FileRead();
    
    //fileSystem->Print();
    
    if (!fileSystem->Remove(FileName)) {
      printf("Perf test: unable to remove %s\n", FileName);
      return;
    }
    stats->Print();
}

