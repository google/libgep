default: all

DIRS=include src test example

PREFIX=/usr
BINDIR=$(DESTDIR)$(PREFIX)/bin
LIBDIR=$(DESTDIR)$(PREFIX)/lib


all:     .protos_done $(addsuffix /all,$(DIRS))
test:    $(addsuffix /test,$(DIRS))
clean:   $(addsuffix /clean,$(DIRS))
install: $(addsuffix /install,$(DIRS))
install-libs: $(addsuffix /install-libs,$(DIRS))

test/test example/all : src/all

.protos_done: test/test.proto example/sgp.proto
	$(MAKE) -C test test.pb.h
	$(MAKE) -C example sgp.pb.h

%/all:
	$(MAKE) -C $* all

%/test:
	$(MAKE) -C $* test

%/clean:
	$(MAKE) -C $* clean

%/install:
	$(MAKE) -C $* install

%/install-libs:
	$(MAKE) -C $* install-libs

clean:
	rm -rf *~ .*~ */*.pb.* *.log .protos_done
