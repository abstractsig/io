/*
 *
 * Io
 *
 * This library implements io_values which provide a common platform upon
 * which higher level languages can be developed.  By higher level we mean
 * languages that are more 'expressive' than C such as Javascript or Python.
 *
 * VERSION HISTORY
 * ===============
 * 1.0	 (2020-01)
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 * USAGE
 * =====
 * Include this file in whatever places need to refer to it.
 * In one C source define IMPLEMENT_IO_CORE prior to the include
 * directive.
 *
 * COMMING SOON
 * ============
 * Collection values: vector, list and map
 * Incremental garbage collection whith cycle detection
 * Layered communication sockets
 * Persistent storage of io_values
 * Additional io language support for Javascript and Python
 * A ms windows io cpu
 * A linux io cpu
 *
 */
#ifndef io_core_H_
#define io_core_H_
#include <configure_io_build.h>
#include <limits.h>

#ifdef USE_LIBC
# include <string.h>
# define io_memset	memset

// utf8 for now, need locale for longer codes
# include <ctype.h>
#define character_is_digit		isdigit
#define character_is_alpha		isalpha
#define character_is_isalnum	isalnum

#define character_to_decimal_digit(c)	((c) - 0x30)

#else
# error "not yet"
#endif

#include <io_math.h>

typedef struct io io_t;
typedef struct io_encoding io_encoding_t;
typedef struct io_pipe io_pipe_t;

#define SHA256_SIZE		32

/*
 *
 * Byte memory manager
 *
 * based on umm (see git hub)
 *
 */
typedef struct memory_info {
	uint32_t total_bytes;
	uint32_t used_bytes;
	uint32_t free_bytes;
} memory_info_t;

#define UMM_BEST_FIT
#undef  UMM_FIRST_FIT

#define UMM_CRITICAL_ENTRY(bm)	{\
												bool __h = enter_io_critical_section(bm->io);
#define UMM_CRITICAL_EXIT(bm)			exit_io_critical_section(bm->io,__h);\
											}

typedef struct PACK_STRUCTURE umm_ptr_t {
	unsigned short int next;
	unsigned short int prev;
} umm_ptr;

typedef struct PACK_STRUCTURE {
	union {
		umm_ptr used;
	} header;
	union {
		umm_ptr free;
		unsigned char data[4];
	} body;
} umm_block_t;

typedef struct {
	io_t *io;
	umm_block_t *heap;
	unsigned int number_of_blocks;
} io_byte_memory_t;

void 	iterate_io_byte_memory_allocations(io_byte_memory_t*,bool (*cb) (void*,void*),void*);
void	initialise_io_byte_memory (io_t*,io_byte_memory_t*);
void*	umm_malloc(io_byte_memory_t*,size_t);
void*	umm_calloc(io_byte_memory_t*,size_t,size_t);
void*	umm_realloc(io_byte_memory_t*,void *ptr, size_t size );
void	umm_free(io_byte_memory_t*,void *ptr );
void	io_byte_memory_get_info (io_byte_memory_t*,memory_info_t *info);

#define io_byte_memory_get_io(bm)	(bm)->io

#define io_byte_memory_allocate umm_malloc
#define io_byte_memory_free umm_free
#define io_byte_memory_reallocate umm_realloc

typedef struct io_value io_value_t;
typedef struct io_value_memory io_value_memory_t;

/*
 *
 * Events
 *
 */
typedef struct io_event io_event_t;
typedef struct io_event_implementation io_event_implementation_t;
typedef void (*io_event_handler_t) (io_event_t*);

struct PACK_STRUCTURE io_event_implementation {
	io_event_implementation_t const *specialisation_of;
	io_pipe_t* (*cast_to_pipe) (io_event_t*);
};

#define IO_EVENT_STRUCT_MEMBERS \
	io_event_implementation_t const *implementation;\
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

#define merge_into_io_event(s,d) \
	(d)->event_handler = (s)->event_handler;\
	(d)->user_value = (s)->user_value;

bool	io_is_event_of_type (io_event_t const*,io_event_implementation_t const*);

extern io_event_t s_null_io_event;
extern EVENT_DATA io_event_implementation_t io_event_base_implementation;

INLINE_FUNCTION io_event_t*
initialise_io_event (
	io_event_t *ev,io_event_handler_t fn,void* user_value
) {
	ev->implementation = &io_event_base_implementation;
	ev->event_handler = fn;
	ev->user_value = user_value;
	ev->next_event = NULL;
	return ev;
}

INLINE_FUNCTION io_event_t*
initialise_typed_io_event (
	io_event_t *ev,io_event_implementation_t const *I,io_event_handler_t fn,void* user_value
) {
	ev->implementation = I;
	ev->event_handler = fn;
	ev->user_value = user_value;
	ev->next_event = NULL;
	return ev;
}

INLINE_FUNCTION io_pipe_t*
io_event_cast_to_pipe (io_event_t *ev) {
	return ev->implementation->cast_to_pipe (ev);
}

//
// alarms
//
typedef struct io_alarm io_alarm_t;

struct PACK_STRUCTURE io_alarm {
	io_event_t *at;					// raised 'at' when
	io_event_t *error;				// raised if at cannot be raised as the correct time
	int64_t when;
	io_alarm_t *next_alarm;
};

extern io_alarm_t s_null_io_alarm;

//
// Pipes
//
typedef struct io_pipe_implementation io_pipe_implementation_t;

#define IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS \
	io_pipe_implementation_t const *specialisation_of;\
	/**/
	
struct PACK_STRUCTURE io_pipe_implementation {
	IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_PIPE_STRUCT_MEMBERS \
	io_event_t ev; /* MUST BE FIRST */\
	io_pipe_implementation_t const *implementation;\
	int16_t size_of_ring;\
	int16_t write_index;\
	int16_t read_index;\
	int16_t overrun;\
	/**/

struct PACK_STRUCTURE io_pipe {
	IO_PIPE_STRUCT_MEMBERS
};

#define io_pipe_event(p)	(&(p)->ev)

INLINE_FUNCTION bool
io_pipe_is_readable (io_pipe_t const *this) {
	return  (this->read_index != this->write_index);
}

INLINE_FUNCTION int16_t
io_pipe_count_occupied_slots (io_pipe_t const *this) {
	int16_t s = this->write_index - this->read_index;
	if (s < 0) s += this->size_of_ring;
	return s;
}

INLINE_FUNCTION int16_t
io_pipe_count_free_slots (io_pipe_t const *this) {
	return this->size_of_ring - io_pipe_count_occupied_slots(this) - 1;
}

INLINE_FUNCTION bool
io_pipe_is_writeable (io_pipe_t const *this) {
	return io_pipe_count_free_slots(this) > 0;
}

//
// byte pipe
//
typedef struct PACK_STRUCTURE io_byte_pipe {
	IO_PIPE_STRUCT_MEMBERS
	uint8_t *byte_ring;
} io_byte_pipe_t;

io_byte_pipe_t* mk_io_byte_pipe (io_byte_memory_t*,uint16_t);
void free_io_byte_pipe (io_byte_pipe_t*,io_byte_memory_t*);
bool		io_byte_pipe_get_byte (io_byte_pipe_t*,uint8_t*);
bool		io_byte_pipe_put_byte (io_byte_pipe_t*,uint8_t);
uint32_t	io_byte_pipe_put_bytes (io_byte_pipe_t*,uint8_t const*,uint32_t);

bool	is_io_byte_pipe_event (io_event_t const*);
bool	is_io_byte_pipe (io_pipe_t const*);

#define io_byte_pipe_count_free_slots(p) io_pipe_count_free_slots ((io_pipe_t const*) (p))
#define io_byte_pipe_is_readable(p) io_pipe_is_readable ((io_pipe_t const*) (p))
#define io_byte_pipe_is_writeable(p) io_pipe_is_writeable ((io_pipe_t const*) (p))

//
// encoding pipe
//
typedef struct PACK_STRUCTURE io_encoding_pipe_implementation {
	IO_PIPE_IMPLEMENTATION_STRUCT_MEMBERS
	io_encoding_t* (*new_encoding) (io_pipe_t*);
} io_encoding_pipe_implementation_t;

typedef struct PACK_STRUCTURE io_encoding_pipe {
	IO_PIPE_STRUCT_MEMBERS
	io_encoding_t **encoding_ring;
	
	void *user_value;
	void* (*user_action) (void*);
	
} io_encoding_pipe_t;

bool	is_io_encoding_pipe_event (io_event_t const*);
bool	is_io_encoding_pipe (io_pipe_t const*);

io_encoding_pipe_t* mk_io_encoding_pipe (io_byte_memory_t*,uint16_t);
void free_io_encoding_pipe (io_encoding_pipe_t*,io_byte_memory_t*);
bool	io_encoding_pipe_get_encoding (io_encoding_pipe_t*,io_encoding_t**);
bool	io_encoding_pipe_put_encoding (io_encoding_pipe_t*,io_encoding_t*);
bool	io_encoding_pipe_peek (io_encoding_pipe_t*,io_encoding_t**);
void	io_encoding_pipe_put (io_t *io,io_pipe_t*,const char*,va_list);

#define io_encoding_pipe_count_occupied_slots(p) io_pipe_count_occupied_slots ((io_pipe_t const*) (p))
#define io_encoding_pipe_count_free_slots(p) io_pipe_count_free_slots ((io_pipe_t const*) (p))
#define io_encoding_pipe_is_readable(p) io_pipe_is_readable ((io_pipe_t const*) (p))
#define io_encoding_pipe_is_writeable(p) io_pipe_is_writeable ((io_pipe_t const*) (p))


INLINE_FUNCTION io_encoding_t*
io_encoding_pipe_new_encoding (io_pipe_t *pipe) {
	return ((io_encoding_pipe_implementation_t const*) pipe->implementation)->new_encoding((io_pipe_t*)pipe);
}

/*
 *
 * References to values
 *
 */
typedef union io_value_reference vref_t;

typedef struct PACK_STRUCTURE io_value_reference_implementation {
	vref_t (*reference) (vref_t);
	void (*unreference) (vref_t);
	void const* (*cast_to_ro_pointer) (vref_t);
	void* (*cast_to_rw_pointer) (vref_t);
	int64_t	(*get_as_builtin_integer) (vref_t);
	io_value_memory_t* (*get_containing_memory) (vref_t);
} io_value_reference_implementation_t;

#if UINTPTR_MAX == 0xffffffff
# define IO_32_BIT_BUILD
#elif UINTPTR_MAX == 0xffffffffffffffff
# define IO_64_BIT_BUILD
#else
# error "what's happening here?"
#endif

union PACK_STRUCTURE io_value_reference {
	struct PACK_STRUCTURE {
		io_value_reference_implementation_t const *implementation;
		union PACK_STRUCTURE {
			intptr_t ptr;
			struct PACK_STRUCTURE {
				uint32_t memory:3;
				uint32_t address:29;
			} p32;
			struct PACK_STRUCTURE {
				uint32_t id:8;
				uint32_t block:24;
			} vm;
		} expando;
	} ref;
	uint64_t all;
};

#define vref_implementation(r)	(r).ref.implementation
#define vref_expando(r)			(r).ref.expando
#define vref_all(r)				(r).all

#ifdef IO_32_BIT_BUILD
COMPILER_VERIFY(sizeof(vref_t) == 8);
#else
# error "only 32bit build supported"
#endif

#define def_ptr_vref_cast(T,P,...) __VA_ARGS__ {\
		.ref.implementation = T,						\
		.ref.expando.ptr = ((intptr_t) (P)),			\
	}

#define def_vref(T,P) (def_ptr_vref_cast(T,P,(vref_t)))

#define umm_vref(I,id,ptr) (vref_t) {\
	.ref.implementation = I,\
	.ref.expando.p32 = {\
		.memory = id,\
		.address = io_value_reference_p32_from_c_pointer(ptr),\
	},\
}

#define io_value_reference_p32_memory(r)	vref_expando(r).p32.memory
#define io_value_reference_p32_address(r)	vref_expando(r).p32.address

// value memory is 8-byte aligned
#define io_value_reference_p32_from_c_pointer(p) 	(((intptr_t)(p)) >> 2)
#define io_value_reference_p32_to_c_pointer(r) 		(io_value_reference_p32_address(r) << 2)

//
// inline io_value reference methods
//
INLINE_FUNCTION vref_t
reference_value (vref_t r_value) {
	return vref_implementation(r_value)->reference(r_value);
}

INLINE_FUNCTION void
unreference_value (vref_t r_value) {
	vref_implementation(r_value)->unreference(r_value);
}

INLINE_FUNCTION void*
vref_cast_to_rw_pointer (vref_t r_value) {
	return vref_implementation(r_value)->cast_to_rw_pointer(r_value);
}

INLINE_FUNCTION void const*
vref_cast_to_ro_pointer (vref_t r_value) {
	return vref_implementation(r_value)->cast_to_ro_pointer(r_value);
}

INLINE_FUNCTION io_value_memory_t*
vref_get_containing_memory (vref_t r_value) {
	return vref_implementation(r_value)->get_containing_memory(r_value);
}


extern EVENT_DATA io_value_reference_implementation_t reference_to_constant_value;

#define INVALID_VREF 				def_vref (NULL,NULL)
#define vref_is_valid(v)			(vref_implementation(v) != NULL)
#define vref_is_invalid(v)			(vref_implementation(v) == NULL)
#define vref_is_equal_to(a,b)		(\
													(vref_implementation(a) == vref_implementation(b)) \
												&& (vref_expando(a).ptr == vref_expando(b).ptr) \
											)
#define vref_is_nil(r)				vref_is_equal_to(r,cr_NIL)
#define vref_not_nil(r)				(!vref_is_equal_to(r,cr_NIL))

void store_vref (vref_t,vref_t*,vref_t);

/*
 *
 * Encoding
 *
 */
typedef struct io_encoding_implementation io_encoding_implementation_t;

typedef vref_t (*io_value_decoder_t) (io_encoding_t*,io_value_memory_t*);

#define IO_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS \
	io_encoding_implementation_t const *specialisation_of;\
	io_encoding_t* (*make_encoding) (io_byte_memory_t*);\
	void (*free) (io_encoding_t*);\
	io_t* (*get_io) (io_encoding_t*);\
	vref_t (*decode_to_io_value) (io_encoding_t*,io_value_decoder_t,io_value_memory_t*);\
	uint32_t (*grow_increment) (io_encoding_t*);\
	int32_t (*limit) (void);\
	size_t (*length) (io_encoding_t const*);\
	bool (*push) (io_encoding_t*,uint8_t const*,uint32_t);\
	bool (*pop) (io_encoding_t*,uint32_t);\
	/**/

struct io_encoding_implementation {
	IO_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_ENCODING_STRUCT_MEMBERS \
	io_encoding_implementation_t const *implementation;\
	union PACK_STRUCTURE {\
		uint32_t all;\
		struct PACK_STRUCTURE {\
			uint16_t reference_count;\
			uint16_t _spare;\
		} bit;\
	} metadata;\
	/**/

struct PACK_STRUCTURE io_encoding {
	IO_ENCODING_STRUCT_MEMBERS
};

#define IO_ENCODING_REFERENCE_COUNT_LIMIT	0xffffUL

#define IO_ENCODING_IMPLEMENATAION(e)		((io_encoding_implementation_t const *) (e))
#define io_encoding_implementation(e)		(e)->implementation
#define io_encoding_reference_count(e)		(e)->metadata.bit.reference_count

bool	io_is_encoding_of_type (io_encoding_t const*,io_encoding_implementation_t const*);
io_encoding_t*	reference_io_encoding (io_encoding_t*);
void	unreference_io_encoding (io_encoding_t*);

//
// inline io_encoding methods
//
INLINE_FUNCTION io_encoding_t*
new_io_encoding (io_encoding_implementation_t const *I,io_byte_memory_t *bm) {
	return I->make_encoding (bm);
}

INLINE_FUNCTION io_t*
io_encoding_get_io (io_encoding_t *encoding) {
	return encoding->implementation->get_io (encoding);
}

INLINE_FUNCTION vref_t
io_encoding_decode_to_io_value (io_encoding_t *encoding,io_value_decoder_t d,io_value_memory_t *vm) {
	return encoding->implementation->decode_to_io_value(encoding,d,vm);
}

INLINE_FUNCTION void
io_encoding_free (io_encoding_t *encoding) {
	encoding->implementation->free (encoding);
}

INLINE_FUNCTION bool
io_encoding_push (io_encoding_t *encoding,uint8_t const *b,uint32_t s) {
	return encoding->implementation->push (encoding,b,s);
}

INLINE_FUNCTION bool
io_encoding_pop (io_encoding_t *encoding,uint32_t s) {
	return encoding->implementation->pop (encoding,s);
}

INLINE_FUNCTION size_t
io_encoding_limit (io_encoding_t *encoding) {
	return encoding->implementation->limit();
}

INLINE_FUNCTION uint32_t
io_encoding_get_grow_increment (io_encoding_t *encoding) {
	return encoding->implementation->grow_increment(encoding);
}

INLINE_FUNCTION size_t
io_encoding_length (io_encoding_t const *encoding) {
	return encoding->implementation->length(encoding);
}

/*
 *
 * Binary Encoding
 *
 */
#define IO_BINARY_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS \
	IO_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS	\
	io_encoding_t* (*copy) (io_encoding_t const*,io_byte_memory_t*);\
	size_t (*fill) (io_encoding_t*,uint8_t,size_t);\
	bool (*append_byte) (io_encoding_t*,uint8_t);\
	bool (*append_bytes) (io_encoding_t*,uint8_t const*,size_t);\
	size_t (*print) (io_encoding_t*,char const*,va_list);\
	void (*reset) (io_encoding_t*);\
	void (*get_ro_bytes) (io_encoding_t const*,uint8_t const**,uint8_t const**);\
	/**/

typedef struct io_binary_encoding_implementation {
	IO_BINARY_ENCODING_IMPLEMENTATION_STRUCT_MEMBERS
} io_binary_encoding_implementation_t;

#define IO_BINARY_ENCODING_STRUCT_MEMBERS \
	IO_ENCODING_STRUCT_MEMBERS	\
	io_byte_memory_t *bm; \
	uint8_t *cursor; \
	uint8_t *data; \
	uint8_t *end; \
	/**/

typedef struct PACK_STRUCTURE {
	IO_BINARY_ENCODING_STRUCT_MEMBERS
} io_binary_encoding_t;

#define io_binary_encoding_byte_memory(this)			((this)->bm)
#define io_binary_encoding_data_size(this)			((this)->cursor - (this)->data)
#define io_binary_encoding_allocation_size(this)	((this)->end - (this)->data)

bool		is_io_binary_encoding (io_encoding_t const*);
size_t	io_encoding_printf (io_encoding_t*,const char*, ... );

//
// inline binary encoding methods
//
// requires that encoding has an io_binary_encoding_implementation_t
//
INLINE_FUNCTION size_t
io_encoding_print (io_encoding_t *encoding,char const *fmt,va_list va) {
	return ((io_binary_encoding_implementation_t const *) encoding->implementation)->print(encoding,fmt,va);
}

INLINE_FUNCTION void
io_encoding_get_ro_bytes (io_encoding_t const *encoding,uint8_t const **begin,uint8_t const **end) {
	((io_binary_encoding_implementation_t const *) encoding->implementation)->get_ro_bytes (encoding,begin,end);
}

INLINE_FUNCTION void
io_encoding_reset (io_encoding_t *encoding) {
	if (is_io_binary_encoding (encoding)) {
		((io_binary_encoding_implementation_t const *) encoding->implementation)->reset (encoding);
	}
}

INLINE_FUNCTION bool
io_encoding_append_byte (io_encoding_t *encoding,uint8_t byte) {
	return ((io_binary_encoding_implementation_t const *) encoding->implementation)->append_byte (encoding,byte);
}

INLINE_FUNCTION bool
io_encoding_append_bytes (io_encoding_t *encoding,uint8_t const* bytes,size_t size) {
	return ((io_binary_encoding_implementation_t const *) encoding->implementation)->append_bytes (encoding,bytes,size);
}

//
// text encoding: a human readable encoding of values
//
typedef int32_t io_character_t;

typedef bool (*io_character_iterator_t) (io_character_t,void*);

bool	is_io_text_encoding (io_encoding_t const*);
bool	io_text_encoding_iterate_characters (io_encoding_t const*,io_character_iterator_t,void*);

extern EVENT_DATA io_binary_encoding_implementation_t io_text_encoding_implementation;

INLINE_FUNCTION io_encoding_t*
mk_io_text_encoding (io_byte_memory_t *bm) {
	return io_text_encoding_implementation.make_encoding(bm);
}

//
// x70 encoding: a wire-format encoding of values
//


//
// int64 encoding
//
typedef struct PACK_STRUCTURE io_value_int64_encoding {
	IO_ENCODING_STRUCT_MEMBERS
	int64_t encoded_value;
} io_value_int64_encoding_t;

#define io_value_int64_encoding_encoded_value(e) (e)->encoded_value

bool encoding_is_io_value_int64 (io_encoding_t*);

extern EVENT_DATA io_encoding_implementation_t io_value_int64_encoding_implementation;
#define def_int64_encoding(VALUE) (io_value_int64_encoding_t) {\
		.implementation = IO_ENCODING_IMPLEMENATAION(&io_value_int64_encoding_implementation),	\
		.encoded_value = VALUE,		\
	}

typedef struct PACK_STRUCTURE io_value_float64_encoding {
	IO_ENCODING_STRUCT_MEMBERS
	float64_t encoded_value;
} io_value_float64_encoding_t;

bool encoding_is_io_value_float64 (io_encoding_t*);

#define io_value_float64_encoding_encoded_value(e) (e)->encoded_value

extern EVENT_DATA io_encoding_implementation_t io_value_float64_encoding_implementation;
#define def_float64_encoding(VALUE) (io_value_float64_encoding_t) {\
		.implementation = IO_ENCODING_IMPLEMENATAION(&io_value_float64_encoding_implementation),	\
		.encoded_value = VALUE,		\
	}

#define IO_MESSAGE_ENCODING_STRUCT_MEMBERS \
	IO_BINARY_ENCODING_STRUCT_MEMBERS \
	uint32_t *stack;\
	/**/

typedef struct PACK_STRUCTURE io_message_encoding {
	IO_MESSAGE_ENCODING_STRUCT_MEMBERS
} io_message_encoding_t;


/*
 *
 * Values
 *
 */
typedef struct io_value_implementation io_value_implementation_t;

typedef struct PACK_STRUCTURE {
	int32_t id;
	uint8_t epoch[SHA256_SIZE];
} io_encoding_id_t;

#define decl_io_value_encoding(Idx) {.id = Idx,.epoch = IO_VALUE_ENCODING_EPOCH}

typedef vref_t (*io_value_action_t) (io_t*,vref_t,vref_t const*);
typedef bool (*io_match_argument_t) (vref_t);

typedef struct io_signature {
	io_match_argument_t const *arg;
	io_value_action_t action;
	char const *name;
	char const *description;
} io_signature_t;

#define io_signature(N,D,A,...)		{(const io_match_argument_t[]) {__VA_ARGS__,NULL},A,N,D}
#define arg_match(NAME)				io_value_is_##NAME
#define END_OF_MODE					{NULL,NULL}

typedef struct io_value_mode {
	char const *name;
	io_signature_t const *signature;
} io_value_mode_t;

#define IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_value_implementation_t const *specialisation_of; \
	io_encoding_id_t encoding; \
	io_value_t* (*initialise) (vref_t,vref_t); \
	void (*free) (io_value_t*); \
	bool (*encode) (vref_t,io_encoding_t*); \
	vref_t (*receive) (io_t*,vref_t,uint32_t,vref_t const*); \
	vref_t (*compare) (io_value_t const*,vref_t); \
	io_value_mode_t const* (*get_modes) (io_value_t const*);\
	/**/

// vref_t (*call) (io_t*,vref_t,uint32_t,vref_t const*,vref_t);

struct PACK_STRUCTURE io_value_implementation {
	IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES
};

#define IO_VALUE_IMPLEMENTATION(I) ((io_value_implementation_t const*) (I))

#define IO_VALUE_STRUCT_MEMBERS \
	io_value_implementation_t const *implementation;	\
	uint16_t reference_count_; \
	uint16_t size_; \
	/**/

struct PACK_STRUCTURE io_value {
	IO_VALUE_STRUCT_MEMBERS
};

#define io_value_reference_count(v)		((v)->reference_count_)
#define io_value_implementation(v)		((v)->implementation)
#define io_value_size(v)				((v)->size_)

void const*	io_typesafe_ro_cast (vref_t,vref_t);
void io_value_free_nop (io_value_t*);
bool default_io_value_encode (vref_t,io_encoding_t*);
vref_t	default_io_value_receive (io_t*,vref_t,uint32_t,vref_t const*);
io_value_t* io_value_initialise_nop (vref_t,vref_t);
io_value_mode_t const* io_value_get_null_modes (io_value_t const*);
vref_t io_value_compare_no_comparison(io_value_t const*,vref_t);

//
// inline io_value methods
//
INLINE_FUNCTION void
io_value_free (io_value_t *value) {
	return value->implementation->free(value);
}

INLINE_FUNCTION io_value_t*
io_value_initialise (vref_t r_value,vref_t r_base) {
	io_value_t const *v = vref_cast_to_ro_pointer (r_value);
	return v->implementation->initialise(r_value,r_base);
}

INLINE_FUNCTION bool
io_value_encode (vref_t r_value,io_encoding_t *c) {
	io_value_t const *v = vref_cast_to_ro_pointer (r_value);
	return v->implementation->encode(r_value,c);
}

INLINE_FUNCTION vref_t
io_value_send (io_t *io,vref_t r_value,uint32_t argc,vref_t const *args) {
	io_value_t const *v = vref_cast_to_ro_pointer (r_value);
	return v->implementation->receive(io,r_value,argc,args);
}

INLINE_FUNCTION io_value_mode_t const*
io_value_get_modes (io_value_t const* v) {
	return v->implementation->get_modes(v);
}

INLINE_FUNCTION vref_t
io_value_compare (io_value_t const *v,vref_t r_other) {
	return v->implementation->compare(v,r_other);
}

INLINE_FUNCTION io_value_implementation_t const*
get_io_value_implementation (vref_t r_value) {
	io_value_t const *b = vref_cast_to_ro_pointer (r_value);
	if (b) {
		return io_value_implementation (b);
	} else {
		return NULL;
	}
}

#define decl_io_value(T,S) \
	.implementation = T,			\
	.reference_count_ = 0,			\
	.size_ = S,						\
	/**/

extern EVENT_DATA io_value_reference_implementation_t reference_to_c_stack_value;


#define IO_MODAL_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES \
	IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_value_mode_t const *modes; \
	/**/

typedef struct PACK_STRUCTURE io_modal_value_implementation {
	IO_MODAL_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES
} io_modal_value_implementation_t;

#define modal_value_static_modes(this) ((io_modal_value_implementation_t const*) (this)->implementation)->modes

#define IO_MODAL_VALUE_STRUCT_PROPERTIES \
	IO_VALUE_STRUCT_MEMBERS \
	io_value_mode_t const *current_mode;\
	/**/

typedef struct PACK_STRUCTURE io_modal_value {
	IO_MODAL_VALUE_STRUCT_PROPERTIES
} io_modal_value_t;

vref_t io_modal_value_receive (io_t*,vref_t,uint32_t,vref_t const*);
io_value_t*	io_modal_value_initialise (vref_t,vref_t);
io_value_mode_t const* io_modal_value_get_modes (io_value_t const*);

#define decl_modal_value_implementation(I,MODES) \
	.specialisation_of = IO_VALUE_IMPLEMENTATION(&univ_value_implementation),	\
	.initialise = I,	\
	.free = io_value_free_nop,	\
	.encode = default_io_value_encode,	\
	.receive = io_modal_value_receive,\
	.compare = NULL, \
	.get_modes = io_modal_value_get_modes,\
	.modes = MODES,\
	/**/

#define decl_io_modal_value(T,S) \
	.implementation = IO_VALUE_IMPLEMENTATION(T),			\
	.reference_count_ = 0,			\
	.size_ = S,						\
	.current_mode = (T)->modes,		\
	/**/

/*
 *
 * Value memory
 *
 */
typedef struct PACK_STRUCTURE io_value_memory_implementation {
	void (*free) (io_value_memory_t*);
	vref_t (*allocate_value) (io_value_memory_t*,io_value_implementation_t const*,size_t);
	vref_t (*new_value) (io_value_memory_t*,io_value_implementation_t const*,size_t,vref_t);

	void (*do_gc) (io_value_memory_t*,int32_t);
	void (*get_info) (io_value_memory_t*,memory_info_t*);
	io_t* (*get_io) (io_value_memory_t*);
	bool (*is_persistant) (io_value_memory_t*);

	void const* (*get_value_ro_pointer) (io_value_memory_t*,vref_t);
	void * (*get_value_rw_pointer) (io_value_memory_t*,vref_t);
} io_value_memory_implementation_t;

#define IO_VALUE_MEMORY_STRUCT_MEMBERS \
	io_value_memory_implementation_t const *implementation;\
	uint32_t id_;\
	/**/

struct PACK_STRUCTURE io_value_memory {
	IO_VALUE_MEMORY_STRUCT_MEMBERS
};

#define io_value_memory_id(vm)	(vm)->id_

//
// inline value memory implementation
//
INLINE_FUNCTION void
free_io_value_memory (io_value_memory_t *mem) {
	return mem->implementation->free(mem);
}

INLINE_FUNCTION vref_t
io_value_memory_new_value (io_value_memory_t *mem,io_value_implementation_t const *I,size_t size,vref_t r_base) {
	return mem->implementation->new_value(mem,I,size,r_base);
}

INLINE_FUNCTION void
io_value_memory_get_info (io_value_memory_t *mem,memory_info_t *info) {
	mem->implementation->get_info(mem,info);
}

INLINE_FUNCTION void
io_value_memory_do_gc (io_value_memory_t *vm,int32_t count) {
	vm->implementation->do_gc(vm,count);
}

INLINE_FUNCTION io_t*
io_value_memory_get_io (io_value_memory_t *vm) {
	return vm->implementation->get_io(vm);
}

io_value_memory_t*	io_get_value_memory_by_id (uint32_t);

typedef struct PACK_STRUCTURE umm_io_value_memory {
	IO_VALUE_MEMORY_STRUCT_MEMBERS
	io_byte_memory_t *bm;
	io_t *io;
} umm_io_value_memory_t;

extern EVENT_DATA io_value_memory_implementation_t umm_value_memory_implementation;

/*
 *-----------------------------------------------------------------------------
 *
 * Io clocks are frequency references, e.g oscillators and dividers.
 *
 * Changing speed needs to traverse the full clock tree
 *
 *-----------------------------------------------------------------------------
 */
typedef struct io_cpu_clock io_cpu_clock_t;

typedef union {
	io_cpu_clock_t const *ro;
	io_cpu_clock_t *rw;
} io_cpu_clock_pointer_t;

typedef struct io_cpu_clock_implementation io_cpu_clock_implementation_t;

#define IO_CPU_CLOCK_IMPLEMENTATION_STRUCT_MEMBERS \
	io_cpu_clock_implementation_t const *specialisation_of;					\
	float64_t (*get_frequency) (io_cpu_clock_pointer_t);						\
	bool (*link_input_to_output) (io_cpu_clock_pointer_t,io_cpu_clock_pointer_t);	\
	bool (*link_output_to_input) (io_cpu_clock_pointer_t,io_cpu_clock_pointer_t);	\
	bool (*start) (io_cpu_clock_pointer_t);										\
	void (*stop) (io_cpu_clock_pointer_t);										\
	/**/

struct PACK_STRUCTURE io_cpu_clock_implementation {
	IO_CPU_CLOCK_IMPLEMENTATION_STRUCT_MEMBERS
};

#define IO_CPU_CLOCK_STRUCT_MEMBERS \
	io_cpu_clock_implementation_t const *implementation;		\
	/**/

struct PACK_STRUCTURE io_cpu_clock {
	IO_CPU_CLOCK_STRUCT_MEMBERS
};

#define NULL_IO_CLOCK					((io_cpu_clock_pointer_t){NULL})
#define IO_CPU_CLOCK(C)					((io_cpu_clock_pointer_t){(io_cpu_clock_t const*) (C)})
#define decl_io_cpu_clock_pointer(C)	{(io_cpu_clock_t const*) (C)}
#define io_cpu_clock_ro_pointer(c) 		(c).ro

bool	io_cpu_clock_link_output_to_input (io_cpu_clock_pointer_t output,io_cpu_clock_pointer_t input);
bool	io_cpu_clock_link_input_to_output (io_cpu_clock_pointer_t input,io_cpu_clock_pointer_t output);
void	io_cpu_clock_stop (io_cpu_clock_pointer_t);
bool	io_cpu_clock_has_implementation (io_cpu_clock_pointer_t,io_cpu_clock_implementation_t const*);
bool	io_cpu_dependant_clock_start_input (io_cpu_clock_pointer_t);

//
// inline clock implementation
//
INLINE_FUNCTION float64_t
io_cpu_clock_get_frequency (io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->get_frequency(c);
}

INLINE_FUNCTION bool
io_cpu_clock_start (io_cpu_clock_pointer_t c) {
	return io_cpu_clock_ro_pointer(c)->implementation->start(c);
}

extern EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_implementation;

#define IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS \
	IO_CPU_CLOCK_STRUCT_MEMBERS					\
	io_cpu_clock_pointer_t const *outputs;		\
	/**/

typedef struct PACK_STRUCTURE io_cpu_clock_source {
	IO_CPU_CLOCK_SOURCE_STRUCT_MEMBERS
} io_cpu_clock_source_t;

#define IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS \
	IO_CPU_CLOCK_STRUCT_MEMBERS		\
	io_cpu_clock_pointer_t input;				\
	/**/

typedef struct PACK_STRUCTURE io_cpu_dependant_clock {
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS
} io_cpu_dependant_clock_t;

bool io_dependant_cpu_clock_start (io_cpu_clock_pointer_t);
float64_t io_dependant_cpu_clock_get_frequency (io_cpu_clock_pointer_t);


#define IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS \
	IO_CPU_DEPENDANT_CLOCK_STRUCT_MEMBERS\
	io_cpu_clock_pointer_t const *outputs;\
	/**/

typedef struct PACK_STRUCTURE io_cpu_clock_function {
	IO_CPU_CLOCK_FUNCTION_STRUCT_MEMBERS
} io_cpu_clock_function_t;

//
// sockets
//

typedef struct io_socket_implementation io_socket_implementation_t;
typedef struct io_socket io_socket_t;

typedef bool (*io_socket_iterator_t) (io_socket_t*,void*);

typedef struct PACK_STRUCTURE io_socket_constructor {

	uint16_t transmit_pipe_length;
	uint16_t receive_pipe_length;
	
} io_socket_constructor_t;

#define io_socket_constructor_receive_pipe_length(c)	(c)->receive_pipe_length
#define io_socket_constructor_transmit_pipe_length(c)	(c)->transmit_pipe_length


#define IO_SOCKET_IMPLEMENTATION_STRUCT_MEMBERS \
	io_socket_implementation_t const *specialisation_of;\
	void (*initialise) (io_socket_t*,io_t*,io_socket_constructor_t const*);\
	void (*free) (io_socket_t*);\
	bool (*open) (io_socket_t*);\
	void (*close) (io_socket_t*);\
	io_t*	(*get_io) (io_socket_t*); \
	io_pipe_t* (*bindt) (io_socket_t*,io_event_t*);\
	io_event_t* (*bindr) (io_socket_t*,io_event_t*);\
	void (*bindc) (io_socket_t*);\
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
	/**/

struct PACK_STRUCTURE io_socket {
	IO_SOCKET_STRUCT_MEMBERS
};

//
// inline io socket implementation
//
INLINE_FUNCTION void
io_socket_initialise (io_socket_t *socket,io_t *io,io_socket_constructor_t const *C) {
	socket->implementation->initialise(socket,io,C);
}

INLINE_FUNCTION bool
io_socket_open (io_socket_t *socket) {
	return socket->implementation->open(socket);
}

INLINE_FUNCTION void
io_socket_close (io_socket_t *socket) {
	socket->implementation->close (socket);
}

INLINE_FUNCTION io_t*
io_socket_get_io (io_socket_t *socket) {
	return socket->implementation->get_io (socket);
}

INLINE_FUNCTION io_encoding_t*
io_socket_new_message (io_socket_t *socket) {
	return socket->implementation->new_message (socket);
}

INLINE_FUNCTION bool
io_socket_send_message (io_socket_t *socket,io_encoding_t *m) {
	return socket->implementation->send_message (socket,m);
}

INLINE_FUNCTION io_event_t*
io_socket_bindr (io_socket_t *socket,io_event_t *ev) {
	return socket->implementation->bindr (socket,ev);
}

INLINE_FUNCTION io_pipe_t*
io_socket_bindt (io_socket_t *socket,io_event_t *ev) {
	return socket->implementation->bindt (socket,ev);
}

//
// cpu io pins
//
typedef uint32_t io_pin_t;
void	io_pin_nop (io_t*,io_pin_t);

//
// interrupts
//
typedef void (*io_interrupt_action_t) (void*);

#define IO_INTERRUPT_HANDLER_STRUCT_MEMBERS \
	io_interrupt_action_t action; \
	void *user_value; \
	/**/
	
typedef struct io_interrupt_handler {
	IO_INTERRUPT_HANDLER_STRUCT_MEMBERS
} io_interrupt_handler_t;

//
// Io
//
// the model for io computation is a single cpu core per io_t instance
//
typedef struct PACK_STRUCTURE io_implementation {
	//
	// core resources
	//
	io_byte_memory_t* (*get_byte_memory) (io_t*);
	io_value_memory_t* (*get_short_term_value_memory) (io_t*);
	io_value_memory_t* (*get_long_term_value_memory) (io_t*);
	void (*do_gc) (io_t*,int32_t);
	io_cpu_clock_pointer_t (*get_core_clock) (io_t*);
	//
	// security
	//
	uint32_t (*get_random_u32) (io_t*);
	//
	// communication
	//
	io_socket_t* (*get_socket) (io_t*,int32_t);
	//
	// events
	//
	void (*dequeue_event) (io_t*,io_event_t*);
	void (*enqueue_event) (io_t*,io_event_t*);
	bool (*next_event) (io_t*);
	bool (*in_event_thread) (io_t*);
	void (*signal_event_pending) (io_t*);
	void (*wait_for_event) (io_t*);
	void (*wait_for_all_events) (io_t*);
	//
	// interrupts
	//
	bool (*enter_critical_section) (io_t*);
	void (*exit_critical_section) (io_t*,bool);
	void (*register_interrupt_handler) (io_t*,int32_t,io_interrupt_action_t,void*);
	bool (*unregister_interrupt_handler) (io_t*,int32_t,io_interrupt_action_t);
	//
	// io pins
	//
	void (*set_io_pin_output) (io_t*,io_pin_t);
	void (*set_io_pin_input) (io_t*,io_pin_t);
	void (*set_io_pin_interrupt) (io_t*,io_pin_t);
	void (*set_io_pin_alternate) (io_t*,io_pin_t);
	int32_t (*read_from_io_pin) (io_t*,io_pin_t);
	void (*write_to_io_pin) (io_t*,io_pin_t,int32_t);
	void (*toggle_io_pin) (io_t*,io_pin_t);
	bool (*valid_pin) (io_t*,io_pin_t);
	//
	// external notifications
	//
	void (*log) (io_t*,char const*,va_list);
	void (*panic) (io_t*,int);
	//
} io_implementation_t;

void add_io_implementation_core_methods (io_implementation_t*);
void add_io_implementation_cpu_methods (io_implementation_t*);
void add_io_implementation_board_methods (io_implementation_t*);
void add_io_implementation_device_methods (io_implementation_t*);

#define IO_STRUCT_MEMBERS \
	io_implementation_t const *implementation;\
	io_event_t *events; \
	io_alarm_t *alarms; \
	/**/

struct PACK_STRUCTURE io {
	IO_STRUCT_MEMBERS
};

void	enqueue_io_event (io_t*,io_event_t*);
void	dequeue_io_event (io_t*,io_event_t*);
bool	do_next_io_event (io_t*);

int io_printf(io_t*,const char *fmt,...);

INLINE_FUNCTION void
initialise_io (io_t *io,io_implementation_t const *I) {
	io->implementation = I;
	io->events = &s_null_io_event;
	io->alarms = &s_null_io_alarm;
}

//
// inline io implementation
//
INLINE_FUNCTION io_byte_memory_t*
io_get_byte_memory (io_t *io) {
	return io->implementation->get_byte_memory(io);
}

INLINE_FUNCTION io_value_memory_t*
io_get_short_term_value_memory (io_t *io) {
	return io->implementation->get_short_term_value_memory(io);
}

INLINE_FUNCTION io_value_memory_t*
io_get_long_term_value_memory (io_t *io) {
	return io->implementation->get_long_term_value_memory(io);
}

INLINE_FUNCTION uint32_t
io_get_random_u32 (io_t *io) {
	return io->implementation->get_random_u32(io);
}

INLINE_FUNCTION void
io_log (io_t *io,char const *fmt,va_list va) {
	io->implementation->log(io,fmt,va);
}

INLINE_FUNCTION void
io_panic (io_t *io,int code) {
	io->implementation->panic(io,code);
}

INLINE_FUNCTION void
io_wait_for_all_events (io_t *io) {
	io->implementation->wait_for_all_events (io);
}

INLINE_FUNCTION void
io_wait_for_event (io_t *io) {
	io->implementation->wait_for_event (io);
}

INLINE_FUNCTION bool
enter_io_critical_section (io_t *io) {
	return io->implementation->enter_critical_section (io);
}

INLINE_FUNCTION void
exit_io_critical_section (io_t *io,bool h) {
	io->implementation->exit_critical_section (io,h);
}

INLINE_FUNCTION void
register_io_interrupt_handler (io_t *io,int32_t h,io_interrupt_action_t a,void *u) {
	io->implementation->register_interrupt_handler (io,h,a,u);
}

INLINE_FUNCTION bool
unregister_io_interrupt_handler (io_t *io,int32_t h,io_interrupt_action_t a) {
	return io->implementation->unregister_interrupt_handler (io,h,a);
}

INLINE_FUNCTION void
io_dequeue_event (io_t *io,io_event_t *ev) {
	io->implementation->dequeue_event (io,ev);
}

INLINE_FUNCTION void
io_enqueue_event (io_t *io,io_event_t *ev) {
	io->implementation->enqueue_event (io,ev);
	io->implementation->signal_event_pending (io);
}

INLINE_FUNCTION void
signal_io_event_pending (io_t *io) {
	io->implementation->signal_event_pending (io);
}

INLINE_FUNCTION bool
next_io_event (io_t *io) {
	return io->implementation->next_event (io);
}

INLINE_FUNCTION void
io_do_gc (io_t *io,int32_t c) {
	io->implementation->do_gc (io,c);
}

INLINE_FUNCTION io_cpu_clock_pointer_t
io_get_core_clock (io_t *io) {
	return io->implementation->get_core_clock (io);
}

INLINE_FUNCTION io_socket_t*
io_get_socket (io_t *io,int32_t s) {
	return io->implementation->get_socket (io,s);
}

INLINE_FUNCTION void
io_set_pin_to_output (io_t *io,io_pin_t p) {
	io->implementation->set_io_pin_output (io,p);
}

INLINE_FUNCTION void
io_set_pin_to_input (io_t *io,io_pin_t p) {
	io->implementation->set_io_pin_input (io,p);
}

INLINE_FUNCTION void
write_to_io_pin (io_t *io,io_pin_t p,int32_t s) {
	io->implementation->write_to_io_pin (io,p,s);
}

INLINE_FUNCTION void
toggle_io_pin (io_t *io,io_pin_t p) {
	io->implementation->toggle_io_pin (io,p);
}

INLINE_FUNCTION int32_t
io_read_pin (io_t *io,io_pin_t p) {
	return io->implementation->read_from_io_pin (io,p);
}

INLINE_FUNCTION bool
io_pin_is_valid (io_t *io,io_pin_t p) {
	return io->implementation->valid_pin (io,p);
}


#define ENTER_CRITICAL_SECTION(E)	\
	{	\
		bool __critical_handle = enter_io_critical_section(E);


#define EXIT_CRITICAL_SECTION(E) \
		exit_io_critical_section (E,__critical_handle);	\
	}

enum {
	IO_PANIC_UNRECOVERABLE_ERROR = 1,
	IO_PANIC_SOMETHING_BAD_HAPPENED,
	IO_PANIC_DEVICE_ERROR,
	IO_PANIC_OUT_OF_MEMORY,
};

/*
 *
 * Particular values
 *
 */

typedef struct PACK_STRUCTURE io_nil_value {
	IO_VALUE_STRUCT_MEMBERS
} io_nil_value_t;

typedef struct PACK_STRUCTURE io_univ_value {
	IO_VALUE_STRUCT_MEMBERS
} io_univ_value_t;

extern EVENT_DATA io_value_implementation_t univ_value_implementation;

//
// numbers
//

typedef struct PACK_STRUCTURE io_number_value {
	IO_VALUE_STRUCT_MEMBERS
} io_number_value_t;

typedef struct PACK_STRUCTURE io_int64_value {
	IO_VALUE_STRUCT_MEMBERS
	int64_t value;
} io_int64_value_t;

vref_t mk_io_int64_value (io_value_memory_t *vm,int64_t value);
bool io_value_get_as_int64 (vref_t,int64_t*);
vref_t io_integer_to_integer_value_decoder (io_encoding_t*,io_value_memory_t*);

vref_t io_text_to_number_value_decoder (io_encoding_t*,io_value_memory_t*);

typedef struct PACK_STRUCTURE io_float64_value {
	IO_VALUE_STRUCT_MEMBERS
	float64_t value;
} io_float64_value_t;

vref_t mk_io_float64_value (io_value_memory_t *vm,float64_t value);
bool io_value_get_as_float64 (vref_t,float64_t*);

//
// binary
//

vref_t mk_io_binary_value_with_const_bytes (io_value_memory_t*,uint8_t const*,int32_t);
vref_t mk_io_binary_value (io_value_memory_t*,uint8_t const*,int32_t);
vref_t mk_io_text_value (io_value_memory_t*,uint8_t const*,int32_t);
vref_t io_text_to_text_value_decoder (io_encoding_t*,io_value_memory_t*);

typedef struct PACK_STRUCTURE io_binary_value {
	IO_VALUE_STRUCT_MEMBERS
	struct PACK_STRUCTURE {
		uint32_t binary_size:24;
		uint32_t inline_bytes:1;
		uint32_t const_bytes:1;
		uint32_t :6;
	} bit;
	union PACK_STRUCTURE {
		uint8_t *rw;
		uint8_t const *ro;
		char const *str;
	} bytes;
	uint8_t inline_bytes[];
} io_binary_value_t;

#define io_binary_value_size(b)				(b)->bit.binary_size
#define io_binary_value_const_bytes(b)		(b)->bit.const_bytes
#define io_binary_value_inline_bytes(b)	(b)->bit.inline_bytes
#define io_binary_value_ro_bytes(b)			(b)->bytes.ro
#define io_binary_value_rw_bytes(b)			(b)->bytes.rw
#define io_binary_value_string(b)			(b)->bytes.str

#define IO_VECTOR_VALUE_STRUCT_MEMBERS \
	IO_VALUE_STRUCT_MEMBERS	\
	uint32_t arity;\
	vref_t values[];\
	/**/

typedef struct PACK_STRUCTURE {
	IO_VECTOR_VALUE_STRUCT_MEMBERS
} io_vector_value_t;

/*
 *
 * global constants
 *
 */
#ifdef IMPLEMENT_IO_CORE

static io_socket_t*
io_core_get_null_socket (io_t *io,int32_t h) {
	return NULL;
}

static io_byte_memory_t*
io_core_get_null_byte_memory (io_t *io) {
	return NULL;
}

static io_value_memory_t*
io_core_get_null_value_memory (io_t *io) {
	return NULL;
}

void
io_pin_nop (io_t *io,io_pin_t p) {
}

static int32_t
read_from_io_pin_nop (io_t *io,io_pin_t p) {
	return 0;
}

static void
write_to_io_pin_nop (io_t *io,io_pin_t p,int32_t v) {
}

static bool 
io_pin_is_always_invalid (io_t *io,io_pin_t p) {
	return false;
}

static const io_implementation_t	io_base = {
	.get_byte_memory = io_core_get_null_byte_memory,
	.get_short_term_value_memory = io_core_get_null_value_memory,
	.get_long_term_value_memory = io_core_get_null_value_memory,
	.do_gc = NULL,
	.get_core_clock = NULL,
	.get_random_u32 = NULL,
	.get_socket = io_core_get_null_socket,
	.dequeue_event = dequeue_io_event,
	.enqueue_event = enqueue_io_event,
	.next_event = do_next_io_event,
	.in_event_thread = NULL,
	.signal_event_pending = NULL,
	.wait_for_event = NULL,
	.wait_for_all_events = NULL,
	.enter_critical_section = NULL,
	.exit_critical_section = NULL,
	.register_interrupt_handler = NULL,
	.unregister_interrupt_handler = NULL,
	.set_io_pin_output = io_pin_nop,
	.set_io_pin_input = io_pin_nop,
	.set_io_pin_interrupt = io_pin_nop,
	.set_io_pin_alternate = io_pin_nop,
	.read_from_io_pin = read_from_io_pin_nop,
	.write_to_io_pin = write_to_io_pin_nop,
	.toggle_io_pin = io_pin_nop,
	.valid_pin = io_pin_is_always_invalid,
	.log = NULL,
	.panic = NULL,
};

void 
add_io_implementation_core_methods (io_implementation_t *io_i) {
	memcpy (io_i,&io_base,sizeof(io_implementation_t));
}

# define decl_particular_value(REF_NAME,VALUE_TYPE,VALUE_VAR) \
	extern EVENT_DATA VALUE_TYPE VALUE_VAR;\
	EVENT_DATA vref_t REF_NAME = def_vref (&reference_to_constant_value,&VALUE_VAR);\
	bool io_value_is_##REF_NAME (vref_t r_value) {\
		return io_typesafe_ro_cast(r_value,REF_NAME);\
	}\
	/**/
#else
# define decl_particular_value(REF_NAME,VALUE_TYPE,VALUE_VAR) \
	extern EVENT_DATA vref_t REF_NAME;\
	bool io_value_is_##REF_NAME (vref_t);\
	/**/
#endif

decl_particular_value(cr_NIL,					io_nil_value_t,cr_nil_v)
decl_particular_value(cr_UNIV,				io_univ_value_t,cr_univ_v)
decl_particular_value(cr_BINARY,				io_binary_value_t,cr_binary_v)
decl_particular_value(cr_CONSTANT_BINARY,	io_binary_value_t,cr_const_binary_v)
decl_particular_value(cr_IO_EXCEPTION,		io_binary_value_t,cr_io_exception_v)
decl_particular_value(cr_SYMBOL,				io_binary_value_t,cr_symbol_v)
decl_particular_value(cr_TEXT,				io_binary_value_t,cr_text_v)
decl_particular_value(cr_NUMBER,				io_number_value_t,cr_number_v)
decl_particular_value(cr_I64_NUMBER,		io_int64_value_t,cr_i64_number_v)
decl_particular_value(cr_F64_NUMBER,		io_int64_value_t,cr_f64_number_v)

decl_particular_value(cr_VECTOR,				io_vector_value_t,cr_vector_v)
decl_particular_value(cr_RESULT,				io_vector_value_t,cr_result_v)
decl_particular_value(cr_RESULT_CONTINUE,	io_vector_value_t,cr_result_continue_v)

decl_particular_value(cr_COMPARE,			io_value_t,cr_compare_v)
decl_particular_value(cr_COMPARE_NO_COMPARISON,	io_value_t,cr_compare_no_comparison_v)

typedef enum {
	CR_NIL_ENCODING_INDEX = 0,
	CR_UNIV_ENCODING_INDEX,
	CR_NUMBER_ENCODING_INDEX,
	CR_I64_INTEGER_ENCODING_INDEX,
	CR_F64_FLOAT_ENCODING_INDEX,
	CR_BINARY_ENCODING_INDEX,
	CR_CONST_BINARY_ENCODING_INDEX,
	CR_TEXT_ENCODING_INDEX,
	CR_SYMBOL_ENCODING_INDEX,
	CR_VECTOR_ENCODING_INDEX,
	CR_RESULT_ENCODING_INDEX,
	CR_RESULT_CONTINUE_ENCODING_INDEX,

	CR_COMPARE_ENCODING_INDEX,
	CR_COMPARE_NO_COMPARISON_ENCODING_INDEX,
	CR_COMPARE_MORE_THAN_ENCODING_INDEX,
	CR_COMPARE_LESS_THAN_ENCODING_INDEX,
	CR_COMPARE_EQUAL_TO_ENCODING_INDEX,

	CR_END_OF_SYSTEM_ENCODING_INDICES,

	CR_INVALID_ENCODING_INDEX = 0x7fffffff,
} io_value_encoding_identifier_t;

extern EVENT_DATA io_value_implementation_t const* io_value_implementation_enum[];
#define get_io_value_implementation_from_encoding_index(I) (io_value_implementation_enum[I])

extern EVENT_DATA io_value_implementation_t io_symbol_value_implementation_with_const_bytes;
#ifdef IMPLEMENT_IO_CORE
# define def_constant_symbol(VAR_NAME,S,LEN) \
	extern EVENT_DATA vref_t VAR_NAME;\
	EVENT_DATA io_binary_value_t __##VAR_NAME = {\
		decl_io_value (\
			&io_symbol_value_implementation_with_const_bytes,sizeof(io_binary_value_t)\
		)\
		.bit = {\
			.binary_size = LEN,\
			.inline_bytes = false,\
			.const_bytes = 1,\
		},\
		.bytes.str = S,\
	};\
	EVENT_DATA vref_t VAR_NAME = def_vref (&reference_to_constant_value,&__##VAR_NAME);\
	vref_t is_symbol_##VAR_NAME (char const *str,size_t len) {\
		if (LEN == len && memcmp(str,S,len) == 0) {\
			return VAR_NAME;\
		} else {\
			return INVALID_VREF;\
		}\
	}\
	bool io_value_is_##VAR_NAME (vref_t r_value) {\
		return vref_is_equal_to(r_value,VAR_NAME);\
	}\
	/**/
#else
# define def_constant_symbol(VAR_NAME,S,len) \
	extern EVENT_DATA vref_t VAR_NAME;\
	extern EVENT_DATA io_binary_value_t __##VAR_NAME;\
	extern vref_t is_symbol_##VAR_NAME (char const*,size_t);\
	bool io_value_is_##VAR_NAME (vref_t);\
	/**/
#endif

def_constant_symbol(cr_OPEN,		"open",4)

//
// vector
//

#define IO_VECTOR_VALUE_STRUCT_MEMBERS_STACK \
	IO_VALUE_STRUCT_MEMBERS					\
	uint32_t arity;								\
	/**/

extern EVENT_DATA io_value_implementation_t io_vector_value_implementation;
#define declare_stack_vector(name,...) \
	struct PACK_STRUCTURE io_vector_value_stack_##name {\
	IO_VECTOR_VALUE_STRUCT_MEMBERS_STACK	\
	vref_t values[(sizeof((vref_t[]){__VA_ARGS__})/sizeof(vref_t))];	\
	};\
	struct io_vector_value_stack_##name name##_v = {\
			decl_io_value (&io_vector_value_implementation,sizeof(struct io_vector_value_stack_##name)) \
			.arity = (sizeof((vref_t[]){__VA_ARGS__})/sizeof(vref_t)),\
			.values = {__VA_ARGS__}\
		};\
	vref_t name = def_vref (&reference_to_c_stack_value,&name##_v);\
	UNUSED(name);

bool		io_vector_value_get_arity (vref_t,uint32_t*);
bool		io_vector_value_get_values (vref_t,vref_t const**);

//
// text decoding
//

typedef struct io_source_decoder io_source_decoder_t;
typedef void (*io_source_decoder_input_t) (io_source_decoder_t*,io_character_t);

typedef struct PACK_STRUCTURE io_source_decoder_context {
	vref_t r_value;
	vref_t *args;
	uint32_t arity;
} io_source_decoder_context_t;

typedef vref_t (*is_symbol_t) (char const *str,size_t);

struct PACK_STRUCTURE io_source_decoder {
	io_t *io;

	io_encoding_t *buffer;
	char const *error;

	io_source_decoder_input_t reset_state;
	io_source_decoder_input_t error_state;
	is_symbol_t const *keywords;

	io_source_decoder_input_t *input_stack;
	io_source_decoder_input_t *current_input;	// top of input stack

	io_source_decoder_context_t *context_stack;
	io_source_decoder_context_t *current_context;	// top of context stack
};

#define io_source_decoder_input_depth(d)		(((d)->current_input - (d)->input_stack) + 1)
#define io_source_decoder_context_depth(d)	 	(((d)->current_context - (d)->context_stack) + 1)
#define io_source_decoder_context(this)			((this)->current_context)
#define io_source_decoder_value(this)			((this)->current_context->r_value)
#define io_source_decoder_input(this)			(*((this)->current_input))
#define io_source_decoder_goto(d,s) 			io_source_decoder_input(d) = (s)
#define io_source_decoder_io(d)					((d)->io)
#define io_source_decoder_byte_memory(d)		io_get_byte_memory(io_source_decoder_io(d))
#define io_source_decoder_set_last_error(d,m) 	(d)->error = (m)
#define io_source_decoder_get_last_error(d)	(d)->error
#define io_source_decoder_has_error(d)			(io_source_decoder_get_last_error(d) != NULL)

io_source_decoder_t* mk_io_source_decoder (
	io_t*,vref_t,io_source_decoder_input_t,io_source_decoder_input_t,is_symbol_t const*
);
void	free_io_source_decoder (io_source_decoder_t*);

bool	io_source_decoder_append_arg (io_source_decoder_t*,vref_t);
void	io_source_decoder_end_of_statement (io_source_decoder_t*);
void	io_source_decoder_next_character (io_source_decoder_t*,uint32_t);
void	io_source_decoder_remove_last_character (io_source_decoder_t*);
void	io_source_decoder_parse (io_source_decoder_t*,const char*);
void	io_source_decoder_push_input (io_source_decoder_t*,io_source_decoder_input_t);
void	io_source_decoder_pop_input (io_source_decoder_t*);
void	io_source_decoder_reset (io_source_decoder_t*);

#ifdef IMPLEMENT_IO_CORE
# define STB_SPRINTF_IMPLEMENTATION
#endif

#ifndef STB_SPRINTF_H_INCLUDE
#define STB_SPRINTF_H_INCLUDE
/*

stb_sprintf - v1.06 - public domain snprintf() implementation

Single file sprintf replacement using stb_sprintf.

EXTRAS:
=======
You can print io_values with the %v style indicator and using
a vref_t as the argument.

*/

#if defined(__has_feature)
   #if __has_feature(address_sanitizer)
      #define STBI__ASAN __attribute__((no_sanitize("address")))
   #endif
#endif
#ifndef STBI__ASAN
#define STBI__ASAN
#endif

#ifdef STB_SPRINTF_STATIC
#define STBSP__PUBLICDEC static
#define STBSP__PUBLICDEF static STBI__ASAN
#else
#ifdef __cplusplus
#define STBSP__PUBLICDEC extern "C"
#define STBSP__PUBLICDEF extern "C" STBI__ASAN
#else
#define STBSP__PUBLICDEC extern
#define STBSP__PUBLICDEF STBI__ASAN
#endif
#endif

#include <stdarg.h> // for va_list()
#include <stddef.h> // size_t, ptrdiff_t

#ifndef STB_SPRINTF_MIN
#define STB_SPRINTF_MIN 512 // how many characters per callback
#endif
typedef char *STBSP_SPRINTFCB(char *buf, void *user, int len);

#ifndef STB_SPRINTF_DECORATE
#define STB_SPRINTF_DECORATE(name) stbsp_##name // define this before including if you want to change the names
#endif

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintf)(char *buf, char const *fmt, va_list va);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsnprintf)(char *buf, int count, char const *fmt, va_list va);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(sprintf)(char *buf, char const *fmt, ...);
STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(snprintf)(char *buf, int count, char const *fmt, ...);

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintfcb)(STBSP_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va);
STBSP__PUBLICDEF void STB_SPRINTF_DECORATE(set_separators)(char comma, char period);

#endif // STB_SPRINTF_H_INCLUDE

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// io core implementation
//
//-----------------------------------------------------------------------------

static io_pipe_t*
cast_io_event_to_null_pipe (io_event_t *ev) {
	return NULL;
}

EVENT_DATA io_event_implementation_t io_event_base_implementation = {
	.specialisation_of = NULL,
	.cast_to_pipe = cast_io_event_to_null_pipe,
};

bool
io_is_event_of_type (
	io_event_t const *ev,io_event_implementation_t const *T
) {
	io_event_implementation_t const *E = ev->implementation;
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

static void
null_event_handler (io_event_t *ev) {
}

io_event_t s_null_io_event = def_io_event (
	null_event_handler,NULL
);

void
enqueue_io_event (io_t *io,io_event_t *ev) {
	ENTER_CRITICAL_SECTION(io);
	if (ev->next_event == NULL) {
		ev->next_event = io->events;
		io->events = ev;
	}
	EXIT_CRITICAL_SECTION(io);
	signal_io_event_pending (io);
}

void
dequeue_io_event (io_t *io,io_event_t *old) {
	io_event_t **list;

	ENTER_CRITICAL_SECTION(io);
	list = &io->events;
	if (
			*list != &s_null_io_event
		&&	old->next_event != NULL
	) {
		if (*list == old) {
			*list = old->next_event;
		} else {
			io_event_t *ev = *list;
			while (ev->next_event != &s_null_io_event) {
				if (ev->next_event == old) {
					ev->next_event = old->next_event;
					break;
				}
				ev = ev->next_event;
			}
		}
	}
	EXIT_CRITICAL_SECTION(io);
	old->next_event = NULL;
}

bool
do_next_io_event (io_t *io) {
	io_event_t **list = &io->events;
	io_event_t *ev;
	bool r = false;

	ENTER_CRITICAL_SECTION(io);

	ev = *list;
	if (ev != &s_null_io_event) {
		if (ev->next_event != &s_null_io_event) {
			io_event_t *last;
			while(ev->next_event->next_event != &s_null_io_event) {
				ev = ev->next_event;
			}
			last = ev->next_event;
			ev->next_event = &s_null_io_event;
			ev = last;
		} else {
			*list = ev->next_event;
		}
	}
	ev->next_event = NULL;
	r = (*list != &s_null_io_event);

	EXIT_CRITICAL_SECTION(io);

	ev->event_handler(ev);

	return r;
}

io_alarm_t s_null_io_alarm = {
	.at = &s_null_io_event,
	.error = &s_null_io_event,
	.when = LLONG_MAX,
	.next_alarm = NULL,
};

int
io_printf (io_t *io,const char *fmt,...) {
	io_socket_t *print = io_get_socket (io,IO_PRINTF_SOCKET);
	int result = 0;
	if (print) {
		io_encoding_t *msg = io_socket_new_message (print);
		va_list va;

		va_start(va, fmt);
		result = io_encoding_print (msg,fmt,va);
		va_end(va);

		io_socket_send_message (print,msg);
	}
	
	return result;
}

//
// clock
//

bool
io_cpu_dependant_clock_start_input (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (
		(io_cpu_dependant_clock_t const*) io_cpu_clock_ro_pointer (clock)
	);

	if (io_cpu_clock_ro_pointer (this->input)) {
		if (io_cpu_clock_start (this->input)) {
			return io_cpu_clock_link_output_to_input (this->input,clock);
		} else {
			return false;
		}
	} else {
		return false;
	}
}

bool
io_cpu_clock_has_implementation (io_cpu_clock_pointer_t clock,io_cpu_clock_implementation_t const *T) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(clock)->implementation;
	bool yes = false;
	do {
		if (I == T) {
			yes = true;
			break;
		}
		I = I->specialisation_of;
	} while (I);

	return yes;
}

//
// Depending on the clocks linking may be done by either the soure or receiver
// end, or both.  Hence the two methods: link_output_to_input and link_input_to_output
//
bool
io_cpu_clock_link_output_to_input (io_cpu_clock_pointer_t output,io_cpu_clock_pointer_t input) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(output)->implementation;
	do {
		if (I->link_output_to_input) {
			return I->link_output_to_input (output,input);
		}
		I = I->specialisation_of;
	} while (I);

	return false;
}

bool
io_cpu_clock_link_input_to_output (io_cpu_clock_pointer_t input,io_cpu_clock_pointer_t output) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(input)->implementation;
	do {
		if (I->link_input_to_output) {
			return I->link_input_to_output (input,output);
		}
		I = I->specialisation_of;
	} while (I);

	return false;
}

void
io_cpu_clock_stop (io_cpu_clock_pointer_t clock) {
	io_cpu_clock_implementation_t const *I = io_cpu_clock_ro_pointer(clock)->implementation;
	do {
		if (I->stop) {
			I->stop (clock);
			break;
		}
		I = I->specialisation_of;
	} while (I);
}

static float64_t
io_cpu_clock_get_frequency_base (io_cpu_clock_pointer_t clock) {
	return 0;
}

static bool
io_cpu_clock_start_base (io_cpu_clock_pointer_t clock) {
	return false;
}

static void
io_cpu_clock_stop_base (io_cpu_clock_pointer_t clock) {

}

static bool
io_cpu_clock_link_output_to_input_base (io_cpu_clock_pointer_t clock,io_cpu_clock_pointer_t input) {
	return io_cpu_clock_link_input_to_output (input,clock);
}

static bool
io_cpu_clock_link_input_to_output_base (io_cpu_clock_pointer_t clock,io_cpu_clock_pointer_t output) {
	// no action required
	return true;
}

EVENT_DATA io_cpu_clock_implementation_t io_cpu_clock_implementation = {
	.specialisation_of = NULL,
	.get_frequency = io_cpu_clock_get_frequency_base,
	.link_output_to_input = io_cpu_clock_link_output_to_input_base,
	.link_input_to_output = io_cpu_clock_link_input_to_output_base,
	.start = io_cpu_clock_start_base,
	.stop = io_cpu_clock_stop_base,
};

float64_t
io_dependant_cpu_clock_get_frequency (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (io_cpu_dependant_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_get_frequency (this->input);
}

bool
io_dependant_cpu_clock_start (io_cpu_clock_pointer_t clock) {
	io_cpu_dependant_clock_t const *this = (io_cpu_dependant_clock_t const*) (
		io_cpu_clock_ro_pointer (clock)
	);
	return io_cpu_clock_start (this->input);
}

//
// pipes
//

static io_pipe_t*
cast_io_pipe_event_to_pipe (io_event_t *ev) {
	// BECAUSE ev is first member in pipe
	return (io_pipe_t*) ev;
}

static EVENT_DATA io_event_implementation_t io_event_implementation_base = {
	.specialisation_of = &io_event_base_implementation,
	.cast_to_pipe = cast_io_pipe_event_to_pipe,
};

static EVENT_DATA io_event_implementation_t io_byte_pipe_event_implementation = {
	.specialisation_of = &io_event_implementation_base,
	.cast_to_pipe = cast_io_pipe_event_to_pipe,
};

bool
is_io_byte_pipe_event (io_event_t const *ev) {
	return io_is_event_of_type (ev,&io_byte_pipe_event_implementation);
}

EVENT_DATA io_event_implementation_t io_encoding_pipe_event_implementation = {
	.specialisation_of = &io_event_implementation_base,
	.cast_to_pipe = cast_io_pipe_event_to_pipe,
};

bool
is_io_encoding_pipe_event (io_event_t const *ev) {
	return io_is_event_of_type (ev,&io_encoding_pipe_event_implementation);
}

static EVENT_DATA io_pipe_implementation_t io_pipe_implementation_base = {
	.specialisation_of = NULL,
};

static bool
io_is_pipe_of_type (
	io_pipe_t const *pipe,io_pipe_implementation_t const *T
) {
	io_pipe_implementation_t const *E = pipe->implementation;
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

static EVENT_DATA io_pipe_implementation_t io_byte_pipe_implementation = {
	.specialisation_of = &io_pipe_implementation_base,
};

bool
is_io_byte_pipe (io_pipe_t const *pipe) {
	return io_is_pipe_of_type (pipe,&io_byte_pipe_implementation);
}

static io_encoding_t*
encoding_pipe_new_encoding (io_pipe_t *pipe) {
	io_encoding_pipe_t *this = (io_encoding_pipe_t*) pipe;
	return this->user_action(this->user_value);
}

static EVENT_DATA io_encoding_pipe_implementation_t io_encoding_pipe_implementation = {
	.specialisation_of = &io_pipe_implementation_base,
	.new_encoding = encoding_pipe_new_encoding,
};

bool
is_io_encoding_pipe (io_pipe_t const *pipe) {
	return io_is_pipe_of_type (
		pipe,(io_pipe_implementation_t const*) &io_encoding_pipe_implementation
	);
}

io_byte_pipe_t*
mk_io_byte_pipe (io_byte_memory_t *bm,uint16_t length) {
	io_byte_pipe_t *this = io_byte_memory_allocate (bm,sizeof(io_byte_pipe_t));
	
	if (this) {
		this->implementation = &io_byte_pipe_implementation,
		initialise_typed_io_event (
			io_pipe_event(this),&io_byte_pipe_event_implementation,NULL,NULL
		);
		this->write_index = this->read_index = 0;
		this->size_of_ring = length;
		this->overrun = 0;
		this->byte_ring = io_byte_memory_allocate (bm,sizeof(uint8_t) * length);
		if (this->byte_ring == NULL) {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
free_io_byte_pipe (io_byte_pipe_t *this,io_byte_memory_t *bm) {
	io_byte_memory_free (bm,this->byte_ring);
	io_byte_memory_free (bm,this);
}

INLINE_FUNCTION int16_t
io_byte_pipe_increment_index (io_byte_pipe_t *this,int16_t i,int16_t n) {
	i += n;
	if (i >= this->size_of_ring) {
		i -= this->size_of_ring;
	}
	return i;
}

bool
io_byte_pipe_get_byte (io_byte_pipe_t *this,uint8_t *byte) {
	if (io_byte_pipe_is_readable (this)) {
		*byte = this->byte_ring[this->read_index];
		this->read_index = io_byte_pipe_increment_index (
			this,this->read_index,1
		);
		return true;
	} else {
		return false;
	}
}

bool
io_byte_pipe_put_byte (io_byte_pipe_t *this,uint8_t byte) {
	int16_t f = io_byte_pipe_count_free_slots(this);
	if (f > 0) {
		int16_t j = this->write_index;
		int16_t i = io_byte_pipe_increment_index(this,j,1);
		this->byte_ring[j] = byte;
		this->write_index = i;
		return true;
	} else {
		return false;
	}
}

uint32_t
io_byte_pipe_put_bytes (io_byte_pipe_t *this,uint8_t const *byte,uint32_t length) {
	uint8_t const *end = byte + length;
	bool ok = true;
	while (byte < end && ok) {
		ok = io_byte_pipe_put_byte (this,*byte++);
	}
	return length - (end - byte);
}

io_encoding_pipe_t*
mk_io_encoding_pipe (io_byte_memory_t *bm,uint16_t length) {
	io_encoding_pipe_t *this = io_byte_memory_allocate (bm,sizeof(io_encoding_pipe_t));
	
	if (this) {
		this->implementation = (
			(io_pipe_implementation_t const*) &io_encoding_pipe_implementation
		),
		initialise_typed_io_event (
			io_pipe_event(this),&io_encoding_pipe_event_implementation,NULL,NULL
		);
		this->write_index = this->read_index = 0;
		this->size_of_ring = length;
		this->overrun = 0;
		this->encoding_ring = io_byte_memory_allocate (bm,sizeof(io_encoding_t*) * length);
		this->user_value = NULL;
		this->user_action = NULL;
		if (this->encoding_ring == NULL) {
			io_byte_memory_free (bm,this);
			this = NULL;
		}
	}
	
	return this;
}

void
free_io_encoding_pipe (io_encoding_pipe_t *this,io_byte_memory_t *bm) {
	io_byte_memory_free (bm,this->encoding_ring);
	io_byte_memory_free (bm,this);
}

INLINE_FUNCTION int16_t
io_encoding_pipe_increment_index (io_encoding_pipe_t *this,int16_t i,int16_t n) {
	i += n;
	if (i >= this->size_of_ring) {
		i -= this->size_of_ring;
	}
	return i;
}

bool
io_encoding_pipe_get_encoding (io_encoding_pipe_t *this,io_encoding_t **encoding) {
	if (io_encoding_pipe_is_readable (this)) {
		*encoding = this->encoding_ring[this->read_index];
		this->read_index = io_encoding_pipe_increment_index (
			this,this->read_index,1
		);
		return true;
	} else {
		return false;
	}
}

bool
io_encoding_pipe_put_encoding (io_encoding_pipe_t *this,io_encoding_t *encoding) {
	int16_t f = io_encoding_pipe_count_free_slots(this);
	if (f > 0) {
		int16_t j = this->write_index;
		int16_t i = io_encoding_pipe_increment_index(this,j,1);
		this->encoding_ring[j] = encoding;
		this->write_index = i;
		return true;
	} else {
		return false;
	}
}

bool
io_encoding_pipe_peek (io_encoding_pipe_t *this,io_encoding_t **encoding) {
	if (io_encoding_pipe_is_readable (this)) {
		*encoding = this->encoding_ring[this->read_index];
		return true;
	} else {
		return false;
	}
}

void
io_encoding_pipe_put (io_t *io,io_pipe_t *this,const char *fmt,va_list va) {
	io_encoding_t *msg = io_encoding_pipe_new_encoding (this);
	io_encoding_print (msg,fmt,va);
	io_encoding_pipe_put_encoding ((io_encoding_pipe_t*) this,msg);
	io_enqueue_event (io,io_pipe_event(this));
}

//
// reference to umm value
//
vref_t
io_reference_to_umm_value_reference (vref_t r_value) {
	io_value_t *value = vref_cast_to_rw_pointer (r_value);
	if (value) {
		io_value_reference_count(value) ++;
	}
	return r_value;
}

static void
free_umm_value (umm_io_value_memory_t *this,io_value_t *value) {
	io_value_free (value);
	umm_free (this->bm,value);
}

static void
io_reference_to_umm_value_unreference (vref_t r_value) {
	io_value_t *value = vref_cast_to_rw_pointer (r_value);
	if (value) {
		if (io_value_reference_count(value)) {
			io_value_reference_count(value) --;
			if (io_value_reference_count(value) == 0) {
				free_umm_value ((umm_io_value_memory_t*) io_get_value_memory_by_id (io_value_reference_p32_memory(r_value)),value);
			}
		}
	}
}

int64_t
io_reference_to_umm_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

void const*
io_reference_to_umm_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) io_value_reference_p32_to_c_pointer(r_value);
}

void*
io_reference_to_umm_value_cast_to_rw_pointer (vref_t r_value) {
	return (void*) io_value_reference_p32_to_c_pointer(r_value);
}

io_value_memory_t*
io_reference_to_umm_value_get_containing_memory (vref_t r_value) {
	return io_get_value_memory_by_id (io_value_reference_p32_memory(r_value));
}

EVENT_DATA io_value_reference_implementation_t reference_to_umm_value = {
	.reference = io_reference_to_umm_value_reference,
	.unreference = io_reference_to_umm_value_unreference,
	.cast_to_ro_pointer = io_reference_to_umm_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_umm_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_umm_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_umm_value_get_containing_memory,
};

//
// umm-heap based io_value memory
//

void
umm_value_memory_free_memory (io_value_memory_t *vm) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	umm_free (this->bm,this);
}

vref_t
umm_value_memory_allocate_value (
	io_value_memory_t *vm,io_value_implementation_t const *I,size_t allocation_size
) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_value_t *new_value = umm_malloc (this->bm,allocation_size);

	*new_value = (io_value_t) {
		decl_io_value (I,allocation_size)
	};

	return umm_vref (&reference_to_umm_value,io_value_memory_id(this),new_value);
}

vref_t
umm_value_memory_new_value (
	io_value_memory_t *vm,
	io_value_implementation_t const *I,
	size_t size,
	vref_t r_base
) {
	vref_t r_new = umm_value_memory_allocate_value (vm,I,size);
	if (vref_is_valid (r_new)) {
		if (!I->initialise (r_new,r_base)) {
			r_new = INVALID_VREF;
		}
	}

	return r_new;
}

struct gc_stack {
	io_value_t** cursor;
	io_value_t** end;
};

bool
umm_value_memory_gc_iterator (void *value,void *user_value) {
	struct gc_stack *stack = user_value;
	*stack->cursor++ = value;
	return stack->cursor < stack->end;
}

#define GC_STACK_LENGTH	8

static void
umm_value_memory_do_gc (io_value_memory_t *vm,int32_t count) {
/*	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_value_t* values[GC_STACK_LENGTH];

	if (count < 0) count = 1000000;

	while (count > 0) {
		struct gc_stack stack = {
			.cursor = values,
			.end = values + GC_STACK_LENGTH,
		};
		bool clean = true;

		iterate_io_byte_memory_allocations (
			this->bm,umm_value_memory_gc_iterator,&stack
		);

		{
			io_value_t** cursor = values;
			while (cursor < stack.cursor) {
				if (io_value_reference_count(*cursor) == 0) {
					free_umm_value (this,*cursor);
				}
				cursor++;
			}
		}

		if (clean && stack.cursor == values) {
			break;
		}
		count--;
	};
*/
}

bool
heap_value_memory_is_persistant (io_value_memory_t *vm) {
	return false;
}

void const*
umm_value_memory_get_value_ro_pointer (io_value_memory_t *vm,vref_t r_value) {
	return vref_cast_to_ro_pointer (r_value);
}

void*
umm_value_memory_get_value_rw_pointer (io_value_memory_t *vm,vref_t r_value) {
	return vref_cast_to_rw_pointer (r_value);
}

void
umm_value_memory_get_info (io_value_memory_t *vm,memory_info_t *info) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	io_byte_memory_get_info (this->bm,info);
}

static io_t*
umm_value_memory_get_io (io_value_memory_t *vm) {
	umm_io_value_memory_t *this = (umm_io_value_memory_t*) vm;
	return this->io;
}

EVENT_DATA io_value_memory_implementation_t umm_value_memory_implementation = {
	.free = umm_value_memory_free_memory,
	.allocate_value = umm_value_memory_allocate_value,
	.new_value = umm_value_memory_new_value,
	.do_gc = umm_value_memory_do_gc,
	.get_info = umm_value_memory_get_info,
	.get_io = umm_value_memory_get_io,
	.is_persistant = heap_value_memory_is_persistant,
	.get_value_ro_pointer = umm_value_memory_get_value_ro_pointer,
	.get_value_rw_pointer = umm_value_memory_get_value_rw_pointer,
};

//
// reference to constant values
//
static vref_t
io_reference_to_constant_value_reference (vref_t r_value) {
	return r_value;
}

static void
io_reference_to_constant_value_unreference (vref_t r_value) {
}

static int64_t
io_reference_to_constant_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

static void const*
io_reference_to_constant_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) vref_expando(r_value).ptr;
}

static void*
io_reference_to_constant_value_cast_to_rw_pointer (vref_t r_value) {
	return NULL;
}

static io_value_memory_t*
io_reference_to_value_get_null_containing_memory (vref_t r_value) {
	return NULL;
}

EVENT_DATA io_value_reference_implementation_t reference_to_constant_value = {
	.reference = io_reference_to_constant_value_reference,
	.unreference = io_reference_to_constant_value_unreference,
	.cast_to_ro_pointer = io_reference_to_constant_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_constant_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_constant_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_value_get_null_containing_memory,
};

//
// reference to constant values
//
static vref_t
io_reference_to_c_stack_value_reference (vref_t r_value) {
	return r_value;
}

static void
io_reference_to_c_stack_value_unreference (vref_t r_value) {
}

static int64_t
io_reference_to_c_stack_value_get_as_builtin_integer (vref_t r_value) {
	return vref_expando(r_value).ptr;
}

static void const*
io_reference_to_c_stack_value_cast_to_ro_pointer (vref_t r_value) {
	return (void const*) vref_expando(r_value).ptr;
}

static void*
io_reference_to_c_stack_value_cast_to_rw_pointer (vref_t r_value) {
	return (void*) vref_expando(r_value).ptr;
}

EVENT_DATA io_value_reference_implementation_t reference_to_c_stack_value = {
	.reference = io_reference_to_c_stack_value_reference,
	.unreference = io_reference_to_c_stack_value_unreference,
	.cast_to_ro_pointer = io_reference_to_c_stack_value_cast_to_ro_pointer,
	.cast_to_rw_pointer = io_reference_to_c_stack_value_cast_to_rw_pointer,
	.get_as_builtin_integer = io_reference_to_c_stack_value_get_as_builtin_integer,
	.get_containing_memory = io_reference_to_value_get_null_containing_memory,
};

//
// Encoding
//

io_encoding_t*
reference_io_encoding (io_encoding_t *encoding) {
	uint32_t new_count = (uint32_t) io_encoding_reference_count (encoding) + 1;
	if (new_count <= IO_ENCODING_REFERENCE_COUNT_LIMIT) {
		io_encoding_reference_count (encoding) = new_count;
	} else {
		io_panic (io_encoding_get_io (encoding),IO_PANIC_UNRECOVERABLE_ERROR);
	}
	return encoding;
}

void
unreference_io_encoding (io_encoding_t *encoding) {
	if (io_encoding_reference_count(encoding)) {
		if (--io_encoding_reference_count(encoding) == 0) {
			io_encoding_free(encoding);
		}
	} else {
		io_panic (io_encoding_get_io (encoding),IO_PANIC_UNRECOVERABLE_ERROR);
	}
}

static vref_t
io_value_encoding_decode_to_io_value (
	io_encoding_t *encoding,io_value_decoder_t decoder,io_value_memory_t *vm
) {
	return decoder (encoding,vm);
}

static io_encoding_t*
mk_null_encoding (io_byte_memory_t *bm) {
	return NULL;
}

static void
free_null_encoding (io_encoding_t *encoder) {
}

static size_t
null_encoding_length (io_encoding_t const *encoder) {
	return 0;
}

static int32_t
null_encoding_limit (void) {
	return 0;
}

static uint32_t
null_encoding_grow_increment (io_encoding_t *encoding) {
	return 0;
}

static io_t*
null_encoding_get_io (io_encoding_t *encoding) {
	return NULL;
}

EVENT_DATA io_encoding_implementation_t io_encoding_implementation_base = {
	.specialisation_of = NULL,
	.make_encoding = mk_null_encoding,
	.free = free_null_encoding,
	.get_io = null_encoding_get_io,
	.decode_to_io_value = io_value_encoding_decode_to_io_value,
	.limit = null_encoding_limit,
	.length = null_encoding_length,
	.grow_increment = null_encoding_grow_increment,
	.push = NULL,
	.pop = NULL,
};

bool
io_is_encoding_of_type (
	io_encoding_t const *encoding,io_encoding_implementation_t const *T
) {
	io_encoding_implementation_t const *E = io_encoding_implementation(encoding);
	bool is = false;
	do {
		is = (E == T);
	} while (!is && (E = E->specialisation_of) != NULL);

	return is && (E != NULL);
}

EVENT_DATA io_encoding_implementation_t io_value_int64_encoding_implementation = {
	.specialisation_of = &io_encoding_implementation_base,
	.make_encoding = mk_null_encoding,
	.free = free_null_encoding,
	.get_io = null_encoding_get_io,
	.decode_to_io_value = io_value_encoding_decode_to_io_value,
	.limit = null_encoding_limit,
	.length = null_encoding_length,
	.grow_increment = null_encoding_grow_increment,
	.push = NULL,
	.pop = NULL,
};

bool encoding_is_io_value_int64 (io_encoding_t *encoding) {
	return io_is_encoding_of_type (
		encoding,IO_ENCODING_IMPLEMENATAION(&io_value_int64_encoding_implementation)
	);
}

EVENT_DATA io_encoding_implementation_t io_value_float64_encoding_implementation = {
	.specialisation_of = &io_encoding_implementation_base,
	.make_encoding = mk_null_encoding,
	.free = free_null_encoding,
	.get_io = null_encoding_get_io,
	.decode_to_io_value = io_value_encoding_decode_to_io_value,
	.limit = null_encoding_limit,
	.length = null_encoding_length,
	.grow_increment = null_encoding_grow_increment,
	.push = NULL,
	.pop = NULL,
};

bool
encoding_is_io_value_float64 (io_encoding_t *encoding) {
	return io_is_encoding_of_type (
		encoding,IO_ENCODING_IMPLEMENATAION(&io_value_float64_encoding_implementation)
	);
}

static vref_t
io_binary_encoding_decode_to_io_value (
	io_encoding_t *encoding,io_value_decoder_t decoder,io_value_memory_t *vm
) {
	return decoder (encoding,vm);
}

static void*
io_text_encoding_initialise (io_binary_encoding_t *this) {
	this->data = io_byte_memory_allocate (
		this->bm,TEXT_ENCODING_INITIAL_SIZE * sizeof(uint8_t)
	);
	if (this->data != NULL) {
		this->cursor = this->data;
		this->end = this->data + TEXT_ENCODING_INITIAL_SIZE;
		this->metadata.all = 0;
	} else {
		io_byte_memory_free (this->bm,this);
		this = NULL;
	}
	return this;
};

static io_encoding_t*
io_text_encoding_new (io_byte_memory_t *bm) {
	io_binary_encoding_t *this = io_byte_memory_allocate (
		bm,sizeof(io_binary_encoding_t)
	);

	if (this != NULL) {
		this->implementation = IO_ENCODING_IMPLEMENATAION (
			&io_text_encoding_implementation
		);
		this->bm = bm;
		this = io_text_encoding_initialise(this);
	}

	return (io_encoding_t*) this;
};

static void
io_text_encoding_free (io_encoding_t *encoding) {
	if (encoding != NULL) {
		io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
		if (this->data) {
			io_byte_memory_free (this->bm,this->data);
		}
		io_byte_memory_free (this->bm,this);
	}
}

static bool
io_text_encoding_grow (io_binary_encoding_t *this) {
	uint32_t old_size = io_binary_encoding_allocation_size (this);
	uint32_t new_size = (
			old_size 
		+	(io_encoding_get_grow_increment((io_encoding_t*) this) * sizeof(uint8_t))
	);

	this->data = io_byte_memory_reallocate (
		this->bm,this->data,new_size
	);

	if (this->data) {
		this->cursor = this->data + old_size;
		this->end = this->data + new_size;
		return true;
	} else {
		return false;
	}
}

static bool
io_text_encoding_append_byte (io_encoding_t *encoding,uint8_t byte) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	if (this->cursor == this->end) {
		if (
				io_encoding_limit (encoding) < 0
			||	io_binary_encoding_data_size (this) < io_encoding_limit (encoding)
		) {
			io_text_encoding_grow (this);
		}
	}

	if (this->cursor < this->end) {
		*this->cursor++ = byte;
		return true;
	} else {
		return false;
	}
}

static void
io_binary_encoding_get_ro_bytes (
	io_encoding_t const *encoding,uint8_t const **begin,uint8_t const **end
) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	*begin = this->data,
	*end = this->cursor;
};

static size_t
io_text_encoding_fill_bytes (
	io_encoding_t *encoding,uint8_t byte,size_t size
) {
	size_t len = size;
	while (len) {
		if (!io_text_encoding_append_byte (encoding,byte)) {
			break;
		}
		len --;
	}
	return size - len;
}

static bool
io_text_encoding_append_bytes (
	io_encoding_t *this,uint8_t const *byte,size_t size
) {
	uint8_t const *end = byte + size;
	while (byte < end) {
		if (!io_text_encoding_append_byte (this,*byte++)) {
			return false;
		}
	}
	return true;
}

struct PACK_STRUCTURE io_text_encoding_print_data {
	io_binary_encoding_t *this;
	char buf[STB_SPRINTF_MIN];
};
char*
io_text_encoding_print_cb (char *buf,void *user,int len) {
	struct io_text_encoding_print_data *info = user;
	io_text_encoding_append_bytes ((io_encoding_t*) info->this,(uint8_t const*)info->buf,len);
	return info->buf;
}

static size_t
io_text_encoding_print (io_encoding_t *encoding,char const *fmt,va_list va) {
	struct io_text_encoding_print_data user = {
		.this = (io_binary_encoding_t*) encoding
	};
	return STB_SPRINTF_DECORATE(vsprintfcb) (
		io_text_encoding_print_cb,&user,user.buf,fmt,va
	);
}

bool
is_io_text_encoding (io_encoding_t const *encoding) {
	return io_is_encoding_of_type (
		encoding,IO_ENCODING_IMPLEMENATAION (&io_text_encoding_implementation)
	);
}

bool
io_text_encoding_iterate_characters (
	io_encoding_t const *encoding,io_character_iterator_t cb,void *user_value
) {
	bool ok = true;

	if (is_io_text_encoding(encoding)) {
		const uint8_t *b,*e;

		io_encoding_get_ro_bytes (encoding,&b,&e);

		while (b < e) {
			if ((*b & 0x80) == 0) {
				if ((ok &= cb(*b,user_value)) == false) break;
			} else {
				// up to U+07FF		110xxxxx 	10xxxxxx
				// up to U+FFFF 	1110xxxx 	10xxxxxx 	10xxxxxx
				// up to U+10FFFF 	11110xxx 	10xxxxxx 	10xxxxxx 	10xxxxxx
			}
			b++;
		}
	}

	return ok;
};

size_t
io_encoding_printf (io_encoding_t *encoding,const char *format, ... ) {
	size_t r = 0;

	if (is_io_text_encoding (encoding)) {
		va_list va;
		va_start(va, format);
		r = io_encoding_print (encoding,format,va);
		va_end(va);
	}

	return r;
}

static void
io_binary_encoding_reset (io_encoding_t *encoding) {
	io_binary_encoding_t *this = (io_binary_encoding_t*) encoding;
	this->cursor = this->data;
}

//
// number of characters
//
static size_t
io_text_encoding_length (io_encoding_t const *encoding) {
	io_binary_encoding_t const *this = (io_binary_encoding_t const*) encoding;
	return io_binary_encoding_data_size (this);
}

static int32_t
io_binary_encoding_nolimit (void) {
	return -1;
}

static uint32_t
io_text_encoding_grow_increment (io_encoding_t *encoding) {
	return TEXT_ENCODING_GROWTH_INCREMENT;
}

static io_t*
io_binary_encoding_get_io (io_encoding_t *encoding) {
	io_binary_encoding_t *this = (io_binary_encoding_t *) encoding;
	return this->bm->io;
}

EVENT_DATA io_binary_encoding_implementation_t io_binary_encoding_implementation = {
	.specialisation_of = &io_encoding_implementation_base,
	.decode_to_io_value = io_binary_encoding_decode_to_io_value,
	.make_encoding = mk_null_encoding,
	.free = free_null_encoding,
	.get_io = io_binary_encoding_get_io,
	.push = NULL,
	.pop = NULL,
	.length = io_text_encoding_length,
	.limit = io_binary_encoding_nolimit,
	.grow_increment = io_text_encoding_grow_increment,
	.copy = NULL,
	.fill = io_text_encoding_fill_bytes,
	.append_byte = io_text_encoding_append_byte,
	.append_bytes = io_text_encoding_append_bytes,
	.print = io_text_encoding_print,
	.reset = io_binary_encoding_reset,
	.get_ro_bytes = io_binary_encoding_get_ro_bytes,
};

bool
is_io_binary_encoding (io_encoding_t const *encoding) {
	return io_is_encoding_of_type (
		encoding,IO_ENCODING_IMPLEMENATAION (&io_binary_encoding_implementation)
	);
}

EVENT_DATA io_binary_encoding_implementation_t io_text_encoding_implementation = {
	.specialisation_of = IO_ENCODING_IMPLEMENATAION (
		&io_binary_encoding_implementation
	),
	.decode_to_io_value = io_binary_encoding_decode_to_io_value,
	.make_encoding = io_text_encoding_new,
	.copy = NULL,
	.free = io_text_encoding_free,
	.push = NULL,
	.pop = NULL,
	.fill = io_text_encoding_fill_bytes,
	.append_byte = io_text_encoding_append_byte,
	.append_bytes = io_text_encoding_append_bytes,
	.print = io_text_encoding_print,
	.reset = io_binary_encoding_reset,
	.get_ro_bytes = io_binary_encoding_get_ro_bytes,
	.length = io_text_encoding_length,
	.limit = io_binary_encoding_nolimit,
};

//
// Values
//

void const*
io_typesafe_ro_cast (vref_t r_value,vref_t r_type) {
	io_value_t const *downcast = vref_cast_to_ro_pointer (r_value);
	io_value_implementation_t const *I = get_io_value_implementation (r_type);

	if (downcast) {
		io_value_implementation_t const *S = io_value_implementation (downcast);
		do {
			if (S == I) {
				break;
			} else {
				S = S->specialisation_of;
			}
		} while (S);

		if (!S) downcast = NULL;
	}

	return downcast;
}

void
io_value_free_nop (io_value_t *value) {
}

io_value_t*
io_value_initialise_nop (vref_t r_value,vref_t r_base) {
	return vref_cast_to_rw_pointer(r_value);
}

vref_t
io_value_compare_no_comparison(io_value_t const *this,vref_t r_other) {
	return cr_COMPARE_NO_COMPARISON;
}

io_value_mode_t const* io_value_get_null_modes (io_value_t const *v) {
	return NULL;
}

bool
default_io_value_encode (vref_t r_value,io_encoding_t *encoding) {
	return true;
}

vref_t
default_io_value_receive (io_t *io,vref_t r_value,uint32_t argc,vref_t const *args) {
	return cr_IO_EXCEPTION;
}

//
// universal value
//

EVENT_DATA io_value_implementation_t univ_value_implementation = {
	.specialisation_of = NULL,
	.encoding = decl_io_value_encoding (CR_UNIV_ENCODING_INDEX),
	.initialise = io_value_initialise_nop,
	.free = io_value_free_nop,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_univ_value_t cr_univ_v = {
	decl_io_value (&univ_value_implementation,sizeof(io_univ_value_t))
};

EVENT_DATA io_value_implementation_t nil_value_implementation = {
	.specialisation_of = &univ_value_implementation,
	.encoding = decl_io_value_encoding (CR_NIL_ENCODING_INDEX),
	.initialise = io_value_initialise_nop,
	.free = io_value_free_nop,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_nil_value_t cr_nil_v = {
	decl_io_value (&nil_value_implementation,sizeof(io_nil_value_t))
};

//
// C number values
//

EVENT_DATA io_value_implementation_t io_number_value_implementation = {
	.specialisation_of = &univ_value_implementation,
	.encoding = decl_io_value_encoding (CR_NUMBER_ENCODING_INDEX),
	.initialise = io_value_initialise_nop,
	.free = io_value_free_nop,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_number_value_t cr_number_v = {
	decl_io_value (&io_number_value_implementation,sizeof(io_number_value_t))
};

static io_value_t*
io_int64_value_initialise (vref_t r_value,vref_t r_base) {
	io_int64_value_t *this =  vref_cast_to_rw_pointer (r_value);
	io_int64_value_t const *base = io_typesafe_ro_cast(r_base,cr_I64_NUMBER);

	if (base != NULL) {
		this->value = base->value;
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static bool
io_int64_value_encode (vref_t r_value,io_encoding_t *encoding) {
	bool result = false;

	io_int64_value_t const *this = io_typesafe_ro_cast(
		r_value,cr_I64_NUMBER
	);
	if (this) {
		if (is_io_text_encoding (encoding)) {
			io_encoding_printf (encoding,"%lld",this->value);
			result = true;
		} else if (encoding_is_io_value_int64 (encoding)) {
			io_value_int64_encoding_t *enc = (io_value_int64_encoding_t*) encoding;
			enc->encoded_value = this->value;
			result = true;
		}
	}

	return result;
}

EVENT_DATA io_value_implementation_t i64_number_value_implementation = {
	.specialisation_of = &io_number_value_implementation,
	.encoding = decl_io_value_encoding (CR_I64_INTEGER_ENCODING_INDEX),
	.initialise = io_int64_value_initialise,
	.free = io_value_free_nop,
	.encode = io_int64_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_int64_value_t cr_i64_number_v = {
	decl_io_value (&i64_number_value_implementation,sizeof(io_int64_value_t))
	.value = 0,
};
EVENT_DATA io_value_t *cr_i64_integer = (io_value_t*) &cr_i64_number_v;

vref_t
mk_io_int64_value (io_value_memory_t *vm,int64_t value) {
	io_int64_value_t base = {
		decl_io_value (
			&i64_number_value_implementation,sizeof(io_int64_value_t)
		)
		.value = value,
	};
	return io_value_memory_new_value (
		vm,
		&i64_number_value_implementation,
		sizeof (io_int64_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

vref_t
io_integer_to_integer_value_decoder (io_encoding_t *encoding,io_value_memory_t *vm) {
	io_value_int64_encoding_t *this = (io_value_int64_encoding_t*) encoding;
	return mk_io_int64_value (vm,this->encoded_value);
}

bool
io_value_get_as_int64 (vref_t r_value,int64_t *value) {
	io_value_int64_encoding_t enc = def_int64_encoding(0);
	if (io_value_encode (r_value,(io_encoding_t*) &enc)) {
		*value = io_value_int64_encoding_encoded_value(&enc);
		return true;
	} else {
		return false;
	}
}

//
// float64
//

static io_value_t*
io_float64_value_initialise (vref_t r_value,vref_t r_base) {
	io_float64_value_t *this =  vref_cast_to_rw_pointer (r_value);
	io_float64_value_t const *base = io_typesafe_ro_cast (r_base,cr_F64_NUMBER);

	if (base != NULL) {
		this->value = base->value;
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static bool
io_float64_value_encode (vref_t r_value,io_encoding_t *encoding) {
	io_float64_value_t const *this = io_typesafe_ro_cast (
		r_value,cr_F64_NUMBER
	);
	bool result = false;

	if (this) {
		if (is_io_text_encoding (encoding)) {
			io_encoding_printf (encoding,"%f",this->value);
			result = true;
		} else if (encoding_is_io_value_float64 (encoding)) {
			io_value_float64_encoding_t *enc = (io_value_float64_encoding_t*) encoding;
			enc->encoded_value = this->value;
			result = true;
		}
	}

	return result;
}

EVENT_DATA io_value_implementation_t f64_number_value_implementation = {
	.specialisation_of = &io_number_value_implementation,
	.encoding = decl_io_value_encoding (CR_F64_FLOAT_ENCODING_INDEX),
	.initialise = io_float64_value_initialise,
	.free = io_value_free_nop,
	.encode = io_float64_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_int64_value_t cr_f64_number_v = {
	decl_io_value (&f64_number_value_implementation,sizeof(io_float64_value_t))
	.value = 0,
};
EVENT_DATA io_value_t *cr_f64_number = (io_value_t*) &cr_f64_number_v;

vref_t
mk_io_float64_value (io_value_memory_t *vm,float64_t value) {
	io_float64_value_t base = {
		decl_io_value (
			&f64_number_value_implementation,sizeof(io_float64_value_t)
		)
		.value = value,
	};
	return io_value_memory_new_value (
		vm,
		&f64_number_value_implementation,
		sizeof (io_float64_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

vref_t
io_float64_value_decoder (io_encoding_t *encoding,io_value_memory_t *vm) {
	io_value_int64_encoding_t *this = (io_value_int64_encoding_t*) encoding;
	return mk_io_int64_value (vm,this->encoded_value);
}

bool
io_value_get_as_float64 (vref_t r_value,float64_t *value) {
	io_value_float64_encoding_t enc = def_float64_encoding(0);
	if (io_value_encode (r_value,(io_encoding_t*) &enc)) {
		*value = io_value_float64_encoding_encoded_value(&enc);
		return true;
	} else {
		return false;
	}
}

//
// number decoder
//
#define IO_BUILTIN_ANY_NUMBER_TYPE_INT64	0
#define IO_BUILTIN_ANY_NUMBER_TYPE_UINT64	1
#define IO_BUILTIN_ANY_NUMBER_TYPE_FLOAT64	2

typedef union io_any_builtin_number_value {
	int64_t i64;
	uint64_t u64;
	float64_t f64;
} io_any_builtin_number_value_t;

typedef struct PACK_STRUCTURE {
	io_any_builtin_number_value_t internal;
	uint32_t type;
} io_any_builtin_number_t;

#define io_any_builtin_number_type(n)		(n).type

#define io_any_builtin_number_i64(n)		(n).internal.i64
#define io_any_builtin_number_f64(n)		(n).internal.f64

#define decl_io_any_builtin_number_i64(v)	{.internal.i64 = v,.type = IO_BUILTIN_ANY_NUMBER_TYPE_INT64}

struct io_number_value_decode_state {
	io_any_builtin_number_t value;
	int64_t fraction;
	float64_t power;
	uint32_t count;
	union PACK_STRUCTURE {
		uint32_t all;
		struct PACK_STRUCTURE {
			uint32_t negative:1;
			uint32_t error:1;
			uint32_t is_float:1;
			uint32_t :29;
		} bit;
	} info;
};

static bool
io_integer_value_decode_iterator (io_character_t c,void *user_value) {
	struct io_number_value_decode_state *state = user_value;

	if (c == '-') {
		if (state->count == 0) {
			state->info.bit.negative = 1;
		} else {
			state->info.bit.error = 1;
		}
	} else if (c == '.') {
		if (state->info.bit.is_float == 0) {
			state->info.bit.is_float = 1;
			io_any_builtin_number_f64(state->value) = (float64_t) (
					io_any_builtin_number_i64(state->value)
			);
			io_any_builtin_number_type(state->value) = IO_BUILTIN_ANY_NUMBER_TYPE_FLOAT64;
			state->power = 1.0;
			state->fraction = 0;
		} else {
			state->info.bit.error = 1;
		}
	} else if (state->info.bit.is_float) {
		int64_t t = (
				(state->fraction * 10)
			+	character_to_decimal_digit(c)
		);
		if (t >= state->fraction) {
			state->fraction = t;
			state->power *= 10.0;
		} else {
			state->info.bit.error = 1;
		}
	} else {
		int64_t t = (
				(io_any_builtin_number_i64 (state->value) * 10)
			+	character_to_decimal_digit(c)
		);
		if (t < io_any_builtin_number_i64(state->value)) {
			// overflow
			return false;
		} else {
			io_any_builtin_number_i64(state->value) = t;
		}
	}

	state->count++;

	return state->info.bit.error == 0;
}

//
// expects buffer to contain a number
//
vref_t
io_text_to_number_value_decoder (io_encoding_t *buffer,io_value_memory_t *vm) {
	struct io_number_value_decode_state s = {
		.value = decl_io_any_builtin_number_i64(0),
		.info.all = 0,
		.count = 0,
	};
	if (
		io_text_encoding_iterate_characters (
			buffer,io_integer_value_decode_iterator,&s
		)
	) {
		switch (io_any_builtin_number_type(s.value)) {
			case IO_BUILTIN_ANY_NUMBER_TYPE_INT64:
				return mk_io_int64_value (
					vm,io_any_builtin_number_i64 (s.value) * (s.info.bit.negative ? -1 : 1)
				);
			case IO_BUILTIN_ANY_NUMBER_TYPE_UINT64:
				// .....
			break;
			case IO_BUILTIN_ANY_NUMBER_TYPE_FLOAT64:
				io_any_builtin_number_f64 (s.value) += 	s.fraction / s.power;
				return mk_io_float64_value (
					vm,io_any_builtin_number_f64 (s.value) * (s.info.bit.negative ? -1 : 1)
				);
		}
	}
	return INVALID_VREF;
}

//
// binary raw-bytes value
//

static io_value_t*
io_binary_value_initialise_nop (vref_t r_value,vref_t r_base) {
	return vref_cast_to_rw_pointer(r_value);
}

static bool
io_binary_value_encode (vref_t r_value,io_encoding_t *encoding) {
	bool ok = false;

	if (is_io_text_encoding (encoding)) {
		io_binary_value_t const *this = vref_cast_to_ro_pointer(r_value);
		io_encoding_append_bytes (
			encoding,io_binary_value_ro_bytes(this),io_binary_value_size(this)
		);
		ok = true;
	}

	return ok;
}

EVENT_DATA io_value_implementation_t binary_value_implementation = {
	.specialisation_of = &univ_value_implementation,
	.encoding = decl_io_value_encoding (CR_BINARY_ENCODING_INDEX),
	.initialise = io_binary_value_initialise_nop,
	.free = io_value_free_nop,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_binary_value_t cr_binary_v = {
	decl_io_value (&binary_value_implementation,sizeof(io_binary_value_t))
	.bytes.ro = NULL,
	.bit.binary_size = 0,
	.bit.const_bytes = 1,
};

static io_value_t*
io_binary_value_initialise_with_const_bytes (vref_t r_value,vref_t r_base) {
	io_binary_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_binary_value_t const *base = io_typesafe_ro_cast(r_base,cr_CONSTANT_BINARY);

	if (base != NULL) {
		io_binary_value_ro_bytes(this) = io_binary_value_ro_bytes(base);
		this->bit = base->bit;
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static io_value_t*
io_binary_value_initialise_with_dynamic_bytes (vref_t r_value,vref_t r_base) {
	io_binary_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_binary_value_t const *base = io_typesafe_ro_cast(r_base,cr_CONSTANT_BINARY);

	if (base != NULL) {
		this->bit = base->bit;
		memcpy (this->inline_bytes,io_binary_value_ro_bytes(base),this->bit.binary_size);
		io_binary_value_rw_bytes(this) = this->inline_bytes;
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

EVENT_DATA io_value_implementation_t binary_value_implementation_with_const_bytes = {
	.specialisation_of = &binary_value_implementation,
	.encoding = decl_io_value_encoding (CR_CONST_BINARY_ENCODING_INDEX),
	.initialise = io_binary_value_initialise_with_const_bytes,
	.free = io_value_free_nop,
	.encode = io_binary_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_binary_value_t cr_const_binary_v = {
	decl_io_value (&binary_value_implementation_with_const_bytes,sizeof(io_binary_value_t))
	.bytes.ro = NULL,
	.bit.binary_size = 0,
	.bit.const_bytes = 1,
};

vref_t
mk_io_binary_value_with_const_bytes (io_value_memory_t *vm,uint8_t const *bytes,int32_t size) {
	io_binary_value_t base = {
		decl_io_value (
			&binary_value_implementation_with_const_bytes,sizeof(io_binary_value_t)
		)
		.bit = {
			.binary_size = size,
			.inline_bytes = false,
			.const_bytes = 1,
		},
		.bytes.ro = bytes,
	};
	return io_value_memory_new_value (
		vm,
		&binary_value_implementation_with_const_bytes,
		sizeof (io_binary_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

EVENT_DATA io_value_implementation_t binary_value_implementation_with_dynamic_bytes = {
	.specialisation_of = &binary_value_implementation_with_const_bytes,
	.initialise = io_binary_value_initialise_with_dynamic_bytes,
	.free = io_value_free_nop,
	.encode = io_binary_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};


static vref_t
mk_io_binary_value_for (io_value_memory_t *vm,io_value_implementation_t const *I,uint8_t const *bytes,int32_t size) {
	io_binary_value_t base = {
		decl_io_value (I,sizeof(io_binary_value_t))
		.bit = {
			.binary_size = size,
			.inline_bytes = false,
			.const_bytes = 0,
		},
		.bytes.ro = bytes,
	};
	return io_value_memory_new_value (
		vm,I,sizeof (io_binary_value_t) + size,def_vref (&reference_to_c_stack_value,&base)
	);
}

vref_t
mk_io_binary_value (io_value_memory_t *vm,uint8_t const *bytes,int32_t size) {
	return mk_io_binary_value_for (vm,&binary_value_implementation_with_dynamic_bytes,bytes,size);
}

static bool
io_text_value_encode (vref_t r_value,io_encoding_t *encoding) {
	bool ok = false;

	if (is_io_text_encoding (encoding)) {
		io_binary_value_t const *this = vref_cast_to_ro_pointer(r_value);
		io_encoding_append_bytes (
			encoding,io_binary_value_ro_bytes(this),io_binary_value_size(this)
		);
		ok = true;
	}
	return ok;
}

vref_t
io_text_to_text_value_decoder (io_encoding_t *encoding,io_value_memory_t *vm) {
	const uint8_t *b,*e;
	io_encoding_get_ro_bytes (encoding,&b,&e);
	return mk_io_text_value (vm,b,e - b);
}

EVENT_DATA io_value_implementation_t io_text_value_implementation = {
	.specialisation_of = &binary_value_implementation_with_dynamic_bytes,
	.encoding = decl_io_value_encoding (CR_TEXT_ENCODING_INDEX),
	.initialise = io_binary_value_initialise_with_dynamic_bytes,
	.free = io_value_free_nop,
	.encode = io_text_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};
EVENT_DATA io_binary_value_t cr_text_v = {
	decl_io_value (&io_text_value_implementation,sizeof(io_binary_value_t))
	.bytes.ro = (uint8_t const*) "text",
	.bit.binary_size = 4,
	.bit.const_bytes = 1,
};

vref_t
mk_io_text_value (io_value_memory_t *vm,uint8_t const *bytes,int32_t size) {
	return mk_io_binary_value_for (vm,&io_text_value_implementation,bytes,size);
}

EVENT_DATA io_value_implementation_t io_symbol_value_implementation_with_const_bytes = {
	.specialisation_of = &binary_value_implementation_with_const_bytes,
	.encoding = decl_io_value_encoding (CR_SYMBOL_ENCODING_INDEX),
	.initialise = io_binary_value_initialise_with_const_bytes,
	.free = io_value_free_nop,
	.encode = io_binary_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_binary_value_t cr_symbol_v = {
	decl_io_value (&io_symbol_value_implementation_with_const_bytes,sizeof(io_binary_value_t))
	.bytes.ro = (uint8_t const*) "symbol",
	.bit.binary_size = 6,
	.bit.const_bytes = 1,
};

EVENT_DATA io_value_implementation_t io_ion_exception_value_implementation_with_const_bytes = {
	.specialisation_of = &binary_value_implementation_with_const_bytes,
	.initialise = io_binary_value_initialise_with_const_bytes,
	.free = io_value_free_nop,
	.encode = io_binary_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_binary_value_t cr_io_exception_v = {
	decl_io_value (&io_ion_exception_value_implementation_with_const_bytes,sizeof(io_binary_value_t))
	.bytes.ro = (uint8_t const*) "exception",
	.bit.binary_size = 9,
	.bit.const_bytes = 1,
};

bool
io_value_is_io_exceptionl (vref_t r_arg) {
	return io_typesafe_ro_cast(r_arg,cr_IO_EXCEPTION) != NULL;
}

vref_t
mk_io_symbol_value_with_const_bytes (io_value_memory_t *vm,uint8_t const *bytes,int32_t size) {
	io_binary_value_t base = {
		decl_io_value (
			&io_symbol_value_implementation_with_const_bytes,sizeof(io_binary_value_t)
		)
		.bit = {
			.binary_size = size,
			.inline_bytes = false,
			.const_bytes = 1,
		},
		.bytes.ro = bytes,
	};
	return io_value_memory_new_value (
		vm,
		&io_symbol_value_implementation_with_const_bytes,
		sizeof (io_binary_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

EVENT_DATA io_value_implementation_t io_symbol_value_implementation_with_volatile_bytes = {
	.specialisation_of = &io_symbol_value_implementation_with_const_bytes,
	.initialise = io_binary_value_initialise_with_const_bytes,
	.free = io_value_free_nop,
	.encode = io_binary_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

io_value_t*
io_modal_value_initialise (vref_t r_value,vref_t r_base) {
	io_modal_value_t *this = vref_cast_to_rw_pointer(r_value);
	this->current_mode = io_value_get_modes((io_value_t*) this);
	return (io_value_t*) this;
}

io_value_mode_t const*
io_modal_value_get_modes (io_value_t const *this) {
	return modal_value_static_modes (this);
}

vref_t
io_modal_value_receive (io_t *io,vref_t r_this,uint32_t argc,vref_t const *args) {
	io_modal_value_t const *this = vref_cast_to_ro_pointer (r_this);
	io_signature_t const *sig;
	io_match_argument_t const *match;
	vref_t const *end_arg = args + argc;

	sig = this->current_mode->signature;
	while (sig->action) {
		vref_t const *arg = args;
		bool ok = true;

		match = sig->arg;

		while (arg < end_arg && *match && vref_is_valid(*arg)) {
			if (!(ok = (*match)(*arg))) {
				break;
			}
			match++;
			arg++;
		}

		if (ok && *match == NULL && arg == end_arg) {
			return sig->action (io,r_this,args);
		}
		sig++;
	}
	return cr_NIL;  // cr_CONTINUE_NO_MATCH;
}

/*
 *
 * Continuation (a value)
 *
 */
typedef struct ion_instruction {
	uint32_t op_code:8;
	uint32_t r0:12;
	uint32_t r1:12;
	uint32_t r2:12;
} ion_instruction_t;

typedef struct PACK_STRUCTURE ion_continuation_value {
	IO_VALUE_STRUCT_MEMBERS
	ion_instruction_t const *instructions;
	struct ion_vm_stack {
		vref_t *reg;
		uint32_t length;
	} stack;
	struct ion_vm_constants {
		vref_t *reg;
		uint32_t length;
	} constants;
} ion_continuation_value_t;


EVENT_DATA io_value_implementation_t ion_continuation_value_implementation = {
	.specialisation_of = &univ_value_implementation,
	.initialise = io_value_initialise_nop,
	.free = io_value_free_nop,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

//
// vector value
//
bool
io_vector_value_get_arity (vref_t r_value,uint32_t *arity) {
	io_vector_value_t const *this = io_typesafe_ro_cast (
		r_value,cr_VECTOR
	);
	if (this) {
		*arity = this->arity;
		return true;
	} else {
		return false;
	}
}

bool
io_vector_value_get_values (vref_t r_value,vref_t const** values) {
	io_vector_value_t const *this = io_typesafe_ro_cast (
		r_value,cr_VECTOR
	);
	if (this) {
		*values = this->values;
		return true;
	} else {
		return false;
	}
}

// needs byte memory
static io_value_t*
io_vector_value_initialise (vref_t r_value,vref_t r_base) {
/*	io_vector_value_t *this = (io_binary_value_t*) value;
	io_vector_value_t const *base = io_typesafe_ro_cast(r_base,cr_CONSTANT_BINARY);

	if (base != NULL) {

	} else {
		this = NULL;
	}
*/
	return vref_cast_to_rw_pointer(r_value);
}

static void
io_vector_value_free (io_value_t *value) {
}

vref_t
mk_io_vector_value (vref_t r_context,vref_t const *const* values) {
	return INVALID_VREF;
}

EVENT_DATA io_value_implementation_t io_vector_value_implementation = {
	.specialisation_of = &univ_value_implementation,
	.encoding = decl_io_value_encoding (CR_VECTOR_ENCODING_INDEX),
	.initialise = io_vector_value_initialise,
	.free = io_vector_value_free,
	.encode = default_io_value_encode,
	.receive = default_io_value_receive,
	.compare = NULL,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_vector_value_t cr_vector_v = {
	decl_io_value (&io_vector_value_implementation,sizeof(io_vector_value_t))
	.arity = 0,
};

#define SPECIALISE_RESULT_IMPLEMENTATION(S,E) \
	.specialisation_of = S, \
	.encoding = decl_io_value_encoding (E),\
	.initialise = io_value_initialise_nop, \
	.free = io_value_free_nop, \
	.encode = default_io_value_encode, \
	.receive = default_io_value_receive, \
	.compare = io_value_compare_no_comparison,\
	.get_modes = io_value_get_null_modes,\
	/**/

EVENT_DATA io_value_implementation_t io_result_value_implementation = {
	SPECIALISE_RESULT_IMPLEMENTATION(&univ_value_implementation,CR_RESULT_ENCODING_INDEX)
};
EVENT_DATA io_vector_value_t cr_result_v = {
	decl_io_value (&io_result_value_implementation,sizeof(io_value_t))
	.arity = 0,
};

EVENT_DATA io_value_implementation_t io_result_continue_value_implementation = {
	SPECIALISE_RESULT_IMPLEMENTATION(&io_result_value_implementation,CR_RESULT_CONTINUE_ENCODING_INDEX)
};
EVENT_DATA io_vector_value_t cr_result_continue_v = {
	decl_io_value (&io_result_continue_value_implementation,sizeof(io_value_t))
	.arity = 0,
};

#define SPECIALISE_COMPARE_IMPLEMENTATION(S,E) \
	.specialisation_of = S, \
	.encoding = decl_io_value_encoding (E),\
	.initialise = io_value_initialise_nop, \
	.free = io_value_free_nop, \
	.encode = default_io_value_encode, \
	.receive = default_io_value_receive, \
	.compare = io_value_compare_no_comparison,\
	.get_modes = io_value_get_null_modes,\
	/**/

EVENT_DATA io_value_implementation_t io_compare_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION(
		&univ_value_implementation,CR_COMPARE_ENCODING_INDEX
	)
};

EVENT_DATA io_value_t cr_compare_v = {
	decl_io_value (&io_compare_value_implementation,sizeof(io_value_t))
};

EVENT_DATA io_value_implementation_t io_compare_no_comparison_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION(
		&io_compare_value_implementation,CR_COMPARE_NO_COMPARISON_ENCODING_INDEX
	)
};

EVENT_DATA io_value_t cr_compare_no_comparison_v = {
	decl_io_value (&io_compare_no_comparison_value_implementation,sizeof(io_value_t))
};

EVENT_DATA io_value_implementation_t io_compare_more_than_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION(
		&io_compare_value_implementation,CR_COMPARE_MORE_THAN_ENCODING_INDEX
	)
};

EVENT_DATA io_value_t cr_compare_more_than_v = {
	decl_io_value (&io_compare_more_than_value_implementation,sizeof(io_value_t))
};

EVENT_DATA io_value_implementation_t io_compare_less_than_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION(
		&io_compare_value_implementation,CR_COMPARE_LESS_THAN_ENCODING_INDEX
	)
};

EVENT_DATA io_value_t cr_compare_less_than_v = {
	decl_io_value (&io_compare_less_than_value_implementation,sizeof(io_value_t))
};

EVENT_DATA io_value_implementation_t io_compare_equal_to_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION(
		&io_compare_value_implementation,CR_COMPARE_EQUAL_TO_ENCODING_INDEX
	)
};

EVENT_DATA io_value_t cr_compare_equal_to_v = {
	decl_io_value (&io_compare_equal_to_value_implementation,sizeof(io_value_t))
};

//
// Io language decoder
//

static bool io_source_decoder_push_context (io_source_decoder_t*,vref_t);

io_source_decoder_t*
mk_io_source_decoder (
	io_t *io,
	vref_t r_context,
	io_source_decoder_input_t parser,
	io_source_decoder_input_t on_error,
	is_symbol_t const *keywords
) {
	io_byte_memory_t *bm = io_get_byte_memory(io);
	io_source_decoder_t *this = io_byte_memory_allocate (
		bm,sizeof(io_source_decoder_t)
	);

	if (this) {
		this->io = io;
		this->buffer = mk_io_text_encoding (bm);
		this->error = NULL;
		this->reset_state = parser;
		this->error_state = on_error;
		this->keywords = keywords;

		this->context_stack = NULL;
		this->current_context = NULL;
		if (io_source_decoder_push_context (this,r_context)) {
			this->input_stack = io_byte_memory_allocate (
				io_source_decoder_byte_memory(this),sizeof(io_source_decoder_input_t*)
			);
			this->current_input = this->input_stack;
			if (!this->input_stack) {
				io_byte_memory_free (io_source_decoder_byte_memory(this),this->context_stack);
				io_byte_memory_free (io_source_decoder_byte_memory(this),this);
				this = NULL;
			}
		} else {
			io_byte_memory_free (io_source_decoder_byte_memory(this),this);
			this = NULL;
		}
		io_source_decoder_goto (this,this->reset_state);
	}

	return this;
}

void
free_io_source_decoder_parse_memory (io_source_decoder_t *this) {
	io_source_decoder_context_t *cursor = this->context_stack;
	while (cursor <= this->current_context) {
		unreference_value (cursor->r_value);
		io_byte_memory_free (io_source_decoder_byte_memory(this),cursor->args);
		cursor++;
	}
	io_byte_memory_free (io_source_decoder_byte_memory(this),this->context_stack);
	io_byte_memory_free (io_source_decoder_byte_memory(this),this->input_stack);
}

void
free_io_source_decoder (io_source_decoder_t *this) {
	free_io_source_decoder_parse_memory (this);
	io_encoding_free(this->buffer);
	io_byte_memory_free (io_source_decoder_byte_memory(this),this);
}

void
io_source_decoder_reset (io_source_decoder_t *this) {
	vref_t r_value = this->context_stack->r_value;

	io_encoding_reset (this->buffer);
	this->error = NULL;

	free_io_source_decoder_parse_memory (this);

	this->context_stack = NULL;
	this->current_context = NULL;
	this->input_stack = NULL;

	if (io_source_decoder_push_context (this,r_value)) {
		this->input_stack = io_byte_memory_allocate (
			io_source_decoder_byte_memory(this),sizeof(io_source_decoder_input_t*)
		);
		this->current_input = this->input_stack;
		if (this->input_stack) {
			io_source_decoder_goto (this,this->reset_state);
		} else {
			io_panic (io_source_decoder_io(this),IO_PANIC_OUT_OF_MEMORY);
		}
	}
}

void
io_source_decoder_remove_last_character (io_source_decoder_t *this) {
}

void
io_source_decoder_next_character (
	io_source_decoder_t *this,uint32_t character
) {
	io_source_decoder_input(this) (this,character);
}

void
io_source_decoder_parse (io_source_decoder_t *this,const char *src) {
	while(*src) {
		io_source_decoder_input(this) (this,*src++);
	}
}

void
io_source_decoder_push_input (io_source_decoder_t *this,io_source_decoder_input_t s) {
	uint32_t depth = io_source_decoder_input_depth(this);
	this->input_stack = io_byte_memory_reallocate (
		io_source_decoder_byte_memory(this),this->input_stack,(depth + 1) * sizeof(io_source_decoder_input_t*)
	);
	this->current_input = this->input_stack + depth;
	io_source_decoder_input(this) = (s);
}

void
io_source_decoder_pop_input (io_source_decoder_t *this) {
	uint32_t depth = io_source_decoder_input_depth(this);
	if (depth > 1) {
		this->input_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->input_stack,(depth - 1) * sizeof(io_source_decoder_input_t*)
		);
		this->current_input = this->input_stack + (depth - 2);
	} else {
		io_source_decoder_set_last_error (
			this,"no item to pop from stack"
		);
		io_source_decoder_goto(this,this->error_state);
	}
}


static bool
io_source_decoder_push_context (io_source_decoder_t *this,vref_t r_value) {
	if (this->context_stack == NULL) {
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->context_stack,sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack;
	} else {
		int32_t depth = this->current_context - this->context_stack;
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),this->context_stack,(depth + 1) * sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack + (depth + 1);
	}

	if (this->context_stack) {
		this->current_context->r_value = reference_value (r_value);
		this->current_context->args = NULL;
		this->current_context->arity = 0;
		return true;
	} else {
		return false;
	}
}

static void
io_source_decoder_pop_context (io_source_decoder_t *this) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);
	vref_t *arg = ctx->args, *end = arg + ctx->arity;
	uint32_t depth = this->current_context - this->context_stack;

	while (arg < end) {
		unreference_value (*arg);
		arg++;
	}

	io_byte_memory_free (io_source_decoder_byte_memory(this),ctx->args);
	ctx->args = NULL;
	ctx->arity = 0;

	if (depth > 0) {
		this->context_stack = io_byte_memory_reallocate (
			io_source_decoder_byte_memory(this),
			this->context_stack,
			(depth) * sizeof(io_source_decoder_context_t)
		);
		this->current_context = this->context_stack + (depth);
	}
}

bool
io_source_decoder_append_arg (io_source_decoder_t *this,vref_t r_value) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);

	ctx->args = io_byte_memory_reallocate (
		io_source_decoder_byte_memory(this),ctx->args,(ctx->arity + 1) * sizeof(vref_t)
	);

	if (ctx->args != NULL) {
		ctx->args[ctx->arity] = reference_value (r_value);
		ctx->arity ++;
		return true;
	} else {
		return false;
	}
}

void
io_source_decoder_end_of_statement (io_source_decoder_t *this) {
	io_source_decoder_context_t *ctx = io_source_decoder_context(this);
	if (ctx->arity) {
		io_value_send (io_source_decoder_io(this),ctx->r_value,ctx->arity,ctx->args);
		io_source_decoder_pop_context (this);
	} else {
		// do i pop ??
	}
}

//
// the byte memory manager
//
#define UMM_DBGLOG_DEBUG(...)
#define DBGLOG_TRACE(...)

#define UMM_FREELIST_MASK (0x8000)
#define UMM_BLOCKNO_MASK  (0x7FFF)


//umm_block_t *umm_heap = NULL;
//unsigned short int umm_numblocks = 0;

#define UMM_NUMBLOCKS(m) (m)->number_of_blocks
#define UMM_BLOCK(m,b)  (m)->heap[b]

#define UMM_NBLOCK(m,b) (UMM_BLOCK(m,b).header.used.next)
#define UMM_PBLOCK(m,b) (UMM_BLOCK(m,b).header.used.prev)
#define UMM_NFREE(m,b)  (UMM_BLOCK(m,b).body.free.next)
#define UMM_PFREE(m,b)  (UMM_BLOCK(m,b).body.free.prev)
#define UMM_DATA(m,b)   (UMM_BLOCK(m,b).body.data)

void
initialise_io_byte_memory (io_t *io,io_byte_memory_t *mem) {
  	// init heap pointer and size, and memset it to 0
	io_memset (mem->heap,0x00,mem->number_of_blocks * sizeof(umm_block_t));
	mem->io = io;
  /* setup initial blank heap structure */
  {
    /* index of the 0th `umm_block_t` */
    const unsigned short int block_0th = 0;
    /* index of the 1st `umm_block_t` */
    const unsigned short int block_1th = 1;
    /* index of the latest `umm_block_t` */
    const unsigned short int block_last = UMM_NUMBLOCKS(mem) - 1;

    /* setup the 0th `umm_block_t`, which just points to the 1st */
    UMM_NBLOCK(mem,block_0th) = block_1th;
    UMM_NFREE(mem,block_0th)  = block_1th;
    UMM_PFREE(mem,block_0th)  = block_1th;

    /*
     * Now, we need to set the whole heap space as a huge free block. We should
     * not touch the 0th `umm_block_t`, since it's special: the 0th `umm_block_t`
     * is the head of the free block list. It's a part of the heap invariant.
     *
     * See the detailed explanation at the beginning of the file.
     */

    /*
     * 1th `umm_block_t` has pointers:
     *
     * - next `umm_block_t`: the latest one
     * - prev `umm_block_t`: the 0th
     *
     * Plus, it's a free `umm_block_t`, so we need to apply `UMM_FREELIST_MASK`
     *
     * And it's the last free block, so the next free block is 0.
     */
    UMM_NBLOCK(mem,block_1th) = block_last | UMM_FREELIST_MASK;
    UMM_NFREE(mem,block_1th)  = 0;
    UMM_PBLOCK(mem,block_1th) = block_0th;
    UMM_PFREE(mem,block_1th)  = block_0th;

    /*
     * latest `umm_block_t` has pointers:
     *
     * - next `umm_block_t`: 0 (meaning, there are no more `umm_block_ts`)
     * - prev `umm_block_t`: the 1st
     *
     * It's not a free block, so we don't touch NFREE / PFREE at all.
     */
    UMM_NBLOCK(mem,block_last) = 0;
    UMM_PBLOCK(mem,block_last) = block_1th;
  }
}

void
io_byte_memory_get_info (io_byte_memory_t *mem,memory_info_t *info) {
	unsigned short int blockNo = UMM_NBLOCK(mem,0) & UMM_BLOCKNO_MASK;

	info->total_bytes = 0;
	info->used_bytes = 0;

	while( UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK ) {
		size_t curBlocks = (UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK ) - blockNo;

		if( UMM_NBLOCK(mem,blockNo) & UMM_FREELIST_MASK ) {
			// free
			info->total_bytes += curBlocks;
		} else {
			// allocated
			info->used_bytes += curBlocks;
		}
		blockNo = UMM_NBLOCK(mem,blockNo) & UMM_BLOCKNO_MASK;
	}

	info->total_bytes *= sizeof(umm_block_t);
	info->used_bytes *= sizeof(umm_block_t);
}

void
iterate_io_byte_memory_allocations (
	io_byte_memory_t *bm,bool (*cb) (void*,void*),void *user_value
) {
	unsigned short int blockNo = UMM_NBLOCK(bm,0) & UMM_BLOCKNO_MASK;

	while( UMM_NBLOCK(bm,blockNo) & UMM_BLOCKNO_MASK ) {
		if( UMM_NBLOCK(bm,blockNo) & UMM_FREELIST_MASK ) {
			if (!cb ((void *)&UMM_DATA(bm,blockNo),user_value)) {
				break;
			}
		}
		blockNo = UMM_NBLOCK(bm,blockNo) & UMM_BLOCKNO_MASK;
	}
}

static unsigned short int umm_blocks( size_t size ) {

  /*
   * The calculation of the block size is not too difficult, but there are
   * a few little things that we need to be mindful of.
   *
   * When a block removed from the free list, the space used by the free
   * pointers is available for data. That's what the first calculation
   * of size is doing.
   */

  if( size <= (sizeof(((umm_block_t *)0)->body)) )
    return( 1 );

  /*
   * If it's for more than that, then we need to figure out the number of
   * additional whole blocks the size of an umm_block_t are required.
   */

  size -= ( 1 + (sizeof(((umm_block_t *)0)->body)) );

  return( 2 + size/(sizeof(umm_block_t)) );
}

/* ------------------------------------------------------------------------ */
/*
 * Split the block `c` into two blocks: `c` and `c + blocks`.
 *
 * - `new_freemask` should be `0` if `c + blocks` used, or `UMM_FREELIST_MASK`
 *   otherwise.
 *
 * Note that free pointers are NOT modified by this function.
 */
static void
umm_split_block (
	io_byte_memory_t *mem,
	unsigned short int c,
    unsigned short int blocks,
    unsigned short int new_freemask
) {
  UMM_NBLOCK(mem,c+blocks) = (UMM_NBLOCK(mem,c) & UMM_BLOCKNO_MASK) | new_freemask;
  UMM_PBLOCK(mem,c+blocks) = c;

  UMM_PBLOCK(mem,UMM_NBLOCK(mem,c) & UMM_BLOCKNO_MASK) = (c+blocks);
  UMM_NBLOCK(mem,c) = (c+blocks);
}

/* ------------------------------------------------------------------------ */

static void umm_disconnect_from_free_list(io_byte_memory_t *mem,unsigned short int c ) {
  /* Disconnect this block from the FREE list */

  UMM_NFREE(mem,UMM_PFREE(mem,c)) = UMM_NFREE(mem,c);
  UMM_PFREE(mem,UMM_NFREE(mem,c)) = UMM_PFREE(mem,c);

  /* And clear the free block indicator */

  UMM_NBLOCK(mem,c) &= (~UMM_FREELIST_MASK);
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_up() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static void umm_assimilate_up(io_byte_memory_t *mem,unsigned short int c ) {

  if( UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_FREELIST_MASK ) {
    /*
     * The next block is a free block, so assimilate up and remove it from
     * the free list
     */

    UMM_DBGLOG_DEBUG( "Assimilate up to next block, which is FREE\n" );

    /* Disconnect the next block from the FREE list */

    umm_disconnect_from_free_list(mem, UMM_NBLOCK(mem,c) );

    /* Assimilate the next block with this one */

    UMM_PBLOCK(mem,UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK) = c;
    UMM_NBLOCK(mem,c) = UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK;
  }
}

/* ------------------------------------------------------------------------
 * The umm_assimilate_down() function assumes that UMM_NBLOCK(c) does NOT
 * have the UMM_FREELIST_MASK bit set!
 */

static unsigned short int umm_assimilate_down(io_byte_memory_t *mem,unsigned short int c, unsigned short int freemask ) {

  UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) = UMM_NBLOCK(mem,c) | freemask;
  UMM_PBLOCK(mem,UMM_NBLOCK(mem,c)) = UMM_PBLOCK(mem,c);

  return( UMM_PBLOCK(mem,c) );
}



/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY() and UMM_CRITICAL_EXIT().
 */

static void umm_free_core(io_byte_memory_t *mem,void *ptr) {

  unsigned short int c;

  /*
   * NOTE:  See the new umm_info() function that you can use to see if a ptr is
   *        on the free list!
   */

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (((char *)ptr)-(char *)(&(mem->heap[0])))/sizeof(umm_block_t);

  UMM_DBGLOG_DEBUG( "Freeing block %6i\n", c );

  /* Now let's assimilate this block with the next one if possible. */

  umm_assimilate_up(mem,c);

  /* Then assimilate with the previous block if possible */

  if( UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) & UMM_FREELIST_MASK ) {

    UMM_DBGLOG_DEBUG( "Assimilate down to next block, which is FREE\n" );

    c = umm_assimilate_down(mem,c, UMM_FREELIST_MASK);
  } else {
    /*
     * The previous block is not a free block, so add this one to the head
     * of the free list
     */

    UMM_DBGLOG_DEBUG( "Just add to head of free list\n" );

    UMM_PFREE(mem,UMM_NFREE(mem,0)) = c;
    UMM_NFREE(mem,c)            = UMM_NFREE(mem,0);
    UMM_PFREE(mem,c)            = 0;
    UMM_NFREE(mem,0)            = c;

    UMM_NBLOCK(mem,c)          |= UMM_FREELIST_MASK;
  }
}

/* ------------------------------------------------------------------------ */

void umm_free(io_byte_memory_t *mem,void *ptr) {


  /* If we're being asked to free a NULL pointer, well that's just silly! */

  if( (void *)0 == ptr ) {
    UMM_DBGLOG_DEBUG( "free a null pointer -> do nothing\n" );
    return;
  }

  /* Free the memory withing a protected critical section */

  UMM_CRITICAL_ENTRY(mem);

  umm_free_core(mem,ptr);

  UMM_CRITICAL_EXIT(mem);
}

/* ------------------------------------------------------------------------
 * Must be called only from within critical sections guarded by
 * UMM_CRITICAL_ENTRY() and UMM_CRITICAL_EXIT().
 */

static void *umm_malloc_core(io_byte_memory_t *mem,size_t size) {
  unsigned short int blocks;
  unsigned short int blockSize = 0;

  unsigned short int bestSize;
  unsigned short int bestBlock;

  unsigned short int cf;

  blocks = umm_blocks( size );

  /*
   * Now we can scan through the free list until we find a space that's big
   * enough to hold the number of blocks we need.
   *
   * This part may be customized to be a best-fit, worst-fit, or first-fit
   * algorithm
   */

  cf = UMM_NFREE(mem,0);

  bestBlock = UMM_NFREE(mem,0);
  bestSize  = 0x7FFF;

  while( cf ) {
    blockSize = (UMM_NBLOCK(mem,cf) & UMM_BLOCKNO_MASK) - cf;

    DBGLOG_TRACE( "Looking at block %6i size %6i\n", cf, blockSize );

#if defined UMM_BEST_FIT
    if( (blockSize >= blocks) && (blockSize < bestSize) ) {
      bestBlock = cf;
      bestSize  = blockSize;
    }
#elif defined UMM_FIRST_FIT
    /* This is the first block that fits! */
    if( (blockSize >= blocks) )
      break;
#else
#  error "No UMM_*_FIT is defined - check umm_malloc_cfg.h"
#endif

    cf = UMM_NFREE(mem,cf);
  }

  if( 0x7FFF != bestSize ) {
    cf        = bestBlock;
    blockSize = bestSize;
  }

  if( UMM_NBLOCK(mem,cf) & UMM_BLOCKNO_MASK && blockSize >= blocks ) {
    /*
     * This is an existing block in the memory heap, we just need to split off
     * what we need, unlink it from the free list and mark it as in use, and
     * link the rest of the block back into the freelist as if it was a new
     * block on the free list...
     */

    if( blockSize == blocks ) {
      /* It's an exact fit and we don't neet to split off a block. */
      UMM_DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - exact\n", blocks, cf );

      /* Disconnect this block from the FREE list */

      umm_disconnect_from_free_list(mem,cf );

    } else {
      /* It's not an exact fit and we need to split off a block. */
      UMM_DBGLOG_DEBUG( "Allocating %6i blocks starting at %6i - existing\n", blocks, cf );

      /*
       * split current free block `cf` into two blocks. The first one will be
       * returned to user, so it's not free, and the second one will be free.
       */
      umm_split_block(mem, cf, blocks, UMM_FREELIST_MASK /*new block is free*/ );

      /*
       * `umm_split_block()` does not update the free pointers (it affects
       * only free flags), but effectively we've just moved beginning of the
       * free block from `cf` to `cf + blocks`. So we have to adjust pointers
       * to and from adjacent free blocks.
       */

      /* previous free block */
      UMM_NFREE(mem,UMM_PFREE(mem,cf) ) = cf + blocks;
      UMM_PFREE(mem,cf + blocks ) = UMM_PFREE(mem,cf);

      /* next free block */
      UMM_PFREE(mem,UMM_NFREE(mem,cf) ) = cf + blocks;
      UMM_NFREE(mem,cf + blocks ) = UMM_NFREE(mem,cf);
    }
  } else {
    /* Out of memory */

    UMM_DBGLOG_DEBUG(  "Can't allocate %5i blocks\n", blocks );

    return( (void *)NULL );
  }

  return( (void *)&UMM_DATA(mem,cf) );
}

/* ------------------------------------------------------------------------ */

void *umm_malloc(io_byte_memory_t *mem,size_t size) {
  void *ptr = NULL;


  /*
   * the very first thing we do is figure out if we're being asked to allocate
   * a size of 0 - and if we are we'll simply return a null pointer. if not
   * then reduce the size by 1 byte so that the subsequent calculations on
   * the number of blocks to allocate are easier...
   */

  if( 0 == size ) {
    UMM_DBGLOG_DEBUG( "malloc a block of 0 bytes -> do nothing\n" );

    return( ptr );
  }

  /* Allocate the memory withing a protected critical section */

  UMM_CRITICAL_ENTRY(mem);

  ptr = umm_malloc_core(mem,size);

  UMM_CRITICAL_EXIT(mem);

  return( ptr );
}

/* ------------------------------------------------------------------------ */

void *umm_realloc(io_byte_memory_t *mem,void *ptr, size_t size) {

  unsigned short int blocks;
  unsigned short int blockSize;
  unsigned short int prevBlockSize = 0;
  unsigned short int nextBlockSize = 0;

  unsigned short int c;

  size_t curSize;

  /*
   * This code looks after the case of a NULL value for ptr. The ANSI C
   * standard says that if ptr is NULL and size is non-zero, then we've
   * got to work the same a malloc(). If size is also 0, then our version
   * of malloc() returns a NULL pointer, which is OK as far as the ANSI C
   * standard is concerned.
   */

  if( ((void *)NULL == ptr) ) {
    UMM_DBGLOG_DEBUG( "realloc the NULL pointer - call malloc()\n" );

    return umm_malloc(mem,size);
  }

  /*
   * Now we're sure that we have a non_NULL ptr, but we're not sure what
   * we should do with it. If the size is 0, then the ANSI C standard says that
   * we should operate the same as free.
   */

  if( 0 == size ) {
    UMM_DBGLOG_DEBUG( "realloc to 0 size, just free the block\n" );

    umm_free(mem,ptr);

    return( (void *)NULL );
  }

  /*
   * Otherwise we need to actually do a reallocation. A naiive approach
   * would be to malloc() a new block of the correct size, copy the old data
   * to the new block, and then free the old block.
   *
   * While this will work, we end up doing a lot of possibly unnecessary
   * copying. So first, let's figure out how many blocks we'll need.
   */

  blocks = umm_blocks( size );

  /* Figure out which block we're in. Note the use of truncated division... */

  c = (((char *)ptr)-(char *)(&(mem->heap[0])))/sizeof(umm_block_t);

  /* Figure out how big this block is ... the free bit is not set :-) */

  blockSize = (UMM_NBLOCK(mem,c) - c);

  /* Figure out how many bytes are in this block */

  curSize   = (blockSize*sizeof(umm_block_t))-(sizeof(((umm_block_t *)0)->header));

  /* Protect the critical section... */
  UMM_CRITICAL_ENTRY(mem);

  /* Now figure out if the previous and/or next blocks are free as well as
   * their sizes - this will help us to minimize special code later when we
   * decide if it's possible to use the adjacent blocks.
   *
   * We set prevBlockSize and nextBlockSize to non-zero values ONLY if they
   * are free!
   */

  if ((UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_FREELIST_MASK)) {
      nextBlockSize = (UMM_NBLOCK(mem,UMM_NBLOCK(mem,c)) & UMM_BLOCKNO_MASK) - UMM_NBLOCK(mem,c);
  }

  if ((UMM_NBLOCK(mem,UMM_PBLOCK(mem,c)) & UMM_FREELIST_MASK)) {
      prevBlockSize = (c - UMM_PBLOCK(mem,c));
  }

  UMM_DBGLOG_DEBUG( "realloc blocks %i blockSize %i nextBlockSize %i prevBlockSize %i\n", blocks, blockSize, nextBlockSize, prevBlockSize );

  /*
   * Ok, now that we're here we know how many blocks we want and the current
   * blockSize. The prevBlockSize and nextBlockSize are set and we can figure
   * out the best strategy for the new allocation as follows:
   *
   * 1. If the new block is the same size or smaller than the current block do
   *    nothing.
   * 2. If the next block is free and adding it to the current block gives us
   *    enough memory, assimilate the next block.
   * 3. If the prev block is free and adding it to the current block gives us
   *    enough memory, remove the previous block from the free list, assimilate
   *    it, copy to the new block.
   * 4. If the prev and next blocks are free and adding them to the current
   *    block gives us enough memory, assimilate the next block, remove the
   *    previous block from the free list, assimilate it, copy to the new block.
   * 5. Otherwise try to allocate an entirely new block of memory. If the
   *    allocation works free the old block and return the new pointer. If
   *    the allocation fails, return NULL and leave the old block intact.
   *
   * All that's left to do is decide if the fit was exact or not. If the fit
   * was not exact, then split the memory block so that we use only the requested
   * number of blocks and add what's left to the free list.
   */

    if (blockSize >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc the same or smaller size block - %i, do nothing\n", blocks );
        /* This space intentionally left blank */
    } else if ((blockSize + nextBlockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using next block - %i\n", blocks );
        umm_assimilate_up(mem,c);
        blockSize += nextBlockSize;
    } else if ((prevBlockSize + blockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using prev block - %i\n", blocks );
        umm_disconnect_from_free_list(mem,UMM_PBLOCK(mem,c) );
        c = umm_assimilate_down(mem,c, 0);
        memmove( (void *)&UMM_DATA(mem,c), ptr, curSize );
        ptr = (void *)&UMM_DATA(mem,c);
        blockSize += prevBlockSize;
    } else if ((prevBlockSize + blockSize + nextBlockSize) >= blocks) {
        UMM_DBGLOG_DEBUG( "realloc using prev and next block - %i\n", blocks );
        umm_assimilate_up(mem,c );
        umm_disconnect_from_free_list(mem,UMM_PBLOCK(mem,c) );
        c = umm_assimilate_down(mem,c, 0);
        memmove( (void *)&UMM_DATA(mem,c), ptr, curSize );
        ptr = (void *)&UMM_DATA(mem,c);
        blockSize += (prevBlockSize + nextBlockSize);
    } else {
        UMM_DBGLOG_DEBUG( "realloc a completely new block %i\n", blocks );
        void *oldptr = ptr;
        if( (ptr = umm_malloc_core(mem,size )) ) {
            UMM_DBGLOG_DEBUG( "realloc %i to a bigger block %i, copy, and free the old\n", blockSize, blocks );
            memcpy( ptr, oldptr, curSize );
            umm_free_core(mem,oldptr );
        } else {
            UMM_DBGLOG_DEBUG(
            	"realloc %i to a bigger block %i failed - return NULL and leave the old block!\n",
				blockSize, blocks
            );
            /* This space intentionally left blank */
        }
        blockSize = blocks;
    }

    /* Now all we need to do is figure out if the block fit exactly or if we
     * need to split and free ...
     */

    if (blockSize > blocks ) {
        UMM_DBGLOG_DEBUG( "split and free %i blocks from %i\n", blocks, blockSize );
        umm_split_block(mem,c, blocks, 0 );
        umm_free_core(mem, (void *)&UMM_DATA(mem,c+blocks) );
    }

    /* Release the critical section... */
    UMM_CRITICAL_EXIT(mem);

    return( ptr );
}

void *umm_calloc(io_byte_memory_t *mem,size_t num, size_t item_size ) {
	void *ret = umm_malloc (mem,(size_t)(item_size * num));

	if (ret) {
	  io_memset (ret, 0x00, (size_t)(item_size * num));
	}

	return ret;
}
#ifdef STB_SPRINTF_IMPLEMENTATION
//
// this lib can use unaligned word access which is generally not good for arm cpus
//
#define STB_SPRINTF_NOUNALIGNED

#define stbsp__uint32 unsigned int
#define stbsp__int32 signed int

#ifdef _MSC_VER
#define stbsp__uint64 unsigned __int64
#define stbsp__int64 signed __int64
#else
#define stbsp__uint64 unsigned long long
#define stbsp__int64 signed long long
#endif
#define stbsp__uint16 unsigned short

#ifndef stbsp__uintptr
#if defined(__ppc64__) || defined(__aarch64__) || defined(_M_X64) || defined(__x86_64__) || defined(__x86_64)
#define stbsp__uintptr stbsp__uint64
#else
#define stbsp__uintptr stbsp__uint32
#endif
#endif

#ifndef STB_SPRINTF_MSVC_MODE // used for MSVC2013 and earlier (MSVC2015 matches GCC)
#if defined(_MSC_VER) && (_MSC_VER < 1900)
#define STB_SPRINTF_MSVC_MODE
#endif
#endif

#ifdef STB_SPRINTF_NOUNALIGNED // define this before inclusion to force stbsp_sprintf to always use aligned accesses
#define STBSP__UNALIGNED(code)
#else
#define STBSP__UNALIGNED(code) code
#endif

#ifndef STB_SPRINTF_NOFLOAT
// internal float utility functions
static stbsp__int32 stbsp__real_to_str(char const **start, stbsp__uint32 *len, char *out, stbsp__int32 *decimal_pos, double value, stbsp__uint32 frac_digits);
static stbsp__int32 stbsp__real_to_parts(stbsp__int64 *bits, stbsp__int32 *expo, double value);
#define STBSP__SPECIAL 0x7000
#endif

static char stbsp__period = '.';
static char stbsp__comma = ',';
static struct
{
   short temp; // force next field to be 2-byte aligned
   char pair[201];
} stbsp__digitpair =
{
  0,
   "00010203040506070809101112131415161718192021222324"
   "25262728293031323334353637383940414243444546474849"
   "50515253545556575859606162636465666768697071727374"
   "75767778798081828384858687888990919293949596979899"
};

STBSP__PUBLICDEF void STB_SPRINTF_DECORATE(set_separators)(char pcomma, char pperiod)
{
   stbsp__period = pperiod;
   stbsp__comma = pcomma;
}

#define STBSP__LEFTJUST 1
#define STBSP__LEADINGPLUS 2
#define STBSP__LEADINGSPACE 4
#define STBSP__LEADING_0X 8
#define STBSP__LEADINGZERO 16
#define STBSP__INTMAX 32
#define STBSP__TRIPLET_COMMA 64
#define STBSP__NEGATIVE 128
#define STBSP__METRIC_SUFFIX 256
#define STBSP__HALFWIDTH 512
#define STBSP__METRIC_NOSPACE 1024
#define STBSP__METRIC_1024 2048
#define STBSP__METRIC_JEDEC 4096

static void stbsp__lead_sign(stbsp__uint32 fl, char *sign)
{
   sign[0] = 0;
   if (fl & STBSP__NEGATIVE) {
      sign[0] = 1;
      sign[1] = '-';
   } else if (fl & STBSP__LEADINGSPACE) {
      sign[0] = 1;
      sign[1] = ' ';
   } else if (fl & STBSP__LEADINGPLUS) {
      sign[0] = 1;
      sign[1] = '+';
   }
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintfcb)(STBSP_SPRINTFCB *callback, void *user, char *buf, char const *fmt, va_list va)
{
   static char hex[] = "0123456789abcdefxp";
   static char hexu[] = "0123456789ABCDEFXP";
   char *bf;
   char const *f;
   int tlen = 0;

   bf = buf;
   f = fmt;
   for (;;) {
      stbsp__int32 fw, pr, tz;
      stbsp__uint32 fl;

      // macros for the callback buffer stuff
      #define stbsp__chk_cb_bufL(bytes)                        \
         {                                                     \
            int len = (int)(bf - buf);                         \
            if ((len + (bytes)) >= STB_SPRINTF_MIN) {          \
               tlen += len;                                    \
               if (0 == (bf = buf = callback(buf, user, len))) \
                  goto done;                                   \
            }                                                  \
         }
      #define stbsp__chk_cb_buf(bytes)    \
         {                                \
            if (callback) {               \
               stbsp__chk_cb_bufL(bytes); \
            }                             \
         }
      #define stbsp__flush_cb()                      \
         {                                           \
            stbsp__chk_cb_bufL(STB_SPRINTF_MIN - 1); \
         } // flush if there is even one byte in the buffer
      #define stbsp__cb_buf_clamp(cl, v)                \
         cl = v;                                        \
         if (callback) {                                \
            int lg = STB_SPRINTF_MIN - (int)(bf - buf); \
            if (cl > lg)                                \
               cl = lg;                                 \
         }

      // fast copy everything up to the next % (or end of string)
      for (;;) {
         while (((stbsp__uintptr)f) & 3) {
         schk1:
            if (f[0] == '%')
               goto scandd;
         schk2:
            if (f[0] == 0)
               goto endfmt;
            stbsp__chk_cb_buf(1);
            *bf++ = f[0];
            ++f;
         }
         for (;;) {
            // Check if the next 4 bytes contain %(0x25) or end of string.
            // Using the 'hasless' trick:
            // https://graphics.stanford.edu/~seander/bithacks.html#HasLessInWord
            stbsp__uint32 v, c;
            v = *(stbsp__uint32 *)f;
            c = (~v) & 0x80808080;
            if (((v ^ 0x25252525) - 0x01010101) & c)
               goto schk1;
            if ((v - 0x01010101) & c)
               goto schk2;
            if (callback)
               if ((STB_SPRINTF_MIN - (int)(bf - buf)) < 4)
                  goto schk1;
            #ifdef STB_SPRINTF_NOUNALIGNED
                if(((stbsp__uintptr)bf) & 3) {
                    bf[0] = f[0];
                    bf[1] = f[1];
                    bf[2] = f[2];
                    bf[3] = f[3];
                } else
            #endif
            {
                *(stbsp__uint32 *)bf = v;
            }
            bf += 4;
            f += 4;
         }
      }
   scandd:

      ++f;

      // ok, we have a percent, read the modifiers first
      fw = 0;
      pr = -1;
      fl = 0;
      tz = 0;

      // flags
      for (;;) {
         switch (f[0]) {
         // if we have left justify
         case '-':
            fl |= STBSP__LEFTJUST;
            ++f;
            continue;
         // if we have leading plus
         case '+':
            fl |= STBSP__LEADINGPLUS;
            ++f;
            continue;
         // if we have leading space
         case ' ':
            fl |= STBSP__LEADINGSPACE;
            ++f;
            continue;
         // if we have leading 0x
         case '#':
            fl |= STBSP__LEADING_0X;
            ++f;
            continue;
         // if we have thousand commas
         case '\'':
            fl |= STBSP__TRIPLET_COMMA;
            ++f;
            continue;
         // if we have kilo marker (none->kilo->kibi->jedec)
         case '$':
            if (fl & STBSP__METRIC_SUFFIX) {
               if (fl & STBSP__METRIC_1024) {
                  fl |= STBSP__METRIC_JEDEC;
               } else {
                  fl |= STBSP__METRIC_1024;
               }
            } else {
               fl |= STBSP__METRIC_SUFFIX;
            }
            ++f;
            continue;
         // if we don't want space between metric suffix and number
         case '_':
            fl |= STBSP__METRIC_NOSPACE;
            ++f;
            continue;
         // if we have leading zero
         case '0':
            fl |= STBSP__LEADINGZERO;
            ++f;
            goto flags_done;
         default: goto flags_done;
         }
      }
   flags_done:

      // get the field width
      if (f[0] == '*') {
         fw = va_arg(va, stbsp__uint32);
         ++f;
      } else {
         while ((f[0] >= '0') && (f[0] <= '9')) {
            fw = fw * 10 + f[0] - '0';
            f++;
         }
      }
      // get the precision
      if (f[0] == '.') {
         ++f;
         if (f[0] == '*') {
            pr = va_arg(va, stbsp__uint32);
            ++f;
         } else {
            pr = 0;
            while ((f[0] >= '0') && (f[0] <= '9')) {
               pr = pr * 10 + f[0] - '0';
               f++;
            }
         }
      }

      // handle integer size overrides
      switch (f[0]) {
      // are we halfwidth?
      case 'h':
         fl |= STBSP__HALFWIDTH;
         ++f;
         break;
      // are we 64-bit (unix style)
      case 'l':
         fl |= ((sizeof(long) == 8) ? STBSP__INTMAX : 0);
         ++f;
         if (f[0] == 'l') {
            fl |= STBSP__INTMAX;
            ++f;
         }
         break;
      // are we 64-bit on intmax? (c99)
      case 'j':
         fl |= (sizeof(size_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      // are we 64-bit on size_t or ptrdiff_t? (c99)
      case 'z':
         fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      case 't':
         fl |= (sizeof(ptrdiff_t) == 8) ? STBSP__INTMAX : 0;
         ++f;
         break;
      // are we 64-bit (msft style)
      case 'I':
         if ((f[1] == '6') && (f[2] == '4')) {
            fl |= STBSP__INTMAX;
            f += 3;
         } else if ((f[1] == '3') && (f[2] == '2')) {
            f += 3;
         } else {
            fl |= ((sizeof(void *) == 8) ? STBSP__INTMAX : 0);
            ++f;
         }
         break;
      default: break;
      }

      // handle each replacement
      switch (f[0]) {
         #define STBSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
         char num[STBSP__NUMSZ];
         char lead[8];
         char tail[8];
         char *s;
         char const *h;
         stbsp__uint32 l, n, cs;
         stbsp__uint64 n64;
#ifndef STB_SPRINTF_NOFLOAT
         double fv;
#endif
         stbsp__int32 dp;
         char const *sn;

      case 's':
         // get the string
         s = va_arg(va, char *);
         if (s == 0)
            s = (char *)"null";
         // get the length
         sn = s;
         for (;;) {
            if ((((stbsp__uintptr)sn) & 3) == 0)
               break;
         lchk:
            if (sn[0] == 0)
               goto ld;
            ++sn;
         }
         n = 0xffffffff;
         if (pr >= 0) {
            n = (stbsp__uint32)(sn - s);
            if (n >= (stbsp__uint32)pr)
               goto ld;
            n = ((stbsp__uint32)(pr - n)) >> 2;
         }
         while (n) {
            stbsp__uint32 v = *(stbsp__uint32 *)sn;
            if ((v - 0x01010101) & (~v) & 0x80808080UL)
               goto lchk;
            sn += 4;
            --n;
         }
         goto lchk;
      ld:

         l = (stbsp__uint32)(sn - s);
         // clamp to precision
         if (l > (stbsp__uint32)pr)
            l = pr;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         // copy the string in
         goto scopy;

      case 'c': // char
         // get the character
         s = num + STBSP__NUMSZ - 1;
         *s = (char)va_arg(va, int);
         l = 1;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;

      case 'n': // weird write-bytes specifier
      {
         int *d = va_arg(va, int *);
         *d = tlen + (int)(bf - buf);
      } break;

#ifdef STB_SPRINTF_NOFLOAT
      case 'A':              // float
      case 'a':              // hex float
      case 'G':              // float
      case 'g':              // float
      case 'E':              // float
      case 'e':              // float
      case 'f':              // float
         va_arg(va, double); // eat it
         s = (char *)"No float";
         l = 8;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;
#else
      case 'A': // hex float
      case 'a': // hex float
         h = (f[0] == 'A') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_parts((stbsp__int64 *)&n64, &dp, fv))
            fl |= STBSP__NEGATIVE;

         s = num + 64;

         stbsp__lead_sign(fl, lead);

         if (dp == -1023)
            dp = (n64) ? -1022 : 0;
         else
            n64 |= (((stbsp__uint64)1) << 52);
         n64 <<= (64 - 56);
         if (pr < 15)
            n64 += ((((stbsp__uint64)8) << 56) >> (pr * 4));
// add leading chars

#ifdef STB_SPRINTF_MSVC_MODE
         *s++ = '0';
         *s++ = 'x';
#else
         lead[1 + lead[0]] = '0';
         lead[2 + lead[0]] = 'x';
         lead[0] += 2;
#endif
         *s++ = h[(n64 >> 60) & 15];
         n64 <<= 4;
         if (pr)
            *s++ = stbsp__period;
         sn = s;

         // print the bits
         n = pr;
         if (n > 13)
            n = 13;
         if (pr > (stbsp__int32)n)
            tz = pr - n;
         pr = 0;
         while (n--) {
            *s++ = h[(n64 >> 60) & 15];
            n64 <<= 4;
         }

         // print the expo
         tail[1] = h[17];
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';
         n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }

         dp = (int)(s - sn);
         l = (int)(s - (num + 64));
         s = num + 64;
         cs = 1 + (3 << 24);
         goto scopy;

      case 'G': // float
      case 'g': // float
         h = (f[0] == 'G') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6;
         else if (pr == 0)
            pr = 1; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000))
            fl |= STBSP__NEGATIVE;

         // clamp the precision and delete extra zeros after clamp
         n = pr;
         if (l > (stbsp__uint32)pr)
            l = pr;
         while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
            --pr;
            --l;
         }

         // should we use %e
         if ((dp <= -4) || (dp > (stbsp__int32)n)) {
            if (pr > (stbsp__int32)l)
               pr = l - 1;
            else if (pr)
               --pr; // when using %e, there is one digit before the decimal
            goto doexpfromg;
         }
         // this is the insane action to get the pr to match %g semantics for %f
         if (dp > 0) {
            pr = (dp < (stbsp__int32)l) ? l - dp : 0;
         } else {
            pr = -dp + ((pr > (stbsp__int32)l) ? (stbsp__int32) l : pr);
         }
         goto dofloatfromg;

      case 'E': // float
      case 'e': // float
         h = (f[0] == 'E') ? hexu : hex;
         fv = va_arg(va, double);
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000))
            fl |= STBSP__NEGATIVE;
      doexpfromg:
         tail[0] = 0;
         stbsp__lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;
         // handle leading chars
         *s++ = sn[0];

         if (pr)
            *s++ = stbsp__period;

         // handle after decimal
         if ((l - 1) > (stbsp__uint32)pr)
            l = pr + 1;
         for (n = 1; n < l; n++)
            *s++ = sn[n];
         // trailing zeros
         tz = pr - (l - 1);
         pr = 0;
         // dump expo
         tail[1] = h[0xe];
         dp -= 1;
         if (dp < 0) {
            tail[2] = '-';
            dp = -dp;
         } else
            tail[2] = '+';
#ifdef STB_SPRINTF_MSVC_MODE
         n = 5;
#else
         n = (dp >= 100) ? 5 : 4;
#endif
         tail[0] = (char)n;
         for (;;) {
            tail[n] = '0' + dp % 10;
            if (n <= 3)
               break;
            --n;
            dp /= 10;
         }
         cs = 1 + (3 << 24); // how many tens
         goto flt_lead;

      case 'f': // float
         fv = va_arg(va, double);
      doafloat:
         // do kilos
         if (fl & STBSP__METRIC_SUFFIX) {
            double divisor;
            divisor = 1000.0f;
            if (fl & STBSP__METRIC_1024)
               divisor = 1024.0;
            while (fl < 0x4000000) {
               if ((fv < divisor) && (fv > -divisor))
                  break;
               fv /= divisor;
               fl += 0x1000000;
            }
         }
         if (pr == -1)
            pr = 6; // default is 6
         // read the double into a string
         if (stbsp__real_to_str(&sn, &l, num, &dp, fv, pr))
            fl |= STBSP__NEGATIVE;
      dofloatfromg:
         tail[0] = 0;
         stbsp__lead_sign(fl, lead);
         if (dp == STBSP__SPECIAL) {
            s = (char *)sn;
            cs = 0;
            pr = 0;
            goto scopy;
         }
         s = num + 64;

         // handle the three decimal varieties
         if (dp <= 0) {
            stbsp__int32 i;
            // handle 0.000*000xxxx
            *s++ = '0';
            if (pr)
               *s++ = stbsp__period;
            n = -dp;
            if ((stbsp__int32)n > pr)
               n = pr;
            i = n;
            while (i) {
               if ((((stbsp__uintptr)s) & 3) == 0)
                  break;
               *s++ = '0';
               --i;
            }
            while (i >= 4) {
               *(stbsp__uint32 *)s = 0x30303030;
               s += 4;
               i -= 4;
            }
            while (i) {
               *s++ = '0';
               --i;
            }
            if ((stbsp__int32)(l + n) > pr)
               l = pr - n;
            i = l;
            while (i) {
               *s++ = *sn++;
               --i;
            }
            tz = pr - (n + l);
            cs = 1 + (3 << 24); // how many tens did we write (for commas below)
         } else {
            cs = (fl & STBSP__TRIPLET_COMMA) ? ((600 - (stbsp__uint32)dp) % 3) : 0;
            if ((stbsp__uint32)dp >= l) {
               // handle xxxx000*000.0
               n = 0;
               for (;;) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = stbsp__comma;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= l)
                        break;
                  }
               }
               if (n < (stbsp__uint32)dp) {
                  n = dp - n;
                  if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                     while (n) {
                        if ((((stbsp__uintptr)s) & 3) == 0)
                           break;
                        *s++ = '0';
                        --n;
                     }
                     while (n >= 4) {
                        *(stbsp__uint32 *)s = 0x30303030;
                        s += 4;
                        n -= 4;
                     }
                  }
                  while (n) {
                     if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                        cs = 0;
                        *s++ = stbsp__comma;
                     } else {
                        *s++ = '0';
                        --n;
                     }
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr) {
                  *s++ = stbsp__period;
                  tz = pr;
               }
            } else {
               // handle xxxxx.xxxx000*000
               n = 0;
               for (;;) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (++cs == 4)) {
                     cs = 0;
                     *s++ = stbsp__comma;
                  } else {
                     *s++ = sn[n];
                     ++n;
                     if (n >= (stbsp__uint32)dp)
                        break;
                  }
               }
               cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
               if (pr)
                  *s++ = stbsp__period;
               if ((l - dp) > (stbsp__uint32)pr)
                  l = pr + dp;
               while (n < l) {
                  *s++ = sn[n];
                  ++n;
               }
               tz = pr - (l - dp);
            }
         }
         pr = 0;

         // handle k,m,g,t
         if (fl & STBSP__METRIC_SUFFIX) {
            char idx;
            idx = 1;
            if (fl & STBSP__METRIC_NOSPACE)
               idx = 0;
            tail[0] = idx;
            tail[1] = ' ';
            {
               if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                  if (fl & STBSP__METRIC_1024)
                     tail[idx + 1] = "_KMGT"[fl >> 24];
                  else
                     tail[idx + 1] = "_kMGT"[fl >> 24];
                  idx++;
                  // If printing kibits and not in jedec, add the 'i'.
                  if (fl & STBSP__METRIC_1024 && !(fl & STBSP__METRIC_JEDEC)) {
                     tail[idx + 1] = 'i';
                     idx++;
                  }
                  tail[0] = idx;
               }
            }
         };

      flt_lead:
         // get the length that we copied
         l = (stbsp__uint32)(s - (num + 64));
         s = num + 64;
         goto scopy;
#endif

      case 'B': // upper binary
      case 'b': // lower binary
         h = (f[0] == 'B') ? hexu : hex;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[0xb];
         }
         l = (8 << 4) | (1 << 8);
         goto radixnum;

      case 'o': // octal
         h = hexu;
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 1;
            lead[1] = '0';
         }
         l = (3 << 4) | (3 << 8);
         goto radixnum;

      case 'p': // pointer
         fl |= (sizeof(void *) == 8) ? STBSP__INTMAX : 0;
         pr = sizeof(void *) * 2;
         fl &= ~STBSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                    // fall through - to X

      case 'X': // upper hex
      case 'x': // lower hex
         h = (f[0] == 'X') ? hexu : hex;
         l = (4 << 4) | (4 << 8);
         lead[0] = 0;
         if (fl & STBSP__LEADING_0X) {
            lead[0] = 2;
            lead[1] = '0';
            lead[2] = h[16];
         }
      radixnum:
         // get the number
         if (fl & STBSP__INTMAX)
            n64 = va_arg(va, stbsp__uint64);
         else
            n64 = va_arg(va, stbsp__uint32);

         s = num + STBSP__NUMSZ;
         dp = 0;
         // clear tail, and clear leading if value is zero
         tail[0] = 0;
         if (n64 == 0) {
            lead[0] = 0;
            if (pr == 0) {
               l = 0;
               cs = (((l >> 4) & 15)) << 24;
               goto scopy;
            }
         }
         // convert to string
         for (;;) {
            *--s = h[n64 & ((1 << (l >> 8)) - 1)];
            n64 >>= (l >> 8);
            if (!((n64) || ((stbsp__int32)((num + STBSP__NUMSZ) - s) < pr)))
               break;
            if (fl & STBSP__TRIPLET_COMMA) {
               ++l;
               if ((l & 15) == ((l >> 4) & 15)) {
                  l &= ~15;
                  *--s = stbsp__comma;
               }
            }
         };
         // get the tens and the comma pos
         cs = (stbsp__uint32)((num + STBSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
         // get the length that we copied
         l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
         // copy it
         goto scopy;

      //
      // special case added to support io values
      //
      case 'v':
      	  {
       		struct io_text_encoding_print_data *data = user;
			io_encoding_t *venc = mk_io_text_encoding (io_binary_encoding_byte_memory(data->this));
			vref_t r_value = va_arg(va, vref_t);
			if (io_value_encode (r_value,venc)) {
				const uint8_t *s,*e;
				io_encoding_get_ro_bytes (venc,&s,&e);
				 // copy the string
				 n = e - s;
				 while (n) {
					stbsp__int32 i;
					stbsp__cb_buf_clamp(i, n);
					n -= i;
					STBSP__UNALIGNED(while (i >= 4) {
					   *(stbsp__uint32 *)bf = *(stbsp__uint32 *)s;
					   bf += 4;
					   s += 4;
					   i -= 4;
					})
					while (i) {
					   *bf++ = *s++;
					   --i;
					}
					stbsp__chk_cb_buf(1);
				 }
				 /*
				*/
			}
			io_encoding_free(venc);
      	  }
          break;

      case 'u': // unsigned
      case 'i':
      case 'd': // integer
         // get the integer and abs it
         if (fl & STBSP__INTMAX) {
            stbsp__int64 i64 = va_arg(va, stbsp__int64);
            n64 = (stbsp__uint64)i64;
            if ((f[0] != 'u') && (i64 < 0)) {
               n64 = (stbsp__uint64)-i64;
               fl |= STBSP__NEGATIVE;
            }
         } else {
            stbsp__int32 i = va_arg(va, stbsp__int32);
            n64 = (stbsp__uint32)i;
            if ((f[0] != 'u') && (i < 0)) {
               n64 = (stbsp__uint32)-i;
               fl |= STBSP__NEGATIVE;
            }
         }

#ifndef STB_SPRINTF_NOFLOAT
         if (fl & STBSP__METRIC_SUFFIX) {
            if (n64 < 1024)
               pr = 0;
            else if (pr == -1)
               pr = 1;
            fv = (double)(stbsp__int64)n64;
            goto doafloat;
         }
#endif

         // convert to string
         s = num + STBSP__NUMSZ;
         l = 0;

         for (;;) {
            // do in 32-bit chunks (avoid lots of 64-bit divides even with constant denominators)
            char *o = s - 8;
            if (n64 >= 100000000) {
               n = (stbsp__uint32)(n64 % 100000000);
               n64 /= 100000000;
            } else {
               n = (stbsp__uint32)n64;
               n64 = 0;
            }
            if ((fl & STBSP__TRIPLET_COMMA) == 0) {
               do {
                  s -= 2;
                  *(stbsp__uint16 *)s = *(stbsp__uint16 *)&stbsp__digitpair.pair[(n % 100) * 2];
                  n /= 100;
               } while (n);
            }
            while (n) {
               if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = stbsp__comma;
                  --o;
               } else {
                  *--s = (char)(n % 10) + '0';
                  n /= 10;
               }
            }
            if (n64 == 0) {
               if ((s[0] == '0') && (s != (num + STBSP__NUMSZ)))
                  ++s;
               break;
            }
            while (s != o)
               if ((fl & STBSP__TRIPLET_COMMA) && (l++ == 3)) {
                  l = 0;
                  *--s = stbsp__comma;
                  --o;
               } else {
                  *--s = '0';
               }
         }

         tail[0] = 0;
         stbsp__lead_sign(fl, lead);

         // get the length that we copied
         l = (stbsp__uint32)((num + STBSP__NUMSZ) - s);
         if (l == 0) {
            *--s = '0';
            l = 1;
         }
         cs = l + (3 << 24);
         if (pr < 0)
            pr = 0;

      scopy:
         // get fw=leading/trailing space, pr=leading zeros
         if (pr < (stbsp__int32)l)
            pr = l;
         n = pr + lead[0] + tail[0] + tz;
         if (fw < (stbsp__int32)n)
            fw = n;
         fw -= n;
         pr -= l;

         // handle right justify and leading zeros
         if ((fl & STBSP__LEFTJUST) == 0) {
            if (fl & STBSP__LEADINGZERO) // if leading zeros, everything is in pr
            {
               pr = (fw > pr) ? fw : pr;
               fw = 0;
            } else {
               fl &= ~STBSP__TRIPLET_COMMA; // if no leading zeros, then no commas
            }
         }

         // copy the spaces and/or zeros
         if (fw + pr) {
            stbsp__int32 i;
            stbsp__uint32 c;

            // copy leading spaces (or when doing %8.4d stuff)
            if ((fl & STBSP__LEFTJUST) == 0)
               while (fw > 0) {
                  stbsp__cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i) {
                     *bf++ = ' ';
                     --i;
                  }
                  stbsp__chk_cb_buf(1);
               }

            // copy leader
            sn = lead + 1;
            while (lead[0]) {
               stbsp__cb_buf_clamp(i, lead[0]);
               lead[0] -= (char)i;
               while (i) {
                  *bf++ = *sn++;
                  --i;
               }
               stbsp__chk_cb_buf(1);
            }

            // copy leading zeros
            c = cs >> 24;
            cs &= 0xffffff;
            cs = (fl & STBSP__TRIPLET_COMMA) ? ((stbsp__uint32)(c - ((pr + cs) % (c + 1)))) : 0;
            while (pr > 0) {
               stbsp__cb_buf_clamp(i, pr);
               pr -= i;
               if ((fl & STBSP__TRIPLET_COMMA) == 0) {
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = '0';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x30303030;
                     bf += 4;
                     i -= 4;
                  }
               }
               while (i) {
                  if ((fl & STBSP__TRIPLET_COMMA) && (cs++ == c)) {
                     cs = 0;
                     *bf++ = stbsp__comma;
                  } else
                     *bf++ = '0';
                  --i;
               }
               stbsp__chk_cb_buf(1);
            }
         }

         // copy leader if there is still one
         sn = lead + 1;
         while (lead[0]) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, lead[0]);
            lead[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy the string
         n = l;
         while (n) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, n);
            n -= i;
            STBSP__UNALIGNED(while (i >= 4) {
               *(stbsp__uint32 *)bf = *(stbsp__uint32 *)s;
               bf += 4;
               s += 4;
               i -= 4;
            })
            while (i) {
               *bf++ = *s++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy trailing zeros
         while (tz) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, tz);
            tz -= i;
            while (i) {
               if ((((stbsp__uintptr)bf) & 3) == 0)
                  break;
               *bf++ = '0';
               --i;
            }
            while (i >= 4) {
               *(stbsp__uint32 *)bf = 0x30303030;
               bf += 4;
               i -= 4;
            }
            while (i) {
               *bf++ = '0';
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // copy tail if there is one
         sn = tail + 1;
         while (tail[0]) {
            stbsp__int32 i;
            stbsp__cb_buf_clamp(i, tail[0]);
            tail[0] -= (char)i;
            while (i) {
               *bf++ = *sn++;
               --i;
            }
            stbsp__chk_cb_buf(1);
         }

         // handle the left justify
         if (fl & STBSP__LEFTJUST)
            if (fw > 0) {
               while (fw) {
                  stbsp__int32 i;
                  stbsp__cb_buf_clamp(i, fw);
                  fw -= i;
                  while (i) {
                     if ((((stbsp__uintptr)bf) & 3) == 0)
                        break;
                     *bf++ = ' ';
                     --i;
                  }
                  while (i >= 4) {
                     *(stbsp__uint32 *)bf = 0x20202020;
                     bf += 4;
                     i -= 4;
                  }
                  while (i--)
                     *bf++ = ' ';
                  stbsp__chk_cb_buf(1);
               }
            }
         break;

      default: // unknown, just copy code
         s = num + STBSP__NUMSZ - 1;
         *s = f[0];
         l = 1;
         fw = fl = 0;
         lead[0] = 0;
         tail[0] = 0;
         pr = 0;
         dp = 0;
         cs = 0;
         goto scopy;
      }
      ++f;
   }
endfmt:

   if (!callback)
      *bf = 0;
   else
      stbsp__flush_cb();

done:
   return tlen + (int)(bf - buf);
}

// cleanup
#undef STBSP__LEFTJUST
#undef STBSP__LEADINGPLUS
#undef STBSP__LEADINGSPACE
#undef STBSP__LEADING_0X
#undef STBSP__LEADINGZERO
#undef STBSP__INTMAX
#undef STBSP__TRIPLET_COMMA
#undef STBSP__NEGATIVE
#undef STBSP__METRIC_SUFFIX
#undef STBSP__NUMSZ
#undef stbsp__chk_cb_bufL
#undef stbsp__chk_cb_buf
#undef stbsp__flush_cb
#undef stbsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(sprintf)(char *buf, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);
   result = STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
   va_end(va);
   return result;
}

typedef struct stbsp__context {
   char *buf;
   int count;
   char tmp[STB_SPRINTF_MIN];
} stbsp__context;

static char *stbsp__clamp_callback(char *buf, void *user, int len)
{
   stbsp__context *c = (stbsp__context *)user;

   if (len > c->count)
      len = c->count;

   if (len) {
      if (buf != c->buf) {
         char *s, *d, *se;
         d = c->buf;
         s = buf;
         se = buf + len;
         do {
            *d++ = *s++;
         } while (s < se);
      }
      c->buf += len;
      c->count -= len;
   }

   if (c->count <= 0)
      return 0;
   return (c->count >= STB_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}

static char * stbsp__count_clamp_callback( char * buf, void * user, int len )
{
   stbsp__context * c = (stbsp__context*)user;

   c->count += len;
   return c->tmp; // go direct into buffer if you can
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE( vsnprintf )( char * buf, int count, char const * fmt, va_list va )
{
   stbsp__context c;
   int l;

   if ( (count == 0) && !buf )
   {
      c.count = 0;

      STB_SPRINTF_DECORATE( vsprintfcb )( stbsp__count_clamp_callback, &c, c.tmp, fmt, va );
      l = c.count;
   }
   else
   {
      if ( count == 0 )
         return 0;

      c.buf = buf;
      c.count = count;

      STB_SPRINTF_DECORATE( vsprintfcb )( stbsp__clamp_callback, &c, stbsp__clamp_callback(0,&c,0), fmt, va );

      // zero-terminate
      l = (int)( c.buf - buf );
      if ( l >= count ) // should never be greater, only equal (or less) than count
         l = count - 1;
      buf[l] = 0;
   }

   return l;
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(snprintf)(char *buf, int count, char const *fmt, ...)
{
   int result;
   va_list va;
   va_start(va, fmt);

   result = STB_SPRINTF_DECORATE(vsnprintf)(buf, count, fmt, va);
   va_end(va);

   return result;
}

STBSP__PUBLICDEF int STB_SPRINTF_DECORATE(vsprintf)(char *buf, char const *fmt, va_list va)
{
   return STB_SPRINTF_DECORATE(vsprintfcb)(0, 0, buf, fmt, va);
}

// =======================================================================
//   low level float utility functions

#ifndef STB_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define STBSP__COPYFP(dest, src)                   \
   {                                               \
      int cn;                                      \
      for (cn = 0; cn < 8; cn++)                   \
         ((char *)&dest)[cn] = ((char *)&src)[cn]; \
   }

// get float info
static stbsp__int32 stbsp__real_to_parts(stbsp__int64 *bits, stbsp__int32 *expo, double value)
{
   double d;
   stbsp__int64 b = 0;

   // load value and round at the frac_digits
   d = value;

   STBSP__COPYFP(b, d);

   *bits = b & ((((stbsp__uint64)1) << 52) - 1);
   *expo = (stbsp__int32)(((b >> 52) & 2047) - 1023);

   return (stbsp__int32)((stbsp__uint64) b >> 63);
}

static double const stbsp__bot[23] = {
   1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
   1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022
};
static double const stbsp__negbot[22] = {
   1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006, 1e-007, 1e-008, 1e-009, 1e-010, 1e-011,
   1e-012, 1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018, 1e-019, 1e-020, 1e-021, 1e-022
};
static double const stbsp__negboterr[22] = {
   -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020, -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
   4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026, -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
   -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032, 2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
   2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,  -4.8596774326570872e-039
};
static double const stbsp__top[13] = {
   1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161, 1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299
};
static double const stbsp__negtop[13] = {
   1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161, 1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299
};
static double const stbsp__toperr[13] = {
   8388608,
   6.8601809640529717e+028,
   -7.253143638152921e+052,
   -4.3377296974619174e+075,
   -1.5559416129466825e+098,
   -3.2841562489204913e+121,
   -3.7745893248228135e+144,
   -1.7356668416969134e+167,
   -3.8893577551088374e+190,
   -9.9566444326005119e+213,
   6.3641293062232429e+236,
   -5.2069140800249813e+259,
   -5.2504760255204387e+282
};
static double const stbsp__negtoperr[13] = {
   3.9565301985100693e-040,  -2.299904345391321e-063,  3.6506201437945798e-086,  1.1875228833981544e-109,
   -5.0644902316928607e-132, -6.7156837247865426e-155, -2.812077463003139e-178,  -5.7778912386589953e-201,
   7.4997100559334532e-224,  -4.6439668915134491e-247, -6.3691100762962136e-270, -9.436808465446358e-293,
   8.0970921678014997e-317
};

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
static stbsp__uint64 const stbsp__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000,
   100000000000,
   1000000000000,
   10000000000000,
   100000000000000,
   1000000000000000,
   10000000000000000,
   100000000000000000,
   1000000000000000000,
   10000000000000000000U
};
#define stbsp__tento19th ((stbsp__uint64)1000000000000000000)
#else
static stbsp__uint64 const stbsp__powten[20] = {
   1,
   10,
   100,
   1000,
   10000,
   100000,
   1000000,
   10000000,
   100000000,
   1000000000,
   10000000000ULL,
   100000000000ULL,
   1000000000000ULL,
   10000000000000ULL,
   100000000000000ULL,
   1000000000000000ULL,
   10000000000000000ULL,
   100000000000000000ULL,
   1000000000000000000ULL,
   10000000000000000000ULL
};
#define stbsp__tento19th (1000000000000000000ULL)
#endif

#define stbsp__ddmulthi(oh, ol, xh, yh)                            \
   {                                                               \
      double ahi = 0, alo, bhi = 0, blo;                           \
      stbsp__int64 bt;                                             \
      oh = xh * yh;                                                \
      STBSP__COPYFP(bt, xh);                                       \
      bt &= ((~(stbsp__uint64)0) << 27);                           \
      STBSP__COPYFP(ahi, bt);                                      \
      alo = xh - ahi;                                              \
      STBSP__COPYFP(bt, yh);                                       \
      bt &= ((~(stbsp__uint64)0) << 27);                           \
      STBSP__COPYFP(bhi, bt);                                      \
      blo = yh - bhi;                                              \
      ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo; \
   }

#define stbsp__ddtoS64(ob, xh, xl)          \
   {                                        \
      double ahi = 0, alo, vh, t;           \
      ob = (stbsp__int64)ph;                \
      vh = (double)ob;                      \
      ahi = (xh - vh);                      \
      t = (ahi - xh);                       \
      alo = (xh - (ahi - t)) - (vh + t);    \
      ob += (stbsp__int64)(ahi + alo + xl); \
   }

#define stbsp__ddrenorm(oh, ol) \
   {                            \
      double s;                 \
      s = oh + ol;              \
      ol = ol - (s - oh);       \
      oh = s;                   \
   }

#define stbsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define stbsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);
// power can be -323 to +350
static void stbsp__raise_to_power10(double *ohi, double *olo, double d, stbsp__int32 power) 
{
   double ph, pl;
   if ((power >= 0) && (power <= 22)) {
      stbsp__ddmulthi(ph, pl, d, stbsp__bot[power]);
   } else {
      stbsp__int32 e, et, eb;
      double p2h, p2l;

      e = power;
      if (power < 0)
         e = -e;
      et = (e * 0x2c9) >> 14; /* %23 */
      if (et > 13)
         et = 13;
      eb = e - (et * 23);

      ph = d;
      pl = 0.0;
      if (power < 0) {
         if (eb) {
            --eb;
            stbsp__ddmulthi(ph, pl, d, stbsp__negbot[eb]);
            stbsp__ddmultlos(ph, pl, d, stbsp__negboterr[eb]);
         }
         if (et) {
            stbsp__ddrenorm(ph, pl);
            --et;
            stbsp__ddmulthi(p2h, p2l, ph, stbsp__negtop[et]);
            stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__negtop[et], stbsp__negtoperr[et]);
            ph = p2h;
            pl = p2l;
         }
      } else {
         if (eb) {
            e = eb;
            if (eb > 22)
               eb = 22;
            e -= eb;
            stbsp__ddmulthi(ph, pl, d, stbsp__bot[eb]);
            if (e) {
               stbsp__ddrenorm(ph, pl);
               stbsp__ddmulthi(p2h, p2l, ph, stbsp__bot[e]);
               stbsp__ddmultlos(p2h, p2l, stbsp__bot[e], pl);
               ph = p2h;
               pl = p2l;
            }
         }
         if (et) {
            stbsp__ddrenorm(ph, pl);
            --et;
            stbsp__ddmulthi(p2h, p2l, ph, stbsp__top[et]);
            stbsp__ddmultlo(p2h, p2l, ph, pl, stbsp__top[et], stbsp__toperr[et]);
            ph = p2h;
            pl = p2l;
         }
      }
   }
   stbsp__ddrenorm(ph, pl);
   *ohi = ph;
   *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e), or in 0x80000000
static stbsp__int32 stbsp__real_to_str(char const **start, stbsp__uint32 *len, char *out, stbsp__int32 *decimal_pos, double value, stbsp__uint32 frac_digits)
{
   double d;
   stbsp__int64 bits = 0;
   stbsp__int32 expo, e, ng, tens;

   d = value;
   STBSP__COPYFP(bits, d);
   expo = (stbsp__int32)((bits >> 52) & 2047);
   ng = (stbsp__int32)((stbsp__uint64) bits >> 63);
   if (ng)
      d = -d;

   if (expo == 2047) // is nan or inf?
   {
      *start = (bits & ((((stbsp__uint64)1) << 52) - 1)) ? "NaN" : "Inf";
      *decimal_pos = STBSP__SPECIAL;
      *len = 3;
      return ng;
   }

   if (expo == 0) // is zero or denormal
   {
      if ((bits << 1) == 0) // do zero
      {
         *decimal_pos = 1;
         *start = out;
         out[0] = '0';
         *len = 1;
         return ng;
      }
      // find the right expo for denormals
      {
         stbsp__int64 v = ((stbsp__uint64)1) << 51;
         while ((bits & v) == 0) {
            --expo;
            v >>= 1;
         }
      }
   }

   // find the decimal exponent as well as the decimal bits of the value
   {
      double ph, pl;

      // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of log10 of all expos 1..2046
      tens = expo - 1023;
      tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

      // move the significant bits into position and stick them into an int
      stbsp__raise_to_power10(&ph, &pl, d, 18 - tens);

      // get full as much precision from double-double as possible
      stbsp__ddtoS64(bits, ph, pl);

      // check if we undershot
      if (((stbsp__uint64)bits) >= stbsp__tento19th)
         ++tens;
   }

   // now do the rounding in integer land
   frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1) : (tens + frac_digits);
   if ((frac_digits < 24)) {
      stbsp__uint32 dg = 1;
      if ((stbsp__uint64)bits >= stbsp__powten[9])
         dg = 10;
      while ((stbsp__uint64)bits >= stbsp__powten[dg]) {
         ++dg;
         if (dg == 20)
            goto noround;
      }
      if (frac_digits < dg) {
         stbsp__uint64 r;
         // add 0.5 at the right position and round
         e = dg - frac_digits;
         if ((stbsp__uint32)e >= 24)
            goto noround;
         r = stbsp__powten[e];
         bits = bits + (r / 2);
         if ((stbsp__uint64)bits >= stbsp__powten[dg])
            ++tens;
         bits /= r;
      }
   noround:;
   }

   // kill long trailing runs of zeros
   if (bits) {
      stbsp__uint32 n;
      for (;;) {
         if (bits <= 0xffffffff)
            break;
         if (bits % 1000)
            goto donez;
         bits /= 1000;
      }
      n = (stbsp__uint32)bits;
      while ((n % 1000) == 0)
         n /= 1000;
      bits = n;
   donez:;
   }

   // convert to string
   out += 64;
   e = 0;
   for (;;) {
      stbsp__uint32 n;
      char *o = out - 8;
      // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant denomiators be damned)
      if (bits >= 100000000) {
         n = (stbsp__uint32)(bits % 100000000);
         bits /= 100000000;
      } else {
         n = (stbsp__uint32)bits;
         bits = 0;
      }
      while (n) {
         out -= 2;
         *(stbsp__uint16 *)out = *(stbsp__uint16 *)&stbsp__digitpair.pair[(n % 100) * 2];
         n /= 100;
         e += 2;
      }
      if (bits == 0) {
         if ((e) && (out[0] == '0')) {
            ++out;
            --e;
         }
         break;
      }
      while (out != o) {
         *--out = '0';
         ++e;
      }
   }

   *decimal_pos = tens;
   *start = out;
   *len = e;
   return ng;
}

#undef stbsp__ddmulthi
#undef stbsp__ddrenorm
#undef stbsp__ddmultlo
#undef stbsp__ddmultlos
#undef STBSP__SPECIAL
#undef STBSP__COPYFP

#endif // STB_SPRINTF_NOFLOAT

// clean up
#undef stbsp__uint16
#undef stbsp__uint32
#undef stbsp__int32
#undef stbsp__uint64
#undef stbsp__int64
#undef STBSP__UNALIGNED

#endif // STB_SPRINTF_IMPLEMENTATION
//
// order MUST match declaration order in io_value_encoding_identifier_t
//
EVENT_DATA io_value_implementation_t const* io_value_implementation_enum[] = {
	IO_VALUE_IMPLEMENTATION(&nil_value_implementation),
	IO_VALUE_IMPLEMENTATION(&univ_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_number_value_implementation),
	IO_VALUE_IMPLEMENTATION(&i64_number_value_implementation),
	IO_VALUE_IMPLEMENTATION(&f64_number_value_implementation),
	IO_VALUE_IMPLEMENTATION(&binary_value_implementation),
	IO_VALUE_IMPLEMENTATION(&binary_value_implementation_with_const_bytes),
	IO_VALUE_IMPLEMENTATION(&io_text_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_symbol_value_implementation_with_const_bytes),
	IO_VALUE_IMPLEMENTATION(&io_vector_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_result_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_result_continue_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_compare_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_compare_no_comparison_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_compare_more_than_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_compare_less_than_value_implementation),
	IO_VALUE_IMPLEMENTATION(&io_compare_equal_to_value_implementation),
};
#endif /* IMPLEMENT_IO_CORE */
#ifdef IMPLEMENT_VERIFY_IO_CORE
#include <verify_io.h>
#endif /* IMPLEMENT_ION_VERIFY */
#endif /* io_core_H_ */
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
/*
------------------------------------------------------------------------------
stb_printf software used under license
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
/*
------------------------------------------------------------------------------
umm software used under license

Copyright (c) 2015 Ralph Hempel

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/
