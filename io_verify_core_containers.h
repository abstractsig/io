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

TEST_BEGIN(test_io_memories_1) {
	io_value_memory_t *vm = io_get_short_term_value_memory (TEST_IO);
	io_byte_memory_t *mem,*bm = io_get_byte_memory (TEST_IO);
	memory_info_t bm_begin,bm_end;

	VERIFY (bm != NULL,NULL);
	VERIFY (io_value_memory_get_io(vm) == TEST_IO,NULL);

	io_byte_memory_get_info (bm,&bm_begin);

	mem = mk_io_byte_memory (TEST_IO,32,UMM_BLOCK_SIZE_8);
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

