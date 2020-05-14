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
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_SECURITY.
 *
 */
#ifndef io_verify_core_security_H_
#define io_verify_core_security_H_

void	run_ut_io_core_security (V_runner_t*);

#ifdef IMPLEMENT_VERIFY_IO_CORE_SECURITY
TEST_BEGIN(test_ecc_secret_generation_1) {

	io_authentication_key_t secret1 = {
		.bytes = {
			0xf1,0x87,0x9a,0xfb,0x28,0x04,0xf5,0x98,
			0x96,0x67,0x56,0x5e,0x24,0xf8,0x34,0xa5,
			0xfe,0xd3,0x71,0xa8,0x25,0xf9,0x3b,0x38,
			0xa4,0x03,0xd3,0x46,0x97,0x93,0x47,0xa9,
		}
	};
	io_authentication_key_t expect_shared1 = {
		.bytes = {
			0x45,0xee,0x7a,0x7c,0x87,0xf5,0x00,0x4f,
			0x5c,0xad,0x4d,0x4a,0x9a,0x8d,0xca,0x01,
			0x76,0xb9,0xb3,0xd1,0x97,0x7a,0xa8,0x34,
			0xed,0x05,0x9f,0xe2,0xdb,0x6b,0xf1,0x19,
		}
	};

	io_authentication_key_t secret2 = {
		.bytes = {
			0x25,0x4d,0x41,0xe8,0xd8,0xb3,0xb6,0x89,
			0xb9,0x47,0xf8,0x20,0xbd,0x37,0x63,0xb4,
			0x9b,0x6c,0xf1,0x80,0xbd,0x23,0x90,0x8b,
			0x09,0xf4,0xc2,0x2b,0xa5,0x10,0xbb,0xb4,
		}
	};
	io_authentication_key_t expect_shared2 = {
		.bytes = {
			0x9b,0xb5,0xeb,0x80,0xa0,0x3f,0xd9,0xd4,
			0x21,0x57,0x6c,0x90,0x7a,0xb9,0x8f,0x98,
			0xd4,0xce,0x0f,0x26,0x65,0x8c,0xff,0x05,
			0x5f,0x3d,0xb6,0x9e,0x29,0x8f,0x96,0x06,
		}
	};

	io_authentication_key_t common1,common2,expect_common = {
		.bytes = {
			0x98,0xb6,0x75,0xb5,0x64,0x12,0x5c,0xa5,
			0x99,0x7b,0x1b,0xae,0xf4,0x09,0xa6,0xc8,
			0xb2,0xd7,0x72,0x96,0x70,0x65,0xea,0x64,
			0xe6,0x1e,0x87,0x18,0xf1,0x46,0x8e,0x5e,
		}
	};
	
	uint8_t k[IO_AUTHENTICATION_KEY_BYTE_LENGTH] = {9};
	io_authentication_key_t shared1,shared2;
	
	curve25519_donna (shared1.bytes,secret1.bytes,k);
	curve25519_donna (shared2.bytes,secret2.bytes,k);
	
	VERIFY (io_authentication_key_test_equal (&shared1,&expect_shared1),NULL);
	VERIFY (io_authentication_key_test_equal (&shared2,&expect_shared2),NULL);

	curve25519_donna (common1.bytes,secret1.bytes,shared2.bytes);
	curve25519_donna (common2.bytes,secret1.bytes,shared2.bytes);

	VERIFY (io_authentication_key_test_equal (&common1,&expect_common),NULL);
	VERIFY (io_authentication_key_test_equal (&common2,&expect_common),NULL);

}
TEST_END

UNIT_SETUP(setup_io_core_security_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_byte_memory_get_info (bm,TEST_MEMORY_INFO);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_security_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end;
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO->used_bytes,NULL);
}

static void
io_core_security_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_ecc_secret_generation_1,
		0
	};
	unit->name = "io core security";
	unit->description = "io core security unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_security_unit_test;
	unit->teardown = teardown_io_core_security_unit_test;
}

void
run_ut_io_core_security (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_core_security_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE_SECURITY */
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


