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
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_VALUES.
 *
 */
#ifndef io_verify_core_values_H_
#define io_verify_core_values_H_
#include <io_verify.h>

void	run_ut_io_core_values (V_runner_t*);

# ifdef IMPLEMENT_VERIFY_IO_CORE_VALUES

#define io_log_flush io_flush_log

TEST_BEGIN(test_io_sprintf_1) {
	char buf[16];
	uint64_t big = (1LL << 32);
	char const *expect = "4294967296";

	stbsp_snprintf (buf,sizeof(buf),"%llu",big);

	VERIFY(strcmp(buf,expect) == 0,NULL);

//	io_log(TEST_IO,IO_INFO_LOG_LEVEL,"%s\n",buf);
//	io_log_flush(TEST_IO);

}
TEST_END


TEST_BEGIN(test_io_text_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
		uint8_t byte;
		
		reference_io_encoding (encoding);
		VERIFY (is_io_text_encoding (encoding),NULL);
		VERIFY (is_io_binary_encoding (encoding),NULL);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		VERIFY (io_encoding_printf (encoding,"%s","abc") == 3,NULL);
		VERIFY (io_encoding_length (encoding) == 3,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 3,NULL);
		VERIFY (memcmp ("abc",b,3) == 0,NULL);
		byte = 0;
		VERIFY (io_encoding_pop_last_byte (encoding,&byte) && byte == 'c',NULL);
		io_encoding_reset (encoding);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		io_encoding_append_byte (encoding,0);

		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_text_encoding_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_text_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
		
		reference_io_encoding (encoding);

		VERIFY (io_encoding_fill (encoding,'a',4) == 4,NULL);
		VERIFY (io_encoding_length (encoding) == 4,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 4,NULL);
		VERIFY (memcmp ("aaaa",b,4) == 0,NULL);

		io_encoding_reset (encoding);
		io_encoding_append_byte (encoding,'c');
		VERIFY (io_encoding_fill (encoding,'b',250) == 250,NULL);
		io_encoding_get_content (encoding,&b,&e);
		VERIFY (memcmp ("cbbb",b,4) == 0,NULL);

		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_x70_encoding_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);

	io_encoding_t *encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		reference_io_encoding (encoding);

		VERIFY (is_io_x70_encoding (encoding),NULL);
		VERIFY (is_io_binary_encoding (encoding),NULL);
		VERIFY (io_encoding_length (encoding) == 0,NULL);

		VERIFY (io_encoding_printf (encoding,"%s","abc") == 3,NULL);
		VERIFY (io_encoding_length (encoding) == 3,NULL);

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 3,NULL);
		VERIFY (memcmp ("abc",b,3) == 0,NULL);

		io_encoding_reset (encoding);
		VERIFY (io_encoding_length (encoding) == 0,NULL);
		
		unreference_io_encoding(encoding);
	}

	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_constant_values_1) {

	VERIFY(vref_get_containing_memory (cr_NIL) == NULL,NULL);

	VERIFY(vref_is_valid(cr_VALUE),NULL);
	VERIFY (io_value_is_equal (cr_VALUE,cr_VALUE),NULL);

	VERIFY(vref_is_valid(cr_NIL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_NIL,cr_NIL) != NULL,NULL);
	VERIFY (io_value_is_equal (cr_NIL,cr_NIL),NULL);

	VERIFY (io_value_is_more (cr_VALUE,cr_NIL),NULL);
	VERIFY (io_value_is_less (cr_NIL,cr_VALUE),NULL);

	VERIFY(vref_is_valid(cr_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_BINARY) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_VALUE) != NULL,NULL);
	VERIFY (io_typesafe_ro_cast(cr_BINARY,cr_NIL) == NULL,NULL);

	VERIFY(vref_is_valid(cr_CONSTANT_BINARY),NULL);
	VERIFY (io_typesafe_ro_cast(cr_CONSTANT_BINARY,cr_BINARY) != NULL,NULL);

	VERIFY(vref_is_valid(cr_SYMBOL),NULL);
	VERIFY (io_typesafe_ro_cast(cr_SYMBOL,cr_CONSTANT_BINARY) != NULL,NULL);
	VERIFY (arg_match(cr_SYMBOL)(cr_SYMBOL),NULL);

	VERIFY(vref_is_valid(cr_IO_EXCEPTION),NULL);

	VERIFY(vref_is_valid(cr_RESULT),NULL);
	VERIFY(vref_is_valid(cr_RESULT_CONTINUE),NULL);

	extern EVENT_DATA io_value_implementation_t nil_value_implementation;
	extern EVENT_DATA io_value_implementation_t i64_number_value_implementation;
	extern EVENT_DATA io_value_implementation_t f64_number_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_binary_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_text_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_cons_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_list_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_map_slot_value_implementation;
	extern EVENT_DATA io_value_implementation_t io_map_value_implementation;
	io_value_implementation_t const* expect[] = {
		&io_value_implementation,
		&nil_value_implementation,
		&io_vector_value_implementation,
		&i64_number_value_implementation,
		&f64_number_value_implementation,
		&io_binary_value_implementation,
		&io_text_value_implementation,
		&io_cons_value_implementation,
		&io_list_value_implementation,
		&io_map_slot_value_implementation,
		&io_map_value_implementation,
	};
	bool ok = true;
	for (int i = 0; i < SIZEOF(expect) && ok; i++) {
		ok = expect[i] == io_get_value_implementation (TEST_IO,expect[i]->name,strlen(expect[i]->name));
	}
	VERIFY(ok,"value implementations match");

}
TEST_END

TEST_BEGIN(test_io_constant_values_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_encoding_t *encoding;
	const uint8_t *b,*e;

	encoding = mk_io_text_encoding (bm);

	VERIFY (io_encoding_printf (encoding,"%v",cr_NIL) == 3,NULL);
	io_encoding_get_content (encoding,&b,&e);
	VERIFY ((e - b) == 3,NULL);
	VERIFY (memcmp ("nil",b,3) == 0,NULL);

	io_encoding_reset (encoding);
	VERIFY (io_encoding_printf (encoding,"%v",cr_VALUE) == 1,NULL);
	io_encoding_get_content (encoding,&b,&e);
	VERIFY ((e - b) == 1,NULL);
	VERIFY (memcmp (".",b,1) == 0,NULL);
	
	io_encoding_free (encoding);
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
		VERIFY(vref_get_containing_memory (r_value) == vm,NULL);

		VERIFY (
				io_value_encode (r_value,(io_encoding_t*) &enc)
			&&	io_value_int64_encoding_encoded_value(&enc) == 42,
			NULL
		);

		r_decoded = io_encoding_decode_to_io_value (
			(io_encoding_t*) &enc,io_integer_to_integer_value_decoder,vm
		);
		if (VERIFY (vref_is_valid (r_decoded),NULL)) {
			vref_t r_small = mk_io_int64_value (vm,-2);
			
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

			VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
			VERIFY (io_value_is_more (r_decoded,r_small),NULL);
			VERIFY (io_value_is_less (r_small,r_value),NULL);
			VERIFY (io_value_not_equal (cr_NIL,r_value),NULL);
			
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

		io_encoding_get_content (encoding,&b,&e);
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

TEST_BEGIN(test_io_int64_value_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_int64_value (vm,42));

	encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		uint8_t expect[] = {
			X70_UINT_VALUE_BYTE,3,
			'i','6','4',
			42,0,0,0,0,0,0,0
		};
		
		VERIFY (io_value_encode (r_value,encoding),NULL);

		io_encoding_get_content (encoding,&b,&e);
		if (
			VERIFY (
				(
						(e - b) == sizeof(expect) 
					&& memcmp (expect,b,sizeof(expect)) == 0
				),
				NULL
			)
		) {
			vref_t r_decoded = io_encoding_decode_to_io_value (
				encoding,io_x70_decoder,vm
			);
		
			if (
				VERIFY (
						vref_is_valid(r_decoded) 
					&&	io_typesafe_ro_cast (r_decoded,cr_I64_NUMBER),
					NULL
				)
			) {
				int64_t i64_value;
				VERIFY (
						io_value_get_as_int64 (r_decoded,&i64_value)
					&&	i64_value == 42,
					NULL
				);
			}
		}
		
		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
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

		io_encoding_get_content (encoding,&b,&e);
		VERIFY ((e - b) == 9,NULL);
		VERIFY (memcmp ("42.100000",b,2) == 0,NULL);

		unreference_value (r_value);
		io_encoding_free(encoding);
	}

	io_do_gc (TEST_IO,-1);
}
TEST_END

TEST_BEGIN(test_io_float64_value_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	io_encoding_t *encoding;
	vref_t r_value;

	io_byte_memory_get_info (bm,&bm_begin);

	r_value = reference_value (mk_io_float64_value (vm,42.1));

	encoding = mk_io_x70_encoding (bm);

	if (VERIFY(encoding != NULL,NULL)) {
		const uint8_t *b,*e;
	
		uint8_t expect[] = {
			X70_UINT_VALUE_BYTE,3,
			'f','6','4',
			205,204,204,204,204,12,69,64
		};
		
		VERIFY (io_value_encode (r_value,encoding),NULL);

		io_encoding_get_content (encoding,&b,&e);
		if (
			VERIFY (
				(
						(e - b) == sizeof(expect) 
					&& memcmp (expect,b,sizeof(expect)) == 0
				),
				NULL
			)
		) {
			vref_t r_decoded = io_encoding_decode_to_io_value (
				encoding,io_x70_decoder,vm
			);
		
			if (
				VERIFY (
						vref_is_valid(r_decoded) 
					&&	io_typesafe_ro_cast (r_decoded,cr_F64_NUMBER),
					NULL
				)
			) {
				vref_t r_small = mk_io_float64_value (vm,12.1);
				float64_t f64_value;
				VERIFY (
						io_value_get_as_float64 (r_decoded,&f64_value)
					&&	io_math_compare_float64_eq (f64_value,42.1),
					NULL
				);
				VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
				VERIFY (io_value_is_less (r_small,r_value),NULL);
				VERIFY (io_value_is_more (r_value,r_small),NULL);
			}
		}

		io_encoding_free(encoding);
	}

	unreference_value (r_value);

	io_do_gc (TEST_IO,-1);
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
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
				io_encoding_get_content (encoding,&b,&e);
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
				io_encoding_get_content (encoding,&b,&e);
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
				io_encoding_get_content (encoding,&b,&e);
				VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
			}

			io_encoding_free(encoding);
		}
		unreference_value (r_value);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_text_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;

	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_text_value (vm,(uint8_t const*)"abc",3));

	if (VERIFY(vref_is_valid(r_value),NULL)) {
		io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
		io_encoding_t *encoding = mk_io_x70_encoding (bm);

		if (VERIFY (io_value_encode (r_value,encoding),NULL)) {

			const uint8_t *b,*e;
		
			uint8_t expect[] = {
				X70_UINT_VALUE_BYTE,4,
				't','e','x','t',
				X70_UINT_VALUE_BYTE,3,
				'a','b','c'
			};

			io_encoding_get_content (encoding,&b,&e);

			if (
				VERIFY (
					(
							(e - b) == sizeof(expect)
						&& memcmp (expect,b,sizeof(expect)) == 0
					),
					NULL
				)
			) {
				vref_t r_decoded = io_encoding_decode_to_io_value (
					encoding,io_x70_decoder,vm
				);
			
				if (
					VERIFY (
							vref_is_valid(r_decoded) 
						&&	io_typesafe_ro_cast (r_decoded,cr_TEXT),
						NULL
					)
				) {
					io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
					io_encoding_t *e1 = mk_io_text_encoding (bm);
					vref_t r_long = mk_io_text_value (vm,(uint8_t const*)"abcd",4);
					
					if (VERIFY (io_value_encode (r_decoded,e1),NULL)) {
						const uint8_t *b,*e;
						io_encoding_get_content (e1,&b,&e);
						VERIFY ((e - b) == 3 && memcmp(b,"abc",3) == 0,NULL);
					}

					VERIFY (io_value_is_equal (r_decoded,r_value),NULL);
					VERIFY (io_value_is_less (r_decoded,r_long),NULL);
					VERIFY (io_value_is_more (r_long,r_value),NULL);
					
					io_encoding_free(e1);
				}
			}
		
			io_encoding_free(encoding);
		}
		unreference_value (r_value);
	}

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
	VERIFY (io_vector_value_get_values (r_vect,&arity,&args) && vref_is_nil(args[0]),NULL);
}
TEST_END

TEST_BEGIN(test_vector_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = reference_value (mk_io_vector_value (vm,0,NULL));
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		uint32_t arity = 42;
		VERIFY (io_vector_value_get_arity (r_value,&arity) && arity == 0,NULL);
		unreference_value (r_value);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_vector_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);
	
	{
		vref_t r_int = mk_io_int64_value (vm,42);
		vref_t args[] = {cr_NIL,r_int};
		vref_t const *values;
		
		r_value = reference_value (mk_io_vector_value (vm,SIZEOF(args),args));
		if (VERIFY(vref_is_valid(r_value),NULL)) {
			uint32_t arity = 42;
			VERIFY (io_vector_value_get_arity (r_value,&arity) && arity == 2,NULL);

			if (VERIFY (io_vector_value_get_values (r_value,&arity,&values),NULL)) {
				VERIFY (vref_is_nil (values[0]),NULL);
				VERIFY (vref_is_equal_to (values[1],r_int),NULL);
			}
			
			unreference_value (r_value);
		}
	}
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_cons_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value,r_car;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = mk_io_cons_value (vm,cr_NIL,cr_NIL,cr_NIL);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_part;
		
		VERIFY (io_typesafe_ro_cast (r_value,cr_CONS) != NULL,NULL);

		r_part = cr_VALUE;
		VERIFY (io_cons_value_get_car (r_value,&r_part) && vref_is_nil(r_part),NULL);
		r_part = cr_VALUE;
		VERIFY (io_cons_value_get_cdr (r_value,&r_part) && vref_is_nil(r_part),NULL);

	}

	r_car =  mk_io_int64_value (vm,-2);
	r_value = mk_io_cons_value (vm,r_car,cr_NIL,cr_NIL);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_part;
		VERIFY (io_typesafe_ro_cast (r_value,cr_CONS) != NULL,NULL);
		
		VERIFY (io_value_is_equal (r_value,r_value),NULL);
		r_part = cr_VALUE;
		VERIFY (io_cons_value_get_car (r_value,&r_part) && vref_is_equal_to(r_part,r_car),NULL);
		r_part = cr_VALUE;
		VERIFY (io_cons_value_get_cdr (r_value,&r_part) && vref_is_nil(r_part),NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_list_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_value;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_value = mk_io_list_value (vm);
	if (VERIFY(vref_is_valid(r_value),NULL)) {
		vref_t r_pop;
		
		VERIFY (io_typesafe_ro_cast (r_value,cr_LIST) != NULL,NULL);
		VERIFY (vref_get_containing_memory (r_value) == vm,NULL);
		
		VERIFY (io_list_value_count (r_value) == 0,NULL);
		
		{
			io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
			io_encoding_t *encoding;
			encoding = mk_io_text_encoding (bm);
			if (VERIFY (io_encoding_printf (encoding,"%v",r_value) == 2,NULL)) {
				const uint8_t *b,*e;
				io_encoding_get_content (encoding,&b,&e);
				VERIFY (memcmp(b,"()",2) == 0,NULL);
			}
			
			io_encoding_free(encoding);
		}
		
		io_list_value_append_value (r_value,cr_NIL);
		VERIFY (io_list_value_count (r_value) == 1,NULL);
		r_pop = cr_LIST;
		VERIFY (io_list_pop_last (r_value,&r_pop) && vref_is_nil (r_pop),NULL);
		VERIFY (io_list_value_count (r_value) == 0,NULL);

		io_list_value_append_value (r_value,cr_NIL);
		VERIFY (io_list_value_count (r_value) == 1,NULL);
		r_pop = cr_LIST;
		VERIFY (io_list_pop_first (r_value,&r_pop) && vref_is_nil (r_pop),NULL);
		VERIFY (io_list_value_count (r_value) == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_list_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_list;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_list = mk_io_list_value (vm);

	io_list_value_append_value (r_list,mk_io_int64_value (vm,42));

	{
		io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
		io_encoding_t *encoding;
		encoding = mk_io_text_encoding (bm);
		if (VERIFY (io_encoding_printf (encoding,"%v",r_list) == 4,NULL)) {
			const uint8_t *b,*e;
			io_encoding_get_content (encoding,&b,&e);
			VERIFY (memcmp(b,"(42)",4) == 0,NULL);
		}
		
		io_encoding_free(encoding);
	}
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

static bool
test_io_map_value_count_cb (vref_t r_slot,void *result) {
	(*((int*) result))++;
	return true;
}

TEST_BEGIN(test_map_value_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_map;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_map = mk_io_map_value (vm,3);
	if (VERIFY(vref_is_valid(r_map),NULL)) {
		vref_t r_mapped;
		int result;
		
		VERIFY (io_typesafe_ro_cast (r_map,cr_MAP) != NULL,NULL);
		VERIFY (vref_get_containing_memory (r_map) == vm,NULL);

		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 0,NULL);
		
		VERIFY (io_map_value_map (r_map,cr_NIL,cr_NIL),NULL);
		VERIFY (
			(
					io_map_value_get_mapping (r_map,cr_NIL,&r_mapped) 
				&& vref_is_nil(r_mapped)
			),
			NULL
		);
		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 1,NULL);
		
		r_mapped = io_map_value_unmap (r_map,cr_NIL);
		VERIFY (vref_is_nil (r_mapped),NULL);

		result = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&result);
		VERIFY(result == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
}
TEST_END

/*
				int64_t i64_value;
				VERIFY (
						io_value_get_as_int64 (r_decoded,&i64_value)
					&&	i64_value == 42,
					NULL
				);

*/
static bool
test_io_map_value_3_cb (vref_t r_slot,void *result) {
	int64_t v;
	if (io_value_get_as_int64 (io_map_slot_value_get_key(r_slot),&v)) {
		int **a = result;
		*(*a) = v;
		(*a)++;
	}
	return true;
}

TEST_BEGIN(test_map_value_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	memory_info_t vm_begin,vm_end;
	vref_t r_map;
	
	io_value_memory_get_info (vm,&vm_begin);

	r_map = mk_io_map_value (vm,3);
	if (VERIFY(vref_is_valid(r_map),NULL)) {
		vref_t r_key[8];
		bool ok;
		int count;
		
		ok = true;
		for (int i = SIZEOF(r_key) - 1; i >= 0; i--) {
			r_key[i] = mk_io_int64_value (vm,i);
			ok &= io_map_value_map (r_map,r_key[i],cr_NIL);
		}
		VERIFY(ok,"all new slots");

		count = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&count);
		VERIFY(count == SIZEOF(r_key),NULL);

		int rr[SIZEOF(r_key)] = {0},*c = rr;
		io_map_value_iterate (r_map,test_io_map_value_3_cb,&c);

		ok = true;
		for (int i = 0; i < SIZEOF(r_key); i++) {
			ok &= (rr[i] == i);
		}
		VERIFY (ok,"increasing order");

		ok = true;
		for (int i = SIZEOF(r_key) - 1; i >= 0; i--) {
			vref_t r_removed = io_map_value_unmap (r_map,r_key[i]);
			ok &= vref_is_nil(r_removed);
		}
		VERIFY(ok,"all slots unmaped");

		count = 0;
		io_map_value_iterate (r_map,test_io_map_value_count_cb,&count);
		VERIFY(count == 0,NULL);
	}

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);
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

static void
io_core_values_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_sprintf_1,
		test_io_text_encoding_1,
		test_io_text_encoding_2,
		test_io_x70_encoding_1,
		test_io_constant_values_1,
		test_io_constant_values_2,
		test_io_int64_value_1,
		test_io_int64_value_2,
		test_io_int64_value_3,
		test_io_float64_value_1,
		test_io_float64_value_2,
		test_io_float64_value_3,
		test_io_binary_value_with_const_bytes_1,
		test_io_binary_value_dynamic_1,
		test_io_text_value_1,
		test_io_text_value_2,
		test_stack_vector_value_1,
		test_vector_value_1,
		test_vector_value_2,
		test_cons_value_1,
		test_list_value_1,
		test_list_value_2,
		test_map_value_1,
		test_map_value_2,
		0
	};
	unit->name = "io_values";
	unit->description = "io core values unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_values_unit_test;
	unit->teardown = teardown_io_core_values_unit_test;
}

void
run_ut_io_core_values (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_core_values_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

# endif /* IMPLEMENT_VERIFY_IO_CORE_VALUES */
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
