TOP:=.
include $(TOP)/rules.mk

default: all

SUBDIRS=include src test example

PREFIX=/usr
BINDIR=$(DESTDIR)$(PREFIX)/bin
LIBDIR=$(DESTDIR)$(PREFIX)/lib

COV_SUBDIRS=$(filter-out include,$(SUBDIRS))


all:     .protos_done $(addsuffix /all,$(SUBDIRS))
test:    $(addsuffix /test,$(SUBDIRS))
tests:   .protos_done $(addsuffix /tests,$(SUBDIRS))
clean::  $(addsuffix /clean,$(SUBDIRS))
install: $(addsuffix /install,$(SUBDIRS))
install-libs: $(addsuffix /install-libs,$(SUBDIRS))

test/test example/all : src/all

.protos_done: test/test.proto example/sgp.proto
	$(MAKE) -C test test.pb.h
	$(MAKE) -C test test_lite.pb.h
	$(MAKE) -C example sgp.pb.h
	$(MAKE) -C example sgp_lite.pb.h

%/all:
	$(MAKE) -C $* all

%/test:
	$(MAKE) -C $* test

%/tests:
	$(MAKE) -C $* tests

%/clean:
	$(MAKE) -C $* clean

%/install:
	$(MAKE) -C $* install

%/install-libs:
	$(MAKE) -C $* install-libs

# Run proper ThreadSanitizer from scratch
ThreadSanitizer:
	rm -f $(TSAN_OUT)/*.log
	$(MAKE) clean
	$(MAKE) test-tsan

# Run proper AddressSanitizer from scratch
AddressSanitizer:
	rm -f $(ASAN_OUT)/*.log
	$(MAKE) clean
	$(MAKE) test-asan

# Run proper host target coverage from scratch
host-coverage:
	$(MAKE) clean
	$(MAKE) USE_COV=1 NO_CLANG=1 test
	$(MAKE) -j1 gen_cov_report

# Build cross-platform image with coverage enabled
cross-coverage:
	$(MAKE) clean
	$(MAKE) USE_COV=1 NO_CLANG=1 COV_OUT_DIR="/tmp/cov_libgep"

# Coverage HTML output directory
COV_HTML_OUT=coverage_html

gen_cov_report: cov
	$(GENINFO) --gcov-tool gcov --no-external --no-recursion $(COV_SUBDIRS) \
      --output-file libgep.info
	$(GENHTML) -o $(COV_HTML_OUT)/ --highlight --legend -t $(COV_HTML_OUT)/ \
      libgep.info

clean::
	rm -rf *~ .*~ */*.pb.* *.log .protos_done
