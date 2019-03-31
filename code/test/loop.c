/* sort.c 
 *    Test program to sort a large number of integers.
 *
 *    Intention is to stress virtual memory system.
 *
 *    Ideally, we could read the unsorted array off of the file system,
 *	and store the result back to the file system!
 */

#include "syscall.h"

int A = 0;

int
main()
{
    int i;
    for (i=0; i<1000; ++i){
        A = A + i;
    }
    Exit(1);
}
