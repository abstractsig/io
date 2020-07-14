/*
 *
 * a shell language for io values
 *
 * LICENSE
 * =======
 * See end of file for license terms.
 *
 */
#ifndef io_shell_H_
#define io_shell_H_
#include <io_core.h>

void	ion_shell_source_decoder_state_error (io_source_decoder_t*,io_character_t);
void	ion_shell_source_decoder_state_begin (io_source_decoder_t*,io_character_t);
io_source_decoder_t* mk_io_shell_source_decoder (io_t*,vref_t,is_symbol_t const*);

//
// for verification tests
//
def_constant_symbol(cr_T_START,		"start",5)
def_constant_symbol(cr_T_STOP,		"stop",4)


#ifdef IMPLEMENT_IO_SHELL
//-----------------------------------------------------------------------------
//
// implementation of io_shell
//
//-----------------------------------------------------------------------------
io_source_decoder_t*
mk_io_shell_source_decoder (io_t *io,vref_t r_value,is_symbol_t const *keywords) {
	return mk_io_source_decoder (
		io,
		r_value,
		ion_shell_source_decoder_state_begin,
		ion_shell_source_decoder_state_error,
		keywords
	);
}

void
ion_shell_source_decoder_state_error (io_source_decoder_t *this,io_character_t c) {
}

static void
ion_shell_source_decoder_state_number (io_source_decoder_t *this,io_character_t c) {
	if (
			character_is_digit (c)
		||	c == '.'
	) {
		io_encoding_append_byte(this->buffer,c);
	} else {
		io_value_memory_t *vm = io_get_short_term_value_memory (this->io);
		vref_t r_value;

		r_value = io_encoding_decode_to_io_value (
			this->buffer,io_text_to_number_value_decoder,vm
		);

		if (vref_is_valid(r_value)) {
			if (!io_source_decoder_append_arg (this,r_value)) {
				io_source_decoder_set_last_error (
					this,"out of memory"
				);
				io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
			}
		} else {
			io_source_decoder_set_last_error (
				this,"parse integer failed"
			);
			io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
		}

		io_source_decoder_pop_input (this);
		io_source_decoder_input(this) (this,c);
	}
}

static void
ion_shell_source_decoder_state_symbol (io_source_decoder_t *this,io_character_t c) {
	if (character_is_isalnum (c)) {
		io_encoding_append_byte(this->buffer,c);
	} else {
		const uint8_t *b,*e;
		is_symbol_t const *cursor = this->keywords;
		vref_t r_sym;

		io_encoding_get_content (this->buffer,&b,&e);

		while (*cursor) {
			if (vref_is_valid((r_sym = (*cursor)((char*) b,e-b)))) {
				break;
			}
			cursor++;
		}

		if (vref_is_valid(r_sym)) {
			if (!io_source_decoder_append_arg (this,r_sym)) {
				io_source_decoder_set_last_error (
					this,"out of memory"
				);
				io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
			}
		} else {
			io_source_decoder_set_last_error (
				this,"unknown command"
			);
			io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
		}

		io_source_decoder_pop_input (this);
		io_source_decoder_input(this) (this,c);
	}
}

static void
ion_shell_source_decoder_state_text (io_source_decoder_t *this,io_character_t c) {
		// '\r' or '\n\ are errors
	if (c != '"') {
		io_encoding_append_byte(this->buffer,c);
	} else {
		io_value_memory_t *vm = io_get_short_term_value_memory (this->io);
		vref_t r_value;

		r_value = io_encoding_decode_to_io_value (
			this->buffer,io_text_to_text_value_decoder,vm
		);

		if (vref_is_valid(r_value)) {
			if (!io_source_decoder_append_arg (this,r_value)) {
				io_source_decoder_set_last_error (
					this,"out of memory"
				);
				io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
			}
		} else {
			io_source_decoder_set_last_error (
				this,"parse text failed"
			);
			io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
		}

		io_source_decoder_pop_input (this);
	}
}

static void
ion_shell_source_decoder_state_line_comment (io_source_decoder_t *this,io_character_t c) {
	if (c == '\n') {
		io_source_decoder_pop_input (this);
	}
}

static void
ion_shell_source_decoder_state_forward_slash (io_source_decoder_t *this,io_character_t c) {
	switch (c) {
		case '/':
			io_source_decoder_goto (this,ion_shell_source_decoder_state_line_comment);
		break;

		default:
			io_source_decoder_pop_input(this);
			io_source_decoder_input(this) (this,'/');
			io_source_decoder_input(this) (this,c);
		break;
	}
}

static void
ion_shell_source_decoder_state_hyphen (io_source_decoder_t *this,io_character_t c) {
	if (character_is_digit (c)) {
		io_source_decoder_goto (this,ion_shell_source_decoder_state_number);
		io_encoding_reset (this->buffer);
		io_encoding_append_byte(this->buffer,'-');
		io_source_decoder_input(this) (this,c);
	} else {
		io_source_decoder_set_last_error (
			this,"hyphen must be followed by a digit"
		);
		io_source_decoder_goto (this,ion_shell_source_decoder_state_error);
	}
}

void
ion_shell_source_decoder_state_begin (io_source_decoder_t *this,io_character_t c) {

	switch (c) {
		case '\r':
		case '\t':
		case ' ':
			// ignore
		break;

		case ';':
		case '\n':
			io_source_decoder_end_of_statement (this);
		break;

		case '/':
			io_source_decoder_push_input (this,ion_shell_source_decoder_state_forward_slash);
		break;

		case '-':
			io_source_decoder_push_input (this,ion_shell_source_decoder_state_hyphen);
		break;

		case '"':
			io_encoding_reset (this->buffer);
			io_source_decoder_push_input (this,ion_shell_source_decoder_state_text);
		break;

		default:
			io_encoding_reset (this->buffer);
			io_encoding_append_byte (this->buffer,c);
			if (character_is_digit (c)) {
				io_source_decoder_push_input (this,ion_shell_source_decoder_state_number);
			} else {
				io_source_decoder_push_input (this,ion_shell_source_decoder_state_symbol);
			}
		break;
	}
}
#endif /* IMPLEMENT_IO_SHELL */
#ifdef IMPLEMENT_VERIFY_IO_SHELL
#include <io_verify.h>
typedef struct PACK_STRUCTURE test_io_shell {
	IO_MODAL_VALUE_STRUCT_MEMBERS
	vref_t r_result[3];
} test_io_shell_value_t;

static vref_t
test_io_shell_value_implementation_sym (io_t *io,vref_t r_test,vref_t const *args) {
	test_io_shell_value_t *this = vref_cast_to_rw_pointer(r_test);
	this->r_result[0] = args[0];
	return cr_RESULT_CONTINUE;
}

static vref_t
test_io_shell_value_implementation_int (io_t *io,vref_t r_test,vref_t const *args) {
	test_io_shell_value_t *this = vref_cast_to_rw_pointer(r_test);
	this->r_result[1] = reference_value(args[0]);
	return cr_RESULT_CONTINUE;
}

static vref_t
test_io_shell_value_implementation_text (io_t *io,vref_t r_test,vref_t const *args) {
	test_io_shell_value_t *this = vref_cast_to_rw_pointer(r_test);
	this->r_result[2] = reference_value(args[0]);
	return cr_RESULT_CONTINUE;
}

EVENT_DATA io_signature_t test_sig[] = {
	io_signature("","",test_io_shell_value_implementation_sym,arg_match(cr_SYMBOL)),
	io_signature("","",test_io_shell_value_implementation_int,arg_match(cr_NUMBER)),
	io_signature("","",test_io_shell_value_implementation_text,arg_match(cr_TEXT)),
	END_OF_MODE
};

static io_value_mode_t modes[] = {
	{"begin",test_sig},
};

EVENT_DATA io_modal_value_implementation_t test_io_shell_value_implementation = {
	decl_modal_value_implementation (io_modal_value_initialise,modes)
};

EVENT_DATA is_symbol_t io_shell_test_keywords[] = {
	is_symbol_cr_T_START,
	is_symbol_cr_T_STOP,
	NULL
};

TEST_BEGIN(test_shell_decoder_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_source_decoder_t *decoder;

	test_io_shell_value_t tester = {
		decl_io_modal_value (
			&test_io_shell_value_implementation,
			sizeof(test_io_shell_value_t)
		)
		.r_result = {INVALID_VREF,INVALID_VREF},
	};

	io_byte_memory_get_info (bm,&bm_begin);

	VERIFY (
		NULL != io_typesafe_ro_cast_to_type (
			(io_value_t const*) &tester,
			IO_VALUE_IMPLEMENTATION(&io_modal_value_implementation)
		),
		NULL
	);

	decoder = mk_io_shell_source_decoder (
		TEST_IO,
		def_vref (&reference_to_c_stack_value,&tester),
		io_shell_test_keywords
	);

	VERIFY (io_source_decoder_input_depth(decoder) == 1,NULL);
	VERIFY (io_source_decoder_context_depth(decoder) == 1,NULL);
	VERIFY (vref_is_invalid(tester.r_result[0]),NULL);

	bool ok = true;
	{
		tester.r_result[0] = INVALID_VREF;
		tester.r_result[1] = INVALID_VREF;
		io_source_decoder_parse (decoder,"start\n");
		ok &= vref_is_equal_to(tester.r_result[0],cr_T_START);
		ok &= vref_is_invalid(tester.r_result[1]);
		ok &= (io_source_decoder_input_depth(decoder) == 1);
		ok &= (io_source_decoder_context_depth(decoder) == 1);

		tester.r_result[0] = INVALID_VREF;
		tester.r_result[1] = INVALID_VREF;
		io_source_decoder_parse (decoder,"stop\n");
		ok &= vref_is_equal_to(tester.r_result[0],cr_T_STOP);
		ok &= vref_is_invalid(tester.r_result[1]);
		ok &= (io_source_decoder_input_depth(decoder) == 1);
		ok &= (io_source_decoder_context_depth(decoder) == 1);
	}
	VERIFY (ok,"all symbols decoded");

	free_io_source_decoder (decoder);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_shell_decoder_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	memory_info_t bm_begin,bm_end;
	io_source_decoder_t *decoder;
	int64_t i64_value;

	test_io_shell_value_t tester = {
		decl_io_modal_value (
			&test_io_shell_value_implementation,
			sizeof(test_io_shell_value_t)
		)
		.r_result = {INVALID_VREF,INVALID_VREF},
	};

	io_byte_memory_get_info (bm,&bm_begin);
	io_value_memory_get_info (vm,&vm_begin);

	decoder = mk_io_shell_source_decoder (
		TEST_IO,
		def_vref (&reference_to_c_stack_value,&tester),
		io_shell_test_keywords
	);

	VERIFY (io_source_decoder_input_depth(decoder) == 1,NULL);

	tester.r_result[0] = INVALID_VREF;
	tester.r_result[1] = INVALID_VREF;
	io_source_decoder_parse (decoder,"42\n");

	VERIFY (
			vref_is_valid (tester.r_result[1])
		&& vref_is_invalid(tester.r_result[0])
		&& io_typesafe_ro_cast (tester.r_result[1],cr_I64_NUMBER) != NULL
		&& io_value_get_as_int64(tester.r_result[1],&i64_value)
		&& i64_value == 42,
		NULL
	);

	unreference_value(tester.r_result[1]);

	tester.r_result[1] = INVALID_VREF;
	io_source_decoder_parse (decoder,"-42\n");
	VERIFY (
			vref_is_valid (tester.r_result[1])
		&& vref_is_invalid(tester.r_result[0])
		&& io_typesafe_ro_cast (tester.r_result[1],cr_I64_NUMBER) != NULL
		&& io_value_get_as_int64(tester.r_result[1],&i64_value)
		&& i64_value == -42,
		NULL
	);

	unreference_value(tester.r_result[1]);

	tester.r_result[1] = INVALID_VREF;
	io_source_decoder_parse (decoder,"42.2\n");

	VERIFY (
		vref_is_valid (tester.r_result[1]),
		NULL
	);

	unreference_value(tester.r_result[1]);

	free_io_source_decoder (decoder);

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_shell_decoder_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	memory_info_t bm_begin,bm_end;
	io_source_decoder_t *decoder;

	test_io_shell_value_t tester = {
		decl_io_modal_value (
			&test_io_shell_value_implementation,
			sizeof(test_io_shell_value_t)
		)
		.r_result = {INVALID_VREF,INVALID_VREF},
	};

	io_byte_memory_get_info (bm,&bm_begin);
	io_value_memory_get_info (vm,&vm_begin);

	decoder = mk_io_shell_source_decoder (
		TEST_IO,
		def_vref (&reference_to_c_stack_value,&tester),
		io_shell_test_keywords
	);

	VERIFY (io_source_decoder_input_depth(decoder) == 1,NULL);

	tester.r_result[0] = INVALID_VREF;
	tester.r_result[1] = INVALID_VREF;
	tester.r_result[2] = INVALID_VREF;
	io_source_decoder_parse (decoder,"\"42\"\n");

	VERIFY (
			vref_is_valid (tester.r_result[2])
			&& vref_is_invalid(tester.r_result[1])
			&& vref_is_invalid(tester.r_result[0])
		&& io_typesafe_ro_cast (tester.r_result[2],cr_TEXT) != NULL,
		NULL
	);

	unreference_value(tester.r_result[2]);

	free_io_source_decoder (decoder);

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

UNIT_SETUP(setup_io_shell_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_shell_unit_test) {
}

static void
io_shell_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_shell_decoder_1,
		test_shell_decoder_2,
		test_shell_decoder_3,
		0
	};
	unit->name = "io shell interpreter";
	unit->description = "io shell interpreter";
	unit->tests = tests;
	unit->setup = setup_io_shell_unit_test;
	unit->teardown = teardown_io_shell_unit_test;
}

void
run_ut_io_shell (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_shell_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

#endif /* IMPLEMENT_VERIFY_IO_SHELL */
#endif /* io_shell_H_ */
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
