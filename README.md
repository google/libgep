GEP: A Generic, Protobuf-Based Client-Server Protocol
=====================================================

\[This document lives at go/gfiber.gep\]

Introduction
------------

GEP (Generic Event Protobuf aka Generic Eh... Protobuf) is a generic
protocol to implement asynchronous, protobuf-based, N-clients/1-server
protocols.

A protocol is defined as a series of protobuf messages that can be
exchanged between a client and a server. The idea is to make it very
easy for a user to create a new protocol by defining:

* (1) the port where the server will listen to requests from (multiple)
clients, and
* (2) the tags/protobuf messages that the protocol will allow passing
back and forth.

![Alt text](http://g.gravizo.com/g?
  digraph G {
    rankdir=LR;
    ClientAPI [shape=box, label="Send\(const Protobuf_1 &msg\n...\nSend\(const Protobuf_n-1 &msg\nRecv\(const Protobuf_n &msg\n...\nRecv\(const Protobuf_m &msg"];
    ClientAPI -> client;
    client -> ClientAPI;
    client -> server [label="protobuf messages"];
    server -> client;
    ServerAPI [shape=box, label="Send\(const Protobuf_n &msg\n...\nSend\(const Protobuf_m &msg\nRecv\(const Protobuf_1 &msg\n...\nRecv\(const Protobuf_n-1 &msg"];
    ServerAPI -> server;
    server -> ServerAPI;
  }
)

    Figure 1: Diagram of a GEP protocol-based client/server.


GEP-based protocols are asynchronous by definition: The sender sends a protobuf
message to the receiver, and it does not wait for an answer from the receiver.
If your protocol needs status/return messages, they need be implemented on top
of GEP.

The asynchronous behavior is coherent with the operation of the main user,
where one server sends and receives messages to/from multiple clients.
The use of protobuf messages allows both clients and servers that keep
their data structures as protobufs themselves. This makes the code
extremely simple, and very easy to serialize (e.g. dump state in the
logs in a systematic way).

The implementation for the GEP protocol is in the
[gfiber repo](https://gfiber-internal.googlesource.com/vendor/google/libgep).
The GEP protocol is the basis for the implementation of:

(1) the [Mints protocol](https://go/gfiber.mints), which is used to
allow communication between sagesrv and the fiber-ads CPE ads manager.
(2) a protocol for sending protobuf messages (user configuration requests)
between SageTV and adsmgr (GEP/SageTV), and
(3) the MCNMP API at gfiber's miniclient.



GEP Operation: Protocol Definition
----------------------------------

A GEP protocol can be defined by creating a class that derives from
GetProtocol, and which implements a couple of functions that map a
set of unique IDs (aka ``tags'') to the protobuf messages that can
be passed back and forth between client and server. In particular,
GetTag() maps a protobuf message to a tag, while GetMessage() maps a
tag to a protobuf message.

    $ cat test_protocol.h
    ...
    class TestProtocol : public GepProtocol {
     ...
      // supported messages
      static constexpr uint32_t MSG_TAG_COMMAND_1 =
          MakeTag('c', 'm', 'd', '1');
      static constexpr uint32_t MSG_TAG_COMMAND_2 =
          MakeTag('c', 'm', 'd', '2');
      ...
      // returns the tag associated to a message.
      virtual uint32_t GetTag(const ::google::protobuf::Message *msg);
      // constructs an object of a given type.
      virtual ::google::protobuf::Message *GetMessage(uint32_t tag);
    };
    ...
    $ cat test_protocol.cc
    uint32_t TestProtocol::GetTag(const ::google::protobuf::Message *msg) {
      if (dynamic_cast<const Command1Message *>(msg) != NULL)
        return MSG_TAG_COMMAND_1;
      else if (dynamic_cast<const Command2Message *>(msg) != NULL)
        return MSG_TAG_COMMAND_2;
      ...
    }

    ::google::protobuf::Message *TestProtocol::GetMessage(uint32_t tag) {
      ::google::protobuf::Message *msg = NULL;
      switch (tag) {
        case MSG_TAG_COMMAND_1:
          msg = new Command1Message();
          break;
        case MSG_TAG_COMMAND_2:
          msg = new Command2Message();
          break;
        ...
      }
      return msg;
    }

    Figure 2: Implementation of TestProtocol, a GEP-based protocol.


GEP Operation: Client/Server Definition
---------------------------------------

After defining the protocol, you must define the client and the server
(both are very similar). The client must derive from GepClient, and
the server from GepServer (these classes implement the network
operations). Then, both client and server must be initialized with a
virtual function table (GepVFT) that maps the subset of tags that each
side wants to listen to, to the callback that must be called on receiving
one.

In the client side, a GepChannel object provides a SendMessage()
function that allows sending protobuf messages to the server. It also
provides a (selectable) socket where to listen for messages from the
server, and a RecvData() function that creates a protobuf message
and dispatches it to the corresponding callback.

The GepClient object performs the obvious client reception side:
select socket, receive message, dispatch it.

In the server side, a GepChannelArray object provides a SendMessage()
function that allows sending protobuf messages to all the clients,
plus a per-client SendMessage() that sends the message to a single
one. It also provides a (selectable) server socket where to listen
for connections from the server, and a (bound size) vector of
GepChannel objects that give access to the (selectable) sockets where
to listen for messages from the different clients (and their
RecvData() function).

The GepServer object performs the obvious server reception side (select
server and per-client sockets, create an object for new clients,
receive message and dispatch it for messages).

GEP Implementation
------------------

Any GEP-derived protocol encodes a protobuf message in the wire using
a very simple wire format:

        0                   1                   2                   3
        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     0 |                            gep_id                             |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     4 |                              tag                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     8 |                           value_len                           |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    12 |                              ...                              |
       |                            message                            |
       |                              ...                              |
       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    Figure 3: A GEP protocol packet in the wire.

Where:

  - gep\_id: unique GEP identificator [4 bytes]. It must consist of `gepp`
    (0x67657070).
  - tag: tag identifying the message being sent [4 bytes]. The tags are
    those used to defined the protocol.
  - value\_len: length of the message [4 bytes]. This is the length of the
    packet (minus the 12-byte header).
  - message: a serialized protobuf message [value\_len bytes]. We are
    currently using a text protobuf. This allows easy debugging of wire
    packets.

