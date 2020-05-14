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
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_SOCKETS.
 *
 */
#ifndef io_verify_core_sockets_H_
#define io_verify_core_sockets_H_
#include <io_verify.h>

void	run_ut_io_core_sockets (V_runner_t*);

# ifdef IMPLEMENT_VERIFY_IO_CORE_SOCKETS

TEST_BEGIN(test_io_address_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	io_address_t a;
	
	io_byte_memory_get_info (bm,&bmbegin);

	VERIFY (compare_io_addresses(io_invalid_address(),io_invalid_address()) == 0,NULL);
	VERIFY (compare_io_addresses(io_invalid_address(),def_io_u8_address(1)) == 1,NULL);
	VERIFY (compare_io_addresses(def_io_u8_address(1),io_invalid_address()) == -1,NULL);

	VERIFY (io_address_size(def_io_u8_address(1)) == 1,NULL);
	VERIFY (io_address_size(def_io_u16_address(1)) == 2,NULL);
	VERIFY (io_address_size(def_io_u32_address(1)) == 4,NULL);
	
	{
		uint8_t t = 42;
		a = mk_io_address(bm,1,&t);
		VERIFY (io_u8_address_value(a) == 42,NULL);
		free_io_address (bm,a);
	}
	
	{
		uint16_t t = 4296;
		a = mk_io_address(bm,2,(uint8_t const*) &t);
		VERIFY (io_u16_address_value(a) == t,NULL);
		free_io_address (bm,a);
	}

	{
		uint32_t t = 0x8000000;
		a = mk_io_address(bm,4,(uint8_t const*) &t);
		VERIFY (io_u32_address_value(a) == t,NULL);
		free_io_address (bm,a);
	}

	{
		uint8_t t[] = {1,0,0,0,0};
		a = mk_io_address (bm,sizeof(t),t);
		VERIFY (io_address_size(a) == sizeof(t),NULL);
		VERIFY (compare_io_addresses (a,def_io_u8_address(1)) == 0,NULL);
		VERIFY (compare_io_addresses (def_io_u8_address(1),a) == 0,NULL);
		VERIFY (compare_io_addresses (def_io_u8_address(2),a) == 1,NULL);
		VERIFY (compare_io_addresses (a,def_io_u8_address(2)) == -1,NULL);
		
		free_io_address (bm,a);
	}

	{
		uint8_t d1[] = {1,0,0,1,0,2};
		uint8_t d2[] = {1,0,0,1,0,2};
		uint8_t d3[] = {1,0,0,0,0,2};
		io_address_t a1 = mk_io_address (bm,sizeof(d1),d1);
		io_address_t a2 = mk_io_address (bm,sizeof(d2),d2);
		io_address_t a3 = mk_io_address (bm,sizeof(d3),d3);

		VERIFY (compare_io_addresses (a1,a2) == 0,NULL);
		VERIFY (compare_io_addresses (a1,a3) == 1,NULL);
		VERIFY (compare_io_addresses (a3,a2) == -1,NULL);

		free_io_address (bm,a1);
		free_io_address (bm,a2);
		free_io_address (bm,a3);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_address_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	io_address_t a,b;
	
	io_byte_memory_get_info (bm,&bmbegin);

	static const uint8_t x[] = {1,0,0,0,1};
	uint8_t t[] = {1,0,0,0,1};
	a = mk_io_address (bm,sizeof(t),t);
	b = def_io_const_address (SIZEOF(x),x);
	
	VERIFY (compare_io_addresses (a,b) == 0,NULL);

	assign_io_address (bm,&a,io_invalid_address());
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_address_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	io_address_t a,b;
	uint8_t buffer[16] = {0};
	
	io_byte_memory_get_info (bm,&bmbegin);

	{
		a = def_io_u8_address (1);
		b = io_invalid_address();
		VERIFY (write_le_io_address (buffer,16,b) == 1,NULL);
		VERIFY (buffer[0] == 0,NULL);
		
		VERIFY (read_le_io_address (bm,buffer,16,&a) == 1,NULL);
		VERIFY (compare_io_addresses (a,b) == 0,NULL);
	}
	
	{
		a = def_io_u8_address (1);
		b = io_invalid_address();
		VERIFY (write_le_io_address (buffer,16,a) == 2,NULL);
		VERIFY (buffer[0] == 1 && buffer[1] == 1,NULL);
		
		VERIFY (read_le_io_address (bm,buffer,16,&b) == 2,NULL);
		VERIFY (compare_io_addresses (a,b) == 0,NULL);
		free_io_address(bm,b);
	}

	{
		static const uint8_t x[] = {1,0,0,0,1};
		a = mk_io_address (bm,sizeof(x),x);
		b = io_invalid_address();
		VERIFY (write_le_io_address (buffer,16,a) == 6,NULL);
		VERIFY (buffer[0] == 5 && buffer[1] == 1,NULL);
		
		VERIFY (read_le_io_address (bm,buffer,16,&b) == 6,NULL);
		VERIFY (compare_io_addresses (a,b) == 0,NULL);
		free_io_address(bm,a);
		free_io_address(bm,b);
	}

	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_adapter_socket_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	
	io_byte_memory_get_info (bm,&bmbegin);

	static const uint8_t bytes[] = {'a',0,0,0,1};
	io_address_t a =  mk_io_address (bm,sizeof(bytes),bytes);
	
	const socket_builder_t net[] = {
		{0,allocate_io_adapter_socket,a,NULL,false,NULL},
	};
	io_socket_t* leaf[SIZEOF(net)];
	free_io_address(bm,a);
	
	build_io_sockets(TEST_IO,leaf,net,SIZEOF(net));
	
	VERIFY (cast_to_io_counted_socket(leaf[0]) != NULL,NULL);
	VERIFY (cast_to_io_adapter_socket(leaf[0]) != NULL,NULL);
	VERIFY (cast_to_io_multiplex_socket(leaf[0]) == NULL,NULL);
	
	io_wait_for_all_events (TEST_IO);
	free_io_sockets (leaf,leaf + SIZEOF(net));
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

uint32_t test_io_adapter_socket_2_result;

void
test_io_adapter_socket_2_rx_event (io_event_t *ev) {
	io_socket_t *this = ev->user_value;

	io_encoding_pipe_t* pipe = cast_to_io_encoding_pipe (
		io_socket_get_receive_pipe (
			this,io_socket_address (this)
		)
	);
	
	if (pipe) {
		io_encoding_t *rx;
		if (io_encoding_pipe_peek (pipe,&rx)) {
			const uint8_t *b,*e;
			io_encoding_get_content (rx,&b,&e);

			if (e - b == 4 && memcmp (b,"gook",4) == 0) {
				test_io_adapter_socket_2_result = 1;
			}
		}
	}
}

TEST_BEGIN(test_io_adapter_socket_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	const io_settings_t bus = {
		.transmit_pipe_length = 3,
		.receive_pipe_length = 3,
	};
	
	io_byte_memory_get_info (bm,&bmbegin);

	const socket_builder_t net[] = {
		{0,allocate_io_adapter_socket,def_io_u8_address(22),NULL,false,BINDINGS({0,1},END_OF_BINDINGS)},
		{1,allocate_io_socket_binary_emulator,def_io_u8_address(11),&bus,false,BINDINGS({1,2},END_OF_BINDINGS)},
		{2,allocate_io_shared_media,io_invalid_address(),&bus,false,NULL},
		{3,allocate_io_adapter_socket,def_io_u8_address(11),NULL,false,BINDINGS({3,4},END_OF_BINDINGS)},
		{4,allocate_io_socket_binary_emulator,def_io_u8_address(22),&bus,false,BINDINGS({4,2},END_OF_BINDINGS)},
	};
	io_socket_t* leaf[SIZEOF(net)];
	
	io_event_t rx;
	
	build_io_sockets(TEST_IO,leaf,net,SIZEOF(net));
	initialise_io_event (&rx,test_io_adapter_socket_2_rx_event,leaf[3]);
	
	io_socket_bind_inner (leaf[3],io_invalid_address(),NULL,&rx);
	
	VERIFY (cast_to_io_counted_socket(leaf[0]) != NULL,NULL);
	VERIFY (cast_to_io_counted_socket(leaf[1]) != NULL,NULL);
	VERIFY (cast_to_io_multiplexer_socket(leaf[1]) != NULL,NULL);

	{
		io_encoding_t *msg = io_socket_new_message(leaf[0]);
		if (VERIFY (msg != NULL,NULL)) {
			io_socket_open (leaf[0],IO_SOCKET_OPEN_CONNECT);
			io_socket_open (leaf[3],IO_SOCKET_OPEN_CONNECT);
			test_io_adapter_socket_2_result = 0;
			io_encoding_append_string (msg,"gook",4);
			VERIFY (io_socket_send_message (leaf[0],msg),NULL);
			io_wait_for_all_events (TEST_IO);
			VERIFY (test_io_adapter_socket_2_result == 1,NULL);
		}
	}
	
	free_io_sockets (leaf,leaf + SIZEOF(net));	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

void
test_io_multiplex_socket_1_notify (io_event_t *ev) {
}

bool
test_io_multiplex_socket_1_make_socket (
	io_t *io,io_address_t address,io_socket_t** inner,io_socket_t **outer
) {
	return false;
}

TEST_BEGIN(test_io_multiplex_socket_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	
	io_byte_memory_get_info (bm,&bmbegin);

	const socket_builder_t net[] = {
		{0,allocate_io_multiplex_socket,io_invalid_address(),NULL,false,NULL},
	};
	
	io_socket_t* mux[1];
	io_notify_event_t ev;
	
	initialise_io_notify (&ev,test_io_multiplex_socket_1_notify,NULL,NULL);
	
	build_io_sockets(TEST_IO,mux,net,1);
	VERIFY (cast_to_io_counted_socket(mux[0]) != NULL,NULL);
	VERIFY (cast_to_io_multiplex_socket (mux[0]) != NULL,NULL);
	VERIFY (cast_to_io_multiplexer_socket (mux[0]) == NULL,NULL);

	VERIFY (
		io_socket_bind_inner_constructor (
			mux[0],io_invalid_address(),test_io_multiplex_socket_1_make_socket,&ev
		),
		NULL
	);
	
	io_socket_free(mux[0]);	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_multiplexer_socket_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend;
	
	io_byte_memory_get_info (bm,&bmbegin);

	const socket_builder_t net[] = {
		{0,allocate_io_multiplexer_socket,io_invalid_address(),NULL,false,NULL},
	};
	io_socket_t* mux[1];

	build_io_sockets(TEST_IO,mux,net,1);	
	VERIFY (cast_to_io_counted_socket(mux[0]) != NULL,NULL);
	VERIFY (cast_to_io_multiplex_socket (mux[0]) != NULL,NULL);
	VERIFY (cast_to_io_multiplexer_socket (mux[0]) != NULL,NULL);

	io_socket_free(mux[0]);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	
}
TEST_END

UNIT_SETUP(setup_io_core_sockets_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_sockets_unit_test) {
}

static void
io_core_sockets_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_address_1,
		test_io_address_2,
		test_io_address_3,
		test_io_adapter_socket_1,
		test_io_adapter_socket_2,
		test_io_multiplex_socket_1,
		test_io_multiplexer_socket_1,
		0
	};
	unit->name = "io sockets";
	unit->description = "io sockets unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_sockets_unit_test;
	unit->teardown = teardown_io_core_sockets_unit_test;
}

void
run_ut_io_core_sockets (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_core_sockets_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE_SOCKETS */
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



