
#OPT ?= -O2 -DNDEBUG       # (A) Production use (optimized mode)
OPT ?= -g2  # (B) Debug mode, w/ full line-level debugging symbols
#OPT ?= -O2 -g2 -DNDEBUG # (C) Profiling mode: opt, but w/debugging symbols

CC=gcc
CXX=g++

THRIFT=/usr/local/bin/thrift
CPP_PARAS = --gen cpp:pure_enums
JAVA_PARAS = --gen java
PERL_PARAS = --gen perl
THRIFT_FLAGS = -o idl

THRIFT_DEFS = idl/recomm_engine.thrift 
BASE_THRIFT_DEFS = idl/fb303.thrift

IDL_SRCS = idl/gen-cpp/FacebookService.cpp \
           idl/gen-cpp/RecommEngine.cpp \
           idl/gen-cpp/StoryManagement.cpp \
           idl/gen-cpp/fb303_constants.cpp \
           idl/gen-cpp/fb303_types.cpp \
           idl/gen-cpp/recomm_engine_types.cpp \
           idl/gen-cpp/recomm_engine_constants.cpp

IDL_OBJECTS = $(IDL_SRCS:.cpp=.o)

CFLAGS += -I./ -I./include -I./src -I./idl/gen-cpp/ -Wall $(OPT) -pthread -fPIC 
CXXFLAGS += -I. -I./include -I./src -I./idl/gen-cpp/ -DHAVE_NETINET_IN_H -Wall $(OPT) -pthread -fPIC 
             
LDFLAGS += -rdynamic -L./lib -L/usr/local/lib
LIBS += -lpthread -ldl -lgflags -lglog -lleveldb -lthrift -lcityhash \
        -lboost_thread -lboost_date_time -lboost_filesystem \
        -lboost_program_options

LIBOBJECTS = src/recomm_engine_handler.o \
	src/story_management_handler.o \
	src/story_profile_storage.o \
	src/redispp.o \
	src/hiredis/async.o \
	src/hiredis/hiredis.o \
	src/hiredis/net.o \
	src/hiredis/sds.o \
	src/redis_cluster.o \
	src/local_dict.o \
	src/user_profile_storage.o \
	src/algo_plugin_manager.o \
	src/indexing/instant_index.o \
	src/indexing/iterator.o \
	src/indexing/clean_old_story_thread.o \
	src/indexing/index_build_thread.o \
	src/indexing/replay_log_writer_thread.o \
	src/indexing/posting.o \
	src/monitor_manager.o \
	src/util/base/timer.o \
	src/util/concurrency/thread.o \
	src/util/string/str_util.o \
	src/retrieval/result_set.o \
	src/retrieval/ranker.o \
	src/retrieval/index_reader.o \
	src/retrieval/retrieval_handler.o

TESTS = test/local_dict_test.cc \
        test/algo_plugin_manager_test.cc \
        test/str_util_test.cc \
        test/user_profile_storage_test.cc


TESTOBJECTS = $(TESTS:.cc=.o)


all: program

check: all_test
	LD_LIBRARY_PATH=./lib ./all_test

$(IDL_SRCS): $(THRIFT_DEFS) $(BASE_THRIFT_DEFS)
	$(THRIFT) $(CPP_PARAS) $(JAVA_PARAS) $(PERL_PARAS) $(THRIFT_FLAGS) $(BASE_THRIFT_DEFS)
	$(THRIFT) $(CPP_PARAS) $(JAVA_PARAS) $(PERL_PARAS) $(THRIFT_FLAGS) $(THRIFT_DEFS)

dev_lib: $(LIBOBJECTS) $(IDL_OBJECTS)
	ar -rs lib/librecomm_engine_dev.a $(LIBOBJECTS) $(IDL_OBJECTS)

clean:
	rm -rf $(TESTOBJECTS) $(LIBOBJECTS) idl/gen-cpp idl/gen-perl idl/gen-java  all_test server.app

lint:
	python cpplint.py src/*.h src/*.cc src/*.cpp

program: $(IDL_OBJECTS) $(LIBOBJECTS) src/server.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(IDL_OBJECTS) $(LIBOBJECTS) src/server.cpp -o server.app $(LIBS)

all_test:$(IDL_OBJECTS) $(LIBOBJECTS) $(TESTOBJECTS)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $(LIBOBJECTS) $(IDL_OBJECTS) $(TESTOBJECTS) -o all_test test/gtest-all.cc test/gtest_main.cc $(LIBS)

.cc.o: 
	$(CXX) $(CXXFLAGS) -c $< -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@


