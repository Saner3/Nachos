/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

int A[64];	/* size of physical memory; with code, we'll run out of space!*/

int
main()
{
    int i, j, tmp;

    /* first initialize the array, in reverse sorted order */
    for (i = 0; i < 64; i++)		
        A[i] = 64 - i;

    /* then sort! */
    for (i = 0; i < 63; i++)
        for (j = i; j < (63 - i); j++)
	   if (A[j] > A[j + 1]) {	/* out of order -> need to swap ! */
	      tmp = A[j];
	      A[j] = A[j + 1];
	      A[j + 1] = tmp;
    	   }
    Halt();
}