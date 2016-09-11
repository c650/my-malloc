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

}