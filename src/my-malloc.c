/*
	my-malloc.c
*/
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "my-malloc.h"

static _mem_session *_session = NULL;
//static int _debug_itr = 0;

#include <stdarg.h>
//#define DEBUG
static void debug(char* fmt, ...) {
	#ifdef DEBUG

	char buf[256];

	va_list ap;
	va_start(ap,fmt);

	vsnprintf(buf, 255, fmt, ap);

	va_end(ap);

	printf("[%3i] %s",_debug_itr++, buf);

	#endif
}

void *my_malloc(size_t bytes) {

	debug("my_malloc(%i) called!\n", (int)bytes);

	if (_session == NULL && _create_session() < 0) {
		return NULL;
	}

	/* Round up to nearest multiple of 4. */
	size_t cnt = (((bytes-1)>>2)<<2) + 4;

	_chunk *c = _find_free_chunk( cnt );

	debug("\t_session->_first_free_chunk = %p\n", _session->_first_free_chunk);
	debug("\t_session->_last_free_chunk = %p\n", _session->_last_free_chunk);
	if (c == NULL) {

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

		if (_session->_first_chunk == NULL)
			_session->_first_chunk = c;

		/*
			The current last chunk should have its
			`next` attr point to c before we make the
			last chunk in the session point to c.
		*/
		if (_session->_last_chunk)
			_session->_last_chunk->next = c;

		_session->_last_chunk = c;

		_session->_chunks_allocated++;

		debug("[*] New chunk at %p\n", c);
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

		if (_session->_first_free_chunk == c) {
			_session->_first_free_chunk = c->next_free;
		}

		if (_session->_last_free_chunk == c) {
			_session->_last_free_chunk = c->prev_free;
		}
		/* end patching of free chunks */

		/* Can we shrink the chunk ? */
		if (c->_chunk_sz >= cnt + (4 + sizeof(_chunk) )) {

			size_t extra = c->_chunk_sz - cnt - sizeof(_chunk);
			c->_chunk_sz = cnt;

			_chunk *new_chunk = (_chunk*)((char*)c + sizeof(_chunk) + c->_chunk_sz);

			new_chunk->_free = FREE;

			if (_session->_first_free_chunk == NULL)
				_session->_first_free_chunk = new_chunk;

			if (_session->_last_free_chunk)
				_session->_last_free_chunk->next_free = new_chunk;

			new_chunk->prev_free = _session->_last_free_chunk;
			_session->_last_free_chunk = new_chunk;
			new_chunk->next_free = NULL;


			new_chunk->_chunk_sz = extra;

			new_chunk->next = c->next;
			c->next = new_chunk;
			new_chunk->prev = c;
			

			_session->_chunks_allocated++;
		}

	}
	debug("\t_session->_first_free_chunk = %p\n", _session->_first_free_chunk);
	debug("\t_session->_last_free_chunk = %p\n", _session->_last_free_chunk);

	return (sizeof(_chunk) + (char*)c);
	/* will break if all resources are used*/

}

void *my_calloc(size_t nmemb, size_t size) {
	
	/* cast to char* to deal with bytes */
	char *mem = (char*)my_malloc(nmemb * size);

	int i = 0, n = nmemb*size;
	for (; i < n; i++) mem[i] = 0x00;

	return (void*)mem;
}

void *my_realloc(void *ptr, size_t size) {
	if (ptr == NULL) {
		return my_malloc(size);
	} else if (size == 0) {
		my_free(ptr);
		return NULL;
	}

	if ( ptr < (void*)((char*)_session->_first_chunk + sizeof(_chunk))
		|| ptr > (void*)((char*)_session->_last_chunk + sizeof(_chunk)) ) {
		
		fprintf(stderr, "%p is not a reallocatable memory space.\n", ptr);
		return NULL;
	}

	_chunk *c = (_chunk*)((char*)ptr - sizeof(_chunk) );

	/* We have to make a bigger chunk if _more_ memory is requested. */
	if (size > c->_chunk_sz) {
		char *new_mem = (char*)my_malloc( size );
		for (int i = 0; i < c->_chunk_sz; i++) {
			new_mem[i] = ((char*)ptr)[i];
		}
		my_free(ptr);
		ptr = (void*)new_mem;
	}

	return ptr;
}

void my_free(void *ptr) {

	debug("my_free(%p)\n", ptr);

	_chunk *c = (_chunk*)((char*)ptr - sizeof(_chunk) );

	/* verify that the pointer is in our heap range */
	if ( c < _session->_first_chunk || c > _session->_last_chunk ) {
		fprintf(stderr, "Invalid Free! %p is out of range!\n", ptr);
		return;
	}

	debug("[*] Freeing chunk at %p\n", c);

	c->_free = FREE;

	/*
		Let's plop the chunk into our free
		chunk linked list...
	*/
	debug("\t_session->_first_free_chunk = %p\n", _session->_first_free_chunk);
	debug("\t_session->_last_free_chunk = %p\n", _session->_last_free_chunk);
	if (_session->_first_free_chunk == NULL) {

		/* if there are no chunks... */
		_session->_first_free_chunk = c;
		_session->_last_free_chunk = c;

		c->prev_free = NULL;
		c->next_free = NULL;

	} else {

		if (_session->_last_free_chunk) /* just in case... */
			_session->_last_free_chunk->next_free = c;

		c->prev_free = _session->_last_free_chunk;

		c->next_free = NULL;
		_session->_last_free_chunk = c;

	} /* end linked list stuff */
	debug("\t_session->_first_free_chunk = %p\n", _session->_first_free_chunk);
	debug("\t_session->_last_free_chunk = %p\n", _session->_last_free_chunk);

	/* Shouldn't have two free chunks back-to-back */
	if (c->next != NULL && c->next->_free == FREE) {
		
		_merge_chunks(c, c->next);

	} else if (c->prev != NULL && c->prev->_free == FREE) {

		_merge_chunks(c->prev, c);

	}
}

int _create_session() {

	void *ptr;

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

	debug("_find_free_chunk(%i) was called!\n", (int)bytes);

	_chunk *curr = _session->_first_free_chunk;

	while(curr != NULL && (curr->_chunk_sz < bytes || curr->_free == NOT_FREE)) {
		
		debug("\tcurr = %p\n", curr);
		
		curr = curr->next_free;
	}

	debug("\t_find_free_chunk(%i) --> %p\n", (int)bytes, curr);
	return curr;
}

void _merge_chunks(_chunk *a, _chunk *b) {

	debug("_merge_chunks was called!\n");
	debug("\tMerging %p and %p\n", a, b);

	debug("\ta->next_free (%p) = b->next_free (%p)\n", a->next_free, b->next_free);
	
	a->next_free = b->next_free;
	if (b->next_free) {
		
		debug("\tb->next_free->prev_free (%p) = a (%p)\n", b->next_free->prev_free, a);
		
		b->next_free->prev_free = a;
	}

	if (_session->_last_free_chunk == b) {
		
		debug("\t%p = _session->_last_free_chunk\n", _session->_last_free_chunk);
		
		_session->_last_free_chunk = a;

		debug("\t%p = _session->_last_free_chunk\n", _session->_last_free_chunk);
	}

	if (_session->_last_chunk == b) {

		debug("\t%p = _session->_last_chunk\n", _session->_last_chunk);
		
		_session->_last_chunk = a;

		debug("\t%p = _session->_last_chunk\n", _session->_last_chunk);

	}

	a->_chunk_sz += b->_chunk_sz + sizeof(_chunk);

	a->next = b->next;
	if (b->next)
		b->next->prev = a;

	_session->_chunks_allocated--;
}