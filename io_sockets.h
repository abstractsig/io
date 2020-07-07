/*
 *
 * layered communication via sockets
 * (included by io_core.h)
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 */
#ifndef io_sockets_H_
#define io_sockets_H_
//
// all io sockets must havs a mtu of at least IO_SOCKET_MINIMUM_MTU
//
#define IO_SOCKET_MINIMUM_MTU 128

typedef struct io_inner_binding io_inner_binding_t;
typedef struct io_inner_constructor_binding io_inner_constructor_binding_t;
typedef struct io_multiplex_socket io_multiplex_socket_t;
typedef struct io_socket_implementation io_socket_implementation_t;
typedef bool (*io_socket_iterator_t) (io_socket_t*,void*);
typedef bool (*io_socket_constructor_t) (io_t*,io_address_t,io_socket_t**,io_socket_t**);

//
// stuff referenced in layers
//

io_inner_binding_t* io_multiplex_socket_find_inner_binding (io_multiplex_socket_t*,io_address_t);

#include <io_layers.h>

//
// socket state
//
// A state machine that executes in the io event loop.  Care must
// be taken by the programmer to ensure that all calls to
// state machine methods are made from within the event loop.
//
typedef struct io_socket_state io_socket_state_t;

typedef enum {
	IO_SOCKET_OPEN_CONNECT,
	IO_SOCKET_OPEN_LISTEN
} io_socket_open_flag_t;

#define IO_SOCKET_STATE_STRUCT_MEMBERS \
	io_socket_state_t const *specialisation_of; \
	const char *name;\
	io_socket_state_t const* (*enter) (io_socket_t*); \
	io_socket_state_t const* (*open) (io_socket_t*,io_socket_open_flag_t); \
	io_socket_state_t const* (*open_for_inner) (io_socket_t*,io_address_t,io_socket_open_flag_t); \
	io_socket_state_t const* (*close) (io_socket_t*); \
	io_socket_state_t const* (*inner_closed) (io_socket_t*,io_address_t); \
	io_socket_state_t const* (*outer_receive_event) (io_socket_t*); \
	io_socket_state_t const* (*outer_transmit_event) (io_socket_t*); \
	io_socket_state_t const* (*timer) (io_socket_t*); \
	io_socket_state_t const* (*inner_send) (io_socket_t*); \
	io_socket_state_t const* (*timer_error) (io_socket_t*); \
	/**/
	
struct PACK_STRUCTURE io_socket_state {
	IO_SOCKET_STATE_STRUCT_MEMBERS
};

#define IO_SOCKET_RE_ENTER_STATE NULL

void io_socket_call_open (io_socket_t*,io_socket_open_flag_t);
void io_socket_call_open_for_inner (io_socket_t*,io_address_t,io_socket_open_flag_t);
void io_socket_call_inner_closed (io_socket_t*,io_address_t);
void io_socket_call_state (io_socket_t*,io_socket_state_t const* (*fn) (io_socket_t*));
io_socket_state_t const* io_socket_state_ignore_event (io_socket_t*);
io_socket_state_t const* io_socket_state_ignore_open_event (io_socket_t*,io_socket_open_flag_t);
io_socket_state_t const* io_socket_state_ignore_open_for_inner_event (io_socket_t*,io_address_t,io_socket_open_flag_t);
io_socket_state_t const* io_socket_state_ignore_inner_closed_event (io_socket_t*,io_address_t);

bool io_socket_try_open (io_socket_t*,io_socket_open_flag_t);

#define io_socket_call_close(s)						io_socket_call_state (s,(s)->State->close)
#define io_socket_call_outer_transmit_event(s)	io_socket_call_state (s,(s)->State->outer_transmit_event)
#define io_socket_call_outer_receive_event(s)	io_socket_call_state (s,(s)->State->outer_receive_event)

extern EVENT_DATA io_socket_state_t io_socket_state;

#define SPECIALISE_IO_SOCKET_STATE(S) \
	.specialisation_of = (io_socket_state_t const*) S, \
	.name = "state", \
	.enter = io_socket_state_ignore_event, \
	.open = io_socket_state_ignore_open_event, \
	.open_for_inner = io_socket_state_ignore_open_for_inner_event,\
	.close = io_socket_state_ignore_event, \
	.inner_closed = io_socket_state_ignore_inner_closed_event,\
	.outer_receive_event = io_socket_state_ignore_event, \
	.outer_transmit_event = io_socket_state_ignore_event, \
	.inner_send = io_socket_state_ignore_event, \
	.timer = io_socket_state_ignore_event, \
	.timer_error = io_socket_state_ignore_event, \
	/**/

typedef struct PACK_STRUCTURE io_socket_open_event {
	IO_EVENT_STRUCT_MEMBERS
	uint32_t flag;
} io_socket_open_event_t;

bool call_io_socket_state_open (io_socket_t*,io_socket_open_flag_t);

INLINE_FUNCTION void
free_io_socket_open_event (io_byte_memory_t *bm,io_socket_open_event_t *this) {
	io_byte_memory_free (bm,this);
}

//
// notify
//
typedef struct PACK_STRUCTURE io_notify_event {
	IO_EVENT_STRUCT_MEMBERS
	io_socket_t *socket;
} io_notify_event_t;

INLINE_FUNCTION void
initialise_io_notify (
	io_notify_event_t *ev,io_event_handler_t fn,void* user_value,io_socket_t *socket
) {
	initialise_io_event ((io_event_t*) ev,fn,user_value);
	ev->socket = socket;	// reference??
}

INLINE_FUNCTION void
free_io_notify (io_notify_event_t *ev) {
	ev->socket = NULL;
}

typedef struct PACK_STRUCTURE io_settings {
	io_encoding_implementation_t const *encoding;
	uint16_t transmit_pipe_length;
	uint16_t receive_pipe_length;
	io_socket_constructor_t make;
	io_notify_event_t *notify;
	uint32_t speed;
} io_settings_t;

#define io_settings_receive_pipe_length(c)	(c)->receive_pipe_length
#define io_settings_transmit_pipe_length(c)	(c)->transmit_pipe_length

#define IO_SOCKET_IMPLEMENTATION_STRUCT_MEMBERS \
	io_socket_implementation_t const *specialisation_of;\
	io_socket_t* (*initialise) (io_socket_t*,io_t*,io_settings_t const*);\
	io_socket_t* (*reference) (io_socket_t*,int32_t);\
	void (*free) (io_socket_t*);\
	bool (*open) (io_socket_t*,io_socket_open_flag_t);\
	void (*close) (io_socket_t*);\
	bool (*is_closed) (io_socket_t const*);\
	bool (*bind_inner) (io_socket_t*,io_address_t,io_event_t*,io_event_t*);\
	bool (*bind_inner_constructor) (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);\
	void (*unbind_inner) (io_socket_t*,io_address_t);\
	bool (*bind_to_outer_socket) (io_socket_t*,io_socket_t*);\
	io_pipe_t* (*get_receive_pipe) (io_socket_t*,io_address_t);\
	io_encoding_t*	(*new_message) (io_socket_t*); \
	bool (*send_message) (io_socket_t*,io_encoding_t*);\
	void (*flush) (io_socket_t*);\
	bool (*iterate_outer_sockets) (io_socket_t*,io_socket_iterator_t,void*);\
	size_t (*mtu) (io_socket_t const*);\
	/**/

struct PACK_STRUCTURE io_socket_implementation {
	IO_SOCKET_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_SOCKET_STRUCT_MEMBERS \
	io_socket_implementation_t const *implementation;\
	io_address_t address;\
	io_socket_state_t const *State;\
	io_t *io;\
	/**/

struct PACK_STRUCTURE io_socket {
	IO_SOCKET_STRUCT_MEMBERS
};

#define io_socket_io(s)				(s)->io
#define io_socket_address(s)		(s)->address
#define io_socket_byte_memory(s)	io_get_byte_memory(io_socket_io(s))

#define IO_SOCKET(s)					((io_socket_t*) (s))
#define INVALID_SOCKET_ID			0xffffffff

void	initialise_io_socket (io_socket_t*,io_t*);
void 	free_io_socket (io_socket_t*);
bool	is_io_socket_of_type (io_socket_t const*,io_socket_implementation_t const*);
bool	is_physical_io_socket (io_socket_t const*);
bool	is_constructed_io_socket (io_socket_t const*);

extern EVENT_DATA io_socket_implementation_t io_socket_implementation_base;
extern EVENT_DATA io_socket_implementation_t io_physical_socket_implementation;

//
// inline io socket implementation
//

INLINE_FUNCTION void
io_socket_enter_current_state (io_socket_t *socket) {
	io_socket_call_state (socket,socket->State->enter);
}

INLINE_FUNCTION io_socket_state_t const*
call_io_socket_state_enter (io_socket_t *socket) {
	return socket->State->enter (socket);
}

INLINE_FUNCTION io_socket_t*
io_socket_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	return socket->implementation->initialise(socket,io,C);
}

INLINE_FUNCTION io_socket_t*
io_socket_increment_reference (io_socket_t *socket,int32_t incr) {
	return socket->implementation->reference(socket,incr);
}

INLINE_FUNCTION io_socket_t*
reference_io_socket (io_socket_t *socket) {
	if (socket) {
		return socket->implementation->reference(socket,1);
	} else {
		return socket;
	}
}

INLINE_FUNCTION io_socket_t*
unreference_io_socket (io_socket_t *socket) {
	return socket->implementation->reference(socket,-1);
}

INLINE_FUNCTION void
io_socket_free (io_socket_t *socket) {
	socket->implementation->free (socket);
}

INLINE_FUNCTION bool
io_socket_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	return socket->implementation->open (socket,flag);
}

INLINE_FUNCTION void
io_socket_close (io_socket_t *socket) {
	socket->implementation->close (socket);
}

INLINE_FUNCTION bool
io_socket_is_closed (io_socket_t const *socket) {
	return socket->implementation->is_closed (socket);
}
#define io_socket_is_open(s)	(!io_socket_is_closed(s))

INLINE_FUNCTION io_encoding_t*
io_socket_new_message (io_socket_t *socket) {
	return socket->implementation->new_message (socket);
}

INLINE_FUNCTION bool
io_socket_send_message (io_socket_t *socket,io_encoding_t *m) {
	return socket->implementation->send_message (socket,m);
}

INLINE_FUNCTION void
io_socket_flush (io_socket_t *socket) {
	socket->implementation->flush (socket);
}

INLINE_FUNCTION bool
io_socket_bind_inner (
	io_socket_t *socket,io_address_t a,io_event_t *tx,io_event_t *rx
) {
	return socket->implementation->bind_inner (socket,a,tx,rx);
}

INLINE_FUNCTION void
io_socket_unbind_inner (io_socket_t *socket,io_address_t a) {
	socket->implementation->unbind_inner (socket,a);
}

INLINE_FUNCTION bool
io_socket_bind_inner_constructor (
	io_socket_t *socket,io_address_t a,io_socket_constructor_t make,io_notify_event_t *notify
) {
	return socket->implementation->bind_inner_constructor (socket,a,make,notify);
}

INLINE_FUNCTION bool
io_socket_bind_to_outer_socket (io_socket_t *socket,io_socket_t *outer) {
	return socket->implementation->bind_to_outer_socket (socket,outer);
}

INLINE_FUNCTION io_pipe_t*
io_socket_get_receive_pipe (io_socket_t *socket,io_address_t a) {
	return socket->implementation->get_receive_pipe (socket,a);
}

INLINE_FUNCTION size_t
io_socket_mtu (io_socket_t *socket) {
	return socket->implementation->mtu (socket);
}

//
// virtual socket
//

io_socket_t*	io_virtual_socket_initialise (io_socket_t*,io_t*,io_settings_t const*);
io_socket_t*	io_virtual_socket_increment_reference (io_socket_t*,int32_t);
void 				io_virtual_socket_free (io_socket_t*);
bool				io_virtual_socket_open (io_socket_t*,io_socket_open_flag_t);
void				io_virtual_socket_close (io_socket_t*);
bool				io_virtual_socket_is_closed (io_socket_t const*);
bool				io_virtual_socket_bind_inner (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
bool				io_virtual_socket_bind_inner_constructor (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);
io_pipe_t*		io_virtual_socket_get_receive_pipe (io_socket_t*,io_address_t);
void				io_virtual_socket_unbind_inner (io_socket_t*,io_address_t);
bool				io_virtual_socket_bind_to_outer_socket (io_socket_t*,io_socket_t*);
io_encoding_t*	io_virtual_socket_new_message (io_socket_t*);
bool				io_virtual_socket_send_message (io_socket_t*,io_encoding_t*);
size_t			io_virtual_socket_mtu (io_socket_t const*);
void				io_virtual_socket_flush (io_socket_t*);

#define SPECIALISE_IO_SOCKET_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.reference = io_virtual_socket_increment_reference, \
	.initialise = io_virtual_socket_initialise, \
	.free = io_virtual_socket_free, \
	.open = io_virtual_socket_open, \
	.close = io_virtual_socket_close, \
	.is_closed = io_virtual_socket_is_closed, \
	.bind_inner = io_virtual_socket_bind_inner, \
	.bind_inner_constructor = io_virtual_socket_bind_inner_constructor,\
	.unbind_inner = io_virtual_socket_unbind_inner,\
	.bind_to_outer_socket = io_virtual_socket_bind_to_outer_socket,\
	.new_message = io_virtual_socket_new_message, \
	.send_message = io_virtual_socket_send_message, \
	.get_receive_pipe = io_virtual_socket_get_receive_pipe, \
	.flush = io_virtual_socket_flush, \
	.iterate_outer_sockets = NULL, \
	.mtu = io_virtual_socket_mtu, \
	/**/

//
// binding containers
//

struct PACK_STRUCTURE io_inner_constructor_binding {
	io_address_t address;
	io_socket_constructor_t make;
	io_notify_event_t *notify;
};

typedef struct PACK_STRUCTURE {
	io_inner_constructor_binding_t *begin;
	io_inner_constructor_binding_t *end;
} io_inner_constructor_bindings_t;

io_inner_constructor_bindings_t* mk_io_inner_constructor_bindings (io_t*);
void free_io_inner_constructor_bindings (io_inner_constructor_bindings_t*,io_t*);
io_inner_constructor_binding_t* io_inner_constructor_bindings_find_binding (io_inner_constructor_bindings_t*,io_address_t);
bool io_inner_constructor_bindings_bind (io_inner_constructor_bindings_t*,io_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);

//
// counted socket
//

#define IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	IO_SOCKET_STRUCT_MEMBERS\
	uint32_t reference_count;\
	/**/

typedef struct PACK_STRUCTURE io_counted_socket {
	IO_COUNTED_SOCKET_STRUCT_MEMBERS
} io_counted_socket_t;

void				initialise_io_counted_socket (io_counted_socket_t*,io_t*);
io_socket_t*	io_counted_socket_increment_reference (io_socket_t*,int32_t);
void 				io_counted_socket_free (io_socket_t*);

#define  SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_SOCKET_IMPLEMENTATION (S)\
	.reference = io_counted_socket_increment_reference,\
	.free = io_counted_socket_free,\
	/**/

extern EVENT_DATA io_socket_implementation_t io_counted_socket_implementation;

INLINE_FUNCTION io_counted_socket_t*
cast_to_io_counted_socket (io_socket_t *socket) {
	if (is_io_socket_of_type (socket,&io_counted_socket_implementation)) {
		return (io_counted_socket_t*) socket;
	} else {
		return NULL;
	}
}

//
// process: a single-inner, single-outer socket
//
#define IO_PROCESS_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_event_t *transmit_available; \
	io_event_t *receive_data_available; \
	io_socket_t *outer_socket; \
	io_event_t outer_transmit_event; \
	io_event_t outer_receive_event; \
	/**/

typedef struct PACK_STRUCTURE io_process_socket {
	IO_PROCESS_SOCKET_STRUCT_MEMBERS
} io_process_socket_t;

io_socket_t* io_process_socket_initialise (io_socket_t*,io_t*,io_settings_t const*);
bool io_process_socket_add_inner_binding (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
bool io_process_socket_bind_to_outer (io_socket_t*,io_socket_t*);
bool io_process_socket_is_closed (io_socket_t const*);
void io_process_socket_close (io_socket_t*);
io_encoding_t* io_process_socket_new_message (io_socket_t*);
bool io_process_socket_send_message (io_socket_t*,io_encoding_t*);

#define  SPECIALISE_IO_PROCESS_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION (S)\
	.initialise = io_process_socket_initialise, \
	.open = io_socket_try_open,\
	.close = io_process_socket_close,\
	.is_closed = io_process_socket_is_closed, \
	.bind_inner = io_process_socket_add_inner_binding, \
	.bind_to_outer_socket = io_process_socket_bind_to_outer, \
	.new_message = io_process_socket_new_message,\
	.send_message = io_process_socket_send_message,\
	/**/

extern EVENT_DATA io_socket_implementation_t io_process_socket_implementation;

//
// adapter for connecting non-socket layers
//
#define IO_ADAPTER_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_event_t *transmit_available; \
	io_event_t *receive_data_available; \
	io_socket_t *outer_socket; \
	/**/

typedef struct PACK_STRUCTURE io_adapter_socket {
	IO_ADAPTER_SOCKET_STRUCT_MEMBERS	
} io_adapter_socket_t;

io_socket_t* allocate_io_adapter_socket (io_t*,io_address_t);
io_socket_t* io_adapter_socket_initialise (io_socket_t*,io_t*,io_settings_t const*);
void io_adapter_socket_free (io_socket_t*);
bool io_adapter_socket_open (io_socket_t*,io_socket_open_flag_t);
void io_adapter_socket_close (io_socket_t*);
bool io_adapter_socket_is_closed (io_socket_t const*);
bool io_adapter_socket_bind (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
void io_adapter_socket_unbind_inner (io_socket_t*,io_address_t);
bool io_adapter_socket_bind_to_outer (io_socket_t*,io_socket_t*);
io_encoding_t* io_adapter_socket_new_message (io_socket_t*);
bool io_adapter_socket_send_message (io_socket_t*,io_encoding_t*);
io_pipe_t* io_adapter_socket_get_receive_pipe (io_socket_t*,io_address_t);
size_t io_adapter_socket_mtu (io_socket_t const*);

#define  SPECIALISE_IO_ADAPTER_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION (S)\
	.initialise = io_adapter_socket_initialise, \
	.free = io_adapter_socket_free, \
	.open = io_adapter_socket_open, \
	.close = io_adapter_socket_close, \
	.is_closed = io_adapter_socket_is_closed, \
	.bind_inner = io_adapter_socket_bind, \
	.unbind_inner = io_adapter_socket_unbind_inner, \
	.bind_to_outer_socket = io_adapter_socket_bind_to_outer, \
	.new_message = io_adapter_socket_new_message, \
	.send_message = io_adapter_socket_send_message, \
	.get_receive_pipe = io_adapter_socket_get_receive_pipe, \
	.mtu = io_adapter_socket_mtu, \
	/**/
	
extern EVENT_DATA io_socket_implementation_t io_adapter_socket_implementation;

INLINE_FUNCTION io_adapter_socket_t*
cast_to_io_adapter_socket (io_socket_t *socket) {
	if (is_io_socket_of_type (socket,&io_adapter_socket_implementation)) {
		return (io_adapter_socket_t*) socket;
	} else {
		return NULL;
	}
}

//
// socket internal to communication stack, i.e. it does not 
// support inner bindings
//
#define IO_LEAF_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_event_t transmit_event; \
	io_event_t receive_event; \
	io_encoding_pipe_t *receive_pipe;\
	io_socket_t *outer_socket; \
	io_inner_constructor_bindings_t *inner_constructors;\
	/**/

typedef struct PACK_STRUCTURE {
	IO_LEAF_SOCKET_STRUCT_MEMBERS
} io_leaf_socket_t;

void initialise_io_leaf_socket (io_leaf_socket_t*,io_t*,io_settings_t const*);
void io_leaf_socket_free (io_socket_t*);
bool io_leaf_socket_bind_to_outer (io_socket_t*,io_socket_t*);
bool io_leaf_socket_bind_inner_constructor (io_socket_t *socket,io_address_t address,io_socket_constructor_t make,io_notify_event_t *);
io_inner_constructor_binding_t* io_leaf_socket_find_inner_constructor (io_socket_t*,io_address_t);

#define SPECIALISE_IO_LEAF_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION (S)\
	.free = io_leaf_socket_free,\
	.bind_inner_constructor = io_leaf_socket_bind_inner_constructor,\
	.bind_to_outer_socket = io_leaf_socket_bind_to_outer,
	/**/

extern EVENT_DATA io_socket_implementation_t io_leaf_socket_implementation;

//
// multiplex socket
//

typedef struct io_inner_constructor {
	io_socket_constructor_t *make;
	io_notify_event_t *notify;
} io_inner_constructor_t;

typedef struct io_inner_port io_inner_port_t;
struct io_inner_port {
	io_encoding_pipe_t *transmit_pipe;
	io_encoding_pipe_t *receive_pipe;
	io_event_t *tx_available;
	io_event_t *rx_available;
};

void free_io_inner_port (io_byte_memory_t*,io_inner_port_t*);

struct PACK_STRUCTURE io_inner_binding {
	io_address_t address;
	io_inner_port_t *port;
};

#define io_inner_binding_address(b)				(b)->address
#define io_inner_binding_port(b)					(b)->port
#define io_inner_binding_transmit_pipe(b)		io_inner_binding_port(b)->transmit_pipe
#define io_inner_binding_receive_pipe(b)		io_inner_binding_port(b)->receive_pipe
#define io_inner_binding_receive_event(b)		io_inner_binding_port(b)->rx_available

#define IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_inner_binding_t *slots; \
	uint32_t number_of_slots; \
	io_inner_constructor_bindings_t *inner_constructors;\
	io_inner_binding_t *transmit_cursor; \
	uint16_t transmit_pipe_length; \
	uint16_t receive_pipe_length; \
	/**/

struct PACK_STRUCTURE io_multiplex_socket {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS
};

#define io_multiplex_socket_has_inner_bindings(s) 	((s)->slots != NULL)

io_socket_t*	allocate_io_multiplex_socket (io_t*,io_address_t);
io_socket_t*	initialise_io_multiplex_socket (io_socket_t*,io_t*,io_settings_t const*);
void				io_multiplex_socket_free (io_socket_t*);
bool				io_multiplex_socket_bind_inner (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
bool 				io_multiplex_socket_bind_inner_constructor (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);
void				io_multiplex_socket_unbind_inner (io_socket_t*,io_address_t);
void				io_multiplex_socket_round_robin_signal_transmit_available (io_multiplex_socket_t*);
io_pipe_t* 		io_multiplex_socket_get_receive_pipe (io_socket_t*,io_address_t);
io_inner_constructor_binding_t* io_multiplex_socket_find_inner_constructor_binding (io_multiplex_socket_t*,io_address_t);
io_inner_binding_t* io_multiplex_socket_get_next_transmit_binding (io_multiplex_socket_t*);

extern EVENT_DATA io_socket_implementation_t io_multiplex_socket_implementation;

INLINE_FUNCTION io_multiplex_socket_t*
cast_to_io_multiplex_socket (io_socket_t *socket) {
	if (is_io_socket_of_type (socket,&io_multiplex_socket_implementation)) {
		return (io_multiplex_socket_t*) socket;
	} else {
		return NULL;
	}
}

#define  SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION (S)\
	.initialise = initialise_io_multiplex_socket,\
	.free = io_multiplex_socket_free,\
	.bind_inner = io_multiplex_socket_bind_inner,\
	.unbind_inner = io_multiplex_socket_unbind_inner,\
	.bind_inner_constructor = io_multiplex_socket_bind_inner_constructor,\
	.get_receive_pipe = io_multiplex_socket_get_receive_pipe, \
	/**/

//
// multiplexer socket
//

#define IO_MULTIPLEXER_SOCKET_STRUCT_MEMBERS \
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	io_event_t transmit_event; \
	io_event_t receive_event; \
	io_socket_t *outer_socket; \
	/**/

typedef struct PACK_STRUCTURE io_multiplexer_socket {
	IO_MULTIPLEXER_SOCKET_STRUCT_MEMBERS
} io_multiplexer_socket_t;

io_socket_t*	allocate_io_multiplexer_socket (io_t*,io_address_t);
io_socket_t*	initialise_io_multiplexer_socket (io_socket_t*,io_t*,io_settings_t const*);
void				io_multiplexer_socket_free (io_socket_t*);
void				close_io_multiplexer_socket (io_multiplexer_socket_t*);
bool				io_multiplexer_socket_bind_to_outer (io_socket_t*,io_socket_t*);
bool				io_multiplexer_socket_send_message (io_socket_t*,io_encoding_t*);
size_t			io_multiplexer_socket_mtu (io_socket_t const*);

#define  SPECIALISE_IO_MULTIPLEXER_SOCKET_IMPLEMENTATION(S) \
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION (S)\
	.initialise = initialise_io_multiplexer_socket,\
	.free = io_multiplexer_socket_free,\
	.bind_to_outer_socket = io_multiplexer_socket_bind_to_outer,\
	.send_message = io_multiplexer_socket_send_message,\
	.mtu = io_multiplexer_socket_mtu,\
	/**/

extern EVENT_DATA io_socket_implementation_t io_multiplexer_socket_implementation;

INLINE_FUNCTION io_multiplexer_socket_t*
cast_to_io_multiplexer_socket (io_socket_t *socket) {
	if (is_io_socket_of_type (socket,&io_multiplexer_socket_implementation)) {
		return (io_multiplexer_socket_t*) socket;
	} else {
		return NULL;
	}
}

//
// director socket
//

typedef struct io_outer_port {
	io_encoding_pipe_t *transmit_pipe;
	io_encoding_pipe_t *receive_pipe;
	io_event_t transmit_event;
	io_event_t receive_event;
} io_outer_port_t;

void free_io_outer_port (io_byte_memory_t*,io_outer_port_t*);

typedef struct PACK_STRUCTURE {
	io_address_t address;
	io_outer_port_t *port;
} io_outer_binding_t;

typedef struct {
	io_outer_binding_t *begin;
	io_outer_binding_t *end;
} io_outer_bindings_t;

#define IO_DIRECTOR_SOCKET_STRUCT_MEMBERS \
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	io_outer_bindings_t *bindings;\
	/**/

typedef struct PACK_STRUCTURE io_director_socket {
	IO_DIRECTOR_SOCKET_STRUCT_MEMBERS
} io_director_socket;
//
// emmulate a socket (for test)
//
typedef struct PACK_STRUCTURE io_socket_emulator {
	IO_MULTIPLEXER_SOCKET_STRUCT_MEMBERS
	
	// events for single outter binding
	io_event_t tx;
	io_event_t rx;
	
} io_socket_emulator_t;

io_socket_t* allocate_io_socket_binary_emulator (io_t*,io_address_t);
io_socket_t* allocate_io_socket_link_emulator (io_t*,io_address_t);

io_socket_t* io_socket_emulator_initialise (io_socket_t*,io_t*,io_settings_t const*);
void io_socket_emulator_free (io_socket_t*);
bool io_socket_emulator_open (io_socket_t*,io_socket_open_flag_t);
void io_socket_emulator_close (io_socket_t*);
bool io_socket_emulator_is_closed (io_socket_t const*);
bool io_socket_emulator_bind_to_outer_socket (io_socket_t*,io_socket_t*);
size_t io_socket_emulator_mtu (io_socket_t const*);

extern EVENT_DATA io_socket_implementation_t io_socket_emulator_implementation;

#define  SPECIALISE_IO_SOCKET_EMULATOR_IMPLEMENTATION(S) \
	SPECIALISE_IO_MULTIPLEXER_SOCKET_IMPLEMENTATION (S)\
	.initialise = io_socket_emulator_initialise, \
	.free = io_socket_emulator_free, \
	.open = io_socket_emulator_open, \
	.close = io_socket_emulator_close, \
	.is_closed = io_socket_emulator_is_closed, \
	.bind_to_outer_socket = io_socket_emulator_bind_to_outer_socket, \
	.mtu = io_socket_emulator_mtu, \
	/**/

//
// emmulate a shared communication media
//
typedef struct PACK_STRUCTURE io_shared_media {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS
} io_shared_media_t;

io_socket_t* allocate_io_shared_media (io_t*,io_address_t);

INLINE_FUNCTION void
free_io_sockets (io_socket_t **cursor,io_socket_t **end) {
	while (cursor < end) {
		unreference_io_socket (*cursor++);
	}
}

//
// socket builder
//
typedef struct PACK_STRUCTURE socket_builder_binding {
	uint32_t inner;
	uint32_t outer;
} socket_builder_binding_t;

typedef io_socket_t* (*allocate_io_socket_t) (io_t*,io_address_t);

typedef struct {
	uint32_t index;
	io_settings_t const *C;
	io_socket_constructor_t make;
	io_address_t address;
	io_notify_event_t *notify;
} io_constructed_socket_t;

typedef struct socket_builder {
	uint32_t index;
	allocate_io_socket_t allocate;
	io_address_t address;
	io_settings_t const *C;
	bool with_open;
	socket_builder_binding_t const* inner_bindings;
} socket_builder_t;

#define END_OF_BINDINGS		{INVALID_SOCKET_ID,INVALID_SOCKET_ID}
#define BINDINGS(...)		(const socket_builder_binding_t []) {__VA_ARGS__,END_OF_BINDINGS}

void	build_io_sockets (io_t*,io_socket_t**,socket_builder_t const*,uint32_t);


#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

bool
is_io_socket_of_type (
	io_socket_t const *socket,io_socket_implementation_t const *T
) {
	io_socket_implementation_t const *E = socket->implementation;
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

bool
is_physical_io_socket (io_socket_t const *socket) {
	return is_io_socket_of_type (socket,&io_physical_socket_implementation);
}

//
// state
//
static void
io_socket_async_open (io_event_t *ev) {
	io_socket_open_event_t *this = (io_socket_open_event_t*) ev;
	io_socket_t *socket = ev->user_value;
	
	io_socket_call_open (socket,this->flag);
	io_byte_memory_free (
		io_socket_byte_memory(socket),ev
	);
}

bool
call_io_socket_state_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	io_socket_open_event_t *this = io_byte_memory_allocate (
		io_socket_byte_memory(socket),sizeof(io_socket_open_event_t)
	);
	
	if (this) {
		initialise_io_event ((io_event_t*) this,io_socket_async_open,socket);
		this->flag = flag;
		io_enqueue_event (io_socket_io (socket),(io_event_t*) this);
	}
	
	return this != NULL;
}

bool
io_socket_try_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	io_socket_call_open (socket,flag);
	return true;
}

void
io_socket_call_open_for_inner (
	io_socket_t *socket,io_address_t address,io_socket_open_flag_t flag
) {
	io_socket_state_t const *current = socket->State;
	io_socket_state_t const *next = socket->State->open_for_inner  (socket,address,flag);
	if (next != current) {
		socket->State = next;
		io_socket_enter_current_state (socket);
	}
}

void
io_socket_call_open (
	io_socket_t *socket,io_socket_open_flag_t flag
) {
	io_socket_state_t const *current = socket->State;
	io_socket_state_t const *next = socket->State->open (socket,flag);
	if (next != current) {
		socket->State = next;
		io_socket_enter_current_state (socket);
	}
}

void
io_socket_call_inner_closed (io_socket_t *socket,io_address_t inner) {
	io_socket_state_t const *current = socket->State;
	io_socket_state_t const *next = socket->State->inner_closed (socket,inner);
	if (next != current) {
		socket->State = next;
		io_socket_enter_current_state (socket);
	}
}

void
io_socket_call_state (
	io_socket_t *socket,io_socket_state_t const* (*fn) (io_socket_t*)
) {
	io_socket_state_t const *current = socket->State;
	io_socket_state_t const *next = fn (socket);
	if (next == NULL) {
		io_socket_enter_current_state (socket);		
	} else if (next != current) {
		socket->State = next;
		io_socket_enter_current_state (socket);
	}
}

io_socket_state_t const*
io_socket_state_ignore_event (io_socket_t *socket) {
	return socket->State;
}

io_socket_state_t const*
io_socket_state_ignore_open_event (io_socket_t *socket,io_socket_open_flag_t flag) {
	return socket->State;
}

io_socket_state_t const*
io_socket_state_ignore_open_for_inner_event (
	io_socket_t *socket,io_address_t a,io_socket_open_flag_t flag
) {
	return socket->State;
}

io_socket_state_t const*
io_socket_state_ignore_inner_closed_event (io_socket_t *socket,io_address_t a) {
	return socket->State;
}

EVENT_DATA io_socket_state_t io_socket_state = {
	SPECIALISE_IO_SOCKET_STATE(NULL)
};

//
// virtual sockets
//

io_socket_t*
io_virtual_socket_initialise (
	io_socket_t *socket,io_t *io,io_settings_t const *C
) {
	return NULL;
}

io_socket_t*
io_virtual_socket_increment_reference (io_socket_t *socket,int32_t incr) {
	return socket;
}

void
io_virtual_socket_free (io_socket_t *socket) {
}

bool
io_virtual_socket_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	return false;
}

void
io_virtual_socket_close (io_socket_t *socket) {
}

bool
io_virtual_socket_is_closed (io_socket_t const *socket) {
	return true;
}
bool
io_virtual_socket_bind_inner (
	io_socket_t *socket,io_address_t address,io_event_t *tx,io_event_t *rx
) {
	return false;
}

bool
io_virtual_socket_bind_inner_constructor (
	io_socket_t *socket,io_address_t address,io_socket_constructor_t make,io_notify_event_t *notify
) {
	return false;
}

void
io_virtual_socket_unbind_inner (io_socket_t *socket,io_address_t address) {
}

bool
io_virtual_socket_bind_to_outer_socket (io_socket_t *socket,io_socket_t *outer) {
	return false;
}

io_encoding_t*
io_virtual_socket_new_message (io_socket_t *socket) {
	return NULL;
}

bool
io_virtual_socket_send_message (io_socket_t *socket,io_encoding_t *msg) {
	return false;
}

io_pipe_t*
io_virtual_socket_get_receive_pipe (io_socket_t *socket,io_address_t address) {
	return NULL;
}

void
io_virtual_socket_flush (io_socket_t *socket) {
}

size_t
io_virtual_socket_mtu (io_socket_t const *socket) {
	return 0;
}

EVENT_DATA io_socket_implementation_t io_socket_implementation_base = {
	SPECIALISE_IO_SOCKET_IMPLEMENTATION (NULL)
};

EVENT_DATA io_socket_implementation_t io_physical_socket_implementation = {
	SPECIALISE_IO_SOCKET_IMPLEMENTATION (&io_socket_implementation_base)
};

//
// socket base
//
/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *
 *
 *               io_socket_default_state_closed
 *                 |    ^
 *          <open> |    | <close>
 *                 v    |                         
 *               io_socket_default_state_open
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_socket_state_t io_socket_default_state_closed;
static EVENT_DATA io_socket_state_t io_socket_default_state_open;

io_socket_state_t const*
io_socket_default_state_closed_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	io_socket_open (socket,flag);
	return &io_socket_default_state_open;
}

static EVENT_DATA io_socket_state_t io_socket_default_state_closed = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "closed",
	.open = io_socket_default_state_closed_open,
};

io_socket_state_t const*
io_socket_default_state_open_close (io_socket_t *socket) {
	io_socket_close (socket);
	return &io_socket_default_state_closed;
}

static EVENT_DATA io_socket_state_t io_socket_default_state_open = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "open",
	.close = io_socket_default_state_open_close,
};

void
initialise_io_socket (io_socket_t *this,io_t *io) {
	this->io = io;
	this->State = &io_socket_default_state_closed;
}

void
free_io_socket (io_socket_t *socket) {
	io_byte_memory_t *bm = io_socket_byte_memory(socket);
	free_io_address (bm,socket->address);
	io_byte_memory_free (bm,socket);
}

//
// counted socket
// 

void
initialise_io_counted_socket (io_counted_socket_t *this,io_t *io) {
	this->reference_count = 0;
	initialise_io_socket((io_socket_t*) this,io);
}

io_socket_t*
io_counted_socket_increment_reference (io_socket_t *socket,int32_t incr) {
	io_counted_socket_t *this = (io_counted_socket_t*) socket;
	if (incr > 0) {
		this->reference_count += incr;
		return socket;
	} else {
		if (this->reference_count > 0) {
			this->reference_count += incr;
		}
		if (this->reference_count == 0) {
			io_socket_free (socket);
		}
		return NULL;
	}
}

void
io_counted_socket_free (io_socket_t *socket) {
	free_io_socket (socket);
}

EVENT_DATA io_socket_implementation_t io_counted_socket_implementation = {
	SPECIALISE_IO_COUNTED_SOCKET_IMPLEMENTATION (&io_socket_implementation_base)
};

//
// leaf socket
//

//
// leaf socket
//
void
initialise_io_leaf_socket (io_leaf_socket_t *this,io_t *io,io_settings_t const *C) {

	initialise_io_counted_socket ((io_counted_socket_t*) this,io);

	this->outer_socket = NULL;
	this->inner_constructors = mk_io_inner_constructor_bindings(io);
	this->receive_pipe = mk_io_encoding_pipe (
		io_get_byte_memory(io),io_settings_receive_pipe_length(C)
	);

}

void
io_leaf_socket_free (io_socket_t *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t *) socket;
	free_io_encoding_pipe (this->receive_pipe,io_socket_byte_memory(socket));
	free_io_inner_constructor_bindings (this->inner_constructors,io_socket_io (this));
	io_counted_socket_free (socket);
}

bool
io_leaf_socket_bind_inner_constructor (
	io_socket_t *socket,io_address_t address,io_socket_constructor_t make,io_notify_event_t *notify
) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	return io_inner_constructor_bindings_bind (
		this->inner_constructors,io_socket_io (this),address,make,notify
	);
}

bool
io_leaf_socket_bind_to_outer (io_socket_t *socket,io_socket_t *outer) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;

	this->outer_socket = outer;

	io_socket_bind_inner (
		outer,
		io_socket_address (socket),
		&this->transmit_event,
		&this->receive_event
	);

	return true;
}

io_inner_constructor_binding_t*
io_leaf_socket_find_inner_constructor (io_socket_t *socket,io_address_t address) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	return io_inner_constructor_bindings_find_binding (
		this->inner_constructors,address
	);
}

EVENT_DATA io_socket_implementation_t io_leaf_socket_implementation = {
	SPECIALISE_IO_LEAF_SOCKET_IMPLEMENTATION (&io_counted_socket_implementation)
};

/*
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 *
 * process socket states
 *
 *               io_process_socket_state_unbound
 *                 |
 *                 v
 *               io_process_socket_state_closed
 *                 |
 *                 v
 *               io_process_socket_state_open_wait
 *                 |
 *          <open> |
 *                 v                           <close>
 *               io_process_socket_state_open --->
 *
 *-----------------------------------------------------------------------------
 *-----------------------------------------------------------------------------
 */
static EVENT_DATA io_socket_state_t io_process_socket_state_closed;
static EVENT_DATA io_socket_state_t io_process_socket_state_open;

io_encoding_t*
io_process_socket_new_message (io_socket_t *socket) {
	io_process_socket_t *this = (io_process_socket_t*) socket;
	if (this->outer_socket != NULL) {

		// i need a ntag layer...

		return io_socket_new_message (this->outer_socket);
	} else {
		return NULL;
	}
}

bool
io_process_socket_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_process_socket_t *this = (io_process_socket_t*) socket;
	bool ok = false;
	if (this->outer_socket != NULL) {
		ok = io_socket_send_message (this->outer_socket,encoding);
	}
	unreference_io_encoding (encoding);
	return ok;
}

io_socket_state_t const*
io_process_socket_state_closed_open_generic (
	io_socket_t *socket,io_socket_open_flag_t flag,io_socket_state_t const *next
) {
	io_process_socket_t *this = (io_process_socket_t*) socket;

	//
	// We can open only if there is no inner binding, otherwise
	// we should use open_for_inner().
	//

	if (this->outer_socket != NULL) {
		io_socket_call_open_for_inner (
			this->outer_socket,io_socket_address (socket),flag
		);
		return next;
	}

	return this->State;
}

io_socket_state_t const*
io_process_socket_state_closed_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	return io_process_socket_state_closed_open_generic (
		socket,flag,&io_process_socket_state_open
	);
}

static EVENT_DATA io_socket_state_t io_process_socket_state_closed = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "p-closed",
	.open = io_process_socket_state_closed_open,
};

io_socket_state_t const*
io_process_socket_state_open_close_generic (
	io_socket_t *socket,io_socket_state_t const *next
) {
	io_process_socket_t *this = (io_process_socket_t*) socket;
	io_socket_call_inner_closed (this->outer_socket,io_socket_address (socket));
	return next;
}

io_socket_state_t const*
io_process_socket_state_open_close (io_socket_t *socket) {
	return io_process_socket_state_open_close_generic (
		socket,&io_process_socket_state_closed
	);
}

static EVENT_DATA io_socket_state_t io_process_socket_state_open = {
	SPECIALISE_IO_SOCKET_STATE (&io_socket_state)
	.name = "p-open",
	.close = io_process_socket_state_open_close,
};


static void
io_process_socket_outer_tx_event (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_outer_transmit_event (socket);
}

static void
io_process_socket_outer_rx_event (io_event_t *ev) {
	io_socket_t *socket = ev->user_value;
	io_socket_call_outer_receive_event (socket);
}

io_socket_t*
io_process_socket_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_process_socket_t *this = (io_process_socket_t*) socket;

	initialise_io_counted_socket ((io_counted_socket_t*) socket,io);

	this->State = &io_process_socket_state_closed;
	this->transmit_available = NULL;
	this->receive_data_available = NULL;

	initialise_io_event (
		&this->outer_transmit_event,io_process_socket_outer_tx_event,this
	);

	initialise_io_event (
		&this->outer_receive_event,io_process_socket_outer_rx_event,this
	);

	return socket;
}

//
// this access data structures that are shared with event thread
//
bool
io_process_socket_add_inner_binding (
	io_socket_t *socket,io_address_t a,io_event_t *tx,io_event_t *rx
) {
	io_process_socket_t *this = (io_process_socket_t*) socket;

	this->transmit_available = tx;
	this->receive_data_available = rx;

	return io_socket_bind_to_outer_socket (socket,this->outer_socket);
}

bool
io_process_socket_bind_to_outer (io_socket_t *socket,io_socket_t *outer) {
	io_process_socket_t *this = (io_process_socket_t*) socket;

	this->outer_socket = outer;

	io_socket_bind_inner (
		outer,
		io_socket_address (socket),
		&this->outer_transmit_event,
		&this->outer_receive_event
	);

	return true;
}

void
io_process_socket_close (io_socket_t *socket) {
	io_socket_call_close (socket);
}

bool
io_process_socket_is_closed (io_socket_t const *socket) {
	return socket->State == &io_process_socket_state_closed;
}

EVENT_DATA io_socket_implementation_t io_process_socket_implementation = {
	SPECIALISE_IO_PROCESS_SOCKET_IMPLEMENTATION (
		&io_counted_socket_implementation
	)
};
//
// adapter sockets have 1:1 inner and outer relations
//

io_socket_t*
io_adapter_socket_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;

	initialise_io_counted_socket ((io_counted_socket_t*) socket,io);
	this->outer_socket = NULL;
	
	this->transmit_available = NULL;
	this->receive_data_available = NULL;
	
	return socket;
}

io_socket_t*
allocate_io_adapter_socket (io_t *io,io_address_t address) {
	io_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_adapter_socket_t)
	);
	socket->implementation = &io_adapter_socket_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

void
io_adapter_socket_free (io_socket_t *socket) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;

	io_dequeue_event (io_socket_io (socket),this->transmit_available);
	io_dequeue_event (io_socket_io (socket),this->receive_data_available);
	
	io_counted_socket_free (socket);
}

bool
io_adapter_socket_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_open (this->outer_socket,flag);
	} else {
		return false;
	}
}

void
io_adapter_socket_close (io_socket_t *socket) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	if (this->outer_socket != NULL) {
		io_socket_unbind_inner (this->outer_socket,io_socket_address(socket));
	}
}

bool
io_adapter_socket_is_closed (io_socket_t const *socket) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_is_closed (this->outer_socket);
	} else {
		return true;
	}
}

bool
io_adapter_socket_bind (
	io_socket_t *socket,io_address_t a,io_event_t *tx,io_event_t *rx
) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;

	this->transmit_available = tx;
	this->receive_data_available = rx;

	return io_socket_bind_to_outer_socket (socket,this->outer_socket);
}

void
io_adapter_socket_unbind_inner (io_socket_t *socket,io_address_t address) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;

	io_dequeue_event (io_socket_io (socket),this->transmit_available);
	io_dequeue_event (io_socket_io (socket),this->receive_data_available);

	this->transmit_available = NULL;
	this->receive_data_available = NULL;

	if (this->outer_socket) {
		io_socket_bind_to_outer_socket (socket,this->outer_socket);
	}
}

bool
io_adapter_socket_bind_to_outer (io_socket_t *socket,io_socket_t *outer) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;

	this->outer_socket = outer;

	io_socket_bind_inner (
		outer,
		io_socket_address (socket),
		this->transmit_available,
		this->receive_data_available
	);
	
	return true;
}

io_encoding_t*
io_adapter_socket_new_message (io_socket_t *socket) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	if (this->outer_socket != NULL) {
		io_encoding_t *message = reference_io_encoding (
			io_socket_new_message (this->outer_socket)
		);
		io_layer_t *inner = io_encoding_get_innermost_layer (message);
		if (inner) {
			io_layer_set_destination_address (inner,message,io_socket_address (socket));
		}
		return message;
	} else {
		return NULL;
	}
}

bool
io_adapter_socket_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	bool ok = false;

	if (this->outer_socket != NULL) {
		ok = io_socket_send_message (this->outer_socket,encoding);
	}
	
	unreference_io_encoding (encoding);
	return ok;
}

io_pipe_t*
io_adapter_socket_get_receive_pipe (io_socket_t *socket,io_address_t address) {
	io_adapter_socket_t *this = (io_adapter_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_get_receive_pipe (this->outer_socket,address);
	} else {
		return NULL;
	}
}

size_t
io_adapter_socket_mtu (io_socket_t const *socket) {
	io_adapter_socket_t const *this = (io_adapter_socket_t const*) socket;
	if (this->outer_socket) {
		return io_socket_mtu (this->outer_socket);
	} else {
		return 0;
	}
}

EVENT_DATA io_socket_implementation_t io_adapter_socket_implementation = {
	SPECIALISE_IO_ADAPTER_SOCKET_IMPLEMENTATION (
		&io_counted_socket_implementation
	)
};

//
//
//

io_inner_constructor_bindings_t*
mk_io_inner_constructor_bindings (io_t *io) {
	io_inner_constructor_bindings_t *this = io_byte_memory_allocate (
		io_get_byte_memory(io),sizeof(io_inner_constructor_bindings_t)
	);
	
	if (this) {
		this->begin = NULL;
		this->end = NULL;
	}
	
	return this;
}

void
free_io_inner_constructor_bindings (
	io_inner_constructor_bindings_t *bindings,io_t *io
) {
	io_inner_constructor_binding_t *cursor = bindings->begin;
	io_byte_memory_t *bm = io_get_byte_memory (io);
	while (cursor < bindings->end) {
		free_io_address (bm,cursor->address);
		io_dequeue_event (io,(io_event_t*) cursor->notify);
		cursor ++;
	}
	io_byte_memory_free (bm,bindings->begin);
	io_byte_memory_free (bm,bindings);
}

io_inner_constructor_binding_t*
io_inner_constructor_bindings_find_binding (
	io_inner_constructor_bindings_t *bindings,io_address_t address
) {
	io_inner_constructor_binding_t *cursor = bindings->begin;
	while (cursor < bindings->end) {
		if (compare_io_addresses (cursor->address,address) == 0) {
			return cursor;
		}
		cursor ++;
	}

	return NULL;
}

bool
io_inner_constructor_bindings_bind (
	io_inner_constructor_bindings_t *this,
	io_t *io,
	io_address_t address,
	io_socket_constructor_t make,
	io_notify_event_t *notify
) {
	io_inner_constructor_binding_t *inner = io_inner_constructor_bindings_find_binding (
		this,address
	);

	if (inner == NULL) {
		uint32_t size = (this->end - this->begin);
		io_inner_constructor_binding_t *bigger = io_byte_memory_reallocate (
			io_get_byte_memory (io),
			this->begin,
			sizeof (io_inner_constructor_binding_t) * (size + 1)
		);
		if (bigger != NULL) {
			this->begin = bigger;
			this->end = bigger + (size + 1);
			inner = this->begin + size;
			inner->address = duplicate_io_address (io_get_byte_memory (io),address);
			inner->make = NULL;
			inner->notify = NULL;
		}
	}
	
	if (inner != NULL) {
		io_dequeue_event (io,(io_event_t*) inner->notify);
		inner->notify = notify;
		inner->make = make;
		return true;
	} else {
		return false;
	}
}

//
// multiplex socket
//

io_inner_port_t*
mk_io_inner_port (io_byte_memory_t *bm,uint16_t tx_length,uint16_t rx_length) {
	io_inner_port_t *this = io_byte_memory_allocate (bm,sizeof(io_inner_port_t));

	if (this) {
		this->tx_available = NULL;
		this->rx_available = NULL;
		this->transmit_pipe = mk_io_encoding_pipe (bm,tx_length);
		if (this->transmit_pipe == NULL) {
			goto nope;
		}
		this->receive_pipe = mk_io_encoding_pipe (bm,rx_length);
		if (this->receive_pipe == NULL) {
			goto nope;
		}
	}
	
	return this;
	
nope:
	io_byte_memory_free(bm,this);
	return NULL;
}

void
free_io_inner_port (io_byte_memory_t *bm,io_inner_port_t *this) {
	free_io_encoding_pipe (this->transmit_pipe,bm);
	free_io_encoding_pipe (this->receive_pipe,bm);
	io_byte_memory_free (bm,this);
}

io_socket_t*
allocate_io_multiplex_socket (io_t *io,io_address_t address) {
	io_multiplex_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_multiplex_socket_t)
	);
	socket->implementation = &io_multiplex_socket_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

io_socket_t*
initialise_io_multiplex_socket (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	initialise_io_counted_socket ((io_counted_socket_t*) this,io);

	this->slots = NULL;
	this->number_of_slots = 0;
	this->transmit_cursor = this->slots;
	this->transmit_pipe_length = io_settings_transmit_pipe_length(C);
	this->receive_pipe_length = io_settings_receive_pipe_length(C);
	this->inner_constructors = mk_io_inner_constructor_bindings(io);
	
	return socket;
}

void
io_multiplex_socket_free (io_socket_t *socket) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_byte_memory_t *bm = io_get_byte_memory (io_socket_io (this));

	{
		io_inner_binding_t *cursor = this->slots;
		io_inner_binding_t *end = cursor + this->number_of_slots;
		while (cursor < end) {
			free_io_address (bm,cursor->address);
			if (cursor->port != NULL) {
				free_io_inner_port (bm,cursor->port);
			}
			cursor++;
		}
		io_byte_memory_free (bm,this->slots);
		this->slots = NULL,
		this->number_of_slots = 0;
		this->transmit_cursor = this->slots;
	}
	
	free_io_inner_constructor_bindings (this->inner_constructors,io_socket_io (this));

	io_counted_socket_free (socket);
}

io_inner_binding_t*
io_multiplex_socket_find_inner_binding (
	io_multiplex_socket_t *this,io_address_t sa
) {
	io_inner_binding_t *remote = NULL;
	io_inner_binding_t *cursor = this->slots;
	io_inner_binding_t *end = cursor + this->number_of_slots;
	
	while (cursor < end) {
		if (compare_io_addresses (cursor->address,sa) == 0) {
			remote = cursor;
			break;
		}
		cursor++;
	}
	
	return remote;
}

io_pipe_t*
io_multiplex_socket_get_receive_pipe (io_socket_t *socket,io_address_t address) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_pipe_t *rx = NULL;
	io_inner_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
	if (inner != NULL) {
		rx = (io_pipe_t*) inner->port->receive_pipe;
	}
	return rx;
}

io_inner_binding_t*
io_multiplex_socket_get_free_binding (io_multiplex_socket_t *this) {
	io_inner_binding_t *cursor = this->slots;
	io_inner_binding_t *end = cursor + this->number_of_slots;
	
	while (cursor < end) {
		if (io_address_is_invalid (cursor->address)) {
			return cursor;
		}
		cursor++;
	}

	return NULL;
}

bool
io_multiplex_socket_bind_inner (
	io_socket_t *socket,io_address_t address,io_event_t *tx,io_event_t *rx
) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_inner_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
	
	if (inner == NULL) {
		inner = io_multiplex_socket_get_free_binding (this);
	}
	
	if (inner == NULL) {
		io_byte_memory_t *bm = io_get_byte_memory (io_socket_io (socket));
		io_inner_port_t *p = mk_io_inner_port (
			bm,
			this->transmit_pipe_length,
			this->receive_pipe_length
		);
		if (p != NULL) {
			io_inner_binding_t *more = io_byte_memory_reallocate (
				bm,this->slots,sizeof(io_inner_binding_t) * (this->number_of_slots + 1)
			);
			if (more != NULL) {
				this->transmit_cursor = (more + (this->transmit_cursor - this->slots));
				this->slots = more;
				this->slots[this->number_of_slots] = (io_inner_binding_t) {
					duplicate_io_address (bm,address),p
				};
				inner = this->slots + this->number_of_slots;
				this->number_of_slots++;
			} else {
				io_panic (io_socket_io (this),IO_PANIC_OUT_OF_MEMORY);
			}
		} else {
			io_panic (io_socket_io (this),IO_PANIC_OUT_OF_MEMORY);
		}
	}

	{
		io_inner_port_t *port = inner->port;
		if(port->tx_available != NULL) {
			io_dequeue_event (io_socket_io (socket),port->tx_available);
		}
		if (port->rx_available != NULL) {
			io_dequeue_event (io_socket_io (socket),port->rx_available);
		}
		port->tx_available = tx;
		port->rx_available = rx;
		reset_io_encoding_pipe (port->transmit_pipe);
		reset_io_encoding_pipe (port->receive_pipe);
	}
	
	return true;
}

io_inner_constructor_binding_t*
io_multiplex_socket_find_inner_constructor_binding (
	io_multiplex_socket_t *this,io_address_t address
) {
	return io_inner_constructor_bindings_find_binding (
		this->inner_constructors,address
	);
}

bool
io_multiplex_socket_bind_inner_constructor (
	io_socket_t *socket,io_address_t address,io_socket_constructor_t make,io_notify_event_t *notify
) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	return io_inner_constructor_bindings_bind (
		this->inner_constructors,io_socket_io (this),address,make,notify
	);
}

void
io_multiplex_socket_unbind_inner (io_socket_t *socket,io_address_t address) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_inner_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
	if (inner != NULL) {
		io_inner_port_t *port = inner->port;
		io_dequeue_event (io_socket_io (socket),port->tx_available);
		io_dequeue_event (io_socket_io (socket),port->rx_available);
		port->tx_available = NULL;
		port->rx_available = NULL;
		assign_io_address (
			io_socket_byte_memory (socket),&inner->address,io_invalid_address()
		);
	}
}

INLINE_FUNCTION void
io_multiplex_socket_increment_transmit_cursor (io_multiplex_socket_t *this) {
	this->transmit_cursor ++;
	if ( (this->transmit_cursor - this->slots) == this->number_of_slots) {
		this->transmit_cursor = this->slots;
	}
}

io_inner_binding_t*
io_multiplex_socket_get_next_transmit_binding (io_multiplex_socket_t *this) {	
	if (io_multiplex_socket_has_inner_bindings (this)) {
		io_inner_binding_t *at = this->transmit_cursor;
		do {
			io_multiplex_socket_increment_transmit_cursor (this);
			if (
				io_encoding_pipe_is_readable (
					this->transmit_cursor->port->transmit_pipe
				)
			) {
				return this->transmit_cursor;
			}

		} while (this->transmit_cursor != at);
	}
	
	return NULL;
}

void
io_multiplex_socket_round_robin_signal_transmit_available (io_multiplex_socket_t *this) {
	io_inner_binding_t *at = this->transmit_cursor;
	
	do {
		io_multiplex_socket_increment_transmit_cursor (this);
		
		io_event_t *ev = this->transmit_cursor->port->tx_available;
		if (ev) {
			//
			// really want to know if this binding wants the tx; if not
			// we can offer to next binding
			//
			io_enqueue_event (io_socket_io (this),ev);
			break;
		}
		
	} while (this->transmit_cursor != at);
}


EVENT_DATA io_socket_implementation_t io_multiplex_socket_implementation = {
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION (&io_counted_socket_implementation)
};

//
// multiplexer socket
//

io_socket_t*
allocate_io_multiplexer_socket (io_t *io,io_address_t address) {
	io_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_multiplexer_socket_t)
	);
	socket->implementation = &io_multiplexer_socket_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return socket;
}

io_socket_t*
initialise_io_multiplexer_socket (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_multiplexer_socket_t *this = (io_multiplexer_socket_t*) socket;
	initialise_io_multiplex_socket (socket,io,C);
	this->outer_socket = NULL;
	return socket;
}

void
io_multiplexer_socket_free (io_socket_t *socket) {
	io_multiplex_socket_free (socket);
}

void
close_io_multiplexer_socket (io_multiplexer_socket_t *this) {
	io_dequeue_event (io_socket_io (this),&this->transmit_event);
	io_dequeue_event (io_socket_io (this),&this->receive_event);
}

bool
io_multiplexer_socket_bind_to_outer (io_socket_t *socket,io_socket_t *outer) {
	io_multiplexer_socket_t *this = (io_multiplexer_socket_t*) socket;

	this->outer_socket = outer;

	io_socket_bind_inner (
		outer,
		io_socket_address(socket),
		&this->transmit_event,
		&this->receive_event
	);
	
	return true;
}

bool
io_multiplexer_socket_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_multiplexer_socket_t *this = (io_multiplexer_socket_t*) socket;
	bool ok = false;

	if (this->outer_socket != NULL) {
		ok = io_socket_send_message (this->outer_socket,encoding);
		
		if (!ok) {
			// try to queue, but this socket needs a reference to its layer implementation
			// (or a bool (*) (io_encoding_t*) pointer) so we can get binding address from
			// encoding
		}
	}
	
	unreference_io_encoding (encoding);
	return ok;
}

size_t
io_multiplexer_socket_mtu (io_socket_t const *socket) {
	io_multiplexer_socket_t const *this = (io_multiplexer_socket_t const*) socket;
	if (this->outer_socket) {
		return io_socket_mtu (this->outer_socket);
	} else {
		return 0;
	}
}

void	
io_multiplexer_socket_outer_receive_event (io_event_t *ev) {
	io_multiplexer_socket_t *this = ev->user_value;
	
	if (this->outer_socket) {
		io_encoding_pipe_t* rx = cast_to_io_encoding_pipe (
			io_socket_get_receive_pipe (
				this->outer_socket,io_socket_address (this)
			)
		);
		if (rx) {
			io_encoding_t *next;
			if (io_encoding_pipe_peek (rx,&next)) {
				io_layer_t *base = io_encoding_get_outermost_layer (next);
				if (base) {
					io_inner_binding_t *inner = io_layer_select_inner_binding (
						base,next,(io_socket_t*) this
					);
					if (inner) {
						if (io_encoding_pipe_put_encoding (inner->port->receive_pipe,next)) {
							if (inner->port->rx_available) {
								io_enqueue_event (io_socket_io (this),inner->port->rx_available);
							}
						}
					}
				}
				io_encoding_pipe_pop_encoding (rx);
			}
		}
	}
}

EVENT_DATA io_socket_implementation_t io_multiplexer_socket_implementation = {
	SPECIALISE_IO_MULTIPLEXER_SOCKET_IMPLEMENTATION (&io_multiplex_socket_implementation)
};

//
// socket emulator
//
static void	
io_socket_emulator_tx_event (io_event_t *ev) {
}

io_socket_t*
io_socket_emulator_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_socket_emulator_t *this = (io_socket_emulator_t*) socket;

	initialise_io_multiplexer_socket (socket,io,C);

	initialise_io_event (
		&this->tx,io_socket_emulator_tx_event,this
	);
	
	initialise_io_event (
		&this->rx,io_multiplexer_socket_outer_receive_event,this
	);

	return socket;
}

void
io_socket_emulator_free (io_socket_t *socket) {
	io_multiplexer_socket_free (socket);
}

bool
io_socket_emulator_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	// always open?
	return true;
}

void
io_socket_emulator_close (io_socket_t *socket) {
}

bool
io_socket_emulator_is_closed (io_socket_t const *socket) {
	return false;
}

bool
io_socket_emulator_bind_to_outer_socket (io_socket_t *socket,io_socket_t *outer) {
	io_socket_emulator_t *this = (io_socket_emulator_t*) socket;
	this->outer_socket = outer;
	return io_socket_bind_inner (
		this->outer_socket,io_socket_address(socket),&this->tx,&this->rx
	);
}

size_t
io_socket_emulator_mtu (io_socket_t const *socket) {
	return 0;
}

EVENT_DATA io_socket_implementation_t io_socket_emulator_implementation = {
	SPECIALISE_IO_SOCKET_EMULATOR_IMPLEMENTATION (
		&io_multiplexer_socket_implementation
	)
};

//
// emulator with binary frame
//
static io_encoding_t*
io_socket_emulator_new_binary_message (io_socket_t *socket) {
	io_encoding_t *message = reference_io_encoding (
		mk_io_packet_encoding (io_get_byte_memory(io_socket_io (socket)))
	);
	if (message) {
		io_layer_t *layer = push_io_binary_transmit_layer (message);
		if (layer) {
			io_layer_set_source_address (layer,message,io_socket_address(socket));
		} else {
			io_panic (io_socket_io(socket),IO_PANIC_OUT_OF_MEMORY);
		}
	}
	
	return message;
}

static bool
io_socket_emulator_send_binary_message (
	io_socket_t *socket,io_encoding_t *encoding
) {
	io_layer_t *layer = get_io_binary_layer (encoding);
	if (layer) {
		io_layer_load_header (layer,encoding);
		return io_multiplexer_socket_send_message (socket,encoding);
	} else {
		unreference_io_encoding (encoding);
		return false;
	}
}

EVENT_DATA io_socket_implementation_t 
io_socket_binary_emulator_implementation = {
	SPECIALISE_IO_SOCKET_EMULATOR_IMPLEMENTATION (
		&io_socket_emulator_implementation
	)
	.new_message = io_socket_emulator_new_binary_message,
	.send_message = io_socket_emulator_send_binary_message,
};

io_socket_t*
allocate_io_socket_binary_emulator (io_t *io,io_address_t address) {
	io_socket_emulator_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_socket_emulator_t)
	);
	socket->implementation = &io_socket_binary_emulator_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

//
// emulator with link frame
//
static io_encoding_t*
io_socket_emulator_new_link_message (io_socket_t *socket) {
	io_encoding_t *message = reference_io_encoding (
		mk_io_packet_encoding (io_get_byte_memory(io_socket_io (socket)))
	);
	if (message) {
		io_layer_t *layer = push_io_link_transmit_layer (message);
		if (layer) {
			io_layer_set_source_address (layer,message,io_socket_address(socket));
		} else {
			io_panic (io_socket_io(socket),IO_PANIC_OUT_OF_MEMORY);
		}
	}
	
	return message;
}

static bool
io_socket_emulator_send_link_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_layer_t *layer = get_io_link_layer (encoding);
	if (layer) {
		io_layer_load_header (layer,encoding);
		return io_multiplexer_socket_send_message (socket,encoding);
	} else {
		unreference_io_encoding (encoding);
		return false;
	}
}

EVENT_DATA io_socket_implementation_t io_socket_link_emulator_implementation = {
	SPECIALISE_IO_SOCKET_EMULATOR_IMPLEMENTATION (
		&io_socket_emulator_implementation
	)
	.new_message = io_socket_emulator_new_link_message,
	.send_message = io_socket_emulator_send_link_message,
};

io_socket_t*
allocate_io_socket_link_emulator (io_t *io,io_address_t address) {
	io_socket_emulator_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_socket_emulator_t)
	);
	socket->implementation = &io_socket_link_emulator_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

//
// media
//
static io_socket_t*
io_shared_media_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	return initialise_io_multiplex_socket (socket,io,C);
}

static void
io_shared_media_free (io_socket_t *socket) {
	io_multiplex_socket_free (socket);
}

static bool
io_shared_media_open (io_socket_t *socket,io_socket_open_flag_t flag) {
	return true;
}

static void
io_shared_media_close (io_socket_t *socket) {
}

static bool
io_shared_media_is_closed (io_socket_t const *socket) {
	return false;
}

static bool
io_shared_media_bind_to_outer_socket (io_socket_t *socket,io_socket_t *outer) {
	return false;
}

static io_encoding_t*
io_shared_media_new_message (io_socket_t *socket) {
	return NULL;
}


io_encoding_t*
io_packet_encoding_copy (io_encoding_t *source_encoding) {
	io_packet_encoding_t *src = (io_packet_encoding_t*) source_encoding;
	
	io_encoding_t *copy = source_encoding->implementation->make_encoding(src->bm);
	io_encoding_append_bytes (
		copy,
		io_encoding_get_byte_stream (source_encoding),
		io_encoding_length(source_encoding)
	);
	
	return copy;
}

static io_encoding_t*
io_shared_media_make_receive_copy (io_packet_encoding_t *source_encoding) {
	io_layer_t *base = io_encoding_get_outermost_layer ((io_encoding_t*) source_encoding);
	if (base) {
		io_encoding_t *copy = io_packet_encoding_copy ((io_encoding_t*) source_encoding);
		io_layer_push_receive_layer (base,copy);
		return copy;
	} else {
		return NULL;
	}
}

//
// forward to all other inner bindings
//
static bool
io_shared_media_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_layer_t *layer = io_encoding_get_outermost_layer (encoding);
	if (layer != NULL) {
		io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
		io_address_t src = io_layer_get_source_address (layer,encoding);
		io_inner_binding_t *cursor = this->slots;
		io_inner_binding_t *end = cursor + this->number_of_slots;
		io_encoding_t *receive_message = NULL;
		
		if (cursor != end) {
			//
			receive_message = io_shared_media_make_receive_copy ((io_packet_encoding_t*) encoding);
			reference_io_encoding (receive_message);
		}

		while (cursor < end) {
			if (
					compare_io_addresses (cursor->address,src) != 0
				&&	io_layer_match_address (layer,cursor->address)
			) {
				
				if (cursor->port->rx_available) {
					io_encoding_pipe_put_encoding (cursor->port->receive_pipe,receive_message);
					io_enqueue_event (io_socket_io (socket),cursor->port->rx_available);	
				}
			}
			cursor++;
		}
 
		if (receive_message != NULL) {
			unreference_io_encoding (receive_message);
		}
	}
	
	return true;
}

static size_t
io_shared_media_mtu (io_socket_t const *socket) {
	//
	// minimum mtu of all attached sockets
	//
	return 0;
}

//
// need a socket to splice stacks
//
EVENT_DATA io_socket_implementation_t io_shared_media_implementation = {
	SPECIALISE_IO_MULTIPLEX_SOCKET_IMPLEMENTATION(&io_multiplex_socket_implementation)
	.initialise = io_shared_media_initialise,
	.free = io_shared_media_free,
	.open = io_shared_media_open,
	.close = io_shared_media_close,
	.is_closed = io_shared_media_is_closed,
	.bind_to_outer_socket = io_shared_media_bind_to_outer_socket,
	.new_message = io_shared_media_new_message,
	.send_message = io_shared_media_send_message,
	.mtu = io_shared_media_mtu,
};

io_socket_t*
allocate_io_shared_media (io_t *io,io_address_t address) {
	io_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_shared_media_t)
	);
	socket->implementation = &io_shared_media_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return socket;
}

//
//
//
void
build_io_sockets (
	io_t *io,io_socket_t **array,socket_builder_t const *construct,uint32_t length
) {
	socket_builder_t const *end = construct + length;
	socket_builder_t const *build;
	build = construct;
	while (build < end) {
		array[build->index] = reference_io_socket (io_socket_initialise (
				build->allocate(io,build->address),io,build->C
			)
		);
		build++;
	}

	build = construct;
	while (build < end) {
		if (build->inner_bindings) {
			socket_builder_binding_t const *link = build->inner_bindings;
			while (link->inner != INVALID_SOCKET_ID) {
				io_socket_bind_to_outer_socket (
					array[link->inner],array[link->outer]
				);
				link++;
			}
		}
		build++;
	}

	build = construct;
	while (build < end) {
		if (build->with_open) {
			// io_socket_open (array[build->index],IO_SOCKET_OPEN_CONNECT);
			call_io_socket_state_open (array[build->index],IO_SOCKET_OPEN_CONNECT);
		}
		build++;
	}
}

#endif /* IMPLEMENT_IO_CORE */
#endif
/*
------------------------------------------------------------------------------
This software is available under 2 licenses -- choose whichever you prefer.
------------------------------------------------------------------------------
ALTERNATIVE A - MIT License
Copyright (c) 2020 Gregor Bruce
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
------------------------------------------------------------------------------
ALTERNATIVE B - Public Domain (www.unlicense.org)
This is free and unencumbered software released into the public domain.
Anyone is free to copy, modify, publish, use, compile, sell, or distribute this
software, either in source code form or as a compiled binary, for any purpose,
commercial or non-commercial, and by any means.
In jurisdictions that recognize copyright laws, the author or authors of this
software dedicate any and all copyright interest in the software to the public
domain. We make this dedication for the benefit of the public at large and to
the detriment of our heirs and successors. We intend this dedication to be an
overt act of relinquishment in perpetuity of all present and future rights to
this software under copyright law.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------
*/
