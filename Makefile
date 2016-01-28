# Copyright Google Inc. Apache 2.0.

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
test-full:    $(addsuffix /test-full,$(SUBDIRS))
test-lite:    $(addsuffix /test-lite,$(SUBDIRS))
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

%/test-full:
	$(MAKE) -C $* test-full

%/test-lite:
	$(MAKE) -C $* test-lite

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
	$(MAKE) -j1 gen_cov_report

# Build cross-platform image with coverage enabled
cross-coverage:
	$(MAKE) clean
	$(MAKE) USE_COV=1 NO_CLANG=1 COV_OUT_DIR="/tmp/cov_libgep"

# Coverage HTML output directory
COV_HTML_OUT=coverage_html

gen_cov_report: cov geninfo-full geninfo-lite
	lcov --add-tracefile libgep-full.info -a libgep-lite.info -o libgep.info
	$(GENHTML) -o $(COV_HTML_OUT)/ --highlight --legend -t $(COV_HTML_OUT)/ \
      libgep.info

geninfo-full:
	$(MAKE) USE_COV=1 NO_CLANG=1 test-full
	$(GENINFO) --gcov-tool gcov --no-external --no-recursion $(COV_SUBDIRS) \
      --output-file libgep-full.info

geninfo-lite:
	$(MAKE) USE_COV=1 NO_CLANG=1 test-lite
	$(GENINFO) --gcov-tool gcov --no-external --no-recursion $(COV_SUBDIRS) \
      --output-file libgep-lite.info

clean::
	rm -rf *~ .*~ */*.pb.* *.log .protos_done
