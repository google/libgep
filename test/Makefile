TOP:=..
TEST_TARGETS= gep_test gep_test_lite

# add the local gep libraries info before the hostdir ones get added
CPPFLAGS+=-I../include
LDFLAGS+=-L../src

include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include

all: .protos_done
	$(MAKE) all_for_real_this_time

all_for_real_this_time: $(TARGETS)

.protos_done: test.proto test_lite.proto
	$(MAKE) test.pb.h
	$(MAKE) test_lite.pb.h

HOST_PROTOC ?= $(HOSTDIR)/usr/bin/protoc
PROTOFLAGS=--cpp_out=. -I.

test.pb.h: test.proto
	echo "Building test.pb.h"
	$(HOST_PROTOC) $(PROTOFLAGS) $<

test_lite.pb.h: test_lite.proto
	echo "Building test_lite.pb.h"
	$(HOST_PROTOC) $(PROTOFLAGS) $<

gep_test : \
    LIBS+=-lprotobuf -lgtest -L../src -lgepserver -lgepclient

gep_test_lite : \
    LIBS+=-lprotobuf-lite -lgtest -L../src -lgepserver-lite -lgepclient-lite

gep_test : \
    test.pb.t.o \
    test_protocol.t.o

test_protocol_lite.o: test_protocol.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_test_lite.o: gep_test.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_test_lite : \
    test_lite.pb.o \
    test_protocol_lite.o

install:

clean::
	rm -f *.pb.* .protos_done
