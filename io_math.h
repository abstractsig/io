/*
 *
 * math support, in particular fixed point arithmetic
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 */
#ifndef io_math_H_
#define io_math_H_
# include <math.h>

#ifndef FLOAT64_COMPARE_EPSILON
# define FLOAT64_COMPARE_EPSILON	0.000001
#endif

#define round_float_to_int(v) ((int)(((v) > 0.0) ? ((v) + 0.5) : ((v) - 0.5)))

int32_t io_math_compare_float64_with_epsilon (float64_t a,float64_t b,float64_t epsilon);
#define io_math_compare_float64(a,b)	io_math_compare_float64_with_epsilon (a,b,FLOAT64_COMPARE_EPSILON)

#define io_math_compare_float64_eq(a,b) (io_math_compare_float64 (a,b) == 0)
#define io_math_compare_float64_ge(a,b) (io_math_compare_float64 (a,b) >= 0)
#define io_math_compare_float64_gt(a,b) (io_math_compare_float64 (a,b) > 0)

//
// will drag in the dsp stuff here
//

typedef int64_t	q64f32_t;

#define Q64fN(v,n) ((v) == 1 ? (1LL << (n)) - 1 : (int64_t)((double)(1LL << (n)) * (double)(v)))
#define Q64F32(v) Q64fN(v,32)


uint32_t		next_prime_u32_integer (uint32_t);
bool			is_u32_integer_prime (uint32_t);
uint32_t		read_le_uint32 (uint8_t const*);
int64_t		read_le_int64 (uint8_t const*);
float64_t	read_le_float64 (uint8_t const*);
float64_t	read_be_float64 (uint8_t const*);

typedef int32_t	q32f31_t;
#ifdef IMPLEMENT_IO_CORE
//-----------------------------------------------------------------------------
//
// io core implementation
//
//-----------------------------------------------------------------------------
uint32_t
read_le_uint32 (uint8_t const* ptr) {
	uint8_t const* ptr8 = (uint8_t const*) ptr;
	return (
			(uint32_t) ptr8[0] 
		+	((uint32_t)ptr8[1] << 8)
		+	((uint32_t)ptr8[2] << 16) 
		+	((uint32_t)ptr8[3] << 24)
	);
}

int64_t
read_le_int64 (uint8_t const *ptr8) {
	return (int64_t) (
			(uint64_t)ptr8[0]
		+	((uint64_t)ptr8[1] << 8ULL) 
		+	((uint64_t)ptr8[2] << 16ULL) 
		+	((uint64_t)ptr8[3] << 24ULL)
		+	((uint64_t)ptr8[4] << 32ULL)
		+	((uint64_t)ptr8[5] << 40ULL)
		+	((uint64_t)ptr8[6] << 48ULL)
		+	((uint64_t)ptr8[7] << 56ULL)
	);
}

float64_t
read_le_float64 (uint8_t const *ptr8) {
	union {
		uint8_t b[8];
		float64_t f;
	} conv = {0};
	memcpy (conv.b,ptr8,8);
	return conv.f;
}

float64_t
read_be_float64 (uint8_t const *ptr8) {
	union {
		uint8_t b[8];
		float64_t f;
	} conv = {0};
	conv.b[0] = ptr8[7];
	conv.b[1] = ptr8[6];
	conv.b[2] = ptr8[5];
	conv.b[3] = ptr8[4];
	conv.b[4] = ptr8[3];
	conv.b[5] = ptr8[2];
	conv.b[6] = ptr8[1];
	conv.b[7] = ptr8[0];
	return conv.f;
}

//
// prime
//

bool
is_u32_integer_prime (uint32_t x) {
	if (x == 2) {
		return true;
	} else if (x < 2 || ((x & 0x1) == 0)) {
		return false;
	} else {
		for (size_t i = 3; true; i+=2) {
			size_t q = x / i;
			if (q < i) {
				return true;
			}
			if (x == q * i) {
				return false;
			}
		}
		return true;
	}
}

uint32_t
next_prime_u32_integer (uint32_t x) {
	if (is_u32_integer_prime(x)) {
		x += 1;
	}
	switch (x) {
		case 0:
		case 1:
		case 2:
			return 2;
		case 3:
			return 3;
		case 4:
		case 5:
			return 5;
		default: {
			uint32_t k = x / 6;
			uint32_t i = x - 6 * k;
			uint32_t o = i < 2 ? 1 : 5;
			x = 6 * k + o;
			for (i = (3 + o) / 2; !is_u32_integer_prime (x); x += i) {
				i ^= 6;
			}
			
			return x;
		}
	}
}

//
// float
//
#define io_math_float_abs(a)	fabs(a)

bool
io_math_compare_float64_equal (float64_t a,float64_t b,float64_t epsilon) {
	float64_t diff = io_math_float_abs (a - b);
	if (a == b) {
		// shortcut, handles infinities
		return true;
	} else if (a == 0 || b == 0) {
		return diff < epsilon;
	} else {
		float64_t absA = io_math_float_abs (a);
		float64_t absB = io_math_float_abs (b);
		return (diff / (absA + absB)) < epsilon;
	}
}

int32_t
io_math_compare_float64_with_epsilon (float64_t a,float64_t b,float64_t epsilon) {
	if (io_math_compare_float64_equal(a,b,epsilon)) {
		return 0;
	} else {
		return (a < b) ? -1 : 1;
	}
}
#endif /* IMPLEMENT_IO_CORE */
// NB: math tests are located in verify_io.h
#endif /* io_math_H_ */
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
