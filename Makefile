
CC =		gcc

## for debugging with gdb
CFLAGS = 	-g -Wall -Werror -DLOCKABILITY_REQUIRED

###CFLAGS = 	-O3 -Wall -Werror -DLOCKABILITY_REQUIRED

ifeq ($(OS), APPLE)
STATIC_LIBS =	-lpthread
else
STATIC_LIBS =	-lpthread -lrt
endif

INCLUDES =	-I.

LIBNAME =	utils_lib.a

LIB_OBJS =	timer_object.o \
		mem_monitor.o \
		lock_object.o \
		stack_object.o \
		queue_object.o \
		linkedlist.o \
		index_object.o \
		avl_tree_object.o \
		table.o \
		dynamic_array.o \
		generic_object_database.o \
		\
		### utils_common.o \
		### event_manager.o \
		### trie_object.o
		### chunk_manager_object.o \

%.o:		%.c %.h \
		pointer_manipulations.h \
		function_types.h \
		mem_monitor.h \
		object_types.h \
		event_types.h 
		$(CC) -c $(CFLAGS) $<

utils_lib.a:	$(LIB_OBJS)
		ar -r $(LIBNAME) $(LIB_OBJS)
		ranlib $(LIBNAME)

test_lock_object:	test_lock_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_lock_object.c \
			    -o test_lock_object $(LIBNAME) $(STATIC_LIBS)

test_stack_object:	test_stack_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_stack_object.c \
			    -o test_stack_object $(LIBNAME) $(STATIC_LIBS)

test_linkedlist:	test_linkedlist.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_linkedlist.c \
			    -o test_linkedlist $(LIBNAME) $(STATIC_LIBS)

test_queue_object:	test_queue_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_queue_object.c \
			    -o test_queue_object $(LIBNAME) $(STATIC_LIBS)

test_chunk_object:	test_chunk_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) \
			-DMEASURE_CHUNKS test_chunk_object.c \
			    -o test_chunk_object $(LIBNAME) $(STATIC_LIBS)

test_chunk_integrity:	test_chunk_integrity.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) \
			-DMEASURE_CHUNKS test_chunk_integrity.c \
			    -o test_chunk_integrity $(LIBNAME) $(STATIC_LIBS)

test_malloc:		test_chunk_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_chunk_object.c \
			    -o test_malloc $(LIBNAME) $(STATIC_LIBS)

test_index_object:	test_index_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_index_object.c \
			    -o test_index_object $(LIBNAME) $(STATIC_LIBS)

test_avl_object:	test_avl_object.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_avl_object.c \
			    -o test_avl_object $(LIBNAME) $(STATIC_LIBS)

test_dynamic_array:	test_dynamic_array.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_dynamic_array.c \
			    -o test_dynamic_array $(LIBNAME) $(STATIC_LIBS)

test_db:		test_db.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db.c -o test_db \
		    		$(LIBNAME) $(STATIC_LIBS)

test_db_load:		test_db_load.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db_load.c -o test_db_load \
		    		$(LIBNAME) $(STATIC_LIBS)

test_db_speed:		test_db_speed.c $(LIBNAME)
			$(CC) $(CFLAGS) $(INCLUDES) test_db_speed.c -o test_db_speed \
		    		$(LIBNAME) $(STATIC_LIBS)

TESTS =		test_lock_object \
		test_stack_object \
		test_linkedlist \
		test_queue_object \
		test_index_object \
		test_avl_object \
		test_dynamic_array \
		test_db \
		test_db_load \
		test_db_speed \
		\
		### test_chunk_object \
		### test_chunk_integrity \
		### test_malloc \

tests:		$(TESTS)

all:		$(LIBNAME) tests

clean:
		@rm -f *.o
		@rm -f $(LIBNAME) $(TESTS)
		@touch *.c *.h




