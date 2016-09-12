/*
	my-malloc.h
*/
#include <stddef.h>

#ifndef MY_MALLOC_HDR
#define MY_MALLOC_HDR

#define FREE 1
#define NOT_FREE 0

/*
	Defining a basic chunk type here.

	It should pad to be the next power of 2 >=
	user-requested amount of memory.

*/
typedef struct _chunk {
	size_t  _chunk_sz; /* not including metadata */

	/* to see if the chunk is free: */
	unsigned char _free;

	struct _chunk  *next,
	              *prev;

} _chunk;

typedef struct _mem_session {
	_chunk *_first_chunk,
	       *_last_chunk;
	size_t _chunks_allocated;
} _mem_session;

extern _mem_session *_session;

void *my_malloc(size_t bytes);
void my_free(void *ptr);

void   _create_session();
_chunk *_find_free_chunk( size_t bytes );

/*
	_merge_chunks
	@param a the chunk into which b is merged
	@param b the chunk to merge into a

	The chunks should be side-by-side in memory.
*/
void _merge_chunks(_chunk *a, _chunk *b);

#endif