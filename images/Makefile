TOP:=..
include ../rules.mk

all:

install:

diagram: protocol_diagram.dot
	dot protocol_diagram.dot -Tsvg > protocol_diagram.svg

clean::
	rm -f *.pb.* .protos_done
