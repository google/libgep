# Copyright Google Inc. Apache 2.0.

TOP:=..
TARGETS=libgepserver.a libgepclient.a libgepserver-lite.a libgepclient-lite.a
CPPFLAGS+=-I. -I.. -I../include

include ../rules.mk

LIBS+=-lprotobuf

all: $(TARGETS)

libgepserver.a: \
    socket_interface.o \
    utils.o \
    gep_protocol.o \
    gep_channel.o \
    gep_channel_array.o \
    gep_server.o
	$(make_lib)

libgepclient.a: \
    socket_interface.o \
    utils.o \
    gep_protocol.o \
    gep_channel.o \
    gep_channel_array.o \
    gep_client.o
	$(make_lib)

utils_lite.o: utils.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_protocol_lite.o: gep_protocol.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_channel_lite.o: gep_channel.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_channel_array_lite.o: gep_channel_array.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_server_lite.o: gep_server.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

gep_client_lite.o: gep_client.cc
	$(CXX) $(TEST_CPPFLAGS) $(TEST_CXXFLAGS) -DGEP_LITE -c -o $@ $<

libgepserver-lite.a: CPPFLAGS+=-DGEP_LITE

libgepserver-lite.a: \
    socket_interface.o \
    utils_lite.o \
    gep_protocol_lite.o \
    gep_channel_lite.o \
    gep_channel_array_lite.o \
    gep_server_lite.o
	$(make_lib)

libgepclient-lite.a: CPPFLAGS+=-DGEP_LITE

libgepclient-lite.a: \
    socket_interface.o \
    utils_lite.o \
    gep_protocol_lite.o \
    gep_channel_lite.o \
    gep_channel_array_lite.o \
    gep_client_lite.o
	$(make_lib)

install:
	$(INSTALL) -D -m 0755 \
		libgepserver.a \
		libgepclient.a \
		libgepserver-lite.a \
		libgepclient-lite.a \
		$(DESTDIR)/usr/lib/

clean::
	rm -f *.o libgepserver.a libgepclient.a libgepserver-lite.a libgepclient-lite.a
