# How an IRC Server Works

Internet Relay Chat (IRC) servers are specialized software programs designed to manage conversations between multiple clients over the internet.

At its core, an IRC server acts as a **central hub and referee** for communication. It doesn't *host* the conversations; it *transmits* them reliably, manages connections, enforces rules, and organizes participants into different channels.

---

### 1. The Architecture

The operation is fundamentally a **Client-Server** model:

* **IRC Client:** The client's only job is to *render* the information it receives and *package* the text to send to the server.
* **IRC Server:** It handles thousands of connections, interprets the commands, and manages the state of every channel and user.

### 2. The Communication Protocol

IRC doesn't use complex, modern messaging protocols (like HTTP/REST). It uses a very simple, **plain-text protocol**.

When a client connects, it sends raw, structured text lines to the server. The server expects lines to follow a specific format, usually starting with a command or a "prefix" (like `PRIVMSG` or `JOIN`).

#### Example:
If you type "Hello everyone" in a channel, the client doesn't send a "message object." It sends a line like this:

```
PRIVMSG #general :Hello everyone
```

The server reads this line, sees the `PRIVMSG` command, looks at the `#general` address, and then broadcasts the payload (`:Hello everyone`) to every other client currently in that channel.

**The Server's Job:**
1. **Parsing:** Read the raw line of text.
2. **Interpretation:** Determine what the user is asking for (`JOIN`, `QUIT`, `PRIVMSG`, etc.).
3. **Execution:** Find all necessary participants and actions to complete the command.

### 3. The Lifecycle of a Connection

#### A. The Handshake (Authentication)
1. **Connection:** The client initiates a TCP/IP connection to the server's port (usually 6667).
2. **Identification:** The client sends its Nickname (e.g., `UserX`) and usually an empty password (IRC defaults to no passwords).
3. **Server Response:** The server responds with an `OFT_USER` message, confirming that the nickname is available and that it´s successfully connected.
4. **Status:** The client is now officially "Connected" to the server.

#### B. Channel Participation (Joining Rooms)
When you type `:join #channelname`, the following happens:
1. The client sends the `JOIN #channelname` command.
2. The server checks its database/memory to see who is already in `#channelname`.
3. The server adds the user ID to the list of participants for that channel.
4. **Broadcasting:** The server sends a "broadcast" message *to everyone in the channel*, stating that a new user has joined: `:UserX HAS JOINED #channelname`.

#### C. Messaging and Broadcasting
When you send a message (`PRIVMSG`), the server acts as a relay:
1. **Input:** Client sends: `PRIVMSG #channelname :Your message`
2. **Processing:** The server verifies that `UserX` has permission to speak in `#channelname`.
3. **Relaying:** The server loops through every client currently connected to `#channelname`.
4. **Output:** For each recipient client, the server sends a copy of the message.

### 4. Key Features Managed by the Server

* **State Management:** The server must remember everything. It tracks: *Who* is connected, *What* channels they are in, *What* their current nicknames are, and *What* their permissions are.
* **Nick Management:** If two users try to connect with the same nickname, the server rejects the second one. It also manages nickname changes (e.g., if a user commands `:nick newname`).
* **Multi-Server Routing (Federation):** This is what makes IRC a "relay." A large IRC network (like Libera.Chat) is actually dozens or hundreds of servers linked together. If you talk to a user on a server in London, the server "relays" your message through its connections to the server in New York, which then delivers it to the recipient.
* **Moderation and Commands:** The server handles special codes and commands (e.g., `MODE` to set permissions, `NOTICE` to send a private message to a single user).

### 5. How Do Servers Scale?

Running a single IRC server to handle millions of users is impossible because the protocol is chatty and stateful (it needs to know *everything* about every user at all times).

Instead, large networks use **Hierarchical Clusters**:

1. **Local Servers:** Clusters of servers serving a specific geographic region or segment.
2. **Backbones:** High-speed fiber connections that link these local servers together, allowing messages to be forwarded across the entire globe seamlessly.
3. **Load Balancing:** Traffic is distributed across multiple entry points to ensure no single server becomes a bottleneck.

An IRC server works as a highly efficient **text-parsing, state-tracking relay switch**. It doesn't provide intelligence or content; it provides the **reliable, low-latency network pipe** necessary for users and their clients to exchange text messages and commands according to the IRC protocol.

## How a Basic IRC Server Uses `poll()` or Similar and Sockets

### 1. The Architectural Overview: The Need for Concurrency

An IRC server doesn't just handle one user at a time; it must handle potentially thousands of simultaneous connections. If a server used a simple, single-threaded loop (blocking I/O), it would have to wait for User A to finish sending data before it could even check if User B sent a message. This is incredibly slow.

**The Goal:** The server needs to check many open sockets at once: *Is Socket 1 ready to send? Is Socket 50 ready to receive? Is Socket 100 ready to send?*

---

### 2. The Basics: Sockets (TCP/IP)

Sockets are the endpoint for network communication. They provide the fundamental mechanism for sending and receiving data over the network.

#### The Socket Lifecycle:
1. **`socket()`**: Create a new communication endpoint (e.g., AF_INET for IPv4).
2. **`bind()`**: Assign a specific port (like 6667 for IRC) to the socket, making the server ready to listen.
3. **`listen()`**: Put the socket into a state where it can accept incoming connections.
4. **`accept()`**: This is **blocking**. The server pauses here until a new client attempts to connect. Once a connection is made, a *new* socket is created for that specific client.

#### The Problem with `accept()`:
When `accept()` returns, the server has a dedicated socket for that user. If the server continues to use a standard `read()` on this client socket, the server will **block** (stop all activity) until the client sends data. If the client is silent, the server stops functioning.

---

### 3. The Solution: I/O Multiplexing (`poll()`, `select()`, `epoll()`)

This is where I/O multiplexing comes in. Instead of calling `read()` on one socket and waiting, the server passes *many* socket descriptors (File Descriptors, or FDs) to a system call, like `poll()`.

#### What `poll()` does:

1. **Setup:** The server creates a list (an array) containing the File Descriptors (FDs) of *all* connected clients.
2. **The Call:** The server calls `poll(fd_array, num_fds, timeout)`.
3. **The Wait:** The operating system kernel takes control. The server thread **blocks** inside `poll()`, but critically, it is not "waiting" for any one socket. It tells the kernel: "Wake me up if *any* of these sockets are ready for reading OR ready for writing."
4. **The Wake-up:** When the kernel detects that, for example, Client A has sent data, the `poll()` call returns.
5. **The Check:** The `poll()` call provides an array of "events." The server loops through this event array:
* *Event found for FD 5:* This means Client 5 has data ready to be read.
* *Event found for FD 12:* This means Client 12 has data ready to be written (perhaps the server needs to send a PING).
6. **Action:** The server executes the necessary action (`read()` for Client 5, `write()` for Client 12) and then returns to step 1 to call `poll()` again.


### 4. Why Not Threads?

A classic alternative to `poll()` is using **Threading** (one thread per user).

* **Threading Pros:** Easier to write the initial logic; blocking `read()` calls naturally work in a separate thread.
* **Threading Cons:** Massive overhead. Creating, managing, and context-switching thousands of threads consumes huge amounts of RAM and CPU cycles.

**`poll()`** (and its modern successor `epoll`) solves this by using a single, highly efficient thread managed by the operating system kernel to handle thousands of connections.

### Summary Table

| Feature | Standard Blocking I/O (Bad for Servers) | I/O Multiplexing (`poll`) |
| :----------------- | :-------------------------------------- | :--------------------------------------------------- |
| **Mechanism** | Call `read()` on Socket A, then wait. | Pass *all* Sockets to the Kernel (`poll`). |
| **Behavior** | Blocks completely on a single socket. | Blocks, but waits for *any* socket to become active. |
| **Concurrency** | Low (Sequential) | High (Cooperative, efficient) |
| **Resource Usage** | Thread-per-user (High overhead) | Single thread handling many sockets |
