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
// packet encodings carry layers
//
typedef struct io_packet_encoding io_packet_encoding_t;

#define IO_LAYER_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_layer_implementation_t const *specialisation_of; \
	io_layer_t* (*make) (io_packet_encoding_t*);\
	io_layer_t* (*swap) (io_layer_t*,io_encoding_t*);\
	io_inner_port_binding_t* (*decode) (io_layer_t*,io_encoding_t*,io_socket_t*);\
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
// inline layer methods
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

INLINE_FUNCTION io_inner_port_binding_t*
io_layer_decode (io_layer_t *layer,io_encoding_t *encoding,io_socket_t *socket) {
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
// binary layer
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

extern EVENT_DATA io_layer_implementation_t io_binary_layer_implementation;

io_layer_t* push_io_binary_transmit_layer (io_encoding_t*);

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

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
// io binary layer
//


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

static io_inner_port_binding_t*
io_binary_layer_receive_decode (
	io_layer_t *layer,io_encoding_t *encoding,io_socket_t* socket
) {
	io_address_t addr = io_layer_get_remote_address (layer,encoding);
	io_multiplex_socket_t *mux = (io_multiplex_socket_t*) socket;
	io_inner_port_binding_t *inner = io_multiplex_socket_find_inner_binding (mux,addr);

	if (inner) {
		return inner;
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

