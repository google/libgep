TOP:=..
TARGETS=libgepserver.a libgepclient.a
include ../rules.mk

CPPFLAGS+=-I. -I.. -I../include
LIBS+=-lprotobuf

all: $(TARGETS)

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

install:
	$(INSTALL) -D -m 0755 \
		libgepserver.a \
		libgepclient.a \
		$(DESTDIR)/usr/lib/

clean::
	rm -f *.o libgepserver.a libgepclient.a
