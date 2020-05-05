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
#ifndef io_layers_H_
#define io_layers_H_


INLINE_FUNCTION io_encoding_t*
io_encoding_dup (io_encoding_t *encoding,io_byte_memory_t *bm) {
	return encoding->implementation->make_encoding(bm);
}

#define def_io_layer_registered_address(b,c,d)		(io_address_t) {.tag.size = 4,.tag.is_volatile = 0,.value.m8 = {'i',b,c,d},}

//
// there has to be a central registery of protocol numbers
//
#define IO_NULL_LAYER_ID		def_io_layer_registered_address('0','0','0')
#define IO_DLC_LAYER_ID			def_io_layer_registered_address('D','L','C')
#define IO_X70_LAYER_ID			def_io_layer_registered_address('X','7','0')
#define IO_MTU_LAYER_ID			def_io_layer_registered_address('M','T','U')
#define IO_NIM_LAYER_ID			def_io_layer_registered_address('N','I','M')

#define NRF_RADIO_LAYER_ID		def_io_layer_registered_address('P','0','1')

//
// multiplex sockets connect to shared communication media
//

typedef struct io_inner_constructor {
	io_socket_constructor_t *make;
	io_notify_event_t *notify;
} io_inner_constructor_t;

typedef struct io_inner_port {
	io_encoding_pipe_t *transmit_pipe;
	io_encoding_pipe_t *receive_pipe;
	io_event_t *tx_available;
	io_event_t *rx_available;
} io_inner_port_t;

void free_io_inner_port (io_byte_memory_t*,io_inner_port_t*);

typedef struct PACK_STRUCTURE {
	io_address_t address;
	io_inner_port_t *port;
} io_inner_port_binding_t;

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
	
typedef struct PACK_STRUCTURE io_multiplex_socket {
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS
} io_multiplex_socket_t;

#define io_multiplex_socket_has_inner_bindings(s) 	((s)->slots != NULL)

io_socket_t*	allocate_io_multiplex_socket (io_t*,io_address_t);
io_socket_t*	initialise_io_multiplex_socket (io_socket_t*,io_t*,io_settings_t const*);
void				io_multiplex_socket_free (io_socket_t*);
bool				io_multiplex_socket_bind_inner (io_socket_t*,io_address_t,io_event_t*,io_event_t*);
bool 				io_multiplex_socket_bind_inner_constructor (io_socket_t*,io_address_t,io_socket_constructor_t,io_notify_event_t*);
void				io_multiplex_socket_unbind_inner (io_socket_t*,io_address_t);
void				io_multiplex_socket_round_robin_signal_transmit_available (io_multiplex_socket_t*);
io_pipe_t* 		io_multiplex_socket_get_receive_pipe (io_socket_t*,io_address_t);
io_inner_port_binding_t*	io_multiplex_socket_find_inner_binding (io_multiplex_socket_t*,io_address_t);
io_inner_port_binding_t*	io_multiplex_socket_get_next_transmit_binding (io_multiplex_socket_t*);

extern EVENT_DATA io_socket_implementation_t io_multiplex_socket_implementation;

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

#define IO_DIRECTING_SOCKET_STRUCT_MEMBERS \
	IO_MULTIPLEX_SOCKET_STRUCT_MEMBERS \
	io_outer_bindings_t *bindings;\
	/**/

typedef struct PACK_STRUCTURE io_directing_socket {
	IO_DIRECTING_SOCKET_STRUCT_MEMBERS
} io_directing_socket_t;

//
// layer metadata
//
typedef struct io_packet_encoding io_packet_encoding_t;

#define IO_LAYER_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_layer_implementation_t const *specialisation_of; \
	io_layer_t* (*make) (io_packet_encoding_t*);\
	io_layer_t* (*swap) (io_layer_t*,io_encoding_t*);\
	io_inner_port_t* (*decode) (io_layer_t*,io_encoding_t*,io_multiplex_socket_t*);\
	io_address_t (*any) (void);\
	void (*free) (io_layer_t*,io_byte_memory_t*);\
	bool (*match_address) (io_layer_t*,io_address_t);\
	void (*get_content) (io_layer_t*,io_encoding_t*,uint8_t const**,uint8_t const**);\
	io_address_t (*get_remote_address) (io_layer_t*,io_encoding_t*);\
	bool (*set_remote_address) (io_layer_t*,io_encoding_t*,io_address_t);\
	io_address_t (*get_local_address) (io_layer_t*,io_encoding_t*);\
	bool (*set_local_address) (io_layer_t*,io_encoding_t*,io_address_t);\
	io_address_t (*get_inner_address) (io_layer_t*,io_encoding_t*);\
	bool (*set_inner_address) (io_layer_t*,io_encoding_t*,io_address_t);\
	/**/

struct PACK_STRUCTURE io_layer_implementation {
	IO_LAYER_IMPLEMENTATION_STRUCT_PROPERTIES
};

#define IO_LAYER_STRUCT_PROPERTIES \
	io_layer_implementation_t const *implementation; \
	uint32_t layer_offset_in_byte_stream;\
	/**/

struct PACK_STRUCTURE io_layer {
	IO_LAYER_STRUCT_PROPERTIES
};

#define io_layer_get_byte_stream(m,e)	(io_encoding_get_byte_stream(e) + (m)->layer_offset_in_byte_stream)
bool	io_layer_has_implementation (io_layer_t const*,io_layer_implementation_t const*);

//
// inline io_value reference methods
//
INLINE_FUNCTION io_layer_t*
make_io_layer (
	io_layer_implementation_t const *I,io_packet_encoding_t *packet
) {
	return I->make(packet);
}

INLINE_FUNCTION void
free_io_layer (io_layer_t *layer,io_byte_memory_t *bm) {
	layer->implementation->free(layer,bm);
}

INLINE_FUNCTION io_layer_t*
io_layer_swap (io_layer_t *layer,io_encoding_t *encoding) {
	return layer->implementation->swap(layer,encoding);
}

INLINE_FUNCTION io_inner_port_t*
io_layer_decode (io_layer_t *layer,io_encoding_t *encoding,io_multiplex_socket_t *socket) {
	return layer->implementation->decode(layer,encoding,socket);
}

INLINE_FUNCTION bool
io_layer_match_address (io_layer_t *layer,io_address_t a) {
	return layer->implementation->match_address (layer,a);
}

INLINE_FUNCTION io_address_t
io_layer_any_address (io_layer_t *layer) {
	return layer->implementation->any ();
}

INLINE_FUNCTION void
io_layer_get_content (io_layer_t *layer,io_encoding_t *encoding,uint8_t const **begin,uint8_t const **end) {
	layer->implementation->get_content (layer,encoding,begin,end);
}

INLINE_FUNCTION bool
io_layer_set_remote_address (io_layer_t *layer,io_encoding_t *packet,io_address_t a) {
	return layer->implementation->set_remote_address (layer,packet,a);
}

INLINE_FUNCTION io_address_t
io_layer_get_remote_address (io_layer_t *layer,io_encoding_t *encoding) {
	return layer->implementation->get_remote_address (layer,encoding);
}

INLINE_FUNCTION io_address_t
io_layer_get_local_address (io_layer_t *layer,io_encoding_t *packet) {
	return layer->implementation->get_local_address (layer,packet);
}

INLINE_FUNCTION bool
io_layer_set_local_address (io_layer_t *layer,io_encoding_t *packet,io_address_t a) {
	return layer->implementation->set_local_address (layer,packet,a);
}

INLINE_FUNCTION io_address_t
io_layer_get_inner_address (io_layer_t *layer,io_encoding_t *packet) {
	return layer->implementation->get_inner_address (layer,packet);
}

INLINE_FUNCTION bool
io_layer_set_inner_address (io_layer_t *layer,io_encoding_t *packet,io_address_t a) {
	return layer->implementation->set_inner_address (layer,packet,a);
}

io_layer_t*		mk_virtual_io_layer (io_packet_encoding_t*);
void				free_virtual_io_layer (io_layer_t*,io_byte_memory_t*);
bool 				virtual_io_layer_match_address (io_layer_t*,io_address_t);
bool				virtual_io_layer_set_local_address (io_layer_t*,io_encoding_t*,io_address_t);
io_address_t	virtual_io_layer_get_invalid_address (io_layer_t*,io_encoding_t*);
io_address_t	virtual_io_layer_get_remote_address (io_layer_t*,io_encoding_t*);
bool				virtual_io_layer_set_inner_address (io_layer_t*,io_encoding_t*,io_address_t);

#define SPECIALISE_VIRTUAL_IO_LAYER_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.make = mk_virtual_io_layer, \
	.free = free_virtual_io_layer, \
	.swap = NULL,\
	.match_address = virtual_io_layer_match_address, \
	.get_remote_address = virtual_io_layer_get_remote_address, \
	.set_remote_address = NULL,\
	.get_local_address = virtual_io_layer_get_invalid_address, \
	.set_local_address = virtual_io_layer_set_local_address, \
	.get_inner_address = virtual_io_layer_get_invalid_address,\
	.set_inner_address = NULL,\
	/**/

typedef struct PACK_STRUCTURE io_layer_map {
	io_address_t id;
	io_layer_implementation_t const *implementation;
} io_layer_map_t;

//
// packet encoding
//
#define IO_PACKET_ENCODING_STRUCT_MEMBERS \
	IO_BINARY_ENCODING_STRUCT_MEMBERS \
	io_layer_t **layers; \
	io_layer_t **end_of_layers; \
	/**/
	
struct PACK_STRUCTURE io_packet_encoding {
	IO_PACKET_ENCODING_STRUCT_MEMBERS
};

io_encoding_t* 	io_packet_encoding_new (io_byte_memory_t*);
void*					initialise_io_packet_encoding (io_packet_encoding_t*);
void 					io_packet_encoding_free (io_encoding_t*);
io_layer_t* 		io_packet_encoding_push_layer (io_encoding_t*,io_layer_implementation_t const*);
void* 				get_packet_encoding_layer (io_encoding_t*,io_layer_implementation_t const*);
void* 				io_packet_encoding_get_inner_layer (io_encoding_t*,io_layer_t*);
void* 				io_packet_encoding_get_outer_layer (io_encoding_t*,io_layer_t*);

extern EVENT_DATA io_encoding_implementation_t io_packet_encoding_implementation;
extern EVENT_DATA io_encoding_layer_api_t io_packet_layer_api;

INLINE_FUNCTION io_encoding_t*
mk_io_packet_encoding (io_byte_memory_t *bm) {
	return io_packet_encoding_implementation.make_encoding(bm);
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

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

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

//
// layers
//
bool
io_layer_has_implementation (
	io_layer_t const *layer,io_layer_implementation_t const *T
) {
	io_layer_implementation_t const *E = layer->implementation;
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

io_layer_t*
mk_virtual_io_layer (io_packet_encoding_t *packet) {	
	return NULL;
}
void
free_virtual_io_layer (io_layer_t *layer,io_byte_memory_t *bm) {
}

bool
virtual_io_layer_match_address (io_layer_t *layer,io_address_t address) {
	return false;
}


io_address_t
virtual_io_layer_get_remote_address (io_layer_t *layer,io_encoding_t *encoding) {
	return io_invalid_address();
}

io_address_t
virtual_io_layer_get_invalid_address (io_layer_t *layer,io_encoding_t *encoding) {
	return io_invalid_address();
}

bool
virtual_io_layer_set_local_address (
	io_layer_t *layer,io_encoding_t *message,io_address_t local
) {
	return false;
}

//
// packet encoding
//
io_encoding_t* 
io_packet_encoding_new (io_byte_memory_t *bm) {
	io_packet_encoding_t *this = io_byte_memory_allocate (
		bm,sizeof(io_packet_encoding_t)
	);

	if (this != NULL) {
		this->implementation = &io_packet_encoding_implementation;
		this->bm = bm;
		this = initialise_io_packet_encoding ((io_packet_encoding_t*) this);
	}

	return (io_encoding_t*) this;
};

void*
initialise_io_packet_encoding (io_packet_encoding_t *this) {
	this = io_binary_encoding_initialise ((io_binary_encoding_t*) this);
	if (this) {
		this->layers = NULL;
		this->end_of_layers = NULL;
	}
	return this;
}

void
free_io_packet_encoding_memory (io_packet_encoding_t *this) {
	io_layer_t **cursor = this->layers;
	while (cursor < this->end_of_layers) {
		free_io_layer (*cursor++,this->bm);
	}
	io_byte_memory_free (this->bm,this->layers);
	io_binary_encoding_free_memory ((io_binary_encoding_t*) this);
}

void
io_packet_encoding_free (io_encoding_t *encoding) {
	io_packet_encoding_t *this = (io_packet_encoding_t*) encoding;
	free_io_packet_encoding_memory (this);
	io_byte_memory_free (this->bm,this);
}

io_layer_t*
io_packet_encoding_push_layer (
	io_encoding_t *encoding,io_layer_implementation_t const *L
) {
	io_packet_encoding_t *packet = (io_packet_encoding_t*) encoding;
	uint32_t number_of_layers = packet->end_of_layers - packet->layers;
	io_layer_t **bigger = io_byte_memory_reallocate (
		packet->bm,packet->layers,sizeof(io_layer_t*) * (number_of_layers + 1)
	);
	
	if (bigger) {
		packet->layers = bigger;
		packet->layers[number_of_layers] = make_io_layer (
			L,(io_packet_encoding_t*) packet
		);
		packet->end_of_layers = packet->layers + (number_of_layers + 1);
		return packet->layers[number_of_layers];
	} else {
		return NULL;
	}
}

void*
get_packet_encoding_layer (io_encoding_t *encoding,io_layer_implementation_t const *L) {
	io_packet_encoding_t *packet = (io_packet_encoding_t*) encoding;

	if (L == NULL) {
		return (packet->layers == NULL) ? NULL : packet->layers[0];
	} else {
		io_layer_t **cursor = packet->layers;
		while (cursor < packet->end_of_layers) {
			if (io_layer_has_implementation (*cursor,L)) {
				return *cursor;
			}
			cursor++;
		}
		return NULL;
	}
}

void*
io_packet_encoding_get_inner_layer (io_encoding_t *encoding,io_layer_t *layer) {
	io_packet_encoding_t *packet = (io_packet_encoding_t*) encoding;

	if (layer == NULL) {
		return packet->end_of_layers ? *(packet->end_of_layers - 1) : NULL;
	} else {
		io_layer_t **cursor = packet->layers;
		while (cursor < (packet->end_of_layers - 1)) {
			if (*cursor == layer) {
				return *(cursor + 1);
			}
			cursor++;
		}

		return NULL;
	}
}

void*
io_packet_encoding_get_outer_layer (io_encoding_t *encoding,io_layer_t *layer) {
	io_packet_encoding_t *packet = (io_packet_encoding_t*) encoding;

	if (layer == NULL) {
		return NULL;
	} else {
		io_layer_t **cursor = packet->layers + 1;
		while (cursor < packet->end_of_layers) {
			if (*cursor == layer) {
				return *(cursor - 1);
			}
			cursor++;
		}

		return NULL;
	}
}

void
io_packet_encoding_get_content (
	io_encoding_t *encoding,uint8_t const **begin,uint8_t const **end
) {
	io_packet_encoding_t *this = (io_packet_encoding_t*) encoding;
	if (this->layers) {
		io_layer_t *inner = *(this->end_of_layers - 1);
		io_layer_get_content (inner,encoding,begin,end);
	} else {
		*begin = this->byte_stream,
		*end = this->cursor;
	}
};

static int32_t
io_packet_encoding_default_limit (void) {
	return (1<<16);
}

EVENT_DATA io_encoding_implementation_t io_packet_encoding_implementation = {
	.specialisation_of = &io_binary_encoding_implementation,
	.decode_to_io_value = io_binary_encoding_decode_to_io_value,
	.make_encoding = io_packet_encoding_new,
	.free = io_packet_encoding_free,
	.get_io = io_binary_encoding_get_io,
	.grow = io_binary_encoding_grow,
	.grow_increment = default_io_encoding_grow_increment,
	.fill = io_binary_encoding_fill_bytes,
	.append_byte = io_binary_encoding_append_byte,
	.append_bytes = io_binary_encoding_append_bytes,
	.pop_last_byte = io_binary_encoding_pop_last_byte,
	.print = io_binary_encoding_print,
	.reset = io_binary_encoding_reset,
	.layer = &io_packet_layer_api,
	.get_byte_stream = io_binary_encoding_get_byte_stream,
	.get_content = io_packet_encoding_get_content,
	.length = io_binary_encoding_length,
	.limit = io_packet_encoding_default_limit,
};

EVENT_DATA io_encoding_layer_api_t io_packet_layer_api = {
	.get_inner_layer = io_packet_encoding_get_inner_layer,
	.get_outer_layer = io_packet_encoding_get_outer_layer,
	.get_layer = get_packet_encoding_layer,
	.push_layer = io_packet_encoding_push_layer,
};
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
					io_inner_port_t *inner = io_layer_decode (
						base,next,(io_multiplex_socket_t*) this
					);
					if (inner) {
						if (io_encoding_pipe_put_encoding (inner->receive_pipe,next)) {
							if (inner->rx_available) {
								io_enqueue_event (io_socket_io (this),inner->rx_available);
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

//
// io binary layer
//

typedef struct PACK_STRUCTURE io_binary_frame {
	uint8_t length[4];
	uint8_t content[];
} io_binary_frame_t;

typedef struct PACK_STRUCTURE io_binary_layer {
	IO_LAYER_STRUCT_PROPERTIES
	io_address_t remote;
	io_address_t local;
} io_binary_layer_t;


EVENT_DATA io_layer_implementation_t io_binary_layer_implementation = {
	SPECIALISE_VIRTUAL_IO_LAYER_IMPLEMENTATION(NULL)
};

static io_address_t
io_binary_layer_any_address (void) {
	return io_any_address();
}

static bool
io_binary_layer_match_address (io_layer_t *layer,io_address_t address) {
	return true;
}

static io_address_t
io_binary_layer_get_any_address (io_layer_t *layer,io_encoding_t *message) {
	return io_layer_any_address (layer);
}

static io_address_t
io_binary_layer_get_remote_address (io_layer_t *layer,io_encoding_t *message) {
	io_binary_layer_t *this = (io_binary_layer_t*) layer;
	return this->remote;
}

static bool
io_binary_layer_set_remote_address (
	io_layer_t *layer,io_encoding_t *message,io_address_t address
) {
	io_packet_encoding_t *packet = (io_packet_encoding_t *) message;
	io_binary_layer_t *this = (io_binary_layer_t*) layer;
	assign_io_address (packet->bm,&this->remote,address);
	return true;
}

static io_address_t
io_binary_layer_get_local_address (io_layer_t *layer,io_encoding_t *message) {
	io_binary_layer_t *this = (io_binary_layer_t*) layer;
	return this->local;
}

static bool
io_binary_layer_set_local_address (
	io_layer_t *layer,io_encoding_t *message,io_address_t address
) {
	io_packet_encoding_t *packet = (io_packet_encoding_t *) message;
	io_binary_layer_t *this = (io_binary_layer_t*) layer;
	assign_io_address (packet->bm,&this->local,address);
	return true;
}

static io_layer_t*
mk_io_binary_layer_type (io_packet_encoding_t *packet,io_layer_implementation_t const *T) {
	io_binary_layer_t *this = io_byte_memory_allocate (packet->bm,sizeof(io_binary_layer_t));

	if (this) {
		this->implementation = T;
		this->layer_offset_in_byte_stream = io_encoding_length ((io_encoding_t*) packet);
		this->local = io_invalid_address();
		this->remote = io_invalid_address();
		io_encoding_fill ((io_encoding_t*) packet,0,sizeof(io_binary_frame_t));
	}
	
	return (io_layer_t*) this;
}

static void
free_io_binary_layer (io_layer_t *layer,io_byte_memory_t *bm) {
	io_binary_layer_t *this = (io_binary_layer_t *) layer;
	free_io_address(bm,this->local);
	free_io_address(bm,this->remote);
	io_byte_memory_free (bm,layer);
}

static void
io_binary_layer_get_content (
	io_layer_t *layer,io_encoding_t *encoding,uint8_t const **begin,uint8_t const **end
) {
	io_binary_frame_t *frame = io_layer_get_byte_stream (layer,encoding);
	
	*begin = frame->content;
	*end = frame->content + read_le_uint32 (frame->length) - sizeof(io_binary_frame_t);
}

static io_layer_t*
io_binary_layer_swap_tx (io_layer_t *layer,io_encoding_t *encoding) {
	extern EVENT_DATA io_layer_implementation_t io_binary_layer_receive_implementation;
	io_layer_t *binary = io_encoding_push_layer (
		encoding,&io_binary_layer_receive_implementation
	);
	
	if (binary) {
		io_layer_set_remote_address (binary,encoding,io_layer_get_remote_address(layer,encoding));
	}
	
	return binary;
}

static io_layer_t*
mk_io_binary_layer_transmit (io_packet_encoding_t *packet) {
	extern EVENT_DATA io_layer_implementation_t io_binary_layer_transmit_implementation;
	return mk_io_binary_layer_type (packet,&io_binary_layer_transmit_implementation);
}

EVENT_DATA io_layer_implementation_t io_binary_layer_transmit_implementation = {
	.specialisation_of = &io_binary_layer_implementation,
	.any = io_binary_layer_any_address,
	.make = mk_io_binary_layer_transmit,
	.free =  free_io_binary_layer,
	.swap =  io_binary_layer_swap_tx,
	.decode = NULL,
	.get_content = io_binary_layer_get_content,
	.match_address =  io_binary_layer_match_address,
	.get_remote_address = io_binary_layer_get_remote_address,
	.set_remote_address = io_binary_layer_set_remote_address,
	.get_local_address = io_binary_layer_get_local_address,
	.set_local_address = io_binary_layer_set_local_address,
	.get_inner_address = io_binary_layer_get_any_address,
	.set_inner_address = NULL,
};

io_layer_t*
push_io_binary_transmit_layer (io_encoding_t *encoding) {
	return io_encoding_push_layer (
		encoding,&io_binary_layer_transmit_implementation
	);
}

static io_layer_t*
mk_io_binary_layer_receive (io_packet_encoding_t *packet) {
	extern EVENT_DATA io_layer_implementation_t io_binary_layer_receive_implementation;
	return mk_io_binary_layer_type (packet,&io_binary_layer_receive_implementation);
}

static io_inner_port_t*
io_binary_layer_receive_decode (
	io_layer_t *layer,io_encoding_t *encoding,io_multiplex_socket_t* socket
) {
	io_address_t addr = io_layer_get_remote_address (layer,encoding);
	io_inner_port_binding_t *inner = io_multiplex_socket_find_inner_binding (socket,addr);

	if (inner) {
		return inner->port;
	}
	
	return NULL;
}

EVENT_DATA io_layer_implementation_t io_binary_layer_receive_implementation = {
	.specialisation_of = &io_binary_layer_implementation,
	.any = io_binary_layer_any_address,
	.make = mk_io_binary_layer_receive,
	.free =  free_io_binary_layer,
	.swap =  NULL,
	.decode = io_binary_layer_receive_decode,
	.get_content = io_binary_layer_get_content,
	.match_address = io_binary_layer_match_address,
	.get_remote_address = io_binary_layer_get_remote_address,
	.set_remote_address = io_binary_layer_set_remote_address,
	.get_local_address = io_binary_layer_get_local_address,
	.set_local_address = io_binary_layer_set_local_address,
	.get_inner_address = io_binary_layer_get_any_address,
	.set_inner_address = NULL,
};

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

io_encoding_t*
make_reveive_copy (io_packet_encoding_t *source_encoding) {
	io_layer_t *base = io_encoding_get_outermost_layer ((io_encoding_t*) source_encoding);
	if (base) {
		io_encoding_t *copy = io_encoding_dup (
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

