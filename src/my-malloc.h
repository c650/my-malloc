/*
	my-malloc.h
*/
#include <stddef.h>

#ifndef MY_MALLOC_HDR
#define MY_MALLOC_HDR

typedef enum _free_t {
	NOT_FREE, FREE
} _free_t;

/*
	Defining a basic chunk type here.

	It should pad to be the next multiple of 4 >=
	user-requested amount of memory.

	next and prev should point to adjacent chunks of memory.
	next_free and prev_free are not guaranteed to point to
	adjacent chunks, but they point to _free_ chunks on each
	side of the current chunk.

*/
typedef struct _chunk {
	size_t  _chunk_sz; /* not including metadata */

	/* to see if the chunk is free: */
	_free_t _free;

	struct _chunk *next,
	              *prev,
	              *next_free,
	              *prev_free;

} _chunk;

typedef struct _mem_session {
	_chunk *_first_chunk,
	       *_last_chunk,
	       *_first_free_chunk,
	       *_last_free_chunk;

	size_t _chunks_allocated;

} _mem_session;

void *my_malloc(size_t bytes);

void *my_calloc(size_t nmemb, size_t size);

void *my_realloc(void *ptr, size_t size);

void my_free(void *ptr);

/*
	_create_session is called when heap memory
	is first requested. Called from the first
	instance of my_malloc. It returns control
	back to my_malloc upon completing its job
	or reaching an error.

	@return 0 if successful, negative if failed.
*/
int   _create_session();

/*
	_find_free_chunk
	@param bytes the number of bytes for which
	the function will look

	@return the pointer to the first free chunk
	of appropriate size found.
*/
_chunk *_find_free_chunk( size_t bytes );

/*
	_merge_chunks merges 2 free chunks that
	are next to each other

	@param a the chunk into which b is merged
	@param b the chunk to merge into a

	The chunks should be side-by-side in memory.
*/
void _merge_chunks(_chunk *a, _chunk *b);

#endif
