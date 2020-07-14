/*
 *
 * NB: included by io_core.h
 *
 */
#ifndef io_value_H_
#define io_value_H_
#include <io_math.h>

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

enum {
	IO_VALUE_ENCODING_FORMAT_X70,
	
	NUMBER_OF_IO_VALUE_ENCODING_FORMATS
} io_value_encoding_format_t;

#define IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_value_implementation_t const *specialisation_of; \
	const char *name;\
	io_value_t* (*initialise) (vref_t,vref_t); \
	void (*free) (io_value_t*); \
	bool (*encode) (vref_t,io_encoding_t*); \
	vref_t (*decode[NUMBER_OF_IO_VALUE_ENCODING_FORMATS]) (io_value_memory_t *vm,uint8_t const**,const uint8_t*);\
	vref_t (*receive) (io_t*,vref_t,uint32_t,vref_t const*); \
	vref_t (*compare) (io_value_t const*,vref_t); \
	io_value_mode_t const* (*get_modes) (io_value_t const*);\
	/**/

struct PACK_STRUCTURE io_value_implementation {
	IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES
};

#define IO_VALUE_IMPLEMENTATION(I) ((io_value_implementation_t const*) (I))

#define IO_VALUE_STRUCT_MEMBERS \
	io_value_implementation_t const *implementation; \
	struct PACK_STRUCTURE {\
		uint16_t reference_count_; \
		uint16_t size_; \
	} tag_;\
	/**/

struct PACK_STRUCTURE io_value {
	IO_VALUE_STRUCT_MEMBERS
};

#define io_value_implementation(v)		((v)->implementation)
#define io_value_reference_count(v)		((v)->tag_.reference_count_)
#define io_value_size(v)					((v)->tag_.size_)

#define decl_io_value(T,S) \
	.implementation = T, \
	.tag_.reference_count_ = 0, \
	.tag_.size_ = S, \
	/**/
	
void const*	io_typesafe_ro_cast (vref_t,vref_t);
void const* io_typesafe_ro_cast_to_type (io_value_t const*,io_value_implementation_t const*);
void io_value_free_nop (io_value_t*);
bool default_io_value_encode (vref_t,io_encoding_t*);
vref_t default_io_value_receive (io_t*,vref_t,uint32_t,vref_t const*);
io_value_t* io_value_initialise_nop (vref_t,vref_t);
io_value_mode_t const* io_value_get_null_modes (io_value_t const*);
vref_t io_value_compare_no_comparison(io_value_t const*,vref_t);
vref_t io_value_send (io_t *io,vref_t r_value,uint32_t argc,...);
vref_t io_decode_x70_to_invalid_value (io_value_memory_t*,uint8_t const**,const uint8_t*);
vref_t decode_x70_to_io_value (io_value_memory_t*,uint8_t const**,const uint8_t*);
bool io_value_encode_base (vref_t,io_encoding_t*);
vref_t io_value_compare_with_value (io_value_t const*,vref_t);

#define SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.specialisation_of = S, \
	.initialise = io_value_initialise_nop,\
	.free = io_value_free_nop, \
	.decode = {decode_x70_to_io_value}, \
	.encode = io_value_encode_base, \
	.receive = default_io_value_receive, \
	.compare = io_value_compare_with_value, \
	.get_modes = io_value_get_null_modes, \
	/**/

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
io_value_sendm (io_t *io,vref_t r_value,uint32_t argc,vref_t const *args) {
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

INLINE_FUNCTION vref_t
compare_io_values (vref_t r_value,vref_t r_other) {
	io_value_t const *v = vref_cast_to_ro_pointer (r_value);
	return io_value_compare (v,r_other);
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

//
// modal value
//
#define IO_MODAL_VALUE_IMPLEMENTATION_STRUCT_MEMBERS \
	IO_VALUE_IMPLEMENTATION_STRUCT_PROPERTIES \
	io_value_mode_t const *modes; \
	/**/

typedef struct PACK_STRUCTURE io_modal_value_implementation {
	IO_MODAL_VALUE_IMPLEMENTATION_STRUCT_MEMBERS
} io_modal_value_implementation_t;

#define modal_value_static_modes(this) ((io_modal_value_implementation_t const*) (this)->implementation)->modes

#define IO_MODAL_VALUE_STRUCT_MEMBERS \
	IO_VALUE_STRUCT_MEMBERS \
	io_value_mode_t const *current_mode;\
	/**/

typedef struct PACK_STRUCTURE io_modal_value {
	IO_MODAL_VALUE_STRUCT_MEMBERS
} io_modal_value_t;

vref_t io_modal_value_receive (io_t*,vref_t,uint32_t,vref_t const*);
io_value_t*	io_modal_value_initialise (vref_t,vref_t);
io_value_mode_t const* io_modal_value_get_modes (io_value_t const*);

extern EVENT_DATA io_value_mode_t null_io_modal_value_modes[];

#define SPECIALISE_IO_MODAL_VALUE_IMPLEMENTATION(S) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.initialise = io_modal_value_initialise,	\
	.receive = io_modal_value_receive,\
	.get_modes = io_modal_value_get_modes,\
	.modes = null_io_modal_value_modes,\
	/**/

#define decl_modal_value_implementation_type(Init,MODES,Imp) \
	.specialisation_of = IO_VALUE_IMPLEMENTATION(Imp),	\
	.initialise = Init,	\
	.free = io_value_free_nop,	\
	.encode = default_io_value_encode,	\
	.receive = io_modal_value_receive,\
	.compare = io_value_compare_no_comparison, \
	.get_modes = io_modal_value_get_modes,\
	.modes = MODES,\
	/**/

#define decl_modal_value_implementation(Init,MODES) \
	decl_modal_value_implementation_type(Init,MODES,&io_modal_value_implementation)

#define decl_io_modal_value(T,S) \
	.implementation = IO_VALUE_IMPLEMENTATION(T), \
	.reference_count_ = 0, \
	.size_ = S, \
	.current_mode = (T)->modes, \
	/**/

#define decl_io_modal_value_m(T,S,M) \
	.implementation = IO_VALUE_IMPLEMENTATION(T), \
	.reference_count_ = 0, \
	.size_ = S, \
	.current_mode = M, \
	/**/
	
extern EVENT_DATA io_modal_value_implementation_t io_modal_value_implementation;


#ifndef IMPLEMENT_IO_CORE
# define decl_particular_value(REF_NAME,VALUE_TYPE,VALUE_VAR) \
	extern EVENT_DATA vref_t REF_NAME;\
	bool io_value_is_##REF_NAME (vref_t);\
	/**/

# define decl_particular_data_value	decl_particular_value

# define def_constant_symbol(VAR_NAME,S,len) \
	extern EVENT_DATA vref_t VAR_NAME;\
	extern EVENT_DATA io_binary_value_t __##VAR_NAME;\
	extern vref_t is_symbol_##VAR_NAME (char const*,size_t);\
	bool io_value_is_##VAR_NAME (vref_t);\
	/**/

#else /* implementing */
# define decl_particular_value_R(REF_NAME,VALUE_TYPE,VALUE_VAR,RI) \
	extern EVENT_DATA VALUE_TYPE VALUE_VAR;\
	EVENT_DATA vref_t REF_NAME = def_vref (RI,&VALUE_VAR);\
	bool io_value_is_##REF_NAME (vref_t r_value) {\
		return io_typesafe_ro_cast(r_value,REF_NAME);\
	}\
	/**/

# define decl_particular_value(REF_NAME,VALUE_TYPE,VALUE_VAR) \
	decl_particular_value_R(REF_NAME,VALUE_TYPE,VALUE_VAR,&reference_to_constant_value)

# define decl_particular_data_value(REF_NAME,VALUE_TYPE,VALUE_VAR) \
	extern VALUE_TYPE VALUE_VAR;\
	EVENT_DATA vref_t REF_NAME = def_vref (&reference_to_data_section_value,&VALUE_VAR);\
	bool io_value_is_##REF_NAME (vref_t r_value) {\
		return io_typesafe_ro_cast(r_value,REF_NAME);\
	}

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
	}

#endif /* IMPLEMENT_IO_CORE */

//
// value
//

extern EVENT_DATA io_value_implementation_t io_value_implementation;
decl_particular_value(cr_VALUE,io_value_t,cr_value_v)

//
// nil
//
typedef struct PACK_STRUCTURE io_nil_value {
	IO_VALUE_STRUCT_MEMBERS
} io_nil_value_t;
decl_particular_value(cr_NIL,io_nil_value_t,cr_nil_v)
//
// modal value
//

//
// number
//

typedef struct PACK_STRUCTURE io_number_value {
	IO_VALUE_STRUCT_MEMBERS
} io_number_value_t;
decl_particular_value(cr_NUMBER,io_number_value_t,cr_number_v)

#define SPECIALISE_IO_NUMBER_VALUE_IMPLEMENTATION(S) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.decode = {io_decode_x70_to_invalid_value}, \
	/**/

//
// int64
//
typedef struct PACK_STRUCTURE io_int64_value {
	IO_VALUE_STRUCT_MEMBERS
	int64_t value;
} io_int64_value_t;

vref_t mk_io_int64_value (io_value_memory_t *vm,int64_t value);
bool io_value_get_as_int64 (vref_t,int64_t*);
vref_t io_integer_to_integer_value_decoder (io_encoding_t*,io_value_memory_t*);
vref_t io_text_to_number_value_decoder (io_encoding_t*,io_value_memory_t*);

decl_particular_value(cr_I64_NUMBER,io_int64_value_t,cr_i64_number_v)

//
// float64
//
typedef struct PACK_STRUCTURE io_float64_value {
	IO_VALUE_STRUCT_MEMBERS
	float64_t value;
} io_float64_value_t;

vref_t mk_io_float64_value (io_value_memory_t *vm,float64_t value);
bool io_value_get_as_float64 (vref_t,float64_t*);

decl_particular_value(cr_F64_NUMBER,io_float64_value_t,cr_f64_number_v)


//
// binary
//
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

vref_t mk_io_binary_value_with_const_bytes (io_value_memory_t*,uint8_t const*,int32_t);
vref_t mk_io_binary_value (io_value_memory_t*,uint8_t const*,int32_t);
vref_t mk_io_text_value (io_value_memory_t*,uint8_t const*,int32_t);
vref_t io_text_to_text_value_decoder (io_encoding_t*,io_value_memory_t*);

decl_particular_value(cr_BINARY,				io_binary_value_t,cr_binary_v)
decl_particular_value(cr_CONSTANT_BINARY,	io_binary_value_t,cr_const_binary_v)
decl_particular_value(cr_IO_EXCEPTION,		io_binary_value_t,cr_io_exception_v)
decl_particular_value(cr_SYMBOL,				io_binary_value_t,cr_symbol_v)
decl_particular_value(cr_TEXT,				io_binary_value_t,cr_text_v)

#define SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION(S) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.initialise = io_binary_value_initialise_nop, \
	.encode = io_binary_value_encode, \
	.decode = {io_binary_decode_x70_value}, \
	.compare = compare_binary_with_other, \
	/**/


//
// symbol
//

//
// result
//

#define SPECIALISE_RESULT_IMPLEMENTATION(S,N) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.name = N,	\
	/**/

//
// vector
//

#define IO_VECTOR_VALUE_STRUCT_MEMBERS \
	IO_VALUE_STRUCT_MEMBERS \
	uint32_t arity; \
	/**/

typedef struct PACK_STRUCTURE {
	IO_VECTOR_VALUE_STRUCT_MEMBERS
	vref_t values[];
} io_vector_value_t;

vref_t	mk_io_vector_value (io_value_memory_t*,uint32_t,vref_t const*);
bool		io_vector_value_get_arity (vref_t,uint32_t*);
bool		io_vector_value_get_values (vref_t,uint32_t*,vref_t const**);

decl_particular_value(cr_VECTOR,				io_vector_value_t,cr_vector_v)
decl_particular_value(cr_RESULT,				io_vector_value_t,cr_result_v)
decl_particular_value(cr_RESULT_CONTINUE,	io_vector_value_t,cr_result_continue_v)

//
// comparison
//
decl_particular_value(cr_COMPARE,			io_value_t,cr_compare_v)
decl_particular_value(cr_COMPARE_MORE,		io_value_t,cr_compare_more_than_v)
decl_particular_value(cr_COMPARE_LESS,		io_value_t,cr_compare_less_than_v)
decl_particular_value(cr_COMPARE_EQUAL,	io_value_t,cr_compare_equal_to_v)
decl_particular_value(cr_COMPARE_NO_COMPARISON,	io_value_t,cr_compare_no_comparison_v)

#define io_value_is_equal(a,b) vref_is_equal_to (compare_io_values (a,b),cr_COMPARE_EQUAL)
#define io_value_not_equal(a,b) vref_not_equal_to (compare_io_values (a,b),cr_COMPARE_EQUAL)
#define io_value_is_more(a,b) vref_is_equal_to (compare_io_values (a,b),cr_COMPARE_MORE)
#define io_value_is_less(a,b) vref_is_equal_to (compare_io_values (a,b),cr_COMPARE_LESS)

INLINE_FUNCTION vref_t
to_comparison_value (int32_t comparison) {
	return (
		(comparison == 0) 
		? cr_COMPARE_EQUAL
		: (comparison > 0)
		? cr_COMPARE_MORE
		: cr_COMPARE_LESS
	);
}

#define SPECIALISE_COMPARE_IMPLEMENTATION(S,N) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	.name = N,	\
	/**/

//
// collection
//

#define SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION(S) \
	SPECIALISE_IO_VALUE_IMPLEMENTATION(S) \
	/**/

decl_particular_value(cr_COLLECTION,io_value_t,cr_collection_v)

//
// cons
//
typedef struct PACK_STRUCTURE {
	IO_VALUE_STRUCT_MEMBERS
	vref_t r_car;
	vref_t r_cdr;
	vref_t r_cpr;
} io_cons_value_t;

vref_t	mk_io_cons_value (io_value_memory_t*,vref_t,vref_t,vref_t);
bool		io_cons_value_get_car (vref_t,vref_t*);
bool		io_cons_value_get_cdr (vref_t,vref_t*);


INLINE_FUNCTION vref_t
io_cons_value_car (vref_t r_this) {
	io_cons_value_t const *this = vref_cast_to_ro_pointer (r_this);
	return this->r_car;
}

INLINE_FUNCTION vref_t
io_cons_value_set_car (vref_t r_this,vref_t r_value) {
	io_cons_value_t *this = vref_cast_to_rw_pointer (r_this);
	unreference_value (this->r_car);
	this->r_car = reference_value (r_value);
	return r_value;
}

INLINE_FUNCTION vref_t
io_cons_value_cdr (vref_t r_this) {
	io_cons_value_t const *this = vref_cast_to_ro_pointer (r_this);
	return this->r_cdr;
}

INLINE_FUNCTION vref_t
io_cons_value_set_cdr (vref_t r_this,vref_t r_value) {
	io_cons_value_t *this = vref_cast_to_rw_pointer (r_this);
	unreference_value (this->r_cdr);
	this->r_cdr = reference_value (r_value);
	return r_value;
}

INLINE_FUNCTION vref_t
io_cons_value_cpr (vref_t r_this) {
	io_cons_value_t const *this = vref_cast_to_ro_pointer (r_this);
	return this->r_cpr;
}

INLINE_FUNCTION vref_t
io_cons_value_set_cpr (vref_t r_this,vref_t r_value) {
	io_cons_value_t *this = vref_cast_to_rw_pointer (r_this);
	this->r_cpr = r_value;	// cpr is not referenced
	return r_value;
}

decl_particular_value(cr_CONS,io_cons_value_t,cr_cons_v)

//
// list
//
typedef struct PACK_STRUCTURE {
	IO_VALUE_STRUCT_MEMBERS
	vref_t r_head;
	vref_t r_tail;
} io_list_value_t;

vref_t	mk_io_list_value (io_value_memory_t*);
void		io_list_value_append_value (vref_t,vref_t);
bool		io_list_value_iterate_elements(vref_t,bool (*) (vref_t,void*),void*);
uint32_t	io_list_value_count (vref_t);
bool		io_list_pop_first (vref_t,vref_t*);
bool		io_list_pop_last (vref_t,vref_t*);

decl_particular_value(cr_LIST,io_list_value_t,cr_list_v)

//
// slot
//

typedef struct PACK_STRUCTURE {
	IO_VALUE_STRUCT_MEMBERS
	vref_t r_key;
	vref_t r_mapped;
} io_map_slot_value_t;

#define io_map_slot_value_key(s)			(s)->r_key
#define io_map_slot_value_mapping(s)	(s)->r_mapped

INLINE_FUNCTION vref_t
io_map_slot_value_get_key (vref_t r_this) {
	io_map_slot_value_t const *this = vref_cast_to_ro_pointer (r_this);
	return this->r_key;
}

INLINE_FUNCTION vref_t
io_map_slot_value_get_mapped_value (vref_t r_this) {
	io_map_slot_value_t const *this = vref_cast_to_ro_pointer (r_this);
	return this->r_mapped;
}

INLINE_FUNCTION vref_t
io_map_slot_value_set_key (vref_t r_this,vref_t r_value) {
	io_map_slot_value_t *this = vref_cast_to_rw_pointer (r_this);
	unreference_value (this->r_key);
	this->r_key = reference_value (r_value);
	return r_value;
}

//
// map
//

typedef struct PACK_STRUCTURE {
	IO_VALUE_STRUCT_MEMBERS
	vref_t r_tree;
	vref_t r_head;
	struct PACK_STRUCTURE {
		uint16_t maximum;
		uint16_t current;
	} depth;
} io_map_value_t;

#define io_map_value_maximum_depth(s)	(s)->depth.maximum
#define io_map_value_current_depth(s)	(s)->depth.current

vref_t	mk_io_map_value (io_value_memory_t*,uint16_t);
bool		io_map_value_map (vref_t,vref_t,vref_t);
vref_t	io_map_value_unmap (vref_t,vref_t);
bool		io_map_value_iterate (vref_t,bool (*) (vref_t,void*),void*);
bool		io_map_value_get_mapping (vref_t,vref_t,vref_t*);


decl_particular_value(cr_SLOT,io_map_slot_value_t,cr_map_slot_v)


//
// map
//
decl_particular_value(cr_MAP,io_map_value_t,cr_map_v)

#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// Implementation
//
//-----------------------------------------------------------------------------

//
// io value, the set of all values
//
void const*
io_typesafe_ro_cast_to_type (io_value_t const *downcast,io_value_implementation_t const *I) {

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

void const*
io_typesafe_ro_cast (vref_t r_value,vref_t r_type) {
	io_value_t const *downcast = vref_cast_to_ro_pointer (r_value);
	io_value_implementation_t const *I = get_io_value_implementation (r_type);
	return io_typesafe_ro_cast_to_type(downcast,I);
}

void
io_value_free_nop (io_value_t *value) {
}

io_value_t*
io_value_initialise_nop (vref_t r_value,vref_t r_base) {
	return vref_cast_to_rw_pointer(r_value);
}

vref_t
io_value_compare_no_comparison (io_value_t const *this,vref_t r_other) {
	return cr_COMPARE_NO_COMPARISON;
}

io_value_mode_t const* io_value_get_null_modes (io_value_t const *v) {
	return NULL;
}

bool
default_io_value_encode (vref_t r_value,io_encoding_t *encoding) {
	return true;
}

void
io_encode_value_implementation_to_x70 (
	io_value_t const *value,io_encoding_t *encoding
) {
	uint32_t len = strlen(value->implementation->name);
	io_x70_encoding_append_uint_value (encoding,len);
	io_encoding_append_bytes (
		encoding,(uint8_t const*) value->implementation->name,len
	);
} 

vref_t
default_io_value_receive (io_t *io,vref_t r_value,uint32_t argc,vref_t const *args) {
	return cr_IO_EXCEPTION;
}

vref_t
io_decode_x70_to_invalid_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return INVALID_VREF;
}

vref_t
io_value_compare_with_value (io_value_t const *this,vref_t r_other) {
	io_value_t const *other = vref_cast_to_ro_pointer (r_other);
	if (other->implementation == this->implementation) {
		return cr_COMPARE_EQUAL;
	} else {
		return cr_COMPARE_MORE;
	}
}

bool
io_value_encode_base (vref_t r_value,io_encoding_t *encoding) {
	bool result = false;

	if (is_io_text_encoding (encoding)) {
		io_encoding_append_byte (encoding,'.');
		result = true;
	} else if (is_io_x70_encoding (encoding)) {
		io_value_t const *this = vref_cast_to_ro_pointer (r_value);
		io_encode_value_implementation_to_x70 (this,encoding);
		result = true;
	}

	return result;
}

vref_t
decode_x70_to_io_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return cr_VALUE;
}

EVENT_DATA io_value_implementation_t io_value_implementation = {
	SPECIALISE_IO_VALUE_IMPLEMENTATION(NULL)
	.name = "value",
};

EVENT_DATA io_value_t cr_value_v = {
	decl_io_value (&io_value_implementation,sizeof(io_value_t))
};

//
// io nil value
//

static vref_t
io_value_compare_with_nil (io_value_t const *this,vref_t r_other) {
	if (io_typesafe_ro_cast (r_other,cr_NIL)) {
		return cr_COMPARE_EQUAL;
	} else {
		return cr_COMPARE_LESS;
	}
}

static bool
io_nil_value_encode (vref_t r_value,io_encoding_t *encoding) {
	bool result = false;

	if (is_io_text_encoding (encoding)) {
		io_encoding_append_string (encoding,"nil",3);
		result = true;
	} else if (is_io_x70_encoding (encoding)) {
		io_value_t const *this = vref_cast_to_ro_pointer (r_value);
		io_encode_value_implementation_to_x70 (this,encoding);
		result = true;
	}

	return result;
}

static vref_t
io_nil_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return cr_NIL;
}

EVENT_DATA io_value_implementation_t nil_value_implementation = {
	SPECIALISE_IO_VALUE_IMPLEMENTATION(&io_value_implementation)
	.name = "nil",
	.decode = {io_nil_decode_x70_value},
	.encode = io_nil_value_encode,
	.compare = io_value_compare_with_nil,
};

EVENT_DATA io_nil_value_t cr_nil_v = {
	decl_io_value (&nil_value_implementation,sizeof(io_nil_value_t))
};

//
// modal
//

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

EVENT_DATA io_value_mode_t null_io_modal_value_modes[] = {
};

EVENT_DATA io_modal_value_implementation_t 
io_modal_value_implementation = {
	SPECIALISE_IO_MODAL_VALUE_IMPLEMENTATION (
		&io_value_implementation
	)
};

//
// number
//

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

EVENT_DATA io_value_implementation_t 
io_number_value_implementation = {
	SPECIALISE_IO_NUMBER_VALUE_IMPLEMENTATION (
		&io_value_implementation
	)
	.name = "number",
};

EVENT_DATA io_number_value_t cr_number_v = {
	decl_io_value (&io_number_value_implementation,sizeof(io_number_value_t))
};

//
// int64
//

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
	io_int64_value_t const *this = io_typesafe_ro_cast (
		r_value,cr_I64_NUMBER
	);
	bool result = false;

	if (this) {
		if (is_io_text_encoding (encoding)) {
			io_encoding_printf (encoding,"%lld",this->value);
			result = true;
		} else if (is_io_x70_encoding (encoding)) {
			io_encode_value_implementation_to_x70 ((io_value_t const*) this,encoding);
			io_encoding_append_bytes (
				encoding,(uint8_t const*) &this->value,sizeof(this->value)
			);
			result = true;
		} else if (encoding_is_io_value_int64 (encoding)) {
			io_value_int64_encoding_t *enc = (io_value_int64_encoding_t*) encoding;
			enc->encoded_value = this->value;
			result = true;
		}
	}

	return result;
}

static vref_t
io_int64_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	if ((e - *b) >= sizeof(int64_t)) {
		int64_t v = read_le_int64 (*b);
		*b += sizeof(int64_t);
		return mk_io_int64_value (vm,v);
	} else {
		return INVALID_VREF;
	}
}


static vref_t
compare_with_i64_number (io_value_t const *v,vref_t r_other) {
	io_value_t const *other = vref_cast_to_ro_pointer (r_other);
	if (io_typesafe_ro_cast_to_type (other,v->implementation)) {
		io_int64_value_t const *this = (io_int64_value_t const*) v;
		io_int64_value_t const *o = (io_int64_value_t const*) other;
		return to_comparison_value (this->value - o->value);
	} else {
		return to_comparison_value (
			strcmp (v->implementation->name,other->implementation->name)
		);
	}
}

EVENT_DATA io_value_implementation_t 
i64_number_value_implementation = {
	SPECIALISE_IO_NUMBER_VALUE_IMPLEMENTATION (
		&io_number_value_implementation
	)
	.name = "i64",
	.initialise = io_int64_value_initialise,
	.encode = io_int64_value_encode,
	.decode = {io_int64_decode_x70_value},
	.compare = compare_with_i64_number,
};

EVENT_DATA io_int64_value_t cr_i64_number_v = {
	decl_io_value (
		&i64_number_value_implementation,
		sizeof(io_int64_value_t)
	)
	.value = 0,
};

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
		} else if (is_io_x70_encoding (encoding)) {
			io_encode_value_implementation_to_x70 ((io_value_t*) this,encoding);
			io_encoding_append_bytes (
				encoding,(uint8_t const*) &this->value,sizeof(this->value)
			);
			result = true;
		} else if (encoding_is_io_value_float64 (encoding)) {
			io_value_float64_encoding_t *enc = (io_value_float64_encoding_t*) encoding;
			enc->encoded_value = this->value;
			result = true;
		}
	}

	return result;
}

static vref_t
io_float64_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	if ((e - *b) >= sizeof(float64_t)) {
		float64_t v = read_le_float64 (*b);
		*b += sizeof(float64_t);
		return mk_io_float64_value (vm,v);
	} else {
		return INVALID_VREF;
	}
}

static vref_t
compare_with_f64_number (io_value_t const *v,vref_t r_other) {
	io_value_t const *other = vref_cast_to_ro_pointer (r_other);
	if (io_typesafe_ro_cast_to_type (other,v->implementation)) {
		io_float64_value_t const *this = (io_float64_value_t const*) v;
		io_float64_value_t const *o = (io_float64_value_t const*) other;
		if (io_math_compare_float64_eq (this->value,o->value)) {
			return cr_COMPARE_EQUAL;
		} else if (io_math_compare_float64_lt (this->value,o->value)) {
			return cr_COMPARE_LESS;
		} else {
			return cr_COMPARE_MORE;
		}
	} else {
		return to_comparison_value (
			strcmp (v->implementation->name,other->implementation->name)
		);
	}
}

EVENT_DATA io_value_implementation_t f64_number_value_implementation = {
	SPECIALISE_IO_NUMBER_VALUE_IMPLEMENTATION (
		&io_number_value_implementation
	)
	.name = "f64",
	.initialise = io_float64_value_initialise,
	.decode = {io_float64_decode_x70_value},
	.encode = io_float64_value_encode,
	.compare = compare_with_f64_number,
	.get_modes = io_value_get_null_modes,
};

EVENT_DATA io_float64_value_t cr_f64_number_v = {
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
// compare
//


EVENT_DATA io_value_implementation_t 
io_compare_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION (
		&io_value_implementation,"ordering"
	)
};

EVENT_DATA io_value_t cr_compare_v = {
	decl_io_value (
		&io_compare_value_implementation,
		sizeof(io_value_t)
	)
};

EVENT_DATA io_value_implementation_t 
io_compare_no_comparison_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION (
		&io_compare_value_implementation,"no-ordering"
	)
};

EVENT_DATA io_value_t cr_compare_no_comparison_v = {
	decl_io_value (
		&io_compare_no_comparison_value_implementation,
		sizeof(io_value_t)
	)
};

EVENT_DATA io_value_implementation_t 
io_compare_more_than_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION (
		&io_compare_value_implementation,"gt"
	)
};

EVENT_DATA io_value_t cr_compare_more_than_v = {
	decl_io_value (
		&io_compare_more_than_value_implementation,
		sizeof(io_value_t)
	)
};

EVENT_DATA io_value_implementation_t 
io_compare_less_than_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION (
		&io_compare_value_implementation,"lt"
	)
};

EVENT_DATA io_value_t cr_compare_less_than_v = {
	decl_io_value (
		&io_compare_less_than_value_implementation,
		sizeof(io_value_t)
	)
};

EVENT_DATA io_value_implementation_t 
io_compare_equal_to_value_implementation = {
	SPECIALISE_COMPARE_IMPLEMENTATION (
		&io_compare_value_implementation,"eq"
	)
};

EVENT_DATA io_value_t cr_compare_equal_to_v = {
	decl_io_value (
		&io_compare_equal_to_value_implementation,
		sizeof(io_value_t)
	)
};

//
// binary
//

static io_value_t*
io_binary_value_initialise_nop (vref_t r_value,vref_t r_base) {
	return vref_cast_to_rw_pointer(r_value);
}

bool
io_binary_value_encode_x70 (io_binary_value_t const *this,io_encoding_t *encoding) {
	io_encode_value_implementation_to_x70 ((io_value_t const*) this,encoding);
	io_x70_encoding_append_uint_value (encoding,io_binary_value_size(this));
	io_encoding_append_bytes (
		encoding,io_binary_value_ro_bytes(this),io_binary_value_size(this)
	);
	return true;
}

static bool
io_binary_value_encode (vref_t r_value,io_encoding_t *encoding) {
	io_binary_value_t const *this = vref_cast_to_ro_pointer(r_value);
	bool ok = false;

	if (is_io_text_encoding (encoding)) {
		io_encoding_append_bytes (
			encoding,io_binary_value_ro_bytes(this),io_binary_value_size(this)
		);
		ok = true;
	} else if (is_io_x70_encoding (encoding)) {
		ok = io_binary_value_encode_x70 (this,encoding);
	}

	return ok;
}

static vref_t
io_binary_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return INVALID_VREF;
}

static vref_t
compare_binary_with_other (io_value_t const *v,vref_t r_other) {
	io_value_t const *other = vref_cast_to_ro_pointer (r_other);
	if (io_typesafe_ro_cast_to_type (other,v->implementation)) {
		io_binary_value_t const *this = (io_binary_value_t const*) v;
		io_binary_value_t const *o = (io_binary_value_t const*) other;
		
		if (io_binary_value_size(this) == io_binary_value_size(o)) {
			return to_comparison_value (
				memcmp (
					io_binary_value_ro_bytes(this),
					io_binary_value_ro_bytes(o),
					io_binary_value_size(this)
				)
			);
		} else if (io_binary_value_size(this) > io_binary_value_size(o)) {
			return cr_COMPARE_MORE;
		} else {
			return cr_COMPARE_LESS;
		}
	} else {
		return to_comparison_value (
			strcmp (v->implementation->name,other->implementation->name)
		);
	}
}

EVENT_DATA io_value_implementation_t 
io_binary_value_implementation = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_value_implementation
	)
	.name = "binary",
};

EVENT_DATA io_binary_value_t cr_binary_v = {
	decl_io_value (&io_binary_value_implementation,sizeof(io_binary_value_t))
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

EVENT_DATA io_value_implementation_t 
io_binary_value_implementation_with_const_bytes = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_binary_value_implementation
	)
	.name = "const-binary",
	.initialise = io_binary_value_initialise_with_const_bytes,
};

EVENT_DATA io_binary_value_t cr_const_binary_v = {
	decl_io_value (&io_binary_value_implementation_with_const_bytes,sizeof(io_binary_value_t))
	.bytes.ro = NULL,
	.bit.binary_size = 0,
	.bit.const_bytes = 1,
};

vref_t
mk_io_binary_value_with_const_bytes (
	io_value_memory_t *vm,uint8_t const *bytes,int32_t size
) {
	io_binary_value_t base = {
		decl_io_value (
			&io_binary_value_implementation_with_const_bytes,
			sizeof(io_binary_value_t)
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
		&io_binary_value_implementation_with_const_bytes,
		sizeof (io_binary_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

EVENT_DATA io_value_implementation_t 
io_binary_value_implementation_with_dynamic_bytes = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_binary_value_implementation_with_const_bytes
	)
	.name = "dynamic-binary",
	.initialise = io_binary_value_initialise_with_dynamic_bytes,
};

static vref_t
mk_io_binary_value_for (
	io_value_memory_t *vm,io_value_implementation_t const *I,uint8_t const *bytes,int32_t size
) {
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
	return mk_io_binary_value_for (
		vm,&io_binary_value_implementation_with_dynamic_bytes,bytes,size
	);
}

static bool
io_text_value_encode (vref_t r_value,io_encoding_t *encoding) {
	io_binary_value_t const *this = vref_cast_to_ro_pointer(r_value);
	bool ok = false;

	if (is_io_text_encoding (encoding)) {
		io_encoding_append_bytes (
			encoding,io_binary_value_ro_bytes(this),io_binary_value_size(this)
		);
		ok = true;
	} else if (is_io_x70_encoding (encoding)) {
		ok = io_binary_value_encode_x70 (this,encoding);
	}
	return ok;
}

static vref_t
io_text_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {
	vref_t r_value = INVALID_VREF;
	uint32_t u;
	
	if (**b == X70_UINT_VALUE_BYTE) {
		*b += 1;
		*b += io_x70_encoding_take_uint_value (*b,e,&u);
		
		if (*b <= (e - u)) {
			r_value = mk_io_text_value (vm,*b,u);
			*b += u;
		}
	}

	return r_value;
}

vref_t
io_text_to_text_value_decoder (io_encoding_t *encoding,io_value_memory_t *vm) {
	const uint8_t *b,*e;
	io_encoding_get_content (encoding,&b,&e);
	return mk_io_text_value (vm,b,e - b);
}

EVENT_DATA io_value_implementation_t io_text_value_implementation = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_binary_value_implementation_with_dynamic_bytes
	)
	.name = "text",
	.initialise = io_binary_value_initialise_with_dynamic_bytes,
	.decode = {io_text_decode_x70_value},
	.encode = io_text_value_encode,
	.compare = compare_binary_with_other,
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

//
// symbol
//

EVENT_DATA io_value_implementation_t 
io_symbol_value_implementation_with_const_bytes = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_binary_value_implementation_with_const_bytes
	)
	.name = "const-symbol",
	.initialise = io_binary_value_initialise_with_const_bytes,
	.decode = {io_decode_x70_to_invalid_value},
};

EVENT_DATA io_binary_value_t cr_symbol_v = {
	decl_io_value (&io_symbol_value_implementation_with_const_bytes,sizeof(io_binary_value_t))
	.bytes.ro = (uint8_t const*) "symbol",
	.bit.binary_size = 6,
	.bit.const_bytes = 1,
};

EVENT_DATA io_value_implementation_t 
io_exception_value_implementation_with_const_bytes = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_binary_value_implementation_with_const_bytes
	)
	.name = "const-exception",
	.initialise = io_binary_value_initialise_with_const_bytes,
	.decode = {io_decode_x70_to_invalid_value},
	.encode = io_binary_value_encode,
};

EVENT_DATA io_binary_value_t cr_io_exception_v = {
	decl_io_value (
		&io_exception_value_implementation_with_const_bytes,
		sizeof(io_binary_value_t)
	)
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

EVENT_DATA io_value_implementation_t 
io_symbol_value_implementation_with_volatile_bytes = {
	SPECIALISE_IO_BINARY_VALUE_IMPLEMENTATION (
		&io_symbol_value_implementation_with_const_bytes
	)
	.name = "dynamic-symbol",
	.initialise = io_binary_value_initialise_with_const_bytes,
	.decode = {io_decode_x70_to_invalid_value},
};

//
// result value
//

EVENT_DATA io_value_implementation_t 
io_result_value_implementation = {
	SPECIALISE_RESULT_IMPLEMENTATION (
		&io_value_implementation,"result-of-receive"
	)
};
EVENT_DATA io_vector_value_t cr_result_v = {
	decl_io_value (&io_result_value_implementation,sizeof(io_value_t))
	.arity = 0,
};

EVENT_DATA io_value_implementation_t 
io_result_continue_value_implementation = {
	SPECIALISE_RESULT_IMPLEMENTATION (
		&io_result_value_implementation,"receive-result-continue"
	)
};

EVENT_DATA io_vector_value_t cr_result_continue_v = {
	decl_io_value (
		&io_result_continue_value_implementation,
		sizeof(io_value_t)
	)
	.arity = 0,
};

//
// vector value
//

static io_value_t*
io_vector_value_initialise (vref_t r_value,vref_t r_base) {
	io_vector_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_vector_value_t const *base = io_typesafe_ro_cast(r_base,cr_VECTOR);

	if (base != NULL) {
		vref_t const *cursor = base->values;
		vref_t const *end = cursor + base->arity;
		vref_t *dest = this->values;
		this->arity = base->arity;
		while (cursor < end) {
			*dest++ = reference_value (*cursor++);
		}
	} else {
		this = NULL;
	}

	return vref_cast_to_rw_pointer(r_value);
}

static void
io_vector_value_free (io_value_t *value) {
	io_vector_value_t *this = (io_vector_value_t*) value;
	vref_t *cursor = this->values;
	vref_t *end = cursor + this->arity;
	while (cursor < end) {
		unreference_value (*cursor++);
	}
}

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
io_vector_value_get_values (vref_t r_value,uint32_t *arity,vref_t const** values) {
	io_vector_value_t const *this = io_typesafe_ro_cast (
		r_value,cr_VECTOR
	);
	if (this) {
		*arity = this->arity;
		*values = this->values;
		return true;
	} else {
		return false;
	}
}

vref_t
io_value_send (io_t *io,vref_t r_value,uint32_t argc,...) {
	uint32_t count = argc;
	vref_t args[argc], *arg = args;
	va_list va;

	va_start(va,argc);
	while (count--) {
		*arg++ = va_arg(va,vref_t);
	}
	va_end(va);

	return io_value_sendm (io,r_value,argc,args);
}

EVENT_DATA io_value_implementation_t io_vector_value_implementation = {
	SPECIALISE_IO_VALUE_IMPLEMENTATION (
		&io_value_implementation
	)
	.name = "vector",
	.initialise = io_vector_value_initialise,
	.free = io_vector_value_free,
};

EVENT_DATA io_vector_value_t cr_vector_v = {
	decl_io_value (&io_vector_value_implementation,sizeof(io_vector_value_t))
	.arity = 0,
};

vref_t
mk_io_vector_value (io_value_memory_t *vm,uint32_t arity,vref_t const* values) {
	io_vector_value_t *base = alloca (sizeof(io_vector_value_t) + (arity * sizeof (vref_t)));
	base->implementation = &io_vector_value_implementation;
	base->tag_.reference_count_ = 0;
	base->tag_.size_ = sizeof(io_vector_value_t) + (arity * sizeof (vref_t));
	base->arity = arity;
	memcpy (base->values,values,arity * sizeof (vref_t));
	return io_value_memory_new_value (
		vm,
		&io_vector_value_implementation,
		sizeof(io_vector_value_t) + (arity * sizeof (vref_t)),
		def_vref (&reference_to_c_stack_value,base)
	);
}

//
// collection
//

EVENT_DATA io_value_implementation_t 
io_collection_value_implementation = {
	SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION (
		&io_value_implementation
	)
	.name = "collection",
};

EVENT_DATA io_value_t cr_collection_v = {
	decl_io_value (&io_collection_value_implementation,sizeof(io_value_t))
};

//
// cons
//

static io_value_t*
io_cons_value_initialise (vref_t r_value,vref_t r_base) {
	io_cons_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_cons_value_t const *base = io_typesafe_ro_cast(r_base,cr_CONS);

	if (base != NULL) {
		this->r_car = reference_value (base->r_car);
		this->r_cdr = reference_value (base->r_cdr);
		this->r_cpr = base->r_cpr;
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static void
io_cons_value_free (io_value_t *value) {
	io_cons_value_t *this = (io_cons_value_t*) (value);
	unreference_value (this->r_car);
	unreference_value (this->r_cdr);
}

static vref_t
compare_cons_value_to (io_value_t const *v,vref_t r_other) {
	io_value_t const *other = vref_cast_to_ro_pointer (r_other);
	if (io_typesafe_ro_cast_to_type (other,v->implementation)) {
		io_cons_value_t const *this = (io_cons_value_t const*) v;
		io_cons_value_t const *o = (io_cons_value_t const*) other;
		vref_t r_cmp = compare_io_values (this->r_car,o->r_car);
		if (vref_is_equal_to (r_cmp,cr_COMPARE_EQUAL)) {
			return compare_io_values (this->r_cdr,o->r_cdr);
		} else {
			return r_cmp;
		}
	} else {
		return to_comparison_value (
			strcmp (v->implementation->name,other->implementation->name)
		);
	}
}

bool
io_cons_value_get_car (vref_t r_this,vref_t *r_car) {
	io_cons_value_t const *this = io_typesafe_ro_cast (r_this,cr_CONS);
	if (this != NULL) {
		*r_car = this->r_car;
		return true;
	} else {
		return false;
	}
}

bool
io_cons_value_get_cdr (vref_t r_this,vref_t *r_cdr) {
	io_cons_value_t const *this = io_typesafe_ro_cast (r_this,cr_CONS);
	if (this != NULL) {
		*r_cdr = this->r_cdr;
		return true;
	} else {
		return false;
	}
}

static bool
io_cons_value_encode (vref_t r_value,io_encoding_t *encoding) {
	io_cons_value_t const *this = vref_cast_to_ro_pointer (r_value);
	bool result = false;

	if (is_io_text_encoding (encoding)) {
		result = true;
		result &= io_value_encode (this->r_car,encoding);
		result &= io_encoding_append_byte (encoding,' ');
		result &= io_value_encode (this->r_cdr,encoding);
	} else if (is_io_x70_encoding (encoding)) {
		io_encode_value_implementation_to_x70 ((io_value_t const*) this,encoding);
		
		result = true;
	}

	return result;
}

static vref_t
io_cons_value_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return cr_NIL;
}

EVENT_DATA io_value_implementation_t io_cons_value_implementation = {
	SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION (
		&io_collection_value_implementation
	)
	.name = "cons",
	.initialise = io_cons_value_initialise,
	.free = io_cons_value_free,
	.decode = {io_cons_value_decode_x70_value},
	.encode = io_cons_value_encode,
	.compare = compare_cons_value_to,
};

EVENT_DATA io_cons_value_t cr_cons_v = {
	decl_io_value (&io_cons_value_implementation,sizeof(io_cons_value_t))
	.r_car = decl_vref(&cr_nil_v),//cr_NIL,
	.r_cdr = decl_vref(&cr_nil_v),//cr_NIL,
	.r_cpr = decl_vref(&cr_nil_v),//cr_NIL,
};

vref_t
mk_io_cons_value (io_value_memory_t *vm,vref_t r_car,vref_t r_cdr,vref_t r_cpr) {
	io_cons_value_t base = {
		decl_io_value (
			&io_cons_value_implementation,sizeof(io_cons_value_t)
		)
		.r_car = r_car,
		.r_cdr = r_cdr,
		.r_cpr = r_cpr,
	};
	return io_value_memory_new_value (
		vm,
		&io_cons_value_implementation,
		sizeof (io_cons_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

//
// list
//

static bool
io_list_value_initialise_iterator (vref_t r_value,void *user_value) {
	vref_t *r_list = user_value;
	io_list_value_append_value (*r_list,r_value);
	return true;
}

static io_value_t*
io_list_value_initialise (vref_t r_value,vref_t r_base) {
	io_list_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_list_value_t const *base = io_typesafe_ro_cast(r_base,cr_LIST);

	if (base != NULL) {
		this->r_head = cr_NIL;
		this->r_tail = cr_NIL;
		io_list_value_iterate_elements (
			r_base,io_list_value_initialise_iterator,&r_value
		);
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static void
io_list_value_free (io_value_t *value) {
	io_list_value_t *this = (io_list_value_t*) (value);
	unreference_value (this->r_head);
	unreference_value (this->r_tail);
}

static bool
io_list_value_text_encode (vref_t r_value,void *user_value) {
	io_encoding_t *encoding = user_value;
	if (
			io_value_encode (r_value,encoding)
		&&	io_encoding_append_byte (encoding,' ')
	) {
		return true;
	} else {
		return false;
	}
}

static bool
io_list_value_encode (vref_t r_value,io_encoding_t *encoding) {
	bool ok = false;

	if (is_io_text_encoding (encoding)) {
		uint32_t len;
		ok = true;
		ok &= io_encoding_append_byte (encoding,'(');
		len = io_encoding_length (encoding);
		io_list_value_iterate_elements (r_value,io_list_value_text_encode,encoding);
		if (io_encoding_length (encoding) > len) {
			uint8_t byte;
			io_encoding_pop_last_byte (encoding,&byte);
		}
		ok &= io_encoding_append_byte (encoding,')');
	} else if (is_io_x70_encoding (encoding)) {

	}

	return ok;
}

EVENT_DATA io_value_implementation_t io_list_value_implementation = {
	SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION (
		&io_collection_value_implementation
	)
	.name = "list",
	.initialise = io_list_value_initialise,
	.free = io_list_value_free,
	.decode = {io_decode_x70_to_invalid_value},
	.encode = io_list_value_encode,
};

EVENT_DATA io_list_value_t cr_list_v = {
	decl_io_value (&io_list_value_implementation,sizeof(io_list_value_t))
	.r_head = decl_vref(&cr_nil_v),//cr_NIL,
	.r_tail = decl_vref(&cr_nil_v),//cr_NIL,
};

vref_t
mk_io_list_value (io_value_memory_t *vm) {
	io_list_value_t base = {
		decl_io_value (
			&io_list_value_implementation,sizeof (io_list_value_t)
		)
		.r_head = cr_NIL,
		.r_tail = cr_NIL,
	};
	return io_value_memory_new_value (
		vm,
		&io_list_value_implementation,
		sizeof (io_list_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

INLINE_FUNCTION vref_t
io_list_value_set_head (io_list_value_t *this,vref_t r_value) {
	unreference_value (this->r_head);
	this->r_head = reference_value (r_value);
	return r_value;
}

INLINE_FUNCTION vref_t
io_list_value_set_tail (io_list_value_t *this ,vref_t r_value) {
	unreference_value (this->r_tail);
	this->r_tail = reference_value (r_value);
	return r_value;
}

void
io_list_value_append_value (vref_t r_this,vref_t r_value) {
	io_list_value_t *this = vref_cast_to_rw_pointer(r_this);
	vref_t r_newtail = mk_io_cons_value (
		vref_get_containing_memory (r_this),r_value,cr_NIL,this->r_tail
	);
	if (vref_is_nil (this->r_head)) {
		this->r_head = reference_value (r_newtail);
	} else {
		io_cons_value_set_cdr (this->r_tail,r_newtail);
	}
	io_list_value_set_tail (this,r_newtail);
}

bool
io_list_value_iterate_elements (
	vref_t r_this,bool (*cb) (vref_t,void*),void *user_data
) {
	io_list_value_t const *this = vref_cast_to_ro_pointer (r_this);
	vref_t r_cons = this->r_head;
	while (vref_not_nil (r_cons)) {
		io_cons_value_t const *cons = vref_cast_to_ro_pointer (r_cons);
		if(!cb(cons->r_car,user_data)) {
			return false;
		}
		r_cons = cons->r_cdr;
	}
	return true;
}

static bool
io_list_value_counter (vref_t r_cons,void *user_value) {
	(*((uint32_t*) user_value)) ++;
	return true;
}

uint32_t
io_list_value_count (vref_t r_this) {
	uint32_t count = 0;
	io_list_value_iterate_elements (r_this,io_list_value_counter,&count);
	return count;
}

bool
io_list_pop_first (vref_t r_this,vref_t *r_first) {
	io_list_value_t *this = vref_cast_to_rw_pointer (r_this);
	if (vref_not_nil (this->r_head)) {
		*r_first = io_cons_value_car (this->r_head);
		if (vref_is_equal_to (this->r_head,this->r_tail)) {
			io_list_value_set_head (this,cr_NIL);
			io_list_value_set_tail (this,cr_NIL);
		} else {
			io_list_value_set_head (this,io_cons_value_cdr(this->r_head));
			io_cons_value_set_cpr (this->r_head,cr_NIL);
		}
		return true;
	} else {
		return false;
	}
}

bool
io_list_pop_last (vref_t r_this,vref_t *r_last) {
	io_list_value_t *this = vref_cast_to_rw_pointer(r_this);
	vref_t r_tail = this->r_tail;

	if (vref_not_nil (r_tail)) {
		*r_last = io_cons_value_car (r_tail);
		if (vref_is_nil (io_cons_value_cpr (r_tail))) {
			io_list_value_set_head (this,cr_NIL);
		} else {
			io_cons_value_set_cdr (r_tail,cr_NIL);
		}
		io_list_value_set_tail (this,io_cons_value_cpr (r_tail));
		return true;
	} else {
		return false;
	}
}

//
// map slot
//

static io_value_t*
io_map_slot_value_initialise (vref_t r_value,vref_t r_base) {
	io_map_slot_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_map_slot_value_t const *base = io_typesafe_ro_cast(r_base,cr_SLOT);

	if (base != NULL) {
		this->r_key = reference_value (base->r_key);
		this->r_mapped = reference_value (base->r_mapped);
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static void
io_map_slot_value_free (io_value_t *value) {
	io_map_slot_value_t *this = (io_map_slot_value_t*) (value);
	unreference_value (this->r_key);
	unreference_value (this->r_mapped);
}

static bool
io_map_slot_value_encode (vref_t r_value,io_encoding_t *encoding) {
	io_map_slot_value_t const *this = vref_cast_to_ro_pointer (r_value);
	bool result = false;

	if (is_io_text_encoding (encoding)) {
		result = true;
		result &= io_value_encode (this->r_key,encoding);
		result &= io_encoding_append_byte (encoding,' ');
		result &= io_value_encode (this->r_mapped,encoding);
	} else if (is_io_x70_encoding (encoding)) {
		io_encode_value_implementation_to_x70 ((io_value_t const*) this,encoding);
		
		result = true;
	}

	return result;
}

static vref_t
io_map_slot_value_decode_x70_value (
	io_value_memory_t *vm,uint8_t const**b,const uint8_t *e
) {	
	return cr_NIL;
}

EVENT_DATA io_value_implementation_t io_map_slot_value_implementation = {
	SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION (
		&io_collection_value_implementation
	)
	.name = "slot",
	.initialise = io_map_slot_value_initialise,
	.free = io_map_slot_value_free,
	.decode = {io_map_slot_value_decode_x70_value},
	.encode = io_map_slot_value_encode,
};

EVENT_DATA io_map_slot_value_t cr_map_slot_v = {
	decl_io_value (&io_map_slot_value_implementation,sizeof(io_map_slot_value_t))
	.r_key = decl_vref(&cr_nil_v),//cr_NIL,
	.r_mapped = decl_vref(&cr_nil_v),//cr_NIL,
};

vref_t 
mk_io_map_slot_value (io_value_memory_t *vm,vref_t r_key,vref_t r_mapping) {
	io_map_slot_value_t base = {
		decl_io_value (
			&io_map_slot_value_implementation,sizeof(io_map_slot_value_t)
		)
		.r_key = r_key,
		.r_mapped = r_mapping,
	};
	return io_value_memory_new_value (
		vm,
		&io_map_slot_value_implementation,
		sizeof (io_map_slot_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

//
// map
//

/*
 *
 *
 * map value
 *
 *  level      1            2            3 ...
 *
 *  +---+ tree
 *  |   |--------------------------------+
 *  |   |------+                         |
 *  +---+ head |                         |
 *             v                         v
 *           +---+  left  +---+  left  +---+
 *     |<--- | # |<-------| # |<-------| # |
 *           |   |        |   |        |   |
 *           +---+        +---+        +---+
 *             | right      | right      | right
 *             |            |            |
 *             v            v            v
 *     left  +---+  left  +---+  left  +---+
 *   <-------|   |<-------|   |<-------|   |
 *           |   |        |   |        |   |
 *           +---+        +---+        +---+
 *             |            | right      | right
 *             v            |            v
 *           +---+          |           ---
 *           |   |          |
 *           |   |          |
 *           +---+          |
 *             |            |
 *             v            v
 *           +---+  left  +---+
 *           |   |<-------|   |
 *           |   |        |   |
 *           +---+        +---+
 *             |            | right
 *             v            v 
 *            ---          ---
 * 
 * All nodes are cons values.
 * 
 * At level 1, store key on left, value in slot.  At level > 1,
 * store key in slot, value on left. Hence both key and value
 * are reachable from tree block.
 *
 */
static io_value_t*
io_map_value_initialise (vref_t r_value,vref_t r_base) {
	io_map_value_t *this = vref_cast_to_rw_pointer(r_value);
	io_map_value_t const *base = io_typesafe_ro_cast(r_base,cr_MAP);
	
	if (base != NULL) {
		this->r_head = reference_value (base->r_head);
		this->r_tree = reference_value (base->r_tree);
		this->depth.maximum = base->depth.maximum;
		this->depth.current = 1;
		
		// iterate r_base
		
	} else {
		this = NULL;
	}

	return (io_value_t*) this;
}

static void
io_map_value_free (io_value_t *value) {
	io_map_value_t *this = (io_map_value_t*) (value);
	unreference_value (this->r_head);
	unreference_value (this->r_tree);
}

EVENT_DATA io_value_implementation_t 
io_map_value_implementation = {
	SPECIALISE_IO_COLLECTION_VALUE_IMPLEMENTATION (
		&io_collection_value_implementation
	)
	.name = "map",
	.initialise = io_map_value_initialise,
	.free = io_map_value_free,
};

EVENT_DATA io_map_value_t cr_map_v = {
	decl_io_value (&io_map_value_implementation,sizeof(io_map_value_t))
	.r_tree = decl_vref(&cr_nil_v),//cr_NIL,
	.r_head = decl_vref(&cr_nil_v),//cr_NIL,
};

//
// map value nodes are cons values
//
#define io_map_value_node_left(n)		((n)->r_car)
#define io_map_value_node_right(n)		((n)->r_cdr)
#define io_map_value_node_get_right		io_cons_value_cdr
#define io_map_value_node_set_right		io_cons_value_set_cdr
#define io_map_value_node_key(n)			((n)->r_cpr)
#define io_map_value_node_slot(n)		((n)->r_cpr)

vref_t
mk_io_map_value (io_value_memory_t *vm,uint16_t maximum_depth) {
	vref_t r_node = mk_io_cons_value (vm,cr_NIL,cr_NIL,cr_NIL);
	io_map_value_t base = {
		decl_io_value (
			&io_map_value_implementation,sizeof (io_map_value_t)
		)
		.r_head = r_node,
		.r_tree = r_node,
		.depth.maximum = maximum_depth,
		.depth.current = 1,
	};
	return io_value_memory_new_value (
		vm,
		&io_map_value_implementation,
		sizeof (io_map_value_t),
		def_vref (&reference_to_c_stack_value,&base)
	);
}

INLINE_FUNCTION vref_t
io_map_value_set_tree (io_map_value_t *this ,vref_t r_value) {
	unreference_value (this->r_tree);
	this->r_tree = reference_value (r_value);
	return r_value;
}

static vref_t
io_map_value_search (io_map_value_t const *this,vref_t r_key,vref_t path[]) {
	int16_t i = io_map_value_current_depth(this) - 1;
	vref_t r_node = cr_NIL,r_temp = this->r_tree;
	io_cons_value_t const *node;
	
	do {
	loop:
		node = vref_cast_to_ro_pointer(r_temp);
		if (vref_not_nil (io_map_value_node_right(node))) {
			io_cons_value_t const *right = vref_cast_to_ro_pointer (
				io_map_value_node_right (node)
			);
			if (io_value_is_less(io_map_value_node_key (right),r_key)) {
				r_temp = io_map_value_node_right (node);
				goto loop;
			}
		}
		if (path) path[i] = r_temp; else r_node = r_temp;
		r_temp = io_map_value_node_left (node);
		i--;
	} while ( i >= 0);
	
	return r_node;
}

bool
io_map_value_map (vref_t r_this,vref_t r_key,vref_t r_value) {
	io_map_value_t const *this = vref_cast_to_ro_pointer (r_this);
	io_value_memory_t *vm = vref_get_containing_memory (r_this);
	vref_t path[io_map_value_maximum_depth (this)];
	io_cons_value_t const *node;
	uint16_t new_level = 1;
	
	io_map_value_search (this,r_key,path);

	node = vref_cast_to_ro_pointer (path[0]);
	if (vref_not_nil (io_map_value_node_right (node))) {
		io_cons_value_t const *right = vref_cast_to_ro_pointer (
			io_map_value_node_right (node)
		);
		if (io_value_is_equal (io_map_value_node_key (right),r_key)) {
			io_map_slot_value_t *left = vref_cast_to_rw_pointer (
				io_map_value_node_left (right)
			);
			unreference_value (io_map_slot_value_mapping(left));
			io_map_slot_value_mapping (left) = reference_value (r_value);
			return false;
		}
	}

	io_t *io = io_value_memory_get_io(vm);
	while (
			new_level < io_map_value_maximum_depth (this) 
		&& io_get_next_prbs_u32 (io) < ((UINT32_MAX * 2) / 3)
	) {
		new_level ++;
	}

	io_cons_value_t *nn = vref_cast_to_rw_pointer (path[0]);
	vref_t r_node = mk_io_cons_value (
		vm,
		mk_io_map_slot_value (vm,r_key,r_value),
		io_map_value_node_right (nn),
		r_key
	);
	io_map_value_node_set_right (path[0],r_node);
	
	if (io_map_value_current_depth(this) < new_level) {
		io_map_value_t *this = vref_cast_to_rw_pointer (r_this);
		for (uint32_t i = io_map_value_current_depth (this); i < new_level; i++) {
			io_map_value_set_tree (
				this,mk_io_cons_value (vm,this->r_tree,cr_NIL,cr_NIL)
			);
		}
		io_map_value_current_depth (this) = new_level;
	}

	return true;
}

static vref_t
io_map_value_get_slot (vref_t r_this,vref_t r_key) {
	io_map_value_t const *this = vref_cast_to_ro_pointer (r_this);
	vref_t r_preceeding = io_map_value_search (this,r_key,NULL);
	io_cons_value_t const *node = vref_cast_to_ro_pointer (r_preceeding);

	if (vref_not_nil (io_map_value_node_right (node))) {
		io_cons_value_t const *right = vref_cast_to_ro_pointer (
			io_map_value_node_right (node)
		);
		if (io_value_is_equal(io_map_value_node_key (right),r_key)) {
			return io_map_value_node_left (right);
		}
	}

	return cr_NIL;
}

bool
io_map_value_get_mapping (vref_t r_this,vref_t r_key,vref_t *r_mapped) {
	vref_t r_slot = io_map_value_get_slot (r_this,r_key);

	if (vref_not_nil (r_slot)) {
		*r_mapped = io_map_slot_value_get_mapped_value (r_slot);
		return true;
	} else {
		return false;
	}
}

bool
io_map_value_iterate (
	vref_t r_this,bool (*cb) (vref_t,void*),void *user_data
) {
	io_map_value_t const *this = vref_cast_to_ro_pointer (r_this);
	vref_t r_node = io_map_value_node_get_right (this->r_head);
	while (vref_not_nil(r_node)) {
		io_cons_value_t const *node = vref_cast_to_ro_pointer(r_node);
		r_node =  io_map_value_node_right (node);
		if (!cb(io_map_value_node_left (node),user_data)) {
			return false;
		}
	}
	return true;
}

static vref_t
io_map_value_remove_helper (
	vref_t r_list,
	io_map_value_t *list,
	vref_t path[],
	vref_t r_key
) {
	io_cons_value_t const *node = vref_cast_to_ro_pointer(path[0]);
	
	if (vref_not_nil (io_map_value_node_right (node))) {
		io_cons_value_t const *temp = vref_cast_to_ro_pointer (
			io_map_value_node_right(node)
		);
		if (io_value_is_equal(io_map_value_node_key (temp),r_key)) {
			io_map_value_t *mlist = NULL;
			vref_t r_removed = io_map_slot_value_get_mapped_value (
				io_map_value_node_left(temp)
			);
			vref_t b = cr_NIL;
			int i;

			/* adjust forward pointers */
			for (i = 0; i < io_map_value_current_depth (list); i++) {
				io_cons_value_t *prev = vref_cast_to_rw_pointer(path[i]);
				node = vref_cast_to_ro_pointer (io_map_value_node_right(prev));
				if (i > 0 && !vref_is_equal_to(io_map_value_node_left(node),b)) break;
				b = io_map_value_node_right(prev);	// node being deleted at this level
				io_map_value_node_set_right (path[i],io_map_value_node_right(node));
			}

			/* adjust header level */
			while (i > 1) {
				node = vref_cast_to_ro_pointer (list->r_tree);
				if (vref_is_nil(io_map_value_node_right(node))) {
					if (mlist == NULL) mlist = vref_cast_to_rw_pointer(r_list);
					io_map_value_current_depth (mlist) --;
					io_map_value_set_tree (list,io_map_value_node_left (node));
				} else {
					break;
				}
				i--;
			}
			
			return r_removed;
		}
	}
	
	return INVALID_VREF;
}

vref_t
io_map_value_unmap (vref_t r_this,vref_t r_key) {
	io_map_value_t *this = vref_cast_to_rw_pointer(r_this);
	vref_t path[io_map_value_maximum_depth(this)];
	io_map_value_search (this,r_key,path);
	return io_map_value_remove_helper (r_this,this,path,r_key);
}

bool
add_core_value_implementations_to_hash (string_hash_table_t *hash) {
	static io_value_implementation_t const * const imp[] = {
		IO_VALUE_IMPLEMENTATION(&nil_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_number_value_implementation),
		IO_VALUE_IMPLEMENTATION(&i64_number_value_implementation),
		IO_VALUE_IMPLEMENTATION(&f64_number_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_binary_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_binary_value_implementation_with_const_bytes),
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
		IO_VALUE_IMPLEMENTATION(&io_cons_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_list_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_map_slot_value_implementation),
		IO_VALUE_IMPLEMENTATION(&io_map_value_implementation),
	};
	bool ok = true;
	
	for (int i = 0; i < SIZEOF(imp) && ok; i++) {
		ok &= string_hash_table_insert (
			hash,imp[i]->name,strlen(imp[i]->name),def_hash_mapping_ro_ptr(imp[i])
		);
	}
	
	// will be false if duplicate name or out-of-memory
	return ok;
}

#endif /* IMPLEMENT_IO_CORE */
#endif
