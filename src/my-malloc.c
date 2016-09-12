/*
	my-malloc.c
*/

#include <stddef.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h> /* just for exit() */

#include "my-malloc.h"

_mem_session *_session = NULL;

void *my_malloc(size_t bytes) {

	/*
		Round up to nearest power of 2.
	*/
	size_t cnt = (((bytes-1)>>2)<<2) + 4;

	if (_session == NULL) {
		_create_session();
	}

	_chunk *c = _find_free_chunk( cnt );

	if (c == NULL) {

		void *ptr = sbrk(0);

		if ( brk(ptr + sizeof(_chunk) + cnt) < 0) {
			perror("couldn't allocate memory");
			exit(2);
		}

		c = (_chunk*)ptr;
		c->_chunk_sz = cnt;
		c->_free = NOT_FREE;

		c->prev = _session->_last_chunk;
		c->next = NULL;

		/*
			The current last chunk should have its
			`next` attr point to c before we make the
			last chunk in the session point to c.
		*/
		if (_session->_last_chunk)
			_session->_last_chunk->next = c;

		_session->_last_chunk = c;

		_session->_chunks_allocated++;

		printf("New chunk at %p\n", c);
		printf("Result of ptr + sizeof(_chunk): %p\n", ptr + sizeof(_chunk));
		printf("Size of a chunk: %i\n", (int)sizeof(_chunk));
		printf("Amount of space requested: %i\n", (int)bytes);
		printf("Amount given: %i\n", (int)cnt);

	} else {

		c->_free = NOT_FREE;

		/* 
			Can we shrink the chunk ?
		*/
		if (c->_chunk_sz >= cnt + (4 + sizeof(_chunk) )) {

			size_t extra = c->_chunk_sz - cnt - sizeof(_chunk);
			c->_chunk_sz = cnt;

			_chunk *new_chunk = (_chunk*)((char*)c + sizeof(_chunk) + c->_chunk_sz);

			new_chunk->_free = FREE;
			new_chunk->_chunk_sz = extra;

			_chunk *tmp  = c->next;

			c->next = new_chunk;
			new_chunk->prev = c;
			new_chunk->next = tmp;

			_session->_chunks_allocated++;

			/* remove new_chunk from memory if `c` was the last chunk until now */
			if (_session->_last_chunk == c) {
				if ( brk(new_chunk) < 0) {
					perror("Couldn't deallocate last chunk...");
				}
				new_chunk = NULL; // just in case...

				_session->_chunks_allocated--;

			}

		}

	}

	return (sizeof(_chunk) + (char*)c);
	/* will break if all resources are used*/

}

void my_free(void *ptr) {

	printf("[*] Now freeing...\n");

	if ( ptr < (void*)((char*)_session->_first_chunk + sizeof(_chunk))
		|| ptr > (void*)((char*)_session->_last_chunk + sizeof(_chunk)) ) {
		perror("Invalid Free. Out of range!");
		exit(3);
	}

	_chunk *c = (_chunk*)((char*)ptr - sizeof(_chunk) );

	c->_free = FREE;

	/* if it's last chunk we should just shrink the p_break */
	if (_session->_last_chunk == c) {

		if ( brk(c) < 0 ) {
			perror("Couldn't deallocate last chunk...");
		}

		c = NULL;
		_session->_last_chunk = NULL;
		_session->_chunks_allocated--;

		if (c == _session->_first_chunk)
			_session->_first_chunk = NULL;

		return;
	}

	/*
		Shouldn't have two free chunks back-to-back
	*/
	if (c->next != NULL && c->next->_free == FREE) {
		
		_merge_chunks(c, c->next);

	} else if (c->prev != NULL && c->prev->_free == FREE) {

		_merge_chunks(c->prev, c);

	}

}

void _create_session() {

	void *ptr = sbrk(0);

	if ( brk(ptr + sizeof(_mem_session) ) < 0 ) {
		perror("Couldn't start memory session!");
		exit(2);
	}

	_session = (_mem_session*)ptr;

	/* go ahead and initialize attributes of _session */
	_session->_first_chunk = NULL;
	_session->_last_chunk = NULL;

	_session->_chunks_allocated = 0;

}

_chunk *_find_free_chunk(size_t bytes) {
	_chunk *curr = _session->_first_chunk;

	while(curr != NULL && (curr->_chunk_sz < bytes || curr->_free == NOT_FREE))
		curr = curr->next;

	return curr;
}

void _merge_chunks(_chunk *a, _chunk *b) {

	a->_chunk_sz += b->_chunk_sz + sizeof(_chunk);

	a->next = b->next;
	if (b->next)
		b->next->prev = a;

	_session->_chunks_allocated--;
}