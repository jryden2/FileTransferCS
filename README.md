# FileTransferCS
UDP File Transfer Client Server example

Usage:
> FileTransferCS [filename] [--server|--client]

Default behaviour uses filename 'temp.txt' and application will behave as both client and server.

To run as two different sessions execute two copies
> FileTransferCS --server
> FileTransferCS myfile.txt --client
> 

Transfer file must exist.  Server will create a subdirectory 'Received' in the current directory

Application can run as a standalone app, passing UDP packets between client and server entities
Command line parameters can be used to isolate the server side and client side behaviour.  Simply execute two copies of the executable, passing the --server command line switch
for server behaviour and the --client command line switch for client side behaviour.

Code layout
DataTransferClient - Core processor responsible for sending client side data and receiving responses
DataTransferServer - Core processor responsible for receiving server side data and sending responses
TransactionManager - Core processor responsible for collecting and re-ordering incoming packets (also used by the client to save outgoing packets for retransmission)
TransactionUnit - Interpreter for converting coded messages into readable data blocks and generating the formatted message data for sending on the wire
UDPUnreliableSenderReceiver - Implements the UDP layers for sending and receiving data over a UDP socket
FileReader - Implements the IReader interface, using the file system
FileWriter - Implements the IWriter interface, using the file system
SimpleLogger - Implements the ILogger interface - currently just prints to stdout
WorkerThreadPool - Implements the IWorkerThreadPool interface - creates and manages worker threads

Features
'SOLID' coding techiniques
All behaviour is injected into the main processors using abstract interfaces
Test based development - all objects are testable by mocking or stubbing injected interfaces
Modern C++ coding - smart pointers (shared/unique), C++14/17 and other standard library usages


Outstanding issues and TODOs
- Sending of large files can overwhelm the UDP transport stack resulting in permanently lost packets including the end packet
- Need a better ACK mechanism for packet transfer
- Although code is currently set up for retransmission requests, missing packets are not detected by the server and the missing packet
  request is never sent
- Transmit port is hard coded to 1234
- Needs more unit tests
- std::filesystem inclusion creates an unusual build error.  Build is only successful when doing a 'rebuild all'.  This requires some investigation.
