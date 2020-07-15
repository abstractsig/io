/*
 *
 * NB: included by io_core.h
 *
 */
#ifndef io_event_H_
#define io_event_H_

typedef struct io_event_implementation io_event_implementation_t;
typedef struct io_event io_event_t;
typedef void (*io_event_handler_t) (io_event_t*);

#define IO_EVENT_IMPLEMENTATION_STRUCT_MEMBERS \
	io_event_implementation_t const * specialisation_of;\
	io_event_handler_t handler;\
	/**/

struct PACK_STRUCTURE io_event_implementation {
	IO_EVENT_IMPLEMENTATION_STRUCT_MEMBERS
};

void null_io_event_handler (io_event_t*);

#define SPECIALISE_IO_EVENT_IMPLEMENTATION(S) \
	.specialisation_of = (S), \
	.handler = null_io_event_handler, \
	/**/

extern EVENT_DATA io_event_implementation_t io_event_implementation;

#define IO_EVENT_STRUCT_MEMBERS \
	io_event_implementation_t const * implementation; \
	void (*event_handler) (io_event_t*);\
	void *user_value;\
	io_event_t *next_event;\
	/**/

struct PACK_STRUCTURE io_event {
	IO_EVENT_STRUCT_MEMBERS
};

#define def_io_event(FN,UV) {\
		.event_handler = FN, \
		.user_value = UV, \
		.next_event = NULL, \
	}

#define io_event_is_valid(ev) 	((ev)->event_handler != NULL)
#define io_event_is_active(ev) 	((ev)->next_event != NULL)

extern io_event_t s_null_io_event;

INLINE_FUNCTION io_event_t*
initialise_io_event (
	io_event_t *ev,io_event_handler_t fn,void* user_value
) {
	ev->implementation = &io_event_implementation;
	ev->event_handler = fn;
	ev->user_value = user_value;
	ev->next_event = NULL;
	return ev;
}

void* typesafe_io_cast_event (io_event_t*,io_event_implementation_t const*);

extern EVENT_DATA io_event_implementation_t io_transmit_available_event_implementation;
extern EVENT_DATA io_event_implementation_t io_data_available_event_implementation;
extern EVENT_DATA io_event_implementation_t io_opened_event_implementation;

INLINE_FUNCTION io_event_t*
initialise_io_transmit_available_event (
	io_event_t *ev,io_event_handler_t fn,void* user_value
) {
	initialise_io_event (ev,fn,user_value);
	ev->implementation = &io_transmit_available_event_implementation;
	return ev;
}

INLINE_FUNCTION io_event_t*
initialise_io_data_available_event (
	io_event_t *ev,io_event_handler_t fn,void* user_value
) {
	initialise_io_event (ev,fn,user_value);
	ev->implementation = &io_data_available_event_implementation;
	return ev;
}

INLINE_FUNCTION io_event_t*
initialise_io_opened_event (
	io_event_t *ev,io_event_handler_t fn,void* user_value
) {
	initialise_io_event (ev,fn,user_value);
	ev->implementation = &io_opened_event_implementation;
	return ev;
}

typedef struct io_event_list {
	io_byte_memory_t *bm;
	io_event_t **list;
	uint32_t length;
} io_event_list_t;

#define io_event_list_length(l)	(l)->length

io_event_list_t* mk_io_event_list (io_byte_memory_t*);
void io_event_list_free (io_event_list_t*);
bool io_event_list_append (io_event_list_t*,io_event_t*);
void io_event_list_remove (io_event_list_t*,io_event_t*);
io_event_t* io_event_list_first_match (io_event_list_t*,io_event_implementation_t const*);


#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------

EVENT_DATA io_event_implementation_t 
io_opened_event_implementation = {
	SPECIALISE_IO_EVENT_IMPLEMENTATION(&io_event_implementation)
};

EVENT_DATA io_event_implementation_t 
io_transmit_available_event_implementation = {
	SPECIALISE_IO_EVENT_IMPLEMENTATION (
		&io_event_implementation
	)
};

EVENT_DATA io_event_implementation_t 
io_data_available_event_implementation = {
	SPECIALISE_IO_EVENT_IMPLEMENTATION (
		&io_event_implementation
	)
};

void*
typesafe_io_cast_event (io_event_t *ev,io_event_implementation_t const *I) {
	if (ev) {
		io_event_implementation_t const *S = ev->implementation;
		do {
			if (S == I) {
				break;
			} else {
				S = S->specialisation_of;
			}
		} while (S);

		if (!S) ev = NULL;
	}
	return ev;
}

EVENT_DATA io_event_implementation_t 
io_event_implementation = {
	SPECIALISE_IO_EVENT_IMPLEMENTATION(NULL)
};

void
null_io_event_handler (io_event_t *ev) {
}

io_event_t s_null_io_event = def_io_event (
	null_io_event_handler,NULL
);


io_event_list_t*
mk_io_event_list (io_byte_memory_t *bm) {
	io_event_list_t *this = io_byte_memory_allocate (
		bm,sizeof(io_event_list_t)
	);
	
	if (this) {
		this->bm = bm;
		this->list = NULL;
		this->length = 0;
	}

	return this;
}

void
io_event_list_free (io_event_list_t *this) {
	io_byte_memory_free (this->bm,this->list);
	io_byte_memory_free (this->bm,this);
}

bool
io_event_list_contains (io_event_list_t *this,io_event_t *ev) {
	io_event_t **cursor = this->list;
	io_event_t **end = cursor + this->length;
	
	while (cursor < end) {
		if (*cursor == ev) {
			return true;
		}
		cursor++;
	}
	
	return false;
}

io_event_t*
io_event_list_first_match (
	io_event_list_t *this,io_event_implementation_t const *I
) {
	io_event_t **cursor = this->list;
	io_event_t **end = cursor + this->length;
	
	while (cursor < end) {
		if (typesafe_io_cast_event (*cursor,I) != NULL) {
			return *cursor;
		}
		cursor++;
	}
	
	return NULL;
}



#endif /* IMPLEMENT_IO_CORE */
#endif
/*
Copyright 2020 Gregor Bruce

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted, provided that the above copyright notice
and this permission notice appear in all copies.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT,
OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/


