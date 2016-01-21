TOP:=..
include ../rules.mk

all: $(TARGETS)

install:
	$(INSTALL) -D -m 0444 \
		gep_common.h \
		gep_server.h \
		gep_protocol.h \
		gep_client.h \
		gep_channel.h \
		gep_channel_array.h \
		gep_utils.h \
		$(DESTDIR)/usr/include/
