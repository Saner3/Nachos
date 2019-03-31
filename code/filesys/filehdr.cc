// filehdr.cc 
//	Routines for managing the disk file header (in UNIX, this
//	would be called the i-node).
//
//	The file header is used to locate where on disk the 
//	file's data is stored.  We implement this as a fixed size
//	table of pointers -- each entry in the table points to the 
//	disk sector containing that portion of the file data
//	(in other words, there are no indirect or doubly indirect 
//	blocks). The table size is chosen so that the file header
//	will be just big enough to fit in one disk sector, 
//
//      Unlike in a real system, we do not keep track of file permissions, 
//	ownership, last modification date, etc., in the file header. 
//
//	A file header can be initialized in two ways:
//	   for a new file, by modifying the in-memory data structure
//	     to point to the newly allocated data blocks
//	   for a file already on disk, by reading the file header from disk
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"

#include "system.h"
#include "filehdr.h"
//----------------------------------------------------------------------
// FileHeader::Allocate
// 	Initialize a fresh file header for a newly created file.
//	Allocate data blocks for the file out of the map of free disk blocks.
//	Return FALSE if there are not enough free blocks to accomodate
//	the new file.
//
//	"freeMap" is the bit map of free disk sectors
//	"fileSize" is the bit map of free disk sectors
//----------------------------------------------------------------------

bool
FileHeader::Allocate(BitMap *freeMap, int fileSize)
{ 

    numBytes = fileSize;
    numSectors  = divRoundUp(fileSize, SectorSize);
    if (numSectors <= NumDirectI){
        if (freeMap->NumClear() < numSectors)
	       return FALSE;		// not enough space
        for (int i = 0; i < numSectors; i++)
        dataSectors[i] = freeMap->Find();
        return TRUE;
    }
    else {
        // how many secondary index Table do we need ?
        int secondaryTableNum = divRoundUp((numSectors - NumDirectI), NumDirectII);
        ASSERT(secondaryTableNum <= NumDirectTable);
        if (freeMap->NumClear() < numSectors + secondaryTableNum)   
           return FALSE;        // not enough space
        int k = 0;    // number of allocated sectors

        // we need secondary index table
        // fisrt, allocate all direct indexs
        for (int i = 0; i < NumDirectI; i++, k++)
            dataSectors[i] = freeMap->Find();

        // then, allocate secondary index table
        for (int i = 0; i < secondaryTableNum; ++i){
            int secondaryIndexTable[NumDirectII] = {0};
            dataSectors[NumDirectI + i] = freeMap->Find();
            for (int j = 0; j < NumDirectII && k < numSectors; ++j,++k){
                secondaryIndexTable[j] = freeMap->Find();
            }
            SetLastAccessTime();
            SetLastModifyTime();
            synchDisk->WriteSector(dataSectors[NumDirectI + i], (char *)secondaryIndexTable); 
        }
        return TRUE;
    }
}

//----------------------------------------------------------------------
// FileHeader::Deallocate
// 	De-allocate all the space allocated for data blocks for this file.
//
//	"freeMap" is the bit map of free disk sectors
//----------------------------------------------------------------------

void 
FileHeader::Deallocate(BitMap *freeMap)
{
    if (numSectors <= NumDirectI){
        for (int i = 0; i < numSectors; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
    
    }
    else {
        // secondary index table is used
        int secondaryTableNum = divRoundUp((numSectors - NumDirectI), NumDirectII);
        for (int i = 0; i < NumDirectI; i++) {
            ASSERT(freeMap->Test((int) dataSectors[i]));  // ought to be marked!
            freeMap->Clear((int) dataSectors[i]);
        }
        for (int i=0; i<secondaryTableNum; ++i){
            // read secondary index table to memory
            int secondaryIndexTable[NumDirectII];
            SetLastAccessTime();
            synchDisk->ReadSector((int) dataSectors[NumDirectI + i], (char *)secondaryIndexTable);
            for (int j=0; j<NumDirectII; ++j){
                if (freeMap->Test((int) secondaryIndexTable[j]))
                    freeMap->Clear((int) secondaryIndexTable[j]);
                else
                    break;
            }
            // deallocate sector of secondary table 
            freeMap->Clear((int) dataSectors[NumDirectI + i]);
        }
    }

    
}

//----------------------------------------------------------------------
// FileHeader::FetchFrom
// 	Fetch contents of file header from disk. 
//
//	"sector" is the disk sector containing the file header
//----------------------------------------------------------------------

void
FileHeader::FetchFrom(int sector)
{
    synchDisk->ReadSector(sector, (char *)this);
    // --- lab 5 ---
    // update access time 
    SetLastAccessTime();
    // -- end lab 5 ---
}

//----------------------------------------------------------------------
// FileHeader::WriteBack
// 	Write the modified contents of the file header back to disk. 
//
//	"sector" is the disk sector to contain the file header
//----------------------------------------------------------------------

void
FileHeader::WriteBack(int sector)
{
    // --- lab 5 ---
    // update modify time 
    SetLastAccessTime();
    SetLastModifyTime();
    // -- end lab 5 ---
    synchDisk->WriteSector(sector, (char *)this); 
}

//----------------------------------------------------------------------
// FileHeader::ByteToSector
// 	Return which disk sector is storing a particular byte within the file.
//      This is essentially a translation from a virtual address (the
//	offset in the file) to a physical address (the sector where the
//	data at the offset is stored).
//
//	"offset" is the location within the file of the byte in question
//----------------------------------------------------------------------

int
FileHeader::ByteToSector(int offset)
{
    //return(dataSectors[offset / SectorSize]);
    int sectorIndex = offset / SectorSize;
    if (sectorIndex < NumDirectI)
        return(dataSectors[sectorIndex]);
    else
    {
        int i = (sectorIndex - NumDirectI) / NumDirectII;
        int offoffset = (sectorIndex - NumDirectI) % NumDirectII;
        // read secondary index table to memory
        int secondaryIndexTable[NumDirectII];
        SetLastAccessTime();
        synchDisk->ReadSector((int) dataSectors[NumDirectI + i], (char *)secondaryIndexTable);
        int sec = secondaryIndexTable[offoffset];
        return sec;
    }
}

//----------------------------------------------------------------------
// FileHeader::FileLength
// 	Return the number of bytes in the file.
//----------------------------------------------------------------------

int
FileHeader::FileLength()
{
    return numBytes;
}

//----------------------------------------------------------------------
// FileHeader::Print
// 	Print the contents of the file header, and the contents of all
//	the data blocks pointed to by the file header.
//----------------------------------------------------------------------

void
FileHeader::Print()
{
    int i, j, k;
    char *data = new char[SectorSize];
    numSectors  = divRoundUp(numBytes, SectorSize);

    printf("FileHeader contents:\n");
    printf("File Creation Time: %s", ctime(&createTime));
    printf("File Last Access Time: %s", ctime(&lastAccessTime));
    printf("File Last Modify Time: %s", ctime(&lastModifyTime));
    printf("File size: %d\nFile blocks:\n", numBytes);
    for (i = k = 0; i < min(numSectors, NumDirectI); i++, k++)
	printf("%d ", dataSectors[i]);

    if (numSectors > NumDirectI){
        int secondaryTableNum = divRoundUp((numSectors - NumDirectI), NumDirectII);
        int secondaryIndexTable[NumDirectII];
        for (j=0; j<secondaryTableNum; ++j){
            // read secondary index table to memory
            SetLastAccessTime();
            synchDisk->ReadSector((int) dataSectors[NumDirectI + j], (char *)secondaryIndexTable);
            for (i = 0; i < NumDirectII && k < numSectors; i++, k++) {
                printf("%d ", secondaryIndexTable[i]);
            }
        }
    }
    printf("\nFile contents:\n");
    for (i = k = 0; i < min(numSectors, NumDirectI); i++) {
	    synchDisk->ReadSector(dataSectors[i], data);
        for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
    	    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
    		    printf("%c", data[j]);
            else
    		    printf("\\%x", (unsigned char)data[j]);
	    }
        printf("\n"); 
    }
    if (numSectors > NumDirectI){
        int secondaryTableNum = divRoundUp((numSectors - NumDirectI), NumDirectII);
        int secondaryIndexTable[NumDirectII];
        for (int t=0; t<secondaryTableNum; ++t){
            // read secondary index table to memory
            SetLastAccessTime();
            synchDisk->ReadSector((int) dataSectors[NumDirectI + t], (char *)secondaryIndexTable);
            
            for (i = 0; i < NumDirectII; i++) {
                synchDisk->ReadSector(secondaryIndexTable[i], data);
                for (j = 0; (j < SectorSize) && (k < numBytes); j++, k++) {
                    if ('\040' <= data[j] && data[j] <= '\176')   // isprint(data[j])
                        printf("%c", data[j]);
                    else
                        printf("\\%x", (unsigned char)data[j]);
                }
                printf("\n"); 
                if (k >= numBytes) break;
            }
        }
    }
    printf("\n");
    delete [] data;
}