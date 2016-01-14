TOP:=..
TEST_TARGETS= gep_test
include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include
LIBS+=-lprotobuf

all: .protos_done
	$(MAKE) all_for_real_this_time

all_for_real_this_time: $(TARGETS)

.protos_done: test.proto
	$(MAKE) test.pb.h

HOST_PROTOC ?= $(HOSTDIR)/usr/bin/protoc
PROTOFLAGS=--cpp_out=. -I.

test.pb.h: test.proto
	echo "Building test.pb.h"
	$(HOST_PROTOC) $(PROTOFLAGS) $<

$(TEST_TARGETS): \
    LIBS+=-lprotobuf-lite -lprotobuf -lgtest -L../src -lgepserver -lgepclient

$(TEST_TARGETS): \
    test.pb.t.o \
    test_protocol.t.o

install:

clean::
	rm -f *.pb.* .protos_done
