/*
	my-malloc.c
*/
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/resource.h> /* get/setrlimit() */

#include "my-malloc.h"

static _mem_session *_session = NULL;

#define DEBUG

#ifdef DEBUG

#include <stdarg.h>
static int _debug_itr = 0;

static void debug(char* fmt, ...) {

	char buf[256];

	va_list ap;
	va_start(ap,fmt);

	vsnprintf(buf, 255, fmt, ap);

	va_end(ap);

	printf("[%3i] %s",_debug_itr++, buf);

}
#else
static void debug(char* fmt, ...) {
	return;
}
#endif

void *my_malloc(size_t bytes) {

	debug("my_malloc(%i)\n", (int)bytes);

	if (!_session && _create_session() < 0) {
		return NULL;
	}

	/* Round up to nearest multiple of 4. */
	size_t cnt = (((bytes-1)>>2)<<2) + 4;

	_chunk *c = _find_free_chunk( cnt );

	if ( !c ) {

		/*  we will extend the program break if we can't
		    find a viable chunk at this time. */

		void *ptr;
		if ( (ptr = sbrk(sizeof(_chunk) + cnt)) == (void*)(-1) ) {
			fprintf(stderr, "Couldn't allocate memory!!\n");
			return NULL;
		}

		c = (_chunk*)ptr;
		c->_chunk_sz = cnt;
		c->_free = NOT_FREE;

		c->prev = _session->_last_chunk;
		c->next = NULL;

		c->prev_free = NULL;
		c->next_free = NULL;

		if ( !_session->_first_chunk )
			_session->_first_chunk = c;

		/*
			The current last chunk should have its
			`next` attr point to c before we make the
			last chunk in the session point to c.
		*/
		if ( _session->_last_chunk )
			_session->_last_chunk->next = c;

		_session->_last_chunk = c;

		_session->_chunks_allocated++;

		debug("\tNew chunk at %p\n", c);
		debug("\t_session->_first_chunk = %p\n", _session->_first_chunk);
		debug("\t_session->_last_chunk = %p\n", _session->_last_chunk);

	} else {

		c->_free = NOT_FREE;

		/* patch free chunks */
		if (c->next_free && c->prev_free) {
			/* runs if c is in the middle of the freed chunks list */
			c->next_free->prev_free = c->prev_free;
			c->prev_free->next_free = c->next_free;

		} else if (c->next_free) {
			/* runs if c is the first of the freed chunks list */
			c->next_free->prev_free = NULL;

		} else if (c->prev_free) {
			/* runs if c is the last of the freed chunks list */
			c->prev_free->next_free = NULL;
		}

		if (_session->_first_free_chunk == c)
			_session->_first_free_chunk = c->next_free;

		if (_session->_last_free_chunk == c)
			_session->_last_free_chunk = c->prev_free;

		/* end patching of free chunks */

	/* -------------------------------------------- */

		/* Can we shrink the chunk?
		   If so, let's create a chunk to handle the rest
		   of the memory, so that we don't take more than
		   we truly need. */
		if (c->_chunk_sz >= cnt + (sizeof(int) + sizeof(_chunk) )) {

			/* called `d` because d comes after c! */
			_chunk *d = (_chunk*)((char*)c + sizeof(_chunk) + c->_chunk_sz);

			size_t extra = c->_chunk_sz - cnt - sizeof(_chunk);
			c->_chunk_sz = cnt;

			d->_chunk_sz = extra;
			d->_free = FREE;

			if ( !_session->_first_free_chunk )
				_session->_first_free_chunk = d;

			if ( _session->_last_free_chunk )
				_session->_last_free_chunk->next_free = d;

			d->prev_free = _session->_last_free_chunk;
			_session->_last_free_chunk = d;
			d->next_free = NULL; /* this _may_ be unnecessary...*/

			d->next = c->next;
			c->next = d;
			d->prev = c;

			_session->_chunks_allocated++;
		}
	}
	debug("\t_session->_first_free_chunk = %p\n", _session->_first_free_chunk);
	debug("\t_session->_last_free_chunk = %p\n", _session->_last_free_chunk);

	/* return `c` offset by the size of a chunk,
	   resulting in a pointer pointing to the
	   beginning of user data space*/
	return (sizeof(_chunk) + (char*)c);
	/* TODO: getrlimit stuff */
}

void *my_calloc(size_t nmemb, size_t size) {

	debug("my_calloc(%i, %i)\n", (int)nmemb, (int)size);

	/* cast to int* to deal with 4-byte intervals */
	int *mem = (int*)my_malloc(nmemb * size);

	int i = 0, n = nmemb*(size/4);
	for (; i < n; i++) mem[i] = 0;

	return (void*)mem;
}

void *my_realloc(void *ptr, size_t size) {

	debug("my_realloc(%p, %i)\n", ptr, (int)size);

	if ( !ptr ) {
		return my_malloc(size);
	} else if (size == 0) {
		my_free(ptr);
		return NULL;
	}

	_chunk *c = (_chunk*)((char*)ptr - sizeof(_chunk) );

	if ( c < _session->_first_chunk || c > _session->_last_chunk + _session->_last_chunk->_chunk_sz ) {
		fprintf(stderr, "%p is not a reallocatable memory space.\n", ptr);
		return NULL;
	}

	/* We have to make a bigger chunk if _more_ memory is requested. */
	if (size > c->_chunk_sz) {

		/* if the next chunk is free... */
		if (c->next->_free == FREE && c->next->_chunk_sz + sizeof(_chunk) + c->_chunk_sz >= size) {

			_chunk *n, *p, *c2;

			/* take c->next (c2) out of the chunks lists */
			c2 = c->next;
			n = c2->next;
			p = c2->prev;

			p->next = n;
			if (n) n->prev = p;

			if (_session->_last_chunk == c2)
				_session->_last_chunk = c2->prev;

			/*
				c2 will never be _session->_first_chunk because `c` comes before it
			*/

			/* take it out of free chunks list */
			n = c2->next_free;
			p = c2->prev_free;

			p->next_free = n;
			if (n) n->prev_free = p;

			/*  the next two conditional assignments will be to
			    NULL if c2 is the only free chunk. */
			if (_session->_last_free_chunk == c2)
				_session->_last_free_chunk = c2->prev_free;

			if (_session->_first_free_chunk == c2)
				_session->_first_free_chunk = c2->next_free;

			c->_chunk_sz += c2->_chunk_sz + sizeof(_chunk);

			_session->_chunks_allocated--;
		} else {

			int *new_mem = my_malloc( size );

			if (!new_mem) return NULL; /* we have to see if my_malloc was successful. */

			/* _chunk_sz will *always* be a multiple of 4. */
			int i = 0, n = c->_chunk_sz / sizeof(int);

			for (; i < n; i++) new_mem[i] = ((int*)ptr)[i];

			my_free(ptr);
			ptr = (void*)new_mem;
		}
	}

	return ptr;
}

void my_free(void *ptr) {

	debug("my_free(%p)\n", ptr);

	_chunk *c = (_chunk*)((char*)ptr - sizeof(_chunk) );

	/* verify that the pointer is in our heap range */
	if ( c < _session->_first_chunk || c > _session->_last_chunk + _session->_last_chunk->_chunk_sz) {
		fprintf(stderr, "[!] Invalid Free! %p is out of range!\n", ptr);
		return;
	}

	debug("\tchunk @ %p; size: %i B\n", c, (int)(c->_chunk_sz) + sizeof(_chunk));

	c->_free = FREE;

	/*  Let's plop the chunk into our free
	    chunk linked list...*/
	if ( !_session->_first_free_chunk ) {

		/* if there are no chunks... */
		_session->_first_free_chunk = c;
		_session->_last_free_chunk = c;

		c->prev_free = NULL;
		c->next_free = NULL;

	} else {

		if (_session->_last_free_chunk) /* just in case... */
			_session->_last_free_chunk->next_free = c;

		c->prev_free = _session->_last_free_chunk;

		_session->_last_free_chunk = c;

	} /* end linked list stuff */

	/* Shouldn't have two free chunks back-to-back */
	if (c->next != NULL && c->next->_free == FREE)
		_merge_chunks(c, c->next);

	/* there could be free chunks on both sides of c... */
	if (c->prev != NULL && c->prev->_free == FREE)
		_merge_chunks(c->prev, c);
}

int _create_session() {

	debug("_create_session()\n");

	void *ptr; /* doing this just in case something fails... */

	if ( (ptr = sbrk(sizeof(_mem_session) )) == (void*)(-1) ) {
		fprintf(stderr, "Failed to start memory session!\n");
		return -1;
	}

	_session = (_mem_session*)ptr;

	/* go ahead and initialize attributes of _session */
	_session->_first_chunk = NULL;
	_session->_last_chunk = NULL;

	_session->_first_free_chunk = NULL;
	_session->_last_free_chunk = NULL;

	_session->_chunks_allocated = 0;

	return 0;
}

_chunk *_find_free_chunk(size_t bytes) {

	_chunk *curr = _session->_first_free_chunk;

	while(curr != NULL && (curr->_chunk_sz < bytes || curr->_free == NOT_FREE)) {
		curr = curr->next_free;
	}

	debug("\t_find_free_chunk(%i) --> %p\n", (int)bytes, curr);
	return curr;
}

void _merge_chunks(_chunk *a, _chunk *b) {

	debug("_merge_chunks(%p, %p)\n", a, b);

	a->next_free = b->next_free;
	if (b->next_free) b->next_free->prev_free = a;

	a->next = b->next;
	if (b->next) b->next->prev = a;

	if (_session->_last_free_chunk == b)
		_session->_last_free_chunk = a;

	if (_session->_last_chunk == b)
		_session->_last_chunk = a;

	a->_chunk_sz += b->_chunk_sz + sizeof(_chunk);

	_session->_chunks_allocated--;
}
