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
		_session->_last_chunk = c;

		_session->_chunks_allocated++;

		printf("New chunk at %p\n", c);
		printf("Result of ptr + sizeof(_chunk): %p\n", ptr + sizeof(_chunk));
		printf("Size of a chunk: %i\n", (int)sizeof(_chunk));
		printf("Amount of space requested: %i\n", (int)bytes);
		printf("Amount given: %i\n", (int)cnt);

	} else {

		c->_free = NOT_FREE;

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

			/* shrink program break if c was the last chunk */
			if (_session->_last_chunk == c) {
				if ( brk(new_chunk) < 0) {
					perror("Couldn't deallocate last chunk...");
				}
				new_chunk = NULL;
				_session->_last_chunk = NULL;
				_session->_chunks_allocated--;

				if (c == _session->_first_chunk)
					_session->_first_chunk = NULL;

				_session->_chunks_allocated--;
			}

		}

	}

	return (sizeof(_chunk) + (char*)c);
	/* will break if all resources are used*/

}

void my_free(void *ptr) {

	printf("[*] Now freeing...\n");

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
		Shouldn't have two frees back-to-back
	*/
	if (c->next != NULL && c->next->_free == FREE) {
		
		_chunk *n = c->next;

		c->_chunk_sz += n->_chunk_sz + sizeof(_chunk);
		
		c->next = n->next;
		if (n->next != NULL)
			n->next->prev = c;

		_session->_chunks_allocated--;

	} else if (c->prev != NULL && c->prev->_free == FREE) {

		_chunk *p = c->prev;

		p->_chunk_sz += c->_chunk_sz + sizeof(_chunk);

		p->next = c->next;
		if (c->next != NULL)
			c->next->prev = p;

		_session->_chunks_allocated--;

	}

}

void _create_session() {

	void *ptr = sbrk(0);

	if ( brk(ptr + sizeof(_mem_session) ) < 0 ) {
		perror("Couldn't start memory session!");
		exit(2);
	}

	printf("here?\n");
	_session = (_mem_session*)ptr;

	printf("here??\n");
	_session->_first_chunk = NULL;
	_session->_last_chunk = NULL;

	printf("what about this?\n");
	_session->_chunks_allocated = 0;

}

_chunk *_find_free_chunk(size_t bytes) {
	_chunk *curr = _session->_first_chunk;

	while(curr != NULL && (curr->_chunk_sz < bytes || curr->_free == NOT_FREE))
		curr = curr->next;

	return curr;
}