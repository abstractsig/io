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
typedef struct io_inner_port_binding io_inner_port_binding_t;
typedef struct io_multiplex_socket io_multiplex_socket_t;
io_inner_port_binding_t*	io_multiplex_socket_find_inner_binding (io_multiplex_socket_t*,io_address_t);

#include <io_layers.h>

typedef struct io_socket_implementation io_socket_implementation_t;
typedef bool (*io_socket_iterator_t) (io_socket_t*,void*);
typedef bool (*io_socket_constructor_t) (io_t*,io_address_t,io_socket_t**,io_socket_t**);

typedef struct PACK_STRUCTURE io_notify_event {
	IO_EVENT_STRUCT_MEMBERS
	io_socket_t *socket;
} io_notify_event_t;

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
	bool (*open) (io_socket_t*);\
	void (*close) (io_socket_t*);\
	bool (*is_closed) (io_socket_t const*);\
	bool (*bind_inner) (io_socket_t*,io_address_t,io_event_t*,io_event_t*);\
	bool (*bind_inner_constructor) (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);\
	void (*unbind_inner) (io_socket_t*,io_address_t);\
	bool (*bind_to_outer_socket) (io_socket_t*,io_socket_t*);\
	io_pipe_t* (*get_receive_pipe) (io_socket_t*,io_address_t);\
	io_encoding_t*	(*new_message) (io_socket_t*); \
	bool (*send_message) (io_socket_t*,io_encoding_t*);\
	bool (*iterate_inner_sockets) (io_socket_t*,io_socket_iterator_t,void*);\
	bool (*iterate_outer_sockets) (io_socket_t*,io_socket_iterator_t,void*);\
	size_t (*mtu) (io_socket_t const*);\
	/**/

struct PACK_STRUCTURE io_socket_implementation {
	IO_SOCKET_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_SOCKET_STRUCT_MEMBERS \
	io_socket_implementation_t const *implementation;\
	io_address_t address;\
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
INLINE_FUNCTION io_socket_t*
io_socket_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	return socket->implementation->initialise(socket,io,C);
}

INLINE_FUNCTION io_socket_t*
io_socket_increment_reference (io_socket_t *socket,int32_t incr) {
	return socket->implementation->reference(socket,incr);
}

#define io_socket_reference(s)	io_socket_increment_reference(s,1)
#define io_socket_unreference(s)	io_socket_increment_reference(s,-11)

INLINE_FUNCTION void
io_socket_free (io_socket_t *socket) {
	socket->implementation->free (socket);
}

INLINE_FUNCTION bool
io_socket_open (io_socket_t *socket) {
	return socket->implementation->open (socket);
}

INLINE_FUNCTION void
io_socket_close (io_socket_t *socket) {
	socket->implementation->close (socket);
}

INLINE_FUNCTION bool
io_socket_is_closed (io_socket_t const *socket) {
	return socket->implementation->is_closed (socket);
}

INLINE_FUNCTION io_encoding_t*
io_socket_new_message (io_socket_t *socket) {
	return socket->implementation->new_message (socket);
}

INLINE_FUNCTION bool
io_socket_send_message (io_socket_t *socket,io_encoding_t *m) {
	return socket->implementation->send_message (socket,m);
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
bool				io_virtual_socket_open (io_socket_t*);
void				io_virtual_socket_close (io_socket_t*);
bool				io_virtual_socket_is_closed (io_socket_t const*);
bool				io_virtual_socket_bind_inner (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
bool				io_virtual_socket_bind_inner_constructor (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);
void				io_virtual_socket_unbind_inner (io_socket_t*,io_address_t);
bool				io_virtual_socket_bind_to_outer_socket (io_socket_t*,io_socket_t*);
io_encoding_t*	io_virtual_socket_new_message (io_socket_t*);
bool				io_virtual_socket_send_message (io_socket_t*,io_encoding_t*);
size_t			io_virtual_socket_mtu (io_socket_t const*);

#define SPECIALISE_IO_VIRTUAL_SOCKET(S) \
	.specialisation_of = S, \
	.reference = io_virtual_socket_increment_reference, \
	.initialise = io_virtual_socket_initialise, \
	.free = io_virtual_socket_free, \
	.open = io_virtual_socket_open, \
	.close = io_virtual_socket_close, \
	.is_closed = io_virtual_socket_is_closed, \
	.bind_to_outer_socket = NULL, \
	.bind_inner = io_virtual_socket_bind_inner, \
	.bind_inner_constructor = io_virtual_socket_bind_inner_constructor,\
	.unbind_inner = io_virtual_socket_unbind_inner,\
	.bind_to_outer_socket = io_virtual_socket_bind_to_outer_socket,\
	.new_message = io_virtual_socket_new_message, \
	.send_message = io_virtual_socket_send_message, \
	.iterate_inner_sockets = NULL, \
	.iterate_outer_sockets = NULL, \
	.mtu = io_virtual_socket_mtu, \
	/**/

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
// leaf socket
//
#define IO_LEAF_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_event_t *transmit_available; \
	io_event_t *receive_data_available; \
	io_socket_t *outer_socket; \
	/**/

typedef struct PACK_STRUCTURE io_leaf_socket {
	IO_LEAF_SOCKET_STRUCT_MEMBERS	
} io_leaf_socket_t;

io_socket_t* allocate_io_leaf_socket (io_t*,io_address_t);
void io_leaf_socket_free (io_socket_t*);

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

struct PACK_STRUCTURE io_inner_port_binding {
	io_address_t address;
	io_inner_port_t *port;
};

typedef struct PACK_STRUCTURE {
	io_address_t address;
	io_socket_constructor_t make;
	io_notify_event_t *notify;
} io_inner_constructor_binding_t;

typedef struct PACK_STRUCTURE {
	io_inner_constructor_binding_t *begin;
	io_inner_constructor_binding_t *end;
} io_inner_constructor_bindings_t;

#define IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	IO_COUNTED_SOCKET_STRUCT_MEMBERS \
	io_inner_port_binding_t *slots; \
	uint32_t number_of_slots; \
	io_inner_constructor_bindings_t inner_constructors;\
	io_inner_port_binding_t *transmit_cursor; \
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
io_inner_port_binding_t*	io_multiplex_socket_get_next_transmit_binding (io_multiplex_socket_t*);

extern EVENT_DATA io_socket_implementation_t io_multiplex_socket_implementation;

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
io_encoding_t*	io_multiplexer_socket_new_message (io_socket_t*);
bool				io_multiplexer_socket_send_message (io_socket_t*,io_encoding_t*);
size_t			io_multiplexer_socket_mtu (io_socket_t const*);

extern EVENT_DATA io_socket_implementation_t io_multiplexer_socket_implementation;


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

io_socket_t* allocate_io_socket_emulator (io_t*,io_address_t);
io_socket_t* io_socket_emulator_initialise (io_socket_t*,io_t*,io_settings_t const*);
void io_socket_emulator_free (io_socket_t*);
bool io_socket_emulator_open (io_socket_t*);
void io_socket_emulator_close (io_socket_t*);
bool io_socket_emulator_is_closed (io_socket_t const*);
bool io_socket_emulator_bind_to_outer_socket (io_socket_t*,io_socket_t*);
size_t io_socket_emulator_mtu (io_socket_t const*);

extern EVENT_DATA io_socket_implementation_t io_socket_emulator_implementation;

//
// specialise an emulator implementation with custom new/send 
// message methods to support different layer types
//
#define SPECIALISE_IO_SOCKET_EMULATOR(N,T) \
	.specialisation_of = &io_socket_emulator_implementation, \
	.initialise = io_socket_emulator_initialise, \
	.reference = io_counted_socket_increment_reference, \
	.free = io_socket_emulator_free, \
	.open = io_socket_emulator_open, \
	.close = io_socket_emulator_close, \
	.is_closed = io_socket_emulator_is_closed, \
	.bind_inner = io_multiplex_socket_bind_inner, \
	.bind_inner_constructor = io_virtual_socket_bind_inner_constructor,\
	.bind_to_outer_socket = io_socket_emulator_bind_to_outer_socket, \
	.new_message = N, \
	.send_message = T, \
	.get_receive_pipe = io_multiplex_socket_get_receive_pipe,\
	.iterate_inner_sockets = NULL, \
	.iterate_outer_sockets = NULL, \
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
		io_socket_free (*cursor++);
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

#define BINDINGS(...)		(const socket_builder_binding_t []) {__VA_ARGS__}
#define END_OF_BINDINGS		{INVALID_SOCKET_ID,INVALID_SOCKET_ID}

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
io_virtual_socket_open (io_socket_t *socket) {
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

size_t
io_virtual_socket_mtu (io_socket_t const *socket) {
	return 0;
}

EVENT_DATA io_socket_implementation_t io_socket_implementation_base = {
	SPECIALISE_IO_VIRTUAL_SOCKET (NULL)
};

EVENT_DATA io_socket_implementation_t io_physical_socket_implementation = {
	SPECIALISE_IO_VIRTUAL_SOCKET (&io_socket_implementation_base)
};

//
// socket base
// 
void
initialise_io_socket (io_socket_t *this,io_t *io) {
	this->io = io;
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

EVENT_DATA io_socket_implementation_t io_counted_socket_implementation = {
	SPECIALISE_IO_VIRTUAL_SOCKET (&io_socket_implementation_base)
};

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

//
// leaf sockets have 1:1 inner and outer relations
//

io_socket_t*
io_leaf_socket_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;

	initialise_io_socket (socket,io);
	this->outer_socket = NULL;
	
	this->transmit_available = NULL;
	this->receive_data_available = NULL;
	
	return socket;
}

io_socket_t*
allocate_io_leaf_socket (io_t *io,io_address_t address) {
	io_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_leaf_socket_t)
	);
	socket->implementation = &io_leaf_socket_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

void
io_leaf_socket_free (io_socket_t *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;

	io_dequeue_event (io_socket_io (socket),this->transmit_available);
	io_dequeue_event (io_socket_io (socket),this->receive_data_available);
	
	io_counted_socket_free (socket);
}

bool
io_leaf_socket_open (io_socket_t *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_open (this->outer_socket);
	} else {
		return false;
	}
}

static void
io_leaf_socket_close (io_socket_t *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	if (this->outer_socket != NULL) {
		io_socket_unbind_inner (this->outer_socket,io_socket_address(socket));
	}
}

static bool
io_leaf_socket_is_closed (io_socket_t const *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_is_closed (this->outer_socket);
	} else {
		return false;
	}
}

static bool
io_leaf_socket_bind (
	io_socket_t *socket,io_address_t a,io_event_t *tx,io_event_t *rx
) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;

	this->transmit_available = tx;
	this->receive_data_available = rx;

	return io_socket_bind_to_outer_socket (socket,this->outer_socket);
}

static bool
io_leaf_socket_bind_to_outer (io_socket_t *socket,io_socket_t *outer) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;

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
io_leaf_socket_new_message (io_socket_t *socket) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	if (this->outer_socket != NULL) {
		io_encoding_t *message = reference_io_encoding (
			io_socket_new_message (this->outer_socket)
		);
		io_layer_t *inner = io_encoding_get_innermost_layer (message);
		if (inner) {
			//
			// Requires that the leaf addresses are setup to 
			//	crossover.
			//
			io_layer_set_remote_address (inner,message,io_socket_address (socket));
		}
		return message;
	} else {
		return NULL;
	}
}

bool
io_leaf_socket_send_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	bool ok = false;

	if (this->outer_socket != NULL) {
		ok = io_socket_send_message (this->outer_socket,encoding);
	}
	
	unreference_io_encoding (encoding);
	return ok;
}

io_pipe_t*
io_leaf_socket_get_receive_pipe (io_socket_t *socket,io_address_t address) {
	io_leaf_socket_t *this = (io_leaf_socket_t*) socket;
	if (this->outer_socket != NULL) {
		return io_socket_get_receive_pipe (this->outer_socket,address);
	} else {
		return NULL;
	}
}

static size_t
io_leaf_socket_mtu (io_socket_t const *socket) {
	io_leaf_socket_t const *this = (io_leaf_socket_t const*) socket;
	if (this->outer_socket) {
		return io_socket_mtu (this->outer_socket);
	} else {
		return 0;
	}
}

EVENT_DATA io_socket_implementation_t io_leaf_socket_implementation = {
	.specialisation_of = &io_counted_socket_implementation,
	.initialise = io_leaf_socket_initialise,
	.reference = io_counted_socket_increment_reference,
	.free = io_leaf_socket_free,
	.open = io_leaf_socket_open,
	.close = io_leaf_socket_close,
	.is_closed = io_leaf_socket_is_closed,
	.bind_to_outer_socket = io_leaf_socket_bind_to_outer,
	.bind_inner = io_leaf_socket_bind,
	.unbind_inner = io_virtual_socket_unbind_inner,
	.new_message = io_leaf_socket_new_message,
	.send_message = io_leaf_socket_send_message,
	.get_receive_pipe = io_leaf_socket_get_receive_pipe,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = io_leaf_socket_mtu,
};

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
	
	this->inner_constructors = (io_inner_constructor_bindings_t) {NULL,NULL};
	
	return socket;
}

void
io_multiplex_socket_free (io_socket_t *socket) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_byte_memory_t *bm = io_get_byte_memory (io_socket_io (this));

	{
		io_inner_port_binding_t *cursor = this->slots;
		io_inner_port_binding_t *end = cursor + this->number_of_slots;	
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
	
	{
		io_inner_constructor_binding_t *cursor = this->inner_constructors.begin;
		while (cursor < this->inner_constructors.end) {
			free_io_address (bm,cursor->address);
			cursor ++;
		}
		io_byte_memory_free (bm,this->inner_constructors.begin);
	}
	
	io_counted_socket_free (socket);
}

io_inner_port_binding_t*
io_multiplex_socket_find_inner_binding (io_multiplex_socket_t *this,io_address_t sa) {
	io_inner_port_binding_t *remote = NULL;
	io_inner_port_binding_t *cursor = this->slots;
	io_inner_port_binding_t *end = cursor + this->number_of_slots;
	
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
	io_inner_port_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
	if (inner != NULL) {
		rx = (io_pipe_t*) inner->port->receive_pipe;
	}
	return rx;
}

io_inner_port_binding_t*
io_multiplex_socket_get_free_binding (io_multiplex_socket_t *this) {
	io_inner_port_binding_t *cursor = this->slots;
	io_inner_port_binding_t *end = cursor + this->number_of_slots;
	
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
	io_inner_port_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
	
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
			io_inner_port_binding_t *more = io_byte_memory_reallocate (
				bm,this->slots,sizeof(io_inner_port_binding_t) * (this->number_of_slots + 1)
			);
			if (more != NULL) {
				this->transmit_cursor = (more + (this->transmit_cursor - this->slots));
				this->slots = more;
				this->slots[this->number_of_slots] = (io_inner_port_binding_t) {
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
		io_dequeue_event (io_socket_io (socket),port->tx_available);
		io_dequeue_event (io_socket_io (socket),port->rx_available);
		port->tx_available = tx;
		port->rx_available = rx;
		reset_io_encoding_pipe (port->transmit_pipe);
		reset_io_encoding_pipe (port->receive_pipe);
	}
	
	return true;
}

io_inner_constructor_binding_t*
io_multiplex_socket_find_inner_constructor (io_multiplex_socket_t *this,io_address_t address) {
	io_inner_constructor_binding_t *cursor = this->inner_constructors.begin;
	while (cursor < this->inner_constructors.end) {
		if (compare_io_addresses (cursor->address,address) == 0) {
			return cursor;
		}
		cursor ++;
	}

	return false;
}

bool
io_multiplex_socket_bind_inner_constructor (
	io_socket_t *socket,io_address_t address,io_socket_constructor_t make,io_notify_event_t *notify
) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_inner_constructor_binding_t *inner = io_multiplex_socket_find_inner_constructor (
		this,address
	);

	if (inner == NULL) {
		uint32_t size = (this->inner_constructors.end - this->inner_constructors.begin);
		io_inner_constructor_binding_t *bigger = io_byte_memory_reallocate (
			io_socket_byte_memory (this),
			this->inner_constructors.begin,
			sizeof (io_inner_constructor_binding_t) * (size + 1)
		);
		if (bigger != NULL) {
			this->inner_constructors.begin = bigger;
			this->inner_constructors.end = bigger + (size + 1);
			inner = this->inner_constructors.begin + size;
			assign_io_address (io_socket_byte_memory (this),&inner->address,address);
		}
	}
	
	if (inner != NULL) {
		io_dequeue_event (io_socket_io (this),(io_event_t*) inner->notify);
		inner->notify = notify;
		inner->make = make;
		return true;
	} else {
		return false;
	}
}

void
io_multiplex_socket_unbind_inner (io_socket_t *socket,io_address_t address) {
	io_multiplex_socket_t *this = (io_multiplex_socket_t*) socket;
	io_inner_port_binding_t *inner = io_multiplex_socket_find_inner_binding (this,address);
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

io_inner_port_binding_t*
io_multiplex_socket_get_next_transmit_binding (io_multiplex_socket_t *this) {	
	if (io_multiplex_socket_has_inner_bindings (this)) {
		io_inner_port_binding_t *at = this->transmit_cursor;
		do {
			if (
				io_encoding_pipe_is_readable (
					this->transmit_cursor->port->transmit_pipe
				)
			) {
				return this->transmit_cursor;
			} else {
				io_multiplex_socket_increment_transmit_cursor (this);
			}

		} while (this->transmit_cursor != at);
	}
	
	return NULL;
}

void
io_multiplex_socket_round_robin_signal_transmit_available (io_multiplex_socket_t *this) {
	io_inner_port_binding_t *at = this->transmit_cursor;
	
	do {
		io_multiplex_socket_increment_transmit_cursor (this);
		
		io_event_t *ev = this->transmit_cursor->port->tx_available;
		if (ev) {
			io_enqueue_event (io_socket_io (this),ev);
			break;
		}
		
	} while (this->transmit_cursor != at);
}

EVENT_DATA io_socket_implementation_t io_multiplex_socket_implementation = {
	.specialisation_of = &io_counted_socket_implementation,
	.reference = io_counted_socket_increment_reference,
	.initialise = initialise_io_multiplex_socket,
	.free = io_multiplex_socket_free,
	.open = io_virtual_socket_open,
	.close = io_virtual_socket_close,
	.is_closed = io_virtual_socket_is_closed,
	.bind_to_outer_socket = NULL,
	.bind_inner = io_multiplex_socket_bind_inner,
	.unbind_inner = io_multiplex_socket_unbind_inner,
	.bind_inner_constructor = io_multiplex_socket_bind_inner_constructor,
	.bind_to_outer_socket = io_virtual_socket_bind_to_outer_socket,
	.new_message = io_virtual_socket_new_message,
	.send_message = io_virtual_socket_send_message,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = io_virtual_socket_mtu,
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

EVENT_DATA io_socket_implementation_t io_multiplexer_socket_implementation = {
	.specialisation_of = &io_multiplex_socket_implementation,
	.reference = io_counted_socket_increment_reference,
	.initialise = initialise_io_multiplexer_socket,
	.free = io_multiplexer_socket_free,
	.open = io_virtual_socket_open,
	.close = io_virtual_socket_close,
	.is_closed = io_virtual_socket_is_closed,
	.bind_to_outer_socket = NULL,
	.bind_inner = io_multiplex_socket_bind_inner,
	.unbind_inner = io_multiplex_socket_unbind_inner,
	.bind_to_outer_socket = io_virtual_socket_bind_to_outer_socket,
	.new_message = io_virtual_socket_new_message,
	.send_message = io_virtual_socket_send_message,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = io_virtual_socket_mtu,
};

//
//
//

//
// socket emulator
//
static void	
io_socket_emulator_tx_event (io_event_t *ev) {
}

static void	
io_socket_emulator_rx_event (io_event_t *ev) {
	io_socket_emulator_t *this = ev->user_value;
	
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
					io_inner_port_binding_t *inner = io_layer_decode (
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
			}
		}
	}
}

io_socket_t*
allocate_io_socket_emulator (io_t *io,io_address_t address) {
	io_socket_emulator_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_socket_emulator_t)
	);
	socket->implementation = &io_socket_emulator_implementation;
	socket->address = duplicate_io_address (io_get_byte_memory (io),address);
	return (io_socket_t*) socket;
}

io_socket_t*
io_socket_emulator_initialise (io_socket_t *socket,io_t *io,io_settings_t const *C) {
	io_socket_emulator_t *this = (io_socket_emulator_t*) socket;

	initialise_io_multiplexer_socket (socket,io,C);

	initialise_io_event (
		&this->tx,io_socket_emulator_tx_event,this
	);
	
	initialise_io_event (
		&this->rx,io_socket_emulator_rx_event,this
	);

	return socket;
}

void
io_socket_emulator_free (io_socket_t *socket) {
	io_multiplex_socket_free (socket);
}

bool
io_socket_emulator_open (io_socket_t *socket) {
	return false;
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

static io_encoding_t*
io_socket_emulator_new_message (io_socket_t *socket) {
	io_encoding_t *message = reference_io_encoding (
		mk_io_packet_encoding (
			io_get_byte_memory(io_socket_io (socket))
		)
	);
	if (message) {
		io_layer_t *layer = push_io_binary_transmit_layer (message);
		if (layer) {
			io_layer_set_local_address (layer,message,io_socket_address(socket));
		} else {
			io_panic (io_socket_io(socket),IO_PANIC_OUT_OF_MEMORY);
		}
	}
	
	return message;
}

static bool
io_socket_emulator_send_binary_message (io_socket_t *socket,io_encoding_t *encoding) {
	io_binary_frame_t *packet = io_encoding_get_byte_stream (encoding);
	write_le_uint32 (packet->length,io_encoding_length (encoding));
	return io_multiplexer_socket_send_message (socket,encoding);
}

EVENT_DATA io_socket_implementation_t io_socket_emulator_implementation = {
	.specialisation_of = &io_multiplexer_socket_implementation,
	.initialise = io_socket_emulator_initialise,
	.reference = io_counted_socket_increment_reference,
	.free = io_socket_emulator_free,
	.open = io_socket_emulator_open,
	.close = io_socket_emulator_close,
	.is_closed = io_socket_emulator_is_closed,
	.bind_inner = io_multiplex_socket_bind_inner,
	.unbind_inner = io_multiplex_socket_unbind_inner,
	.bind_inner_constructor = io_virtual_socket_bind_inner_constructor,
	.bind_to_outer_socket = io_socket_emulator_bind_to_outer_socket,
	.new_message = io_socket_emulator_new_message,
	.send_message = io_socket_emulator_send_binary_message,
	.get_receive_pipe = io_multiplex_socket_get_receive_pipe,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = io_socket_emulator_mtu,
};


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
io_shared_media_open (io_socket_t *socket) {
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

static io_encoding_t*
make_reveive_copy (io_packet_encoding_t *source_encoding) {
	io_layer_t *base = io_encoding_get_outermost_layer ((io_encoding_t*) source_encoding);
	if (base) {
		io_encoding_t *copy = io_encoding_duplicate (
			(io_encoding_t*) source_encoding,source_encoding->bm
		);
		io_layer_t *rx_layer = io_layer_swap (base,copy);
		
		if (rx_layer) {
			io_encoding_reset (copy);
			io_encoding_append_bytes (
				copy,
				io_encoding_get_byte_stream ((io_encoding_t*) source_encoding),
				io_encoding_length((io_encoding_t*) source_encoding)
			);
			io_layer_set_local_address (
				rx_layer,
				copy,
				io_layer_get_remote_address(base,(io_encoding_t*) source_encoding)
			);
		}
		
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
		io_address_t src = io_layer_get_local_address (layer,encoding);
		io_inner_port_binding_t *cursor = this->slots;
		io_inner_port_binding_t *end = cursor + this->number_of_slots;
		io_encoding_t *receive_message = NULL;
		
		if (cursor != end) {
			//
			receive_message = make_reveive_copy ((io_packet_encoding_t*) encoding);
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
	.specialisation_of = &io_multiplex_socket_implementation,
	.initialise = io_shared_media_initialise,
	.reference = io_counted_socket_increment_reference,
	.free = io_shared_media_free,
	.open = io_shared_media_open,
	.close = io_shared_media_close,
	.is_closed = io_shared_media_is_closed,
	.bind_inner = io_multiplex_socket_bind_inner,
	.unbind_inner = io_multiplex_socket_unbind_inner,
	.bind_to_outer_socket = io_shared_media_bind_to_outer_socket,
	.new_message = io_shared_media_new_message,
	.send_message = io_shared_media_send_message,
	.get_receive_pipe = io_multiplex_socket_get_receive_pipe,
	.iterate_inner_sockets = NULL,
	.iterate_outer_sockets = NULL,
	.mtu = io_shared_media_mtu,
};

io_socket_t*
allocate_io_shared_media (io_t *io,io_address_t address) {
	io_socket_t *socket = io_byte_memory_allocate (
		io_get_byte_memory (io),sizeof(io_shared_media_t)
	);
	socket->implementation = &io_shared_media_implementation;
	assign_io_address (io_get_byte_memory (io),&socket->address,address);
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
		array[build->index] = io_socket_initialise (
			build->allocate(io,build->address),io,build->C
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
			io_socket_open (array[build->index]);
		}
		build++;
	}
}

#endif /* IMPLEMENT_IO_CORE */
#endif
