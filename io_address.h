/*
 *
 * (included by io_core.h)
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 */
#ifndef io_address_H_
#define io_address_H_
typedef struct io_address_2 io_address2_t;
typedef struct io_address_implementation io_address_implementation_t;

struct io_address_implementation {
	io_address_implementation_t const *specialisation_of;
	uint32_t (*size) (io_address2_t);
	io_address2_t (*duplicate) (io_address2_t);
	io_address2_t (*copy) (io_address2_t,io_byte_memory_t*);
	void (*free) (io_address2_t);
	int32_t (*compare) (io_address2_t,io_address2_t);
	uint8_t const* (*ro_bytes) (io_address2_t);
	uint8_t (*get_u8_value) (io_address2_t);
	uint16_t (*get_u16_value) (io_address2_t);
	uint32_t (*get_u24_value) (io_address2_t);
	uint32_t (*get_u32_value) (io_address2_t);
};

#define IO_ADDRESS_STRUCT_MEMBERS \
	io_address_implementation_t const *implementation; \
	uint32_t value; \
	/**/
	
struct PACK_STRUCTURE io_address_2 {
	IO_ADDRESS_STRUCT_MEMBERS
};

uint32_t io_address2_invalid_size(io_address2_t);
io_address2_t duplicate_short_io_address (io_address2_t);
io_address2_t copy_short_io_address (io_address2_t,io_byte_memory_t*);
void free_short_io_address (io_address2_t);
uint8_t io_address2_invalid_u8_value(io_address2_t);
uint16_t io_address2_invalid_u16_value(io_address2_t);
uint32_t io_address2_invalid_u32_value(io_address2_t);

#define SPECIALISE_IO_ADDRESS_IMPLEMENTATION(S) \
	.specialisation_of = S,\
	.size = io_address2_invalid_size, \
	.copy = copy_short_io_address, \
	.duplicate = duplicate_short_io_address, \
	.free = free_short_io_address, \
	.get_u8_value = io_address2_invalid_u8_value,\
	.get_u16_value = io_address2_invalid_u16_value,\
	.get_u32_value = io_address2_invalid_u32_value,\
	/**/

INLINE_FUNCTION io_address2_t
copy_io_address2 (io_address2_t a,io_byte_memory_t *bm) {
	return a.implementation->copy(a,bm);
}

INLINE_FUNCTION io_address2_t
duplicate_io_address2 (io_address2_t a) {
	return a.implementation->duplicate(a);
}

INLINE_FUNCTION uint32_t
io_address2_size (io_address2_t a) {
	return a.implementation->size(a);
}

INLINE_FUNCTION int32_t
io_address2_compare (io_address2_t a,io_address2_t b) {
	return a.implementation->compare(a,b);
}

INLINE_FUNCTION uint8_t
io_address2_get_u8_value (io_address2_t a) {
	return a.implementation->get_u8_value(a);
}

INLINE_FUNCTION uint16_t
io_address2_get_u16_value (io_address2_t a) {
	return a.implementation->get_u16_value(a);
}

INLINE_FUNCTION uint32_t
io_address2_get_u32_value (io_address2_t a) {
	return a.implementation->get_u32_value(a);
}

INLINE_FUNCTION uint8_t const*
io_address2_get_ro_bytes (io_address2_t a) {
	return a.implementation->ro_bytes(a);
}

extern EVENT_DATA io_address_implementation_t u8_io_address_implementation;
#define def_io_u8_address2(a) (io_address2_t) {\
		.implementation = &u8_io_address_implementation,\
		.value = a,\
	}

extern EVENT_DATA io_address_implementation_t u16_io_address_implementation;
#define def_io_u16_address2(a) (io_address2_t) {\
		.implementation = &u16_io_address_implementation,\
		.value = a,\
	}


extern EVENT_DATA io_address_implementation_t u32_io_address_implementation;
#define def_io_u32_address2(a) (io_address2_t) {\
		.implementation = &u32_io_address_implementation,\
		.value = a,\
	}

#define IO_CONST_BYTE_ADDRESS_STRUCT_MEMBERS \
	IO_ADDRESS_STRUCT_MEMBERS \
	uint8_t const *bytes;\
	/**/

struct PACK_STRUCTURE io_const_byte_address_2 {
	IO_CONST_BYTE_ADDRESS_STRUCT_MEMBERS
};

#define def_io_const_address2(s,ro)	(io_address_t) {\
		.implementation = &const_byte_io_address_implementation,\
		.value = s,\
	}


#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

uint32_t
io_address2_invalid_size (io_address2_t a) {
	return 0;
}

io_address2_t
duplicate_short_io_address (io_address2_t a) {
	return a;
}

io_address2_t
copy_short_io_address (io_address2_t a,io_byte_memory_t *bm) {
	return a;
}

void
free_short_io_address (io_address2_t a) {
	// nothing to do
}

uint8_t
io_address2_invalid_u8_value(io_address2_t a) {
	return 0;
}

uint16_t
io_address2_invalid_u16_value(io_address2_t a) {
	return 0;
}

uint32_t
io_address2_invalid_u24_value(io_address2_t a) {
	return 0;
}

uint32_t
io_address2_invalid_u32_value(io_address2_t a) {
	return 0;
}

static uint32_t
u8_io_address2_size (io_address2_t a) {
	return sizeof(uint8_t);
}

static int32_t
u8_io_address2_compare (io_address2_t a,io_address2_t b) {
	int32_t cmp;
	
	switch (io_address2_size(b)) {
		case 0:
			cmp = 1;
		break;
		
		case 1:
			cmp = (io_address2_get_u8_value (a) ==  io_address2_get_u8_value (b))
					? 0
					:io_address2_get_u8_value (a) > io_address2_get_u8_value (b)
					? 1 
					: -1;
		break;
		
		default:
			cmp = -1;
		break;
	}
	
	return cmp;
}

static uint8_t
u8_io_address_get_u8_value (io_address2_t a) {
	return a.value;
}

EVENT_DATA io_address_implementation_t u8_io_address_implementation = {
	SPECIALISE_IO_ADDRESS_IMPLEMENTATION(NULL)
	.size = u8_io_address2_size,
	.compare = u8_io_address2_compare,
	.get_u8_value = u8_io_address_get_u8_value,
};

static uint32_t
u16_io_address2_size (io_address2_t a) {
	return sizeof(uint16_t);
}

static int32_t
u16_io_address2_compare (io_address2_t a,io_address2_t b) {
	int32_t cmp;
	
	switch (io_address2_size(b)) {
		case 0:
		case 1:
			cmp = 1;
		break;
		
		case 2:
			cmp = (io_address2_get_u16_value (a) ==  io_address2_get_u16_value (b))
					? 0
					:io_address2_get_u16_value (a) > io_address2_get_u16_value (b)
					? 1 
					: -1;
		break;
		
		default:
			cmp = -1;
		break;
	}
	
	return cmp;
}

static uint16_t
u16_io_address_get_u16_value (io_address2_t a) {
	return a.value;
}

EVENT_DATA io_address_implementation_t u16_io_address_implementation = {
	SPECIALISE_IO_ADDRESS_IMPLEMENTATION(NULL)
	.size = u16_io_address2_size,
	.compare = u16_io_address2_compare,
	.get_u16_value = u16_io_address_get_u16_value,
};

static uint32_t
u32_io_address2_size (io_address2_t a) {
	return sizeof(uint32_t);
}

static int32_t
u32_io_address2_compare (io_address2_t a,io_address2_t b) {
	int32_t cmp;
	
	switch (io_address2_size(b)) {
		case 0:
		case 1:
		case 2:
		case 3:
			cmp = 1;
		break;
		
		case 4:
			cmp = (io_address2_get_u32_value (a) ==  io_address2_get_u32_value (b))
					? 0
					:io_address2_get_u32_value (a) > io_address2_get_u32_value (b)
					? 1 
					: -1;
		break;
		
		default:
			cmp = -1;
		break;
	}
	
	return cmp;
}

static uint32_t
u32_io_address_get_u32_value (io_address2_t a) {
	return a.value;
}

EVENT_DATA io_address_implementation_t u32_io_address_implementation = {
	SPECIALISE_IO_ADDRESS_IMPLEMENTATION(NULL)
	.size = u32_io_address2_size,
	.compare = u32_io_address2_compare,
	.get_u32_value = u32_io_address_get_u32_value,
};

EVENT_DATA io_address_implementation_t const_byte_io_address_implementation = {
	SPECIALISE_IO_ADDRESS_IMPLEMENTATION(NULL)
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
