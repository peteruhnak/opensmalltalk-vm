/****************************************************************************
*   PROJECT: Mac allocate memory from somewhere for the image
*   FILE:    sqMacMemory.c
*   CONTENT: 
*
*   AUTHOR:  John Maloney, John McIntosh, and others.
*   ADDRESS: 
*   EMAIL:   johnmci@smalltalkconsulting.com
*   RCSID:   $Id$
*
*   NOTES: 
*  Feb 22nd, 2002, JMM moved code into 10 other files, see sqMacMain.c for comments
*  Mar  8th, 2002, JMM Must unmap view first then free.
*  3.2.8b1 July 24th, 2002 JMM support for os-x plugin under IE 5.x
*  3.5.1b5 June 25th, 2003 JMM get memory upper limit from os-x user preferences
 3.8.11b1 Mar 4th, 2006 JMM refactor, cleanup and add headless support
*	3.8.14b1 Oct	,2006 JMM browser rewrite
3.8.15b1 Oct, 2008 lessons learned from iphone port

*****************************************************************************/

#include "sq.h" 
#include "sqMacMemory.h"
#include "sqMacMain.h"

#include <sys/mman.h>
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

extern usqInt  gMaxHeapSize;
static usqInt	gHeapSize;
 static  void *startOfmmapForANONMemory,*startOfmmapForImageFile;
 static 	 size_t fileRoundedUpToPageSize,freeSpaceRoundedUpToPageSize;

/* compute the desired memory allocation */

extern unsigned char *memory;

usqInt	sqGetAvailableMemory() {
	 return gMaxHeapSize;
 }
 
 usqInt sqAllocateMemoryMac(sqInt minHeapSize, sqInt *desiredHeapSize, FILE * f,usqInt headersize) {
	 void  *possibleLocation,*startOfAnonymousMemory;
	 off_t fileSize;
	 struct stat sb;
	 size_t pageSize= getpagesize();
	 size_t pageMask= ~(pageSize - 1);
	 
	#define valign(x)	((x) & pageMask)
	#pragma unused(minHeapSize,desiredHeapSize)
     
	 possibleLocation = 500*1024*1024;
    gHeapSize = gMaxHeapSize;

	 /* Lets see about mmap the image file into a chunk of memory at the 100MB boundary rounding up to the page size
	  Then we on the next page anonymously allocate the required free space for young space*/
	 
	 /* Thanks to David Pennell for suggesting this */
	 
	 fstat(fileno((FILE *)f), &sb);
	 fileSize = sb.st_size;
	 fileRoundedUpToPageSize = valign(fileSize+pageSize-1);
	 startOfAnonymousMemory = (void *) ((size_t) fileRoundedUpToPageSize + (size_t) possibleLocation);
	 
	 startOfmmapForImageFile = mmap(possibleLocation, fileRoundedUpToPageSize, PROT_READ|PROT_WRITE, MAP_FILE|MAP_PRIVATE,fileno((FILE *)f), 0);

	 if (startOfmmapForImageFile != possibleLocation) {
			fprintf(stderr, "errno %d\n", errno);
			perror("startOfmmapForImageFile failed");
			exit(42);
	 }
	 
	 freeSpaceRoundedUpToPageSize = valign(gMaxHeapSize)-fileRoundedUpToPageSize+pageSize;
	 startOfmmapForANONMemory = mmap(startOfAnonymousMemory, freeSpaceRoundedUpToPageSize, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED,0,0);
 
	 if (startOfmmapForANONMemory != startOfAnonymousMemory) {
		 fprintf(stderr, "errno %d\n", errno);
		 perror("startOfmmapForANONMemory failed");
		 exit(42);
	 }
	 
	 return (usqInt) startOfmmapForImageFile+headersize;
}

sqInt sqGrowMemoryBy(sqInt memoryLimit, sqInt delta) {
    if ((usqInt) memoryLimit + (usqInt) delta - (usqInt) memory > gMaxHeapSize)
        return memoryLimit;
   
    gHeapSize += delta;
    return memoryLimit + delta;
}

sqInt sqShrinkMemoryBy(sqInt memoryLimit, sqInt delta) {
    return sqGrowMemoryBy(memoryLimit,0-delta);
}

sqInt sqMemoryExtraBytesLeft(Boolean flag) {
#pragma unused(flag)
        return gMaxHeapSize - gHeapSize;
}

void sqMacMemoryFree() {
	if (memory == nil) 
		return;
	munmap(startOfmmapForImageFile,fileRoundedUpToPageSize);
	munmap(startOfmmapForANONMemory,freeSpaceRoundedUpToPageSize);
	memory = NULL;
}