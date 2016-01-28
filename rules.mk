# Copyright Google Inc. Apache 2.0.

TOP:=$(shell /bin/pwd)/$(TOP)

default: all

NO_CLANG:=
USE_TSAN:=
USE_ASAN:=
USE_MSAN:=
USE_COV:=
COV_OUT_DIR:=

GENINFO:=geninfo
GENHTML:=genhtml

# symbolizer needed to decode MSAN/ASAN traces
SYMBOLIZER_PATH=$(shell which llvm-symbolizer-3.4)

# Test logs output directory
TEST_OUT=$(TOP)/
# Sanitizer logs output directory
TSAN_OUT=$(TOP)/tsan_logs
ASAN_OUT=$(TOP)/asan_logs

ifneq ($(NO_CLANG),)
HOST_CC:=gcc
HOST_CXX:=g++
else
HOST_CC:=clang
HOST_CXX:=clang++
endif

ifeq ($(CROSS_PREFIX),)
CC:=$(HOST_CC)
CXX:=$(HOST_CXX)
CPPFLAGS+=-I$(HOSTDIR)/usr/include
LDFLAGS+=-L$(HOSTDIR)/usr/lib
else
CC:=$(CROSS_PREFIX)gcc
CXX:=$(CROSS_PREFIX)g++
endif
LD:=$(CROSS_PREFIX)ld
AR:=$(CROSS_PREFIX)ar
RANLIB:=$(CROSS_PREFIX)ranlib
STRIP:=$(CROSS_PREFIX)strip
INSTALL:=install

# C-Preprocessor flags
CPPFLAGS+=-D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS=-Wall -Werror -g -fPIC -Wswitch-enum
CXXFLAGS=-Wall -Werror -g -fPIC -Wswitch-enum -Wextra -Wno-sign-compare \
    -Wno-unused-parameter -std=c++0x
ifeq ($(NO_CLANG),)
ifeq ($(CROSS_PREFIX),)
CFLAGS+=-Wshadow
CXXFLAGS+=-Wshadow
endif
endif
LDFLAGS+=-rdynamic
LIBS=-lpthread -lrt

# add current project version
LIBGEP_VERSION:= $(shell git describe --abbrev=4 --dirty --always --tags)
CPPFLAGS+=-DLIBGEP_VERSION=\"$(LIBGEP_VERSION)\"

# Thread Sanitizer
ifneq ($(USE_TSAN),)
CFLAGS+=-fsanitize=thread
CXXFLAGS+=-fsanitize=thread
LDFLAGS+=-fsanitize=thread
endif

# Address Sanitizer
ifneq ($(USE_ASAN),)
CFLAGS+=-fsanitize=address -fno-omit-frame-pointer
CXXFLAGS+=-fsanitize=address -fno-omit-frame-pointer
LDFLAGS+=-fsanitize=address
endif

# Memory Sanitizer
ifneq ($(USE_MSAN),)
CFLAGS+=-fsanitize=memory -fno-omit-frame-pointer
CXXFLAGS+=-fsanitize=memory -fno-omit-frame-pointer
LDFLAGS+=-fsanitize=memory
endif

# Code coverage
ifneq ($(NO_OPTIMIZATION),)
CFLAGS+=-O0
CXXFLAGS+=-O0
else ifeq ($(USE_COV),)
# not running coverage, enable optimization
CFLAGS+=-O3
CXXFLAGS+=-O3
else
CFLAGS+=--coverage
CXXFLAGS+=--coverage
LDFLAGS+=--coverage
ifneq ($(COV_OUT_DIR),)
CFLAGS+=-fprofile-dir=$(COV_OUT_DIR)
CXXFLAGS+=-fprofile-dir=$(COV_OUT_DIR)
LDFLAGS+=-fprofile-dir=$(COV_OUT_DIR)
endif
ifeq ($(NO_CLANG),)
CLANG_COV_FLAGS+=-Xclang -coverage-cfg-checksum -Xclang \
   -coverage-no-function-names-in-data -Xclang -coverage-version='408*' \
   --coverage
CFLAGS+=$(CLANG_COV_FLAGS)
CXXFLAGS+=$(CLANG_COV_FLAGS)
endif
endif

# Test Flags
TEST_CPPFLAGS=$(CPPFLAGS)
TEST_CFLAGS=$(CFLAGS)
TEST_CXXFLAGS=$(CXXFLAGS)
ifneq ($(NO_CLANG),)
TEST_CXXFLAGS+=-Wno-unused-local-typedefs
endif
TEST_LDFLAGS=$(LDFLAGS)
TEST_LIBS=$(LIBS) -lgtest -pthread


.PHONY: clean
clean:: $(patsubst %,%/clean,$(SUBDIRS))
	rm -f *.[oa] *~ .??*~ *.so *.gcno *.gcda *.gcov *.info
	rm -f $(TEST_TARGETS) $(TARGETS)


%/all:
	$(MAKE) -C $* all
%/test:
	$(MAKE) -C $* test
%/test-full:
	$(MAKE) -C $* test-full
%/test-lite:
	$(MAKE) -C $* test-lite
%/tests:
	$(MAKE) -C $* tests
%/clean:
	$(MAKE) -C $* clean
%/cov:
	$(MAKE) -C $* cov
%/test-tsan:
	$(MAKE) -C $* test-tsan
%/test-asan:
	$(MAKE) -C $* test-asan


.PHONY: subdirs
subdirs: $(patsubst %,%/all,$(SUBDIRS))


# You can run 'make tests' to build all the test programs without running
# them.  Usually you want to check compilation before you check
# functionality (since some tests take a while to run) so this saves
# time.
tests: all
	$(MAKE) build_tests

build_tests: \
    $(patsubst %,%/tests,$(SUBDIRS)) \
    $(TEST_TARGETS)

# Almost like "test: tests runtests", but this method forces tests to
# always completely finish before runtests begins.
test: tests
	$(MAKE) -k runtests

runtests: \
    $(patsubst %,%/test,$(SUBDIRS)) \
    $(patsubst %,%.test,$(TEST_TARGETS))

# add full vs. lite tests (which reuse the same source code)
test-full: tests
	$(MAKE) -k runtests-full

runtests-full: \
    $(patsubst %,%/test,$(SUBDIRS)) \
    $(patsubst %,%.test,$(TEST_TARGETS_FULL))

test-lite: tests
	$(MAKE) -k runtests-lite

runtests-lite: \
    $(patsubst %,%/test,$(SUBDIRS)) \
    $(patsubst %,%.test,$(TEST_TARGETS_LITE))


cov: $(patsubst %,%/cov,$(COV_SUBDIRS))

tests-tsan:
	$(MAKE) USE_TSAN=1 tests

test-tsan: tests-tsan
	$(MAKE) -k USE_TSAN=1 runtests-tsan

runtests-tsan: \
    $(patsubst %,%/test-tsan,$(SUBDIRS)) \
    $(patsubst %,%.tsan,$(filter-out $(SKIP_TSAN_TARGETS),$(TEST_TARGETS)))

tests-asan:
	$(MAKE) USE_ASAN=1 tests

test-asan: tests-asan
	$(MAKE) -k USE_ASAN=1 runtests-asan

runtests-asan: \
    $(patsubst %,%/test-asan,$(SUBDIRS)) \
    $(patsubst %,%.asan,$(filter-out $(SKIP_ASAN_TARGETS),$(TEST_TARGETS)))

msan:
	$(MAKE) USE_MSAN=1 tests
	MSAN_SYMBOLIZER_PATH=$(SYMBOLIZER_PATH) $(MAKE) -j1 USE_MSAN=1 test

# TODO(apenwarr): could use real autodepends here.
#  This declares a dependency of every object on every header, which is
#  a bit heavyweight, but simple and safe.
$(patsubst %.cc,%.o,$(wildcard *.cc)) \
$(patsubst %.c,%.o,$(wildcard *.c)): $(shell find $(TOP) -name '*.h')


# Pattern rules for test targets.
# TODO(apenwarr): eliminate this. Tests shouldn't need special CFLAGS.
#   Currently some of the object files act differently when compiled with
#   different flags, which is bad practice; you want to test the actual
#   code that will run on the target platform.  But let's keep it this
#   way for now in order to not mix Makefile changes with code changes.
#   (In particular, some of the files only define main() if a particular
#   flag is set.  It would be better to split main() into its own file.)
%.t.o: %.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -c -o $@ $<
%.t.o: %.c
	$(CC) $(TEST_CPPFLAGS) $(TEST_CFLAGS) -c -o $@ $<
%_test: %_test.t.o
	$(CXX) $(TEST_LDFLAGS) -o $@ -Wl,--start-group \
      $(filter-out $*.o,$^) -Wl,--end-group $(TEST_LIBS)
%: %.t.o
	$(CXX) $(TEST_LDFLAGS) -o $@ $(filter-out $*.o,$^) $(LIBS)


# Pattern rules for normal targets
%.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<
%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<
%: %.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

# Rule to run tests. It tees the output into a .log file. If the test runs
# successfully the log file is removed.
%.test: %
	GTEST_COLOR=1 stdbuf -o0 ./$* 2>&1 | tee $(TEST_OUT)/$*.log; \
    if [ $${PIPESTATUS[0]} -eq 0 ]; then rm $(TEST_OUT)/$*.log; else exit 1; fi

# Rule to run tests with TSAN. It runs the test and feeds the test output
# (stdout) into .log and the ThreadSanitizer output (stderr) into both .log and
# .tsan.log. If the test ran successfully, i.e. no errors were found in either
# the test itself or by ThreadSanitizer, it deletes these files.
%.tsan: %
	(GTEST_COLOR=1 stdbuf -o0 ./$* 3>&1 1>&2 2>&3 | tee $(TSAN_OUT)/$*.tsan.log; \
    if [ $${PIPESTATUS[0]} -ne 0 ]; then exit 1; fi) > $(TSAN_OUT)/$*.log 2>&1 \
    && rm $(TSAN_OUT)/$*.log $(TSAN_OUT)/$*.tsan.log

# Rule to run tests with ASAN. It runs the test and feeds the test output
# (stdout) into .log and the AddressSanitizer output (stderr) into both .log
# and .asan.log. If the test ran successfully, i.e. no errors were found in
# either the test itself or by AddressSanitizer, it deletes these files.
%.asan: %
	(GTEST_COLOR=1 ASAN_SYMBOLIZER_PATH=$(SYMBOLIZER_PATH) stdbuf -o0 ./$* \
    3>&1 1>&2 2>&3 | tee $(ASAN_OUT)/$*.asan.log; \
    if [ $${PIPESTATUS[0]} -ne 0 ]; then exit 1; fi) > $(ASAN_OUT)/$*.log 2>&1 \
    && rm $(ASAN_OUT)/$*.log $(ASAN_OUT)/$*.asan.log

define make_lib
	$(AR) q $@.new $(filter %.o,$^)
	mv $@.new $@
endef

# protobuf defines
ifneq ($(PROTOBUF_PREFIX),)
HOST_PROTOC ?= $(PROTOBUF_PREFIX)/bin/protoc
else
HOSTDIR_MAIN = $(HOSTDIR)/usr
HOST_PROTOC ?= $(HOSTDIR_MAIN)/bin/protoc
endif


PROTOC_FLAGS=--cpp_out=. -I.

PROTO_CPPFLAGS=-I. -I$(PROTOBUF_PREFIX)/include
PROTOFULL_LDFLAGS=-L$(PROTOBUF_PREFIX)/lib -lprotobuf
PROTOLITE_LDFLAGS=-L$(PROTOBUF_PREFIX)/lib -lprotobuf-lite

