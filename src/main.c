#include <stdio.h>
#include <stdlib.h>

#include "my-malloc.h"

int main(void) {

	int *num, *arr;
	long *lnum;

	num = my_malloc(sizeof(int));
	*num = 10101;
	printf("num = %p\n*num = %i\n", num, *num);
	my_free(num);

	lnum = my_malloc(sizeof(long));
	*lnum = 101010;
	printf("lnum = %p\n*lnum = %li\n", lnum, *lnum);
	my_free(lnum);

	arr = my_malloc(sizeof(int) * 5);
	for (int i = 0; i < 5; i++) {
		arr[i] = i<<1 ^ 0x45;
	}
	my_free(arr);

	printf("Testing my_calloc\n");
	long *larr = my_calloc(16, sizeof(long));
	for (int i = 0; i < 16; i++) {
		printf("%li ", larr[i]);
		larr[i] = 10101;
		printf("%li ", larr[i]);
	}
	printf("\n");

	my_free(larr);

	larr = my_calloc(17, sizeof(long));
	for (int i = 0; i < 17; i++) {
		printf("%li ", larr[i]);
		larr[i] = 595959;	
	}
	printf("\n");

	int *buf = (int*)my_malloc(sizeof(int));

	printf("Reallocate larr to 300\n");
	long *larr_tmp = NULL;
	if ((larr_tmp = my_realloc(larr, 300)) == NULL) {
		perror("couldn't reallocate larr");
		exit(6);
	}
	printf("larr = %p\nlarr_tmp = %p\n", larr, larr_tmp);
	larr = larr_tmp;

	/*
		larr[17-299] wont be initialized formally so this will cause
		valgrind to freak out.
	
	for (int i = 0; i < 300; i++) {
		printf("%li ", larr[i]);
	}
	printf("\n");
	*/
	my_free(larr);
	my_free(buf);

}