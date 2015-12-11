TOP:=.
TARGETS=libgepserver.a libgepclient.a
TEST_TARGETS= gep_test
include rules.mk

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

$(TEST_TARGETS): LIBS+=-lprotobuf-lite -lprotobuf -lgtest

$(TEST_TARGETS): \
    libgepserver.a \
    libgepclient.a \
    test.pb.t.o \
    test_protocol.t.o

install:
	$(INSTALL) -D -m 0444 \
		gep_server.h \
		gep_protocol.h \
		gep_client.h \
		gep_channel.h \
		gep_channel_array.h \
		$(DESTDIR)/usr/include/
	$(INSTALL) -D -m 0755 \
		libgepserver.a \
		libgepclient.a \
		$(DESTDIR)/usr/lib/

clean::
	rm -f *.pb.* .protos_done
