
CC =		gcc

## for debugging with gdb
CFLAGS =	-g -std=gnu99 -Wall -Wextra -Wno-unused-parameter -Werror
CFLAGS =	-std=gnu99 -O3 -Wall -Wextra -Wno-unused-parameter -Werror

ifeq ($(OS), APPLE)
STATIC_LIBS =	-lpthread
else
STATIC_LIBS =	-lpthread -lrt
endif

INCLUDES =	-I.

LIBNAME =	utils_lib.a

LIB_OBJS =	debug_framework.o \
		timer_object.o \
		mem_monitor_object.o \
		lock_object.o \
		bitlist_object.o \
		stack_object.o \
		queue_object.o \
		ordered_list.o \
		dl_list_object.o \
		chunk_manager.o \
		index_object.o \
		avl_tree_object.o \
		table.o \
		dynamic_array_object.o \
		radix_tree_object.o \
		scheduler.o \
		event_manager.o \
		object_manager.o \
		### test_data_generator.o \
		### utils_common.o \

%.o:		%.c %.h 
		$(CC) -c $(CFLAGS) $<

utils_lib.a:	$(LIB_OBJS)
		ar -r $(LIBNAME) $(LIB_OBJS)
		ranlib $(LIBNAME)

test_debug_framework:	test_debug_framework.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_debug_framework.c \
				-o test_debug_framework $(LIBNAME) $(STATIC_LIBS)

test_lock_object:	test_lock_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_lock_object.c \
				-o test_lock_object $(LIBNAME) $(STATIC_LIBS)

test_lock_speed:	test_lock_speed.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_lock_speed.c \
				-o test_lock_speed $(LIBNAME) $(STATIC_LIBS)

test_bitlist:		test_bitlist.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_bitlist.c \
				-o test_bitlist $(LIBNAME) $(STATIC_LIBS)

test_stack_object:	test_stack_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_stack_object.c \
				-o test_stack_object $(LIBNAME) $(STATIC_LIBS)

test_ordered_list:	test_ordered_list.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_ordered_list.c \
				-o test_ordered_list $(LIBNAME) $(STATIC_LIBS)

test_queue_object:	test_queue_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_queue_object.c \
				-o test_queue_object $(LIBNAME) $(STATIC_LIBS)

test_chunk_manager: test_chunk_manager.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_chunk_manager.c \
				-o test_chunk_manager $(LIBNAME) $(STATIC_LIBS)

test_chunk_integrity:	test_chunk_integrity.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_chunk_integrity.c \
				-o test_chunk_integrity $(LIBNAME) $(STATIC_LIBS)

test_malloc:		test_chunk_manager.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) -DUSE_MALLOC \
				test_chunk_manager.c -o test_malloc \
				$(LIBNAME) $(STATIC_LIBS)

test_index_object:	test_index_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_index_object.c \
				-o test_index_object $(LIBNAME) $(STATIC_LIBS)

test_avl_object:	test_avl_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_avl_object.c \
				-o test_avl_object $(LIBNAME) $(STATIC_LIBS)

test_radix_tree:		test_radix_tree.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_radix_tree.c \
				-o test_radix_tree $(LIBNAME) $(STATIC_LIBS)

test_radix_tree2:		test_radix_tree2.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_radix_tree2.c \
				-o test_radix_tree2 $(LIBNAME) $(STATIC_LIBS)

test_dynamic_array: test_dynamic_array.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_dynamic_array.c \
				-o test_dynamic_array $(LIBNAME) $(STATIC_LIBS)

test_db:		test_db.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db.c -o test_db \
					$(LIBNAME) $(STATIC_LIBS)

test_db_load:		test_db_load.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db_load.c -o test_db_load \
					$(LIBNAME) $(STATIC_LIBS)

test_db_speed:		test_db_speed.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db_speed.c \
				-o test_db_speed $(LIBNAME) $(STATIC_LIBS)

test_delay:		test_delay.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_delay.c -o test_delay \
					$(LIBNAME) $(STATIC_LIBS)

test_scheduler:		test_scheduler.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_scheduler.c \
				-o test_scheduler $(LIBNAME) $(STATIC_LIBS)

test_event_manager:	test_event_manager.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_event_manager.c \
				-o test_event_manager $(LIBNAME) $(STATIC_LIBS)

enhanced_counters:	enhanced_counters.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) enhanced_counters.c \
				-o enhanced_counters $(LIBNAME) $(STATIC_LIBS)

TESTS =		test_debug_framework \
		test_lock_object \
		test_lock_speed \
		test_stack_object \
		test_bitlist \
		test_ordered_list \
		test_queue_object \
		test_chunk_manager \
		test_malloc \
		test_chunk_integrity \
		test_index_object \
		test_avl_object \
		test_dynamic_array \
		test_radix_tree \
		test_radix_tree2 \
		test_event_manager \
		test_db \
		test_db_load \
		test_db_speed \
		test_delay \
		test_scheduler \
		enhanced_counters \
		\

tests:		$(TESTS)

all:		$(LIBNAME) tests

clean:
		@rm -f *.o
		@rm -f $(LIBNAME) $(TESTS)
		@touch *.c *.h




