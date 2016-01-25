TOP:=..
TARGETS= client server client_lite server_lite
include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include

all: .protos_done
	$(MAKE) all_for_real_this_time

all_for_real_this_time: $(TARGETS)

.protos_done: sgp.proto
	$(MAKE) sgp.pb.h
	$(MAKE) sgp_lite.pb.h

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

client : LIBS+=-lprotobuf -L../src/ -lgepclient

server : LIBS+=-lprotobuf -L../src/ -lgepserver


# lite targets
sgp_lite.pb.h: sgp_lite.proto
	echo "Building sgp_lite.pb.h"
	$(HOST_PROTOC) $(PROTOFLAGS) $<

sgp_protocol_lite.o: sgp_protocol.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

sgp_client_lite.o: sgp_client.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

sgp_server_lite.o: sgp_server.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

client_lite.o: client.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

server_lite.o: server.cc
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DGEP_LITE -c -o $@ $<

client_lite: client_lite.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

server_lite: server_lite.o
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

client_lite: \
    sgp_lite.pb.t.o \
    sgp_protocol_lite.o \
    sgp_client_lite.o

server_lite: \
    sgp_lite.pb.t.o \
    sgp_protocol_lite.o \
    sgp_server_lite.o

client_lite : LIBS+=-lprotobuf-lite -L../src/ -lgepclient-lite

server_lite : LIBS+=-lprotobuf-lite -L../src/ -lgepserver-lite

runtests: client server client_lite server_lite
	./run_client_and_server.sh ./client ./server 9008
	./run_client_and_server.sh ./client_lite ./server_lite 9009

install:

clean::
	rm -f *.pb.* .protos_done client server client_lite server_lite
