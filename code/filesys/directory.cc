// directory.cc 
//	Routines to manage a directory of file names.
//
//	The directory is a table of fixed length entries; each
//	entry represents a single file, and contains the file name,
//	and the location of the file header on disk.  The fixed size
//	of each directory entry means that we have the restriction
//	of a fixed maximum size for file names.
//
//	The constructor initializes an empty directory of a certain size;
//	we use ReadFrom/WriteBack to fetch the contents of the directory
//	from disk, and to write back any modifications back to disk.
//
//	Also, this implementation has the restriction that the size
//	of the directory cannot expand.  In other words, once all the
//	entries in the directory are used, no more files can be created.
//	Fixing this is one of the parts to the assignment.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "utility.h"
#include "filehdr.h"
#include "directory.h"

//----------------------------------------------------------------------
// Directory::Directory
// 	Initialize a directory; initially, the directory is completely
//	empty.  If the disk is being formatted, an empty directory
//	is all we need, but otherwise, we need to call FetchFrom in order
//	to initialize it from disk.
//
//	"size" is the number of entries in the directory
//----------------------------------------------------------------------

Directory::Directory(int size)
{
    table = new DirectoryEntry[size];
    tableSize = size;
    for (int i = 0; i < tableSize; i++)
	table[i].inUse = FALSE;
}

//----------------------------------------------------------------------
// Directory::~Directory
// 	De-allocate directory data structure.
//----------------------------------------------------------------------

Directory::~Directory()
{ 
    delete [] table;
} 

//----------------------------------------------------------------------
// Directory::FetchFrom
// 	Read the contents of the directory from disk.
//
//	"file" -- file containing the directory contents
//----------------------------------------------------------------------

void
Directory::FetchFrom(OpenFile *file)
{
    (void) file->ReadAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::WriteBack
// 	Write any modifications to the directory back to disk
//
//	"file" -- file to contain the new directory contents
//----------------------------------------------------------------------

void
Directory::WriteBack(OpenFile *file)
{
    (void) file->WriteAt((char *)table, tableSize * sizeof(DirectoryEntry), 0);
}

//----------------------------------------------------------------------
// Directory::FindIndex
// 	Look up file name in directory, and return its location in the table of
//	directory entries.  Return -1 if the name isn't in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::FindIndex(char *name)
{
    for (int i = 0; i < tableSize; i++)
        if (table[i].inUse)
        if (!table[i].isLong)
        if (!strncmp(getName(i), name, FileLongNameMaxLen))
	       return i;
        
    return -1;		// name not in directory
}

//----------------------------------------------------------------------
// Directory::Find
// 	Look up file name in directory, and return the disk sector number
//	where the file's header is stored. Return -1 if the name isn't 
//	in the directory.
//
//	"name" -- the file name to look up
//----------------------------------------------------------------------

int
Directory::Find(char *name)
{
    char *namebuffer = new char[50];
    strcpy(namebuffer, name);

    // suppose "name" can be a path: root/OS/homework/HW1
    char *firstpart;               // root
    char *secondpart;              // OS/homework/HW1
    DividePath(&namebuffer, &firstpart, &secondpart);
    ASSERT(firstpart != NULL);
    int i = FindIndex(firstpart);
    if (i == -1) return -1;

    int sonSector = table[i].sector;
    if (secondpart == NULL)
        return sonSector;
    else{
        // else, son is a directory, FIND in son recursively
        Directory *son = new Directory(NumDirEntries);
        OpenFile *sonFile = new OpenFile(sonSector);
        son->FetchFrom(sonFile);
        int finalSector = son->Find(secondpart);
        delete son;
        delete sonFile;
        return finalSector;
    }
    /*
    int i = FindIndex(name);

    if (i != -1)
	return table[i].sector;
    return -1;*/
}

//----------------------------------------------------------------------
// Directory::Add
// 	Add a file into the directory.  Return TRUE if successful;
//	return FALSE if the file name is already in the directory, or if
//	the directory is completely full, and has no more space for
//	additional file names.
//
//	"name" -- the name of the file being added
//	"newSector" -- the disk sector containing the added file's header
//----------------------------------------------------------------------

bool
Directory::Add(char *name, int newSector, bool dir)
{ 
    char *namebuffer = new char[50];
    strcpy(namebuffer, name);

    // suppose "name" can be a path: root/OS/homework/HW1
    char *firstpart;               // root
    char *secondpart;              // OS/homework/HW1
    DividePath(&namebuffer, &firstpart, &secondpart);
    ASSERT(firstpart != NULL);

    int sonIndex = FindIndex(firstpart);
    // if it is a directory but does not exist
    if (sonIndex == -1 && secondpart != NULL) return FALSE;
    // if it is a file but exist
    if (sonIndex != -1 && secondpart == NULL) return FALSE;

    if(secondpart != NULL){
    //  son is a directory, ADD in son recursively
        ASSERT(table[sonIndex].type == 1);
        int sonSector = table[sonIndex].sector;
        Directory *son = new Directory(NumDirEntries);
        OpenFile *sonFile = new OpenFile(sonSector);
        son->FetchFrom(sonFile);
        int success = son->Add(secondpart, newSector, dir);
        if (success) son->WriteBack(sonFile);
        delete son;
        delete sonFile;
        return success;
    }
    // else, add file in this directory
    // if (FindIndex(name) != -1)
	// return FALSE;
    int lenName = strlen(firstpart);
    int numEntries = 0; // how many entries does it need?
    if (lenName > FileNameMaxLen){
        lenName -= FileNameMaxLen;
        numEntries ++;
        while (lenName > FileLongNamePerEntry){
            lenName -= FileLongNamePerEntry;
            numEntries ++;
        }
    }
    for (int i = 0; i+numEntries < tableSize; ++i){
        bool found = TRUE;
        // is there consecutive numEntries free entries
        for (int j = i; j <= i+numEntries; ++j){
            if (table[j].inUse)
                found = FALSE;
        }
        if (found) {
            char *namecopy = firstpart;
            table[i].inUse = TRUE;
            table[i].isLong = FALSE;
            table[i].beginLong = FALSE;
            strncpy(table[i].shortname, namecopy, FileNameMaxLen); 
            namecopy += FileNameMaxLen;
            table[i].sector = newSector;
            if (dir) table[i].type = 1;
            else table[i].type = 0;
            
            // it is the beginning of a long name entry
            if (numEntries > 0){
                table[i].beginLong = TRUE;
                for (int j=1; j<=numEntries; ++j){
                    table[i+j].inUse = TRUE;
                    table[i+j].isLong = TRUE;
                    table[i+j].lastEntry = FALSE;
                    strncpy(table[i+j].longname, namecopy, FileLongNamePerEntry); 
                    namecopy += FileLongNamePerEntry;
                }
                table[i+numEntries].lastEntry = TRUE;
            }
            return TRUE;
        }
	}
    return FALSE;	// no space.  Fix when we have extensible files.
    
}

//----------------------------------------------------------------------
// Directory::Remove
// 	Remove a file name from the directory.  Return TRUE if successful;
//	return FALSE if the file isn't in the directory. 
//
//	"name" -- the file name to be removed
//----------------------------------------------------------------------

bool
Directory::Remove(char *name)
{ 
    char *namebuffer = new char[50];
    strcpy(namebuffer, name);

    // suppose "name" can be a path: root/OS/homework/HW1
    char *firstpart;               // root
    char *secondpart;              // OS/homework/HW1
    DividePath(&namebuffer, &firstpart, &secondpart);
    ASSERT(firstpart != NULL);
    int i = FindIndex(firstpart);
    // if son does not exist
    if (i == -1) return FALSE;

    if (secondpart != NULL){
        ASSERT(table[i].type == 1);
    //  son is a directory, REMOVE in son recursively
        int sonSector = table[i].sector;
        Directory *son = new Directory(NumDirEntries);
        OpenFile *sonFile = new OpenFile(sonSector);
        son->FetchFrom(sonFile);
        int success = son->Remove(secondpart);
        if (success) son->WriteBack(sonFile);
        delete son;
        delete sonFile;
        return success;
    }
    //int i = FindIndex(name);

    //if (i == -1)
	//   return FALSE; 		// name not in directory
    ASSERT(table[i].isLong == FALSE);
    table[i].inUse = FALSE;
    if (table[i].beginLong){
        while(!table[++i].lastEntry){
            ASSERT(table[i].isLong);
            table[i].inUse = FALSE;
        }
        table[i].inUse = FALSE;
    }
    return TRUE;	
}

//----------------------------------------------------------------------
// Directory::List
// 	List all the file names in the directory. 
//----------------------------------------------------------------------

void
Directory::List()
{
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse && !table[i].isLong){
        if (table[i].type){
	        printf("dir %s\n{", getName(i));
            int sonSector = table[i].sector;
            Directory *son = new Directory(NumDirEntries);
            OpenFile *sonFile = new OpenFile(sonSector);
            son->FetchFrom(sonFile);
            son->List();
            delete son;
            delete sonFile;
            printf("}\n");
        }
        else {
            printf("file %s\n", getName(i));
        }
    }   
}

//----------------------------------------------------------------------
// Directory::Print
// 	List all the file names in the directory, their FileHeader locations,
//	and the contents of each file.  For debugging.
//----------------------------------------------------------------------

void
Directory::Print()
{ 
    FileHeader *hdr = new FileHeader;

    printf("Directory contents:\n");
    for (int i = 0; i < tableSize; i++)
	if (table[i].inUse && !table[i].isLong) {
        if (table[i].type){
            // directory, print recursively
            printf("Dir %s, Sector %d:{\n", getName(i), table[i].sector);
            int sonSector = table[i].sector;
            Directory *son = new Directory(NumDirEntries);
            OpenFile *sonFile = new OpenFile(sonSector);
            son->FetchFrom(sonFile);
            son->Print();
            delete son;
            delete sonFile;
            printf("}\n");
        }
        else{
    	    printf("File Name: %s, Sector: %d\n", getName(i), table[i].sector);
    	    hdr->FetchFrom(table[i].sector);
    	    hdr->Print();
        }
	}
    printf("\n");
    delete hdr;
}

//-------lab 5-------
// Directory::getName
//  return the name of i-th directory entry file
//-------------------
char *Directory::getName(int i)
{
    char *name = NULL;
    ASSERT(!table[i].isLong);
    if (!table[i].beginLong){
        // short name
        name = table[i].shortname;
    }
    else{
        // long name
        name = new char[FileLongNameMaxLen];
        strcpy(name, table[i++].shortname);
        while(!table[i].lastEntry){
            ASSERT(i < tableSize);
            strcat(name, table[i].longname);
            i++;
        }
        ASSERT(i < tableSize);
        strcat(name, table[i].longname);
    }
    return name;
}

void 
Directory::DividePath(char **path, char **first, char **second){
    //  path: root/OS/homework/HW1
    //  first: root
    //  second: OS/homework/HW1
    char *divide = strchr((*path), '/');
    if (divide == NULL){
        (*first) = (*path);
        (*second) = NULL;
    }
    else {
        (*first) = (*path);
        (*second) = divide + 1;
        *divide = '\0';
    }
    return;
}