# Copyright Google Inc. Apache 2.0.

TOP:=..

TEST_TARGETS_FULL= \
    utils_test \
    gep_protocol_test \
    gep_channel_test \
    gep_channel_array_test \
    gep_client_test \
    gep_server_test \
    gep_end_to_end_test

TEST_TARGETS_LITE= \
    gep_protocol_test_lite \
    gep_channel_test_lite \
    gep_channel_array_test_lite \
    gep_client_test_lite \
    gep_server_test_lite \
    gep_end_to_end_test_lite

TEST_TARGETS= $(TEST_TARGETS_FULL) $(TEST_TARGETS_LITE)

# add the local gep libraries info before the hostdir ones get added
CPPFLAGS+=-I../include -I../src $(PROTO_CPPFLAGS)
LDFLAGS+=-L../src

include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include

all: .protos_done
	$(MAKE) all_for_real_this_time

all_for_real_this_time: $(TARGETS)

.protos_done: test.proto test_lite.proto
	$(MAKE) test.pb.h
	$(MAKE) test_lite.pb.h

test.pb.h: test.proto
	echo "Building test.pb.h"
	$(HOST_PROTOC) $(PROTOC_FLAGS) $<

test_lite.pb.h: test_lite.proto
	echo "Building test_lite.pb.h"
	$(HOST_PROTOC) $(PROTOC_FLAGS) $<

$(TEST_TARGETS_FULL) : \
    LIBS+=$(PROTOFULL_LDFLAGS) -lgtest -L../src -lgepserver -lgepclient

$(TEST_TARGETS_LITE) : \
    LIBS+=$(PROTOLITE_LDFLAGS) -lgtest -L../src -lgepserver-lite -lgepclient-lite

$(TEST_TARGETS_FULL) : \
    test.pb.t.o \
    test_protocol.t.o \
    gep_test_lib.t.o

$(TEST_TARGETS_LITE) : \
    test_lite.pb.o \
    test_protocol.l.o \
    gep_test_lib.l.o

# Pattern rules for *_lite targets.
%.l.o: %.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

test_lite.pb.o: test_lite.pb.h

test_lite.pb.o: test_lite.pb.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

%_test_lite: %_test.l.o
	$(CXX) $(TEST_LDFLAGS) -o $@ -Wl,--start-group \
      $(filter-out $*.o,$^) -Wl,--end-group $(TEST_LIBS)


install:

clean::
	rm -f *.pb.* .protos_done
