*This project has been created as part of the 42 curriculum by <foogungb> <aielo>*

# ft_irc — IRC Server

## Table of Contents

- [Description](#description)
  - [Technical Overview](#technical-overview)
    - [IRC Server Overview](#irc-server-overview)
    - [Functions Overview](#functions-overview)
- [Instructions](#instructions)
  - [Requirements](#requirements)
  - [Compilation](#compilation)
  - [Execution](#execution)
  - [Makefile Commands](#makefile-commands)
- [Resources](#resources)
  - [Documentation](#documentation)
  - [AI Usage](#ai-usage)
- [Project Status](#project-status)
- [Authors](#authors)

## Description

**ft_irc** is a C++ project focused on implementing an Internet Relay Chat (IRC) server.

The goal of this project is to understand network programming, TCP/IP communication, socket management, and the client-server architecture while implementing the core functionality of an IRC server.

The server is designed to accept incoming client connections, authenticate users with a password, and handle IRC communication according to the project's requirements.

The project also explores important C++ concepts, including object-oriented design, error handling, resource management, and modular code organization.

### Technical Overview

- **Language:** C++
- **Standard:** C++98, as required by the project subject
- **Protocol:** Internet Relay Chat (IRC)
- **Networking:** TCP/IP sockets
- **Build system:** Makefile

#### IRC Server Overview

- [IRC Server Markdown file](42_docs/HowIrcServerWorks.md)

#### Functions Overview

- [Functions Explanation Markdown file](42_docs/FunctionsList.md)

## Instructions

### Requirements

To compile and run the project, you need:

- A C++ compiler, such as `c++`
- `make`
- A Unix-like operating system

### Compilation

Clone the repository and navigate to its root directory:

```bash
git clone <repository-url>
cd <repository-directory>
```

Compile the project:

```bash
make
```

This generates the `ircserv` executable.

### Execution

Run the server by providing a port number and a connection password:

```bash
./ircserv <port> <password>
```

Example:

```bash
./ircserv 6667 mypassword
```

Arguments:

- `port`: The TCP port on which the server listens. It must be an integer between `1` and `65535`, and the port must be available.
- `password`: The password required for client authentication.

The server may report an error if the arguments are invalid, the port is unavailable, or the operating system prevents the requested socket operation.

### Makefile Commands

```bash
make        # Compile the project
make clean  # Remove object files
make fclean # Remove object files and the executable
make re     # Rebuild the project
```

## Resources

The following resources provide background information on IRC, network programming, and C++.

### Documentation

- [RFC 1459 — Internet Relay Chat Protocol](https://www.rfc-editor.org/rfc/rfc1459)  
  The original IRC protocol specification, describing the protocol's architecture, messages, and commands.

- [RFC 2812 — Internet Relay Chat: Client Protocol](https://www.rfc-editor.org/rfc/rfc2812)  
  A reference for IRC client protocol messages and command formats.

- [Linux manual pages — `socket(2)`](https://man7.org/linux/man-pages/man2/socket.2.html)  
  Documentation for creating communication endpoints.

- [Linux manual pages — `bind(2)`](https://man7.org/linux/man-pages/man2/bind.2.html)  
  Explains how to associate a socket with a local address and port.

- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)  
  A practical introduction to TCP/IP networking, sockets, and client-server communication.

- [cppreference — C++ language reference](https://en.cppreference.com/w/cpp)  
  A reference for C++ syntax, standard library facilities, and language features.

- [PowerCert Animated videos - Network Ports Explained](https://youtu.be/g2fT-g9PX9o?si=xK0UCGMHGc02t7qx)
  A guide to uderstanding ports, different port proviledges, and available ranges.

### AI Usage

AI tools, including ChatGPT, were used as supplementary learning and development aids during the project.

Their use included:

- **C++ concepts:** clarifying language features, compilation errors, and appropriate coding conventions.
- **Error handling:** reviewing input validation, exception handling, and error-reporting approaches.
- **Code review:** discussing implementation choices, identifying potential logical errors, and considering improvements to code structure.
- **Documentation:** assisting with the organization and drafting of this README.

AI-generated suggestions were used as guidance rather than treated as authoritative solutions. Implementation decisions, testing, and verification against the project subject remain the responsibility of the project authors.

## Project Status

This project is under development. Implemented features and supported IRC commands will be documented here as they are completed and tested.

## Authors

- <foogungb> <aielo>
