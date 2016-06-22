/*!
[config]
name: atomic_cmpxchg global
clc_version_min: 11

[test]
name: simple int
kernel_name: simple_int
dimensions: 1
global_size: 1 0 0
arg_out: 0 buffer int[2]  5 -4
arg_in:  0 buffer int[2] -4 -4
arg_in:  1 buffer int[2] -4  3
arg_in:  2 buffer int[2]  5  5

[test]
name: simple uint
kernel_name: simple_uint
dimensions: 1
global_size: 1 0 0
arg_out: 0 buffer uint[2] 5 4
arg_in:  0 buffer uint[2] 4 4
arg_in:  1 buffer uint[2] 4 3
arg_in:  2 buffer uint[2] 5 5

[test]
name: threads int
kernel_name: threads_int
dimensions: 1
global_size: 8 0 0
local_size:  8 0 0
arg_out: 0 buffer int[1] 8
arg_in:  0 buffer int[1] 0

[test]
name: threads uint
kernel_name: threads_uint
dimensions: 1
global_size: 8 0 0
local_size:  8 0 0
arg_out: 0 buffer uint[1] 8
arg_in:  0 buffer uint[1] 0

!*/

#define SIMPLE_TEST(TYPE) \
kernel void simple_##TYPE(global TYPE *initial, global TYPE *compare, global TYPE *value) { \
	atomic_cmpxchg(initial, compare[0], value[0]); \
	atomic_cmpxchg(initial+1, compare[1], value[1]); \
}

#define THREADS_TEST(TYPE) \
kernel void threads_##TYPE(global TYPE *out) { \
	int i; \
	barrier(CLK_GLOBAL_MEM_FENCE); \
	TYPE id = get_global_id(0); \
	for(i = 0; i < get_global_size(0); i++){ \
		if (i == id){ \
			atomic_cmpxchg(out, id, id+1); \
		} \
		barrier(CLK_GLOBAL_MEM_FENCE); \
	} \
}

SIMPLE_TEST(int)
SIMPLE_TEST(uint)

THREADS_TEST(int)
THREADS_TEST(uint)
