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
 * In one C/C++ define IMPLEMENT_VERIFY_IO_CORE.
 *
 */
#ifndef verify_io_core_H_
#define verify_io_core_H_
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


#ifdef IMPLEMENT_VERIFY_IO_CORE

#define vprintf(r,fmt,...) io_printf(r->io,fmt,##__VA_ARGS__)

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
		runner->total_failed++;
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
	vprintf (runner,"\n");
	if (runner->total_failed == 0) {
		vprintf (
			runner,"%-*s%-*scompleted with %d test%s",
			DBP_FIELD1,"self test",
			DBP_FIELD2,DEVICE_NAME,
			runner->total_tests,
			(runner->total_tests == 1) ? "":"s"
		);
		if (runner->skipped_unit_count > 0) {
			vprintf (
				runner," (%u unit%s skipped)",
				runner->skipped_unit_count,plural(runner->skipped_unit_count)
			);
		}
		vprintf (runner," OK\n");
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
# ifdef IMPLEMENT_VERIFY_IO_CORE_VALUES

TEST_BEGIN(test_io_text_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;

		reference_io_encoding (encoding);
		VERIFY (is_io_text_encoding (encoding),NULL);
		VERIFY (is_io_binary_encoding (encoding),NULL);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		VERIFY (io_encoding_printf (encoding,"%s","abc") == 3,NULL);
		VERIFY (io_encoding_length (encoding) == 3,NULL);

		io_encoding_get_ro_bytes (encoding,&b,&e);
		VERIFY ((e - b) == 3,NULL);
		VERIFY (memcmp ("abc",b,3) == 0,NULL);

		io_encoding_reset (encoding);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		io_encoding_append_byte (encoding,0);

		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_constant_values_1) {

	VERIFY(vref_is_valid(cr_UNIV),NULL);

	VERIFY(vref_is_valid(cr_NIL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_NIL,cr_NIL) != NULL,NULL);

	VERIFY(vref_is_valid(cr_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_BINARY) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_UNIV) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_NIL) == NULL,NULL);

	VERIFY(vref_is_valid(cr_CONSTANT_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_CONSTANT_BINARY,cr_BINARY) != NULL,NULL);

	VERIFY(vref_is_valid(cr_SYMBOL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_SYMBOL,cr_CONSTANT_BINARY) != NULL,NULL);
	VERIFY (arg_match(cr_SYMBOL)(cr_SYMBOL),NULL);

	VERIFY(vref_is_valid(cr_IO_EXCEPTION),NULL);

	VERIFY(vref_is_valid(cr_RESULT),NULL);
	VERIFY(vref_is_valid(cr_RESULT_CONTINUE),NULL);

	VERIFY (
		get_io_value_implementation_from_encoding_index(CR_UNIV_ENCODING_INDEX) == &univ_value_implementation,
		NULL
	);
}
TEST_END

TEST_BEGIN(test_io_int64_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_I64_NUMBER);
		io_value_int64_encoding_t enc = def_int64_encoding(24);
		vref_t r_decoded;

		VERIFY (v != NULL,NULL);
		VERIFY (io_typesafe_ro_cast (r_value,cr_NUMBER) != NULL,NULL);

		VERIFY (
				io_value_encode (r_value,(io_encoding_t*) &enc)
			&&	io_value_int64_encoding_encoded_value(&enc) == 42,
			NULL
		);

		r_decoded = io_encoding_decode_to_io_value (
			(io_encoding_t*) &enc,io_integer_to_integer_value_decoder,vm
		);
		if (VERIFY (vref_is_valid (r_decoded),NULL)) {
			int64_t i64_value;

			reference_value (r_decoded);

			v = io_typesafe_ro_cast (r_decoded,cr_I64_NUMBER);
			if (VERIFY (v != NULL,NULL)) {
				io_value_int64_encoding_t dc = def_int64_encoding(24);
				VERIFY (
						io_value_encode (r_value,(io_encoding_t*) &dc)
					&&	io_value_int64_encoding_encoded_value(&enc) == 42,
					NULL
				);
			}

			VERIFY (io_value_get_as_int64(r_decoded,&i64_value) && i64_value == 42,NULL);

			unreference_value (r_decoded);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_int64_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end,vm_begin,vm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);
	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;

		VERIFY (io_encoding_printf (encoding,"%v",r_value) == 2,NULL);

		io_encoding_get_ro_bytes (encoding,&b,&e);
		VERIFY ((e - b) == 2,NULL);
		VERIFY (memcmp ("42",b,2) == 0,NULL);

		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_float64_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_float64_encoding_t enc = def_float64_encoding(0.0);
		io_value_t const *v;
		float64_t f64;

		v = io_typesafe_ro_cast (r_value,cr_F64_NUMBER);
		VERIFY (v != NULL,NULL);
		VERIFY (io_typesafe_ro_cast (r_value,cr_NUMBER) != NULL,NULL);

		VERIFY (
				io_value_encode (r_value,(io_encoding_t*) &enc)
			&&	io_math_compare_float64_eq (io_value_float64_encoding_encoded_value(&enc),42.1),
			NULL
		);

		VERIFY (
				io_value_get_as_float64 (r_value,&f64)
			&&	io_math_compare_float64_eq (f64,42.1),
			NULL
		);

		//io_printf (TEST_IO,"\nf = %2.2f",f64);

		unreference_value (r_value);
	}

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_float64_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_t r_value;

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_encoding_t *encoding = mk_io_text_encoding (io_get_byte_memory (TEST_IO));
		const uint8_t *b,*e;

		VERIFY (io_encoding_printf (encoding,"%v",r_value) == 9,NULL);

		io_encoding_get_ro_bytes (encoding,&b,&e);
		VERIFY ((e - b) == 9,NULL);
		VERIFY (memcmp ("42.100000",b,2) == 0,NULL);

		unreference_value (r_value);
		io_encoding_free(encoding);
	}
}
TEST_END

TEST_BEGIN(test_io_binary_value_with_const_bytes_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_binary_value_with_const_bytes (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_CONSTANT_BINARY);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_ro_bytes (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_binary_value_dynamic_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_binary_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_BINARY);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_ro_bytes (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_text_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_text_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_value_t const *v = io_typesafe_ro_cast (r_value,cr_TEXT);

		if (VERIFY (v != NULL,NULL)) {
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding = mk_io_text_encoding (bm);

			if (VERIFY (io_value_encode (r_value,encoding),NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_ro_bytes (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
	}

	unreference_value (r_value);

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_stack_vector_value_1) {
	declare_stack_vector (r_vect,cr_NIL);
	uint32_t arity;
	vref_t const *args;

	VERIFY (io_vector_value_get_arity (r_vect,&arity) && arity == 1,NULL);
	VERIFY (io_vector_value_get_values (r_vect,&args) && vref_is_nil(args[0]),NULL);

}
TEST_END

UNIT_SETUP(setup_io_core_values_unit_test) {
	io_byte_memory_get_info (io_get_byte_memory (TEST_IO),TEST_MEMORY_INFO);
	io_value_memory_get_info (io_get_short_term_value_memory (TEST_IO),TEST_MEMORY_INFO + 1);
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_values_unit_test) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_end,vm_end;

	io_do_gc (TEST_IO,-1);

	io_value_memory_get_info (io_get_short_term_value_memory (TEST_IO),&vm_end);
	VERIFY (vm_end.used_bytes == TEST_MEMORY_INFO[1].used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == TEST_MEMORY_INFO[0].used_bytes,NULL);
}

void
io_core_values_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_text_encoding_1,
		test_io_constant_values_1,
		test_io_int64_value_1,
		test_io_int64_value_2,
		test_io_float64_value_1,
		test_io_float64_value_2,
		test_io_binary_value_with_const_bytes_1,
		test_io_binary_value_dynamic_1,
		test_io_text_value_1,
		test_stack_vector_value_1,
		0
	};
	unit->name = "io_values";
	unit->description = "io core values unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_values_unit_test;
	unit->teardown = teardown_io_core_values_unit_test;
}
# endif /* IMPLEMENT_VERIFY_IO_CORE_VALUES */

TEST_BEGIN(test_io_memories_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);

	VERIFY (bm != NULL,NULL);
	VERIFY (io_value_memory_get_io(vm) == TEST_IO,NULL);

}
TEST_END

TEST_BEGIN(test_io_byte_pipe_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);

	io_byte_pipe_t *pipe = mk_io_byte_pipe (bm,8);
	if (VERIFY (pipe != NULL,NULL)) {
		uint8_t data = 0;
		VERIFY (!io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_put_byte (pipe,42),NULL);
		VERIFY (io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_get_byte (pipe,&data) && data == 42,NULL);
		VERIFY (!io_byte_pipe_is_readable (pipe) == 0,NULL);
		
		VERIFY (is_io_byte_pipe_event (io_pipe_event (pipe)),NULL);
		free_io_byte_pipe (pipe,bm);
	}
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

UNIT_SETUP(setup_io_internalss_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_internals_unit_test) {
}

static void
io_internals_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_memories_1,
		test_io_byte_pipe_1,
		0
	};
	unit->name = "io internals";
	unit->description = "io internals unit test";
	unit->tests = tests;
	unit->setup = setup_io_internalss_unit_test;
	unit->teardown = teardown_io_internals_unit_test;
}

#ifdef IMPLEMENT_VERIFY_IO_GRAPHICS
# include <io_graphics.h>
#endif

#ifdef IMPLEMENT_VERIFY_IO_SHELL
# include <io_shell.h>
#endif

#ifdef IMPLEMENT_VERIFY_IO_DEVICE
# include <io_device.h>
#endif

void
run_ut_io (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_internals_unit_test,
		#ifdef IMPLEMENT_VERIFY_IO_CORE_VALUES
		io_core_values_unit_test,
		#endif
		#ifdef IMPLEMENT_VERIFY_IO_GRAPHICS
		io_graphics_unit_test,
		#endif
		#ifdef IMPLEMENT_VERIFY_IO_SHELL
		io_shell_unit_test,
		#endif
		#ifdef IMPLEMENT_VERIFY_IO_CPU
		io_cpu_unit_test,
		#endif
		#ifdef IMPLEMENT_VERIFY_IO_DEVICE
		io_device_unit_test,
		#endif
		0
	};
	V_run_unit_tests(runner,test_set);
}

void
verify_io (V_runner_t *runner) {
	run_ut_io (runner);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE */
#endif /* verify_io_core_H_ */
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
