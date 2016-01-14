TOP:=..
TARGETS= client server
include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include
LIBS+=-lprotobuf -L../src -lgepserver -lgepclient

all: .protos_done
	$(MAKE) all_for_real_this_time

all_for_real_this_time: $(TARGETS)

.protos_done: sgp.proto
	$(MAKE) sgp.pb.h

HOST_PROTOC ?= $(HOSTDIR)/usr/bin/protoc
PROTOFLAGS=--cpp_out=. -I.

sgp.pb.h: sgp.proto
	echo "Building sgp.pb.h"
	$(HOST_PROTOC) $(PROTOFLAGS) $<

client: \
    sgp.pb.t.o \
    sgp_protocol.t.o \
    sgp_client.t.o

server: \
    sgp.pb.t.o \
    sgp_protocol.t.o \
    sgp_server.t.o

runtests: client server
	./run_client_and_server.sh

install:

clean::
	rm -f *.pb.* .protos_done client server
