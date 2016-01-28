SGP: A Simple GEP-based Protocol
================================

This directory contains an example GEP-based protocol called SGP.
SGP is a simple protocol where server and clients can send to each
other a set pre-defined protocol buffer objects.

The directory contains the following files:

* `sgp.proto`: the list of messages that can be sent (defined as protocol
  buffers). We also include `sgp_lite.proto`, the protobuf-lite version
  of the list of messages.
* `sgp_client.h|cc`: the SGP client stub. It defines an abstract class that
  can be filled up in order to define an SGP-based client.
* `sgp_server.h|cc`: the SGP server stub. It defines an abstract class that
  can be filled up in order to define an SGP-based server.
* `sgp_protocol.h|cc`: the SGP protocol definition. It defines the default
  port and tags used in the wire.
* `client.cc`/`server.cc`: simple client and server based on the SGP stubs.

Notes:

* in SGP both client(s) and server can send and receive the same protocol
  messages. Your protocol may have messages that are only sent in one
  direction.
* the `sgp_client.h|cc`, `sgp_server.h|cc`, and `sgp_protocol.cc` files
  are almost completely boilerplate that could be generated from the
  `sgp_protocol.h` file. They only information the user needs to include
  is the list of protocol buffer messages that can be sent in each
  direction, the default port, and the tags associated to the different
  messages that can be sent. This should be relatively easy to generate
  automatically from an
  [IDL](https://en.wikipedia.org/wiki/Interface_description_language)
  (maybe something similar to [gRPC](http://www.grpc.io/)), which would
  allow autogen stubs.

