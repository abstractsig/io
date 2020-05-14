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
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE.
 *
 */
#ifndef io_verify_H_
#define io_verify_H_
#include <io_core.h>

typedef struct test_runner V_runner_t;
typedef void (*V_test_t) (V_runner_t*);

typedef enum {
	VERIFY_UNIT_CONTINUE,
	VERIFY_UNIT_SKIP,
} verify_unit_setup_t;

typedef verify_unit_setup_t (*V_test_setup_t) (V_runner_t*);

typedef struct _v_unit_test {
	const char* name;
	const char* description;
	V_test_t const *tests;
	V_test_setup_t setup;
	V_test_t teardown;
	struct _v_unit_test* next_unit;
} V_unit_test_t;

typedef void (*unit_test_t) (V_unit_test_t*);

struct test_runner {
	uint32_t test_result;
	uint32_t total_tests;
	uint32_t total_failed;
	uint32_t unit_count;
	uint32_t skipped_unit_count;
	const char *current_test_name;
	io_t *io;
	memory_info_t minfo[2];
	vref_t user_value;
};

#define TEST_BEGIN(test_name) void test_name (V_runner_t *vrunner) {\
	vrunner->current_test_name = #test_name;

#define TEST_END 	\
}

#define TEST_IO				(vrunner->io)
#define TEST_MEMORY_INFO	(vrunner->minfo)
#define TEST_USER_VALUE		(vrunner->user_value)

#define VERIFY(pred,desc) V_record_test_result(vrunner,pred,""#pred"",desc,__FILE__, __LINE__)
#define UNIT_SETUP(name) static verify_unit_setup_t name (V_runner_t *vrunner)
#define UNIT_TEARDOWN(name) static void name (V_runner_t *vrunner)

unsigned int	V_record_test_result(V_runner_t*,uint32_t,const char*,const char*,const char* file,unsigned int line);
void			V_run_unit_tests(V_runner_t*,unit_test_t const*);
uint32_t		print_unit_test_report (V_runner_t*);


#ifdef IMPLEMENT_IO_VERIFY
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

#define vprintf(r,fmt,...) io_printf((r)->io,fmt,##__VA_ARGS__)

unsigned int
V_record_test_result (
	V_runner_t *runner,
	uint32_t result,
	const char* predicate,
	const char* description,
	const char* file,
	unsigned int line
) {
	runner->test_result = (runner->test_result && !!(result));
	runner->total_tests++;

	if(!(result)) {
		runner->total_failed++;	// NB: This line used as a breakpoint, do not move
	}

	return result;
}

void
V_init_unit(V_unit_test_t *new_unit) {
	new_unit->name = "";
	new_unit->description = "";
	new_unit->tests = NULL;
	new_unit->setup = NULL;
	new_unit->teardown = NULL;
	new_unit->next_unit = NULL;
}

void
V_run_rest(V_runner_t *runner,V_unit_test_t *unit) {
	V_test_t const *test;

	test = unit->tests;
	if (unit->setup != NULL) {
		runner->current_test_name = unit->name;
		switch (unit->setup(runner)) {
			case VERIFY_UNIT_SKIP:
				runner->skipped_unit_count++;
				return;

			case VERIFY_UNIT_CONTINUE:
			break;
		}
	}

	while((*test) != NULL) {
		runner->unit_count ++;
		(*test)(runner);
		test++;
	}

	if (unit->teardown != NULL) {
		runner->current_test_name = unit->name;
		unit->teardown(runner);
	}
}

void
V_start_tests(V_runner_t *runner) {
	runner->test_result = 1;
	runner->total_tests = 0;
	runner->total_failed = 0;
	runner->unit_count = 0;
	runner->skipped_unit_count = 0;
	runner->current_test_name = NULL;
}

void
V_run_unit_tests(V_runner_t *runner,unit_test_t const * unit_tests) {
	static V_unit_test_t unit;
	uint32_t i = 0;

	while(unit_tests[i] != 0) {
		V_init_unit(&unit);
		unit_tests[i++](&unit);
		V_run_rest(runner,&unit);
	}
}

uint32_t
print_unit_test_report (V_runner_t *runner) {
	if (runner->total_failed == 0) {
		vprintf (
			runner,"%-*s%-*scompleted with %d test%s OK\n",
			DBP_FIELD1,DEVICE_NAME,
			DBP_FIELD2,"self test",
			runner->total_tests,
			(runner->total_tests == 1) ? "":"s"
		);
		if (runner->skipped_unit_count > 0) {
			vprintf (
				runner,"%-*s(%u unit%s skipped)\n",
				DBP_FIELD1+DBP_FIELD2,"",
				runner->skipped_unit_count,plural(runner->skipped_unit_count)
			);
		}
	} else {
		vprintf (
			runner,"%-*s%-*sfailed, %d tests of %d\n",
			DBP_FIELD1,"self test",
			DBP_FIELD2,DEVICE_NAME,
			runner->total_failed,
			runner->total_tests
		);
	}
	return runner->total_failed == 0;
}
#endif /* IMPLEMENT_IO_VERIFY */
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
