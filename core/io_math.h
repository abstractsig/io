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



typedef int32_t	q32f31_t;
#ifdef IMPLEMENT_IO_CORE

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
