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
typedef struct {
	size_t  _chunk_sz; /* not including metadata */

	struct _chunk *next, /* sizeof(_chunk*) = 8 */
	              *prev,
	              *next_free,
	              *prev_free;

	               /* to see if the chunk is free: */
	_free_t _free; /* putting this at the end to avoid
	               in-memory padding of attributes. */

} _chunk;

/*
	struct _mem_session contains two doubly-linked lists:
	the first manages _all_ chunks; the second contains
	_only_ chunks that are FREE.

	The struct also contains an attribute _chunks_allocated
	that keeps track of the number of chunks currently in RAM.
*/
typedef struct {
	_chunk *_first_chunk, /* sizeof(_chunk*) = 8 */
	       *_last_chunk,
	       *_first_free_chunk,
	       *_last_free_chunk;

	size_t _chunks_allocated;
	size_t max_size;

	/* all attributes of this struct are 8-bytes on my machine
	 	and this is good because it means that there won't be
		any padding, hogging memory. #EmbeddedSystemsBeLike */
} _mem_session;

typedef struct {

	_mem_session *sessions;
	size_t num_sessions;

} _outer_mem_session;

/*
	This does just what the normal malloc does,
	but probably in a less efficient manner.

	In the first call to my_malloc, a _mem_session
	will be initialized (by _create_session()).

	The function will then round the amount of
	requested memory up to the nearest integer-worth
	of size (4 bytes, usually).

	It will then call _find_free_chunk() to search for
	any free chunks of appropriate size, meaning any chunk
	at least as big as `(((bytes-1)>>2)<<2) + 4`, not including
	the sizeof(_chunk), which is a constant for all chunks.

	Upon finding a suitable chunk (it will return the first
	suitable chunk it sees), the function will proceed to see
	if it can split that chunk up into two smaller chunks:
	the first, an appropriately sized chunk to be given to
	the user; the second, the remaining space. This only
	happens if there is enough "extra fat" to fit sizeof(_chunk)
	AND one int-worth of data (sizeof(_chunk) + sizeof(int)).
		Whether or not the chunk is split, the user receives
	a pointer equal to &chunk (the beginning of the relevant
	chunk) + sizeof(_chunk).

	If there are no suitable, free chunks available, my_malloc()
	will proceed to make a new one by expanding the program break.
	If it is unable to do so, an error is printed, but NULL is
	returned to the user. Otherwise, the chunk is initialized and
	the proper address (see last paragraph) is returned to the user.

	@param bytes the number of bytes requested by the user. A pointer
	to a contiguous block in memory of _at least_ this number of bytes
	is returned. If unsuccessful, NULL is returned.
*/
void *my_malloc(size_t bytes);

/*
	my_calloc() calls my_malloc() and then proceeds to
	zero out the data 4 (sizeof(int)) at a time.

	@param nmemb the number of elements to allocate
	memory for.
	@param size the size of each element.

	NOTE: nmemb * size <= the total amount of memory
	to be allocated.
*/
void *my_calloc(size_t nmemb, size_t size);

/*
	my_realloc() first checks to see if the current _chunk
	is big enough to just return.

	If the chunk is not at least `size` bytes, then one of
	two things happen:
		1) if the adjacent chunk is free and large enough,
		then that chunk is uninitialized and its memory is given
		to the chunk being `realloc`ed. A relevant address is returned.
		2) my_malloc() is called. If successful, the data from `ptr` is
		copied into the new memory returned by my_malloc(). `ptr` itself
		is freed by my_free().

	@param ptr the pointer to be expanded.
	@param size the new size requested. If it is smaller, my_realloc()
	(at this point) does nothing but return `ptr`. If larger, my_realloc()
	follows the procedures outlined above.
*/
void *my_realloc(void *ptr, size_t size);

/*
	my_free() frees marks the chunk at `ptr - sizeof(_chunk)`
	as FREE and inserts it at the end of a freed chunks
	doubly-linked list.

	If adjacent chunks are also free, my_free() calls merge_chunks()
	to combine the chunks into one larger chunk.

	@param ptr the pointer the user wishes to free. It must be
	a pointed that was `my_malloc()`ed.
*/
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
