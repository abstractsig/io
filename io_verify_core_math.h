/*
 *
 * Verify Io
 *
 * A unit test library for testing io
 *
 * LICENSE
 * See end of file for license terms.
 *
 * USAGE
 * Include this file in whatever places need to refer to it.
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_MATH.
 *
 */
#ifndef io_verify_core_math_H_
#define io_verify_core_math_H_
#include <io_verify.h>

void	run_ut_io_core_math (V_runner_t*);

#ifdef IMPLEMENT_VERIFY_IO_CORE_MATH
TEST_BEGIN(test_compiler_struct_handling_1) {
	//
	// implementation 'overloading' depends on this capability
	// in the compiler
	//
	struct {
		int a;
	} v = {
		.a = 2,
		.a = 4,
	};
	VERIFY (v.a == 4,NULL);
}
TEST_END

TEST_BEGIN(test_io_is_prime_1) {
	VERIFY (!is_u32_integer_prime (0),NULL);
	VERIFY (!is_u32_integer_prime (1),NULL);
	VERIFY (is_u32_integer_prime (2),NULL);

	uint32_t p[] = {163,277,409,547};
	bool ok = true;
	for (int i = 0; i < SIZEOF(p) && ok; i++) {
		ok &= is_u32_integer_prime (p[i]);
	}
	VERIFY (ok,"all prime");
}
TEST_END

TEST_BEGIN(test_io_next_prime_1) {
	uint32_t p[][2] = {{0,2},{1,2},{7,11},{163,167},{20,23},{50,53}};
	bool ok = true;
	for (int i = 0; i < SIZEOF(p); i++) {
		ok &= (next_prime_u32_integer (p[i][0]) == p[i][1]);
	}
	VERIFY (ok,"all next primes ok");
}
TEST_END

TEST_BEGIN(test_read_le_uint32_1) {
	uint8_t p[8] = {0x42,0,0,0};
	int64_t v = read_le_uint32 (p);
	VERIFY (v = 42,NULL);
}
TEST_END

TEST_BEGIN(test_read_le_int64_1) {
	uint8_t p[8] = {0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,};
	int64_t v = read_le_int64 (p);
	VERIFY (v = -1,NULL);
}
TEST_END

TEST_BEGIN(test_read_float64_1) {
	uint8_t p[8] = {205,204,204,204,204,12,69,64};
	float64_t v = read_le_float64 (p);	
	VERIFY (io_core_math_compare_float64_eq (v,42.1),NULL);
}
TEST_END

TEST_BEGIN(test_number_gcd) {
	uint32_t numbers[4];
	
	VERIFY(gcd_uint32(48,18) == 6,NULL);
	VERIFY(gcd_uint32(10000000,1000000) == 1000000,NULL);
	VERIFY(gcd_uint32(17,10) == 1,NULL);
	VERIFY(gcd_uint32(1000,1000) == 1000,NULL);
	
	numbers[0] = 48;
	numbers[1] = 18;
	VERIFY(gcd_uint32_reduce (numbers,2) == 6,NULL);

	numbers[0] = 48;
	numbers[1] = 18;
	numbers[2] = 96;
	VERIFY(gcd_uint32_reduce (numbers,3) == 6,NULL);

}
TEST_END

UNIT_SETUP(setup_io_core_math_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_math_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

static void
io_core_math_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_compiler_struct_handling_1,
		test_io_is_prime_1,
		test_io_next_prime_1,
		test_read_le_uint32_1,
		test_read_le_int64_1,
		test_read_float64_1,
		test_number_gcd,
		0
	};
	unit->name = "io math";
	unit->description = "io math unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_math_unit_test;
	unit->teardown = teardown_io_core_math_unit_test;
}

void
run_ut_io_core_math (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_core_math_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE_MATH */
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
