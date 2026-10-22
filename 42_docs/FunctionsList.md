# ft_irc External Functions

## Sockets

* **socket**: creates an endpoint for communication and returns a file descriptor that refers to that endpoint.
The file descriptor returned by a successful call will be the lowest-numbered file descriptor not currently open for the process.

* **close**: closes a file descriptor, so that it no longer refers to any file and may be reused. 

* **setsockopt**: manipulate options for the socket referred to by the file descriptor sockfd.

* **getsockname**: returns the current address to which the socket sockfd is bound, in the buffer pointed to by addr.

* **getprotobyname**: returns a protoent structure for the entry from the database that matches the protocol name name.  A connection is opened to the database if necessary.

* **gethostbyname**: obsolete, use getaddrinfo.

* **getaddrinfo**: returns one or more addrinfo structures, each of which contains an Internet address that can be specified in a call to `bind()` or `connect()`.  

* **freeaddrinfo**: free addrinfo.

* **bind**: when a socket is created with ´socket()´, it exists in a name space (address family) but has no address assigned to it. ´bind()´ assigns the address specified by addr to the socket referred to by the file descriptor sockfd.

* **connect**: initiate a connection on a socket.

* **listen**: listen for connections on a socket.

* **accept**: accept a connection on a socket.

* **send**: send a message on a socket.

* **recv**: receive a message from a socket.

## Conversions

* **htons**: converts the unsigned short integer hostshort from host byte order to network byte order.

* **htonl**: converts the unsigned integer hostlong from host byte order to network byte order.

* **ntohs**: converts the unsigned short integer netshort from network byte order to host byte order.

* **ntohl**: converts the unsigned integer netlong from network byte order to host byte order.

* **inet_addr**: converts the Internet host address cp from IPv4 numbers-and-dots notation into binary data in network  byte order.

* **inet_ntoa**: converts the Internet host address in, given in network byte order, to a string in IPv4 dotted-decimal notation.

* **inet_ntop**: convert IPv4 and IPv6 addresses from binary to text form

## Signals

* **signal**: signal handling, sets the disposition of the signal signum to handler.

* **sigaction**: is used to change the action taken by a process on receipt of a specific signal. 

* **sigemptyset**: initializes the signal set given by set to empty, with all signals excluded from the set.

* **sigfillset**: initializes set to full, including all signals.

* **sigaddset**: add signal signum to set.

* **sigdelset**: delete signal signum from set.

* **sigismember**: tests whether signum is a member of set.

## File Descriptors

* **lseek**: repositions the file offset of the open file description associated with the file descriptor fd to the argument offset.

* **fstat**: get file status.

* **fcntl**: manipulate file descriptor.

## Multiplexing

* **poll**: waits for one of a set of file descriptors to become ready to perform I/O. 

* **epoll**: monitors multiple file descriptors to see if I/O is possible on any of them.

* **select**: obsolete, can monitor only file descriptors numbers that are less than FD_SETSIZE (1024)—an unreasonably low limit for many modern applications.

* **kqueue**: is a kernel event queue that allows efficient input and output event pipelines between the kernel and userland. 
