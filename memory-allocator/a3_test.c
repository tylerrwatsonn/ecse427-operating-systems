/*
 * =====================================================================================
 *
 *	Filename:  		a3_test.c
 *
 * 	Description:	Example of testing code of MyMalloc.
 *
 *  Version:  		1.0
 *  Created:  		6/11/2020 9:30:00 AM
 *  Revised:  		-
 *  Compiler:  		gcc
 *
 *  Authors:  		Devarun Bhattacharya, 
 * 					Mohammad Mushfiqur Rahman
 * 
 * 	Intructions:	Please address all the "TODO"s in the code below and modify 
 * 					them accordingly. No need to modify anything else, unless you 
 * 					find a bug in the tester! Don't modify the tester to circumvent 
 * 					the bug in your code!
 * =====================================================================================
 */

/* Includes */
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "sma.h"

int main(int argc, char *argv[])
{
	int i, count = 0;
	void *ptr, *limitafter = NULL, *limitbefore = NULL;
	char *c[32], *ct;
	char str[60];

	// Test 1: Find the holes
	puts("Test 1: Hole finding test...");

	// Allocating 32 kbytes of memory..
	for (i = 0; i < 32; i++)
	{
		c[i] = (char *)sma_malloc(1024);
		// sprintf(str, "c[i]: %p", c[i]);
		// puts(str);
	}

	// Now deallocating some of the slots ..to free
	for (i = 10; i < 18; i++)
	{
		sma_free(c[i]);
		// sprintf(str, "Freeing c[i]: %p", c[i]);
		// puts(str);
	}

	// Allocate some storage .. this should go into the freed storage
	ct = (char *)sma_malloc(5 * 1024);
	// sprintf(str, "CT : %p", ct);
	// puts(str);

	// Testing if you are finding the available holes
	if (ct < c[31])
		puts("\t\t\t\t PASSED\n");
	else
		puts("\t\t\t\t FAILED\n");

	// Test 2: Program Break expansion Test
	puts("Test 2: Program break expansion test...");

	count = 0;
	for (i = 1; i < 40; i++)
	{
		limitbefore = sbrk(0);
		ptr = sma_malloc(1024 * 32 * i);
		limitafter = sbrk(0);

		if (limitafter > limitbefore)
			count++;
	}

	// Testing if the program breaks are incremented correctly
	if (count > 0 && count < 40)
		puts("\t\t\t\t PASSED\n");
	else
		puts("\t\t\t\t FAILED\n");

	// Test 3: Worst Fit Test
	puts("Test 3: Check for Worst Fit algorithm...");
	// Sets Policy to Worst Fit
	sma_mallopt(WORST_FIT);

	// Allocating 512 kbytes of memory..
	for (i = 0; i < 32; i++)
		c[i] = (char *)sma_malloc(16 * 1024);

	// Now deallocating some of the slots ..to free
	// One chunk of 5x16 kbytes
	sma_free(c[31]);
	sma_free(c[30]);
	sma_free(c[29]);
	sma_free(c[28]);
	sma_free(c[27]);

	// One chunk of 3x16 kbytes
	sma_free(c[25]);
	sma_free(c[24]);
	sma_free(c[23]);

	// One chunk of 2x16 kbytes
	sma_free(c[20]);
	sma_free(c[19]);

	// One chunk of 3x16 kbytes
	sma_free(c[10]);
	sma_free(c[9]);
	sma_free(c[8]);

	// One chunk of 2x16 kbytes
	sma_free(c[5]);
	sma_free(c[4]);

	char *cp2 = (char *)sma_malloc(16 * 1024 * 2);

	// Testing if the correct hole has been allocated
	if (cp2 != NULL)
	{
		if (cp2 == c[27] || cp2 == c[28] || cp2 == c[29] || cp2 == c[30])
			puts("\t\t\t\t PASSED\n");
		else
			puts("\t\t\t\t FAILED\n");
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	//	Freeing cp2
	sma_free(cp2);

	// Test 4: Next Fit Test
	puts("Test 4: Check for Next Fit algorithm...");
	// Sets Policy to Next Fit
	sma_mallopt(NEXT_FIT);

	char *cp3 = (char *)sma_malloc(16 * 1024 * 3);
	char *cp4 = (char *)sma_malloc(16 * 1024 * 2);

	// Testing if the correct holes have been allocated
	if (cp3 == c[8] && cp3 != NULL)
	{
		if (cp4 == c[19])
		{
			// sprintf(str, "C[19]: %p", c[19]);
			// puts(str);
			// sprintf(str, "CP4: %p", cp4);
			// puts(str);

			puts("\t\t\t\t PASSED\n");
		}
		else
		{
			puts("\t\t\t\t FAILED\n");
		}
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	// Test 5: Realloc test (with Next Fit)
	puts("Test 5: Check for Reallocation with Next Fit...");
	cp3 = (char *)sma_realloc(cp3, 16 * 1024 * 5);
	cp4 = (char *)sma_realloc(cp4, 16 * 1024 * 3);

	if (cp3 == c[27] && cp3 != NULL && cp4 == c[8] && cp4 != NULL)
	{
		puts("\t\t\t\t PASSED\n");
	}
	else
	{
		puts("\t\t\t\t FAILED\n");
	}

	//	Test 6: Print Stats
	puts("Test 6: Print SMA Statistics...");
	puts("===============================");
	sma_mallinfo();

	return (0);
}