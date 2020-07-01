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
 * In one C-source file define IMPLEMENT_VERIFY_IO_CORE_CONTAINERS.
 *
 */
#ifndef io_verify_core_containers_H_
#define io_verify_core_containers_H_
#include <io_verify.h>

void	run_ut_io_core_containers (V_runner_t*);

#ifdef IMPLEMENT_VERIFY_IO_CORE_CONTAINERS
//-----------------------------------------------------------------------------
//
// implementation
//
//-----------------------------------------------------------------------------

/*
 *-----------------------------------------------------------------------------
 *
 * STACK_RAM_BLOCK_MEMORY --
 *
 * macro to declare and initialise a byte memory using the C stack
 * for storage
 *
 *-----------------------------------------------------------------------------
 */
#define STACK_IO_BYTE_MEMORY(name,size,block_size_bits,IO) \
	uint8_t name##_bytes[size];	\
	io_byte_memory_t name##_s = {\
		.heap = (umm_block_t*) name##_bytes,\
		.number_of_blocks = ((size) >> (block_size_bits)),\
		.block_size_n = block_size_bits,\
	};	\
	initialise_io_byte_memory (	\
		IO,&name##_s,block_size_bits	\
	);\
	io_byte_memory_t *name = &name##_s; \
	UNUSED(name);\

/*
 *-----------------------------------------------------------------------------
 *
 * API for byte memory tests
 *  
 *-----------------------------------------------------------------------------
 */
typedef struct {
	uint16_t block;
	bool  is_free;
	uint16_t next;
	uint16_t prev;
	uint16_t next_free;
	uint16_t prev_free;
	bool print;
} io_byte_memory_test_values_t;

bool io_byte_memory_test_block_is_free (io_byte_memory_t*,uint16_t);
uint16_t io_byte_memory_test_get_block_prev (io_byte_memory_t*,uint16_t);
uint16_t io_byte_memory_test_get_block_next (io_byte_memory_t*,uint16_t);
uint16_t io_byte_memory_test_get_block_prev_free (io_byte_memory_t*,uint16_t);
uint16_t io_byte_memory_test_get_block_next_free (io_byte_memory_t*,uint16_t);

bool
io_byte_memory_tests (
	io_byte_memory_t *memory,
	io_byte_memory_test_values_t const *test,
	io_byte_memory_test_values_t const *end
) {
	bool result = true;
	
	while (test < end) {
		if (test->print) {
			#if 1
			io_printf (
				io_byte_memory_io(memory),
				"Test block %u, is free=%u,next=%u, prev=%u, nf=%u, pf=%u\n",
				test->block,
				io_byte_memory_test_block_is_free(memory,test->block),
				io_byte_memory_test_get_block_next(memory,test->block),
				io_byte_memory_test_get_block_prev(memory,test->block),
				io_byte_memory_test_get_block_next_free(memory,test->block),
				io_byte_memory_test_get_block_prev_free(memory,test->block)
			);
			#endif
		}

		result &= (
				(test->is_free 	== io_byte_memory_test_block_is_free(memory,test->block))
			&&	(test->next			== io_byte_memory_test_get_block_next(memory,test->block))
			&&	(test->prev			== io_byte_memory_test_get_block_prev(memory,test->block))
			&&	(test->next_free	== io_byte_memory_test_get_block_next_free(memory,test->block))
			&&	(test->prev_free	== io_byte_memory_test_get_block_prev_free(memory,test->block))
		);
		test++;
	}
	
	return result;
}

static bool
test_io_byte_memory_1 (io_t *io,uint32_t bs) {
	bool result = true;
	
	STACK_IO_BYTE_MEMORY(bm,256,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(bm);
	io_byte_memory_test_values_t test[] = {
		{0				, false, 1        , 0, 1, 1,false},
		{1				, true,  lastblock, 0, 0, 0,false},
		{lastblock	, false, 0			, 1, 0, 0,false}
	};
	
	result &= io_byte_memory_tests (bm,test,test + SIZEOF(test));
	result &= io_byte_memory_allocate(bm,0) == NULL;
	result &= io_byte_memory_tests (bm,test,test + SIZEOF(test));
	
	return result;
}

static bool
test_io_byte_memory_2 (io_t *io,uint32_t bs) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(bm,256,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(bm);
	memory_info_t meminfo;
	void *obj;

	{
		io_byte_memory_test_values_t test[] = {
			{0,			false,	1,				0, 2, 2,false},
			{1,			false,	2,				0, 0, 0,false},
			{2,			true,		lastblock,	1, 0, 0,false},
			{lastblock,	false,	0,				2, 0, 0,false}
		};
		
		obj = io_byte_memory_allocate ((io_byte_memory_t*) bm,1);
		result &= io_byte_memory_tests (bm,test,test + SIZEOF(test));
		result &= io_byte_memory_free ((io_byte_memory_t*) bm,obj) == IO_MEMORY_FREE_OK;
		result &= io_byte_memory_free ((io_byte_memory_t*) bm,obj) == IO_MEMORY_FREE_ERROR_ALREADY_FREE;
		result &= io_byte_memory_free (bm,(void*) &meminfo) == IO_MEMORY_FREE_ERROR_NOT_IN_MEMORY;

		io_byte_memory_get_info (bm,&meminfo);
		result &= meminfo.used_bytes == 0;
	}

	{
		io_byte_memory_test_values_t test[] = {
			{0				, false, 1        , 0, 1, 1,false},
			{1				, true,  lastblock, 0, 0, 0,false},
			{lastblock	, false, 0			, 1, 0, 0,false}
		};
		result &= io_byte_memory_tests (bm,test,test + SIZEOF(test));
	}
	
	obj = io_byte_memory_allocate(bm,255);
	result &= (obj == NULL);

	return result;
}

static bool
test_io_byte_memory_3 (io_t *io,uint32_t bs) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(bm,256,bs,io);
	STACK_IO_BYTE_MEMORY(memory,256,UMM_BLOCK_SIZE_1N,io);
	uint16_t lastblock = io_byte_memory_last_block(memory);
	memory_info_t meminfo;
	void *obj;

	io_byte_memory_test_values_t test_empty[] = {
		{0				, false, 1        , 0, 1, 1,false},
		{1				, true,  lastblock, 0, 0, 0,false},
		{lastblock	, false, 0			, 1, 0, 0,false}
	};

	{
		io_byte_memory_test_values_t test[] = {
			{0,			false,	1,				0, 4, 4,false},
			{1,			false,	4,				0, 0, 0,false},
			{4,			true,		lastblock,	1, 0, 0,false},
			{lastblock,	false,	0,				4, 0, 0,false}
		};
		
		obj = io_byte_memory_allocate (memory,15);

		result &= (obj != NULL);
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
		
		result &= (
			io_byte_memory_free ((io_byte_memory_t*) memory,obj) == IO_MEMORY_FREE_OK
		);

		io_byte_memory_get_info ((io_byte_memory_t*) memory,&meminfo);
		result &= (meminfo.used_bytes == 0);
	}

	{
		result &= (
			io_byte_memory_tests (
				memory,test_empty,test_empty + SIZEOF(test_empty)
			)
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{0,			false,	1,				0, 0, 0,false},
			{1,			false,	lastblock,	0, 0, 0,false},
			{lastblock,	false,	0,				1, 0, 0,false}
		};

		// 236 = (256 - 16) - 4
		obj = io_byte_memory_allocate(memory,236);

		result &= (obj != NULL);
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
		
		result &= (
			io_byte_memory_free (memory,obj) == IO_MEMORY_FREE_OK
		);
	}
	
	{
		result &= (
			io_byte_memory_tests (
				memory,test_empty,test_empty + SIZEOF(test_empty)
			)
		);
	}
	
	return result;
}

static bool
test_io_byte_memory_4 (io_t *io,uint32_t bs) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(memory,256,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(memory);
	memory_info_t meminfo;
	void* obj[5];
	uint32_t i;
	
	for (i = 0; i < SIZEOF(obj); i++) {
		obj[i] = io_byte_memory_allocate (memory,4);
		result &= (obj[i] != NULL);
	}
	
	for (i = 0; i < (SIZEOF(obj) - 1); i++) {
		result &= (obj[i] != obj[i + 1]);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{0,			false, 1,			0, 6, 6,false},
			{1,			false, 2,			0, 0, 0,false},
			{2,			false, 3,			1, 0, 0,false},
			{3,			false, 4,			2, 0, 0,false},
			{4,			false, 5,			3, 0, 0,false},
			{5,			false, 6,			4, 0, 0,false},
			{6,			true, lastblock,	5, 0, 0,false},
			{lastblock, false, 0,			6, 0, 0,false}
		};
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	for (i = 0; i < SIZEOF(obj) && result; i++) {
		result &= (io_byte_memory_free (memory,obj[i]) == IO_MEMORY_FREE_OK);
	}
	
	io_byte_memory_get_info (memory,&meminfo);
	result &= (meminfo.used_bytes == 0);
	
	return result;
}

static bool
test_io_byte_memory_5 (io_t *io,uint32_t bs,uint32_t chunk_size) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(memory,1024,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(memory);
	memory_info_t meminfo;
	void* obj[5];
	uint32_t i;
	
	for (i = 0; i < SIZEOF(obj) && result; i++) {
		obj[i] = io_byte_memory_allocate (memory,chunk_size);
		result &= (obj[i] != NULL);
	}
	
	for (i = 0; i < (SIZEOF(obj) - 1) && result; i++) {
		result &= (obj[i] != obj[i + 1]);
	}

	{
		// 15 blocks
		bool pr = false;
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0, 16, 16,pr},
			{ 1,			false,  4,			 0,  0,  0,pr},
			{ 4,			false,  7,			 1,  0,  0,pr},
			{ 7,			false, 10,			 4,  0,  0,pr},
			{10,			false, 13,			 7,  0,  0,pr},
			{13,			false, 16,			10,  0,  0,pr},
			{16,			true, lastblock,	13,  0,  0,pr},
			{lastblock, false, 0,			16,  0,  0,pr}
		};
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	for (i = 0; i < SIZEOF(obj) && result; i++) {
		result &= (io_byte_memory_free (memory,obj[i]) == IO_MEMORY_FREE_OK);
	}
	
	io_byte_memory_get_info (memory,&meminfo);
	result &= (meminfo.used_bytes == 0);
	
	return result;
}

static bool
test_io_byte_memory_6 (io_t *io,uint32_t bs,uint32_t chunk_size) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(memory,1024,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(memory);
	memory_info_t meminfo;
	void* obj[5];
	uint32_t i;
	
	for (i = 0; i < SIZEOF(obj) && result; i++) {
		obj[i] = io_byte_memory_allocate ((io_byte_memory_t*) memory,chunk_size);
		result &= (obj[i] != NULL);
	}
	
	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0, 10, 16,false},
			{ 1,			false,  4,			 0,  0,  0,false},
			{ 4,			false,  7,			 1,  0,  0,false},
			{ 7,			false, 10,			 4,  0,  0,false},
			{10,			true,  13,			 7, 16,  0,false},
			{13,			false, 16,			10,  0,  0,false},
			{16,			true, lastblock,	13,  0, 10,false},
			{lastblock, false, 0,			16,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_free (memory,obj[3]) == IO_MEMORY_FREE_OK
		);

		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  7, 16,false},
			{ 1,			false,  4,			 0,  0,  0,false},
			{ 4,			false,  7,			 1,  0,  0,false},
			{ 7,			true,  13,			 4, 16,  0,false},
			{13,			false, 16,			 7,  0,  0,false},
			{16,			true, lastblock,	13,  0,  7,false},
			{lastblock, false, 0,			16,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_free (memory,obj[2]) == IO_MEMORY_FREE_OK
		);

		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  7,  7,false},
			{ 1,			false,  4,			 0,  0,  0,false},
			{ 4,			false,  7,			 1,  0,  0,false},
			{ 7,			true,  lastblock,	 4,  0,  0,false},
			{lastblock, false, 0,			 7,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_free (memory,obj[4]) == IO_MEMORY_FREE_OK
		);

		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  1,  7,false},
			{ 1,			true,   4,			 0,  7,  0,false},
			{ 4,			false,  7,			 1,  0,  0,false},
			{ 7,			true,  lastblock,	 4,  0,  1,false},
			{lastblock, false, 0,			 7,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_free (memory,obj[0]) == IO_MEMORY_FREE_OK
		);

		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  1,  1,false},
			{ 1,			true,  lastblock,	 0,  0,  0,false},
			{lastblock, false, 0,			 1,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_free (memory,obj[1]) == IO_MEMORY_FREE_OK
		);

		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}
	
	io_byte_memory_get_info (memory,&meminfo);
	result &= (meminfo.used_bytes == 0);
	
	return result;
}

static bool
test_io_byte_memory_7 (io_t *io,uint32_t bs) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(memory,256,bs,io);
	memory_info_t meminfo;
	void* obj;
	
	obj = io_byte_memory_allocate (memory,0);
	result &= (obj == NULL);
	
	obj = io_byte_memory_reallocate (memory,NULL,0);
	result &= (obj == NULL);

	obj = io_byte_memory_reallocate (memory,NULL,4);
	result &= (obj != NULL);

	io_byte_memory_get_info (memory,&meminfo);
	result &= (meminfo.used_bytes > 0);
	
	result &= (
		io_byte_memory_free (memory,obj) == IO_MEMORY_FREE_OK
	);
	
	io_byte_memory_get_info (memory,&meminfo);
	result &= (meminfo.used_bytes == 0);	
	return result;
}

static bool
test_io_byte_memory_8 (io_t *io,uint32_t bs,uint32_t chunk_size) {
	bool result = true;
	STACK_IO_BYTE_MEMORY(memory,1024,bs,io);
	uint16_t lastblock = io_byte_memory_last_block(memory);
	memory_info_t meminfo;
	void* obj[5];
	uint32_t i;
	
	for (i = 0; i < SIZEOF(obj); i++) {
		obj[i] = io_byte_memory_allocate (memory,chunk_size);
		result &= (obj[i] != NULL);
	}
	
	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0, 16, 16,false},
			{ 1,			false,  4,			 0,  0,  0,false},
			{ 4,			false,  7,			 1,  0,  0,false},
			{ 7,			false, 10,			 4,  0,  0,false},
			{10,			false, 13,			 7,  0,  0,false},
			{13,			false, 16,			10,  0,  0,false},
			{16,			true, lastblock,	13,  0,  0,false},
			{lastblock, false, 0,			16,  0,  0,false}
		};
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		bool pr = false;
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  7, 22,pr},
			{ 1,			false,  4,			 0,  0,  0,pr},
			{ 4,			false,  7,			 1,  0,  0,pr},
			{ 7,			true,  10,			 4, 22,  0,pr},
			{10,			false, 13,			 7,  0,  0,pr},
			{13,			false, 16,			10,  0,  0,pr},
			{16,			false, 22,			13,  0,  0,pr},
			{22,			true, lastblock,	16,  0,  7,pr},
			{lastblock, false, 0,			22,  0,  0,pr}
		};
		
		obj[2] = io_byte_memory_reallocate (memory,obj[2],chunk_size * 2);
		result &= (obj[2] != NULL);
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	{
		io_byte_memory_test_values_t test[] = {
			{ 0,			false,  1,			 0,  1,  1,false},
			{ 1,			true,  lastblock,	 0,  0,  0,false},
			{lastblock, false, 0,			 1,  0,  0,false}
		};
		
		for (i = 0; i < SIZEOF(obj) && result; i++) {
			result &= (io_byte_memory_free (memory,obj[i]) == IO_MEMORY_FREE_OK);
		}
		
		result &= (
			io_byte_memory_tests (memory,test,test + SIZEOF(test))
		);
	}

	io_byte_memory_get_info ((io_byte_memory_t*) memory,&meminfo);
	result &= (meminfo.used_bytes == 0);
	
	return result;
}

TEST_BEGIN(test_io_byte_memory_block_size_1) {
	VERIFY (test_io_byte_memory_1(TEST_IO,UMM_BLOCK_SIZE_1N),NULL);
	VERIFY (test_io_byte_memory_2(TEST_IO,UMM_BLOCK_SIZE_1N),NULL);
	VERIFY (test_io_byte_memory_3(TEST_IO,UMM_BLOCK_SIZE_1N),NULL);
	VERIFY (test_io_byte_memory_4(TEST_IO,UMM_BLOCK_SIZE_1N),NULL);
	VERIFY (test_io_byte_memory_5(TEST_IO,UMM_BLOCK_SIZE_1N,20),NULL);
	VERIFY (test_io_byte_memory_6(TEST_IO,UMM_BLOCK_SIZE_1N,20),NULL);
	VERIFY (test_io_byte_memory_7(TEST_IO,UMM_BLOCK_SIZE_1N),NULL);
	VERIFY (test_io_byte_memory_8(TEST_IO,UMM_BLOCK_SIZE_1N,20),NULL);
}
TEST_END

TEST_BEGIN(test_io_byte_memory_block_size_2) {
	uint32_t bs = UMM_BLOCK_SIZE_2N;
	VERIFY (test_io_byte_memory_1(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_2(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_3(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_4(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_5(TEST_IO,bs,44),NULL);
	VERIFY (test_io_byte_memory_6(TEST_IO,bs,44),NULL);
	VERIFY (test_io_byte_memory_7(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_8(TEST_IO,bs,44),NULL);
}
TEST_END

TEST_BEGIN(test_io_byte_memory_block_size_3) {
	uint32_t bs = UMM_BLOCK_SIZE_3N;
	VERIFY (test_io_byte_memory_1(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_2(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_3(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_4(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_5(TEST_IO,bs,88),NULL);
	VERIFY (test_io_byte_memory_6(TEST_IO,bs,88),NULL);
	VERIFY (test_io_byte_memory_7(TEST_IO,bs),NULL);
	VERIFY (test_io_byte_memory_8(TEST_IO,bs,88),NULL);
}
TEST_END

UNIT_SETUP(setup_io_byte_memory_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_byte_memory_unit_test) {
}

static void
io_byte_memory_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_byte_memory_block_size_1,
		test_io_byte_memory_block_size_2,
		test_io_byte_memory_block_size_3,
		0
	};
	unit->name = "byte memory";
	unit->description = "byte memory block size 1 unit test";
	unit->tests = tests;
	unit->setup = setup_io_byte_memory_unit_test;
	unit->teardown = teardown_io_byte_memory_unit_test;
}

TEST_BEGIN(test_io_memories_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *mem,*bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	VERIFY (bm != NULL,NULL);
	VERIFY (io_value_memory_get_io(vm) == TEST_IO,NULL);

	io_byte_memory_get_info (bm,&bm_begin);

	mem = mk_io_byte_memory (TEST_IO,32,UMM_BLOCK_SIZE_1N);
	if (VERIFY (mem != NULL,NULL)) {
		memory_info_t begin,end;
		void *data;
		
		io_byte_memory_get_info (mem,&begin);
		VERIFY (begin.used_bytes == 0,NULL);
		
		data = io_byte_memory_allocate (mem,12);
		VERIFY (data != NULL,NULL);
	
		io_byte_memory_free (mem,data);
	
		io_byte_memory_get_info (mem,&end);
		VERIFY (end.used_bytes == begin.used_bytes,NULL);
		
		free_io_byte_memory (mem);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_memories_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);
	
	io_value_memory_t *vm = mk_umm_io_value_memory (
		TEST_IO,1024,INVALID_MEMORY_ID
	);
	
	if (VERIFY (vm != NULL,NULL)) {
		memory_info_t begin,end;
		vref_t r_value;
		bool ok;
		
		io_value_memory_get_info (vm,&begin);
		VERIFY (begin.used_bytes == 0,NULL);

		ok = true;
		for (int i = 0; i < 8; i++) {
			r_value = mk_io_int64_value (vm,42);
			ok &= vref_is_valid(r_value);
		}
		
		VERIFY (ok,"values created");
		io_value_memory_do_gc (vm,-1);
		io_value_memory_get_info (vm,&end);
		VERIFY (end.used_bytes == 0,NULL);

		ok = true;
		for (int i = 0; i < 16; i++) {
			r_value = mk_io_int64_value (vm,42);
			ok &= vref_is_valid(r_value);
		}
		
		VERIFY (ok,"values created");
		io_value_memory_do_gc (vm,-1);
		io_value_memory_get_info (vm,&end);
		VERIFY (end.used_bytes == 0,NULL);
		
		umm_value_memory_free_memory (vm);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
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
		VERIFY (io_byte_pipe_is_writeable (pipe),NULL);
		VERIFY (io_byte_pipe_get_byte (pipe,&data) && data == 42,NULL);
		VERIFY (!io_byte_pipe_is_readable (pipe),NULL);
		VERIFY (io_byte_pipe_is_writeable (pipe),NULL);
		
		VERIFY (is_io_byte_pipe ((io_pipe_t*) (pipe)),NULL);
		
		free_io_byte_pipe (pipe,bm);
	}
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_encoding_pipe_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	io_byte_memory_get_info (bm,&bm_begin);

	io_encoding_pipe_t *pipe = mk_io_encoding_pipe (bm,4);
	if (VERIFY (pipe != NULL,NULL)) {
		io_encoding_t *data = NULL, *encoding;
		VERIFY (!io_encoding_pipe_is_readable (pipe),NULL);
		encoding = mk_io_text_encoding (bm);
		VERIFY (io_encoding_pipe_put_encoding (pipe,encoding),NULL);
		VERIFY (io_encoding_pipe_is_readable (pipe),NULL);
		VERIFY (io_encoding_pipe_is_writeable (pipe),NULL);
		VERIFY (io_encoding_pipe_peek (pipe,&data) && data == encoding,NULL);
		io_encoding_pipe_pop_encoding (pipe);
		
		VERIFY (!io_encoding_pipe_is_readable (pipe),NULL);
		VERIFY (io_encoding_pipe_is_writeable (pipe),NULL);

		VERIFY (cast_to_io_encoding_pipe ((io_pipe_t*) pipe) != NULL,NULL);

		free_io_encoding_pipe (pipe,bm);
	}
	
	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_value_pipe_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;
	memory_info_t vm_begin,vm_end;
	
	io_value_memory_get_info (vm,&vm_begin);
	io_byte_memory_get_info (bm,&bm_begin);

	io_value_pipe_t *pipe = mk_io_value_pipe (bm,3);
	if (VERIFY (pipe != NULL,NULL)) {
		vref_t values[] = {
			mk_io_int64_value (vm,42),
			mk_io_int64_value (vm,43),
			mk_io_int64_value (vm,44),
		};
		vref_t r_value;
		
		VERIFY (!io_value_pipe_is_readable (pipe),NULL);
		VERIFY (io_value_pipe_put_value (pipe,values[0]),NULL);
		VERIFY (io_value_pipe_is_readable (pipe),NULL);

		VERIFY (io_value_pipe_put_value (pipe,values[1]),NULL);

		VERIFY (!io_value_pipe_put_value (pipe,values[2]),NULL);
		
		VERIFY (io_value_pipe_get_value (pipe,&r_value),NULL);
		VERIFY (vref_is_equal_to (r_value,values[0]),NULL);
		VERIFY (io_value_pipe_is_readable (pipe),NULL);
		VERIFY (io_value_pipe_get_value (pipe,&r_value),NULL);
		VERIFY (vref_is_equal_to (r_value,values[1]),NULL);
		VERIFY (!io_value_pipe_is_readable (pipe),NULL);
		VERIFY (!io_value_pipe_get_value (pipe,&r_value),NULL);
		
		free_io_value_pipe (pipe,bm);
	}

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vm_end);
	VERIFY (vm_end.used_bytes == vm_begin.used_bytes,NULL);

	io_byte_memory_get_info (bm,&bm_end);
	VERIFY (bm_end.used_bytes == bm_begin.used_bytes,NULL);
}
TEST_END

TEST_BEGIN(test_io_tls_sha256_1) {
	io_sha256_context_t ctx;
	uint8_t output[32];
	uint8_t expect[32] = {
		0xe3,0xb0,0xc4,0x42,0x98,0xfc,0x1c,0x14,
		0x9a,0xfb,0xf4,0xc8,0x99,0x6f,0xb9,0x24,
		0x27,0xae,0x41,0xe4,0x64,0x9b,0x93,0x4c,
		0xa4,0x95,0x99,0x1b,0x78,0x52,0xb8,0x55
	};

	io_sha256_start (TEST_IO,&ctx);
	io_sha256_update (TEST_IO,&ctx,NULL,0);
	io_sha256_finish (TEST_IO,&ctx,output);
	VERIFY (memcmp (output,expect,32) == 0,NULL);
}
TEST_END

/*
 *-----------------------------------------------------------------------------
 *
 * test_io_tls_sha256_2 --
 *
 *-----------------------------------------------------------------------------
 */
TEST_BEGIN(test_io_tls_sha256_2) {
	io_sha256_context_t ctx;
	uint8_t data[] = {0x81,0xa7,0x23,0xd9,0x66};
	uint8_t expect[SHA256_SIZE] = {
		0x75,0x16,0xfb,0x8b,0xb1,0x13,0x50,0xdf,
		0x2b,0xf3,0x86,0xbc,0x3c,0x33,0xbd,0x0f,
		0x52,0xcb,0x4c,0x67,0xc6,0xe4,0x74,0x5e,
		0x04,0x88,0xe6,0x2c,0x2a,0xea,0x26,0x05
	};
	uint8_t output[32];

	io_sha256_start (TEST_IO,&ctx);
	io_sha256_update (TEST_IO,&ctx,data,sizeof(data));
	io_sha256_finish (TEST_IO,&ctx,output);
	
	VERIFY (memcmp (output,expect,SHA256_SIZE) == 0,NULL);
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_vref_bucket_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		VERIFY (!vref_hash_table_contains (hash,cr_NIL),NULL);
		free_vref_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_vref_bucket_hash_table (bm,7);
	VERIFY (vref_hash_table_insert (hash,cr_NIL),NULL);
	VERIFY (vref_hash_table_contains (hash,cr_NIL),NULL);
	VERIFY (vref_hash_table_remove (hash,cr_NIL),NULL);
	VERIFY (!vref_hash_table_contains (hash,cr_NIL),NULL);

	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END
			
TEST_BEGIN(test_vref_bucket_hash_table_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_a;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,7);

	r_a = mk_io_int64_value (vm,42);
	VERIFY (vref_hash_table_insert (hash,r_a),NULL);
	VERIFY (vref_hash_table_contains (hash,r_a),NULL);

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_4) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_value[16];
	bool ok;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,30);

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		r_value[i] = mk_io_int64_value (vm,i);
		ok &= vref_hash_table_insert (hash,r_value[i]);
	}
	VERIFY (ok,"values all added");
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= vref_hash_table_contains (hash,r_value[i]);
	}
	VERIFY (ok,"values all present");
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_vref_bucket_hash_table_5) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	vref_hash_table_t *hash;
	memory_info_t bmbegin,bmend;
	memory_info_t vmbegin,vmend;
	vref_t r_value[16];
	bool ok;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);
	
	hash = mk_vref_bucket_hash_table (bm,30);

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		r_value[i] = mk_io_int64_value (vm,i);
		ok &= vref_hash_table_insert (hash,r_value[i]);
	}
	VERIFY (ok,"values all added");
	
	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes > vmbegin.used_bytes,NULL);	

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= vref_hash_table_remove (hash,r_value[i]);
	}
	VERIFY (ok,"values all removed");

	ok = true;
	for (uint32_t i = 0; i < SIZEOF(r_value); i++) {
		ok &= !vref_hash_table_contains (hash,r_value[i]);
	}
	VERIFY (ok,"values all gone");
	
	free_vref_hash_table (hash);
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_value_memory_do_gc (vm,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		const char* key = "abc";
		char data[] = {5,6,0,7};
		string_hash_table_mapping_t map;
		
		VERIFY (!string_hash_table_map (hash,key,strlen(key),&map),NULL);
		VERIFY (string_hash_table_insert (hash,key,strlen(key),def_hash_mapping_i32(42)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,key,strlen(key),&map) && map.i32 == 42,NULL);

		VERIFY (string_hash_table_insert (hash,data,sizeof(data),def_hash_mapping_i32(9)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,data,sizeof(data),&map) && map.i32 == 9,NULL);

		VERIFY (!string_hash_table_insert (hash,data,sizeof(data),def_hash_mapping_i32(22)),NULL);
		map.i32 = 0;
		VERIFY (string_hash_table_map (hash,data,sizeof(data),&map) && map.i32 == 22,NULL);

		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_2) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		uint32_t len = hash->table_size;
		bool ok = true;
		
		for (int i = 0; i < 120 && ok; i++) {
			char c = 'a' + i;
			ok &= string_hash_table_insert (hash,&c,1,def_hash_mapping_i32(i));
		}
		VERIFY (ok,"all inserted");
		// table grew
		VERIFY (hash->table_size > len,NULL);

		ok = true;
		for (int i = 0; i < 48 && ok; i++) {
			string_hash_table_mapping_t v;
			char c = 'a' + i;
			ok &= string_hash_table_map (hash,&c,1,&v) && v.i32 == i;
		}
		VERIFY (ok,"all mapped");
		
		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_string_hash_table_3) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	string_hash_table_t *hash;
	memory_info_t begin,end;

	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_string_hash_table (bm,7);
	if (VERIFY (hash != NULL,NULL)) {
		string_hash_table_mapping_t map;
		
		VERIFY (string_hash_table_insert (hash,NULL,0,def_hash_mapping_i32(0)),NULL);

		map.i32 = -1;
		VERIFY (string_hash_table_map (hash,NULL,0,&map) && map.i32 == 0,NULL);

		free_string_hash_table (hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

int
test_io_pq_sort_1_compare (void const *a,void const *b) {
	int c = ((int) a) - ((int) b);
	return (c > 0) ? 1 : (c < 0) ? -1 : 0;
}

TEST_BEGIN(test_io_pq_sort_1) {
	int data[] = {5,1,4,2};
	int expect[] = {1,2,4,5};
	bool ok;
	
	pq_sort ((void**) data,SIZEOF(data),test_io_pq_sort_1_compare);
	
	ok = true;
	for (int i = 0; i < SIZEOF(data) && ok; i++) {
		ok &= expect[i] == data[i];
	}
	VERIFY(ok,"sorted");
}
TEST_END

TEST_BEGIN(test_io_pq_sort_2) {
	int a[] = {12,43,13,5,8,10,11,9,20,17};
	int r[] = {5,8,9,10,11,12,13,17,20,43};
	int i,ok = 1;
	
	pq_sort ((void**) a,SIZEOF(a),test_io_pq_sort_1_compare);
	for (i = 0; i < SIZEOF(a); i++){
		ok &= (a[i] == r[i]);
	}

	VERIFY(ok,"all sorted");	
}
TEST_END

static bool
test_constrained_hash_purge_entry_helper (
	vref_t a,vref_t b,void *u
) {
	uint32_t *prune_count = (uint32_t*) u;
	*prune_count += 1;	
	return true;
}

static void
test_constrained_hash_begin_purge_helper (void *u) {
}

TEST_BEGIN(test_io_constrained_hash_table_1) {
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	io_constrained_hash_t *hash;
	memory_info_t begin,end;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&begin);
	
	hash = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (hash != NULL,NULL)) {
		
		free_io_constrained_hash (bm,hash);
	}
	
	io_byte_memory_get_info (bm,&end);
	VERIFY (end.used_bytes == begin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_2) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		vref_t r_last = cr_NIL;
		uint32_t i,c;
		
		VERIFY(cht_get_table_size(cht) >= size,"table size");
		size = cht_get_table_size(cht);
		
		for (i = 0, c = 0; i < size; i++) {
			io_constrained_hash_entry_t *e = &cht_entry_at_index(cht,i);
			e->value = mk_io_int64_value (vm,i);
			r_last = e->value;
			c += e->info.free;
		}
		VERIFY(c == size,"table initialised");
		VERIFY(cht_count(cht) == 0,"empty");

		cht_entry_at_index(cht,size - 1).info.free = 0;
		cht_sort(cht);
		VERIFY (
			vref_is_equal_to (
				cht_ordered_entry_at_index(cht,0)->value,r_last
			),
			"order by age"
		);

		VERIFY(!cht_has_key(cht,cr_NIL),NULL);
		
		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_3) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		NULL
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		vref_t r_key,r_value;
		
		r_key = mk_io_int64_value (vm,0);
		r_value = mk_io_int64_value (vm,1);
		
		cht_set_value(cht,r_key,r_value);
		VERIFY (cht_count(cht) == 1,NULL);
		VERIFY (cht_has_key(cht,r_key),NULL);
		VERIFY (vref_is_equal_to (cht_get_value(cht,r_key),r_value),NULL);

		VERIFY (cht_unset (cht,r_key),NULL);
		VERIFY (!cht_has_key(cht,r_key),NULL);

		VERIFY (!cht_has_key(cht,cr_NIL),NULL);

		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

TEST_BEGIN(test_io_constrained_hash_table_4) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *bm = io_get_byte_memory (TEST_IO);
	memory_info_t bmbegin,bmend,vmbegin,vmend;
	io_constrained_hash_t *cht;
	uint32_t size = 17;
	uint32_t prune_count = 0;
	io_byte_memory_get_info (bm,&bmbegin);
	io_value_memory_get_info (vm,&vmbegin);

	cht = mk_io_constrained_hash (
		bm,
		size,
		test_constrained_hash_begin_purge_helper,
		test_constrained_hash_purge_entry_helper,
		&prune_count
	);
	
	if (VERIFY (cht != NULL,NULL)) {
		int i;
		for(i = 0; i < cht_get_entry_limit (cht); i++) {
			cht_set_value (
				cht,mk_io_int64_value (vm,i),cr_NIL
			);
		}

		VERIFY(prune_count == 0,"no pruning yet");
		cht_set_value (
			cht,mk_io_int64_value (vm,i),cr_NIL
		);

		VERIFY(prune_count == 2,"entries pruned");

		free_io_constrained_hash (bm,cht);
	}
	
	io_byte_memory_get_info (bm,&bmend);
	VERIFY (bmend.used_bytes == bmbegin.used_bytes,NULL);	

	io_do_gc (TEST_IO,-1);
	io_value_memory_get_info (vm,&vmend);
	VERIFY (vmend.used_bytes == vmbegin.used_bytes,NULL);	
}
TEST_END

UNIT_SETUP(setup_io_core_containers_unit_test) {
	return VERIFY_UNIT_CONTINUE;
}

UNIT_TEARDOWN(teardown_io_core_containers_unit_test) {
}

static void
io_core_containers_unit_test (V_unit_test_t *unit) {
	static V_test_t const tests[] = {
		test_io_memories_1,
		test_io_memories_2,
		test_io_byte_pipe_1,
		test_io_encoding_pipe_1,
		test_io_value_pipe_1,
		test_io_tls_sha256_1,
		test_io_tls_sha256_2,
		test_vref_bucket_hash_table_1,
		test_vref_bucket_hash_table_2,
		test_vref_bucket_hash_table_3,
		test_vref_bucket_hash_table_4,
		test_vref_bucket_hash_table_5,
		test_string_hash_table_1,
		test_string_hash_table_2,
		test_string_hash_table_3,
		test_io_pq_sort_1,
		test_io_constrained_hash_table_1,
		test_io_constrained_hash_table_2,
		test_io_constrained_hash_table_3,
		test_io_constrained_hash_table_4,
		0
	};
	unit->name = "io containers";
	unit->description = "io containers unit test";
	unit->tests = tests;
	unit->setup = setup_io_core_containers_unit_test;
	unit->teardown = teardown_io_core_containers_unit_test;
}

void
run_ut_io_core_containers (V_runner_t *runner) {
	static const unit_test_t test_set[] = {
		io_byte_memory_unit_test,
		io_core_containers_unit_test,
		0
	};
	V_run_unit_tests(runner,test_set);
}

#endif /* IMPLEMENT_VERIFY_IO_CORE_CONTAINERS */
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

