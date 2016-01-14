SGP: A Simple GEP-based Protocol
================================

This directory contains an example GEP-based protocol called SGP.
SGP is a simple protocol buffers-based, client/server protocol: server
and clients can send to each other a set of protocol buffer-defined objects.

The directory contains the following files:

* sgp.proto: the list of messages that can be sent (defined as protocol
  buffers).
* sgp\_client.h|cc: the SGP client stub. It defines an abstract class that
  can be filled up in order to define an SGP-based client.
* sgp\_server.h|cc: the SGP server stub. It defines an abstract class that
  can be filled up in order to define an SGP-based server.
* sgp\_protocol.h|cc: the SGP protocol definition. It defines the default
  port and tags used in the wire.
* client.cc/server.cc: simple client and server based on the SFP stubs.

Notes:

* in SGP both client(s) and server can send and receive the same protocol
  messages. Your protocol may have messages that are only sent in one
  direction.
* the sgp\_client, sgp\_server, and sgp\_protocol files are almost completely
  boilerplate. The only items defined are the default port, the tags
  associated to the different messages that can be sent, and which tags
  are used in the wire for each message. This should be relatively easy
  to autogen from a file defining these values, which would allow
  autogen stubs.

