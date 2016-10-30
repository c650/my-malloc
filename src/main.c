#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "my-malloc.h"

#define BUF__SIZ 8192

int main(void) {

	srand(time(NULL));

	int *num, *arr;
	long *lnum;

	printf("ALLOCATE NUM\n");
	num = my_malloc(sizeof(int));
	*num = 10101;

	printf("FREE NUM\n");
	my_free(num);

	printf("ALLOCATE LNUM\n");
	lnum = my_malloc(sizeof(long));

	*lnum = 101010;

	printf("FREE LNUM\n");
	my_free(lnum);

	printf("ALLOCATE ARR\n");
	arr = my_malloc(sizeof(int) * 5);
	for (int i = 0; i < 5; i++) {
		arr[i] = i<<1 ^ 0x45;
	}

	printf("FREE ARR\n");
	my_free(arr);

	printf("ALLOCATE LARR\n");
	long *larr = my_calloc(16, sizeof(long));
	for (int i = 0; i < 16; i++) {
		larr[i] = 10101;
	}

	printf("FREE LARR\n");
	my_free(larr);

	printf("C-ALLOCATE LARR\n");
	larr = my_calloc(17, sizeof(long));
	for (int i = 0; i < 17; i++) {
		larr[i] = 595959;
	}

	printf("ALLOCATE BUF\n");
	int *buf = (int*)my_malloc(sizeof(int));

	printf("REALLOCATE LARR\n");
	long *larr_tmp = NULL;
	if ((larr_tmp = my_realloc(larr, 30 * sizeof(long) )) == NULL) {
		perror("couldn't reallocate larr");
		exit(6);
	}
	larr = larr_tmp;

	printf("FREE LARR\n");
	my_free(larr);

	printf("FREE BUF\n");
	my_free(buf);

	printf("ALLOCATE LARR\n");
	larr = (long*)my_malloc(sizeof(long) * 50);

	printf("ALLOCATE LARR_TMP\n");
	larr_tmp = (long*)my_malloc(sizeof(long) * 100);

	printf("FREE LARR_TMP\n");
	my_free(larr_tmp);

	printf("REALLOCATE LARR\n");
	my_realloc(larr, sizeof(long)*150);

	printf("FREE LARR\n");
	my_free(larr);

	printf("ALLOCATE BIG STRING\n");
	char *cptr = (char*)my_calloc(BUF__SIZ+1, sizeof(char));

	for (int i = 0; i < BUF__SIZ; i++) {
		cptr[i] = (char)(rand() % 95 + 32);
	}
	cptr[BUF__SIZ] = 0x00;

	printf("cptr = %s\n", cptr);

	my_free(cptr);

}
