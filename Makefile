TOP:=.
TARGETS=libgepserver.a libgepclient.a libtest.a
TEST_TARGETS= gep_test
include ./rules.mk

CPPFLAGS+=-I. -I..
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

libtest.a: \
    test.pb.o \
    test_protocol.o
	$(make_lib)

libgepserver.a: \
    utils.o \
    gep_protocol.o \
    gep_channel.o \
    gep_channel_array.o \
    gep_server.o
	$(make_lib)

libgepclient.a: \
    utils.o \
    gep_protocol.o \
    gep_channel.o \
    gep_channel_array.o \
    gep_client.o
	$(make_lib)

$(TEST_TARGETS): LIBS+=-lprotobuf-lite -lprotobuf -lgtest -lgtest_main \
    libgepserver.a \
    libgepclient.a \
    libtest.a

clean::
	rm -f *.pb.* .protos_done
