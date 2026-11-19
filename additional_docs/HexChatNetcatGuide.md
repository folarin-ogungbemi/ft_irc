# HexChat / netcat Quick Reference Guide

## Testing with Hexchat

### Setup

1. Launch the server: `./ircserv <port> <password>`
2. Open HexChat → **Network List** → **Add** a new network
3. Click **Edit** and set:
   - **Servers:** `127.0.0.1/<port>` (e.g. `127.0.0.1/6667`)
   - **Nick name**, **User name**, **Real name** of your choice
   - **Login method:** *Server password* → enter the server `<password>`
4. **Connect**, then use the commands below (type them in any text field,
   prefixed by `/`).

### Implemented Commands

#### Registration / Connection

- PASS - Authenticate with the server password (sent by HexChat at login)  
- NICK - Set or change the client's nickname  
- USER - Set the username and real name (sent by HexChat at login)  
- PING/PONG - Keep-alive: reply to the server's ping to stay connected  
- QUIT - Close the connection cleanly

#### Messaging

- JOIN - Join a channel (creates it if it doesn't exist)
- PRIVMSG - Send a private message to a user or to a channel (channel messages are forwarded to every member)  

#### Channel Operator Commands

- KICK - Eject a client from the channel
- INVITE - Invite a client to a channel
- TOPIC - Change or view the channel topic
- MODE - Change the channel's mode:

    +i: Set/remove Invite-only channel  
    +t: Set/remove the restrictions of the TOPIC command to channel operators  
    +k: Set/remove the channel key (password)  
    +o: Give/take channel operator privilege  
    +l: Set/remove the user limit to channel

#### Misc Commands

- PART - Leave the channel
- LIST - List all channels
- WHO - List everybody on the IRC network

### Testing Scenarios

To verify the requirements with HexChat:

- **Authentication:** start two instances with matching nick/user/pass — duplicates must be rejected.
- **Channel broadcast:** `JOIN #test` on both clients, send a message with one, check the other receives it.
- **Operators:** the first user to `JOIN` a channel becomes its operator (`@nick`).
- **KICK:** `KICK #test nick` removes the user, who also leaves the channel.
- **INVITE:** with `+i` set, only invited users may `JOIN #test`.
- **TOPIC:** with `+t` set, only operators can change topic.
- **MODE +k:** `/mode #test +k password` — joining then requires `/join #test password`.
- **MODE +o:** `/mode #test +o nick` grants operator; `/mode #test -o nick` revokes it.
- **MODE +l:** `/mode #test +l 2` — a third `JOIN` gets rejected.

## Testing with netcat (CLI)

One-liner to simulate a full registration and join:

```bash
printf 'PASS password\r\nNICK cli_tester\r\nUSER tester 0 * :CLI Tester\r\nJOIN #test\r\nPRIVMSG #test :hello from nc\r\n' | nc -q 5 127.0.0.1 6667
```

`-q 5` keeps the connection open 5 seconds after stdin closes.

### Interactive session

```bash
nc -C -N 127.0.0.1 6667
PASS password
NICK cli_tester
USER tester 0 * :CLI Tester
```

Then type commands by hand.

### Multi-client test

Run one interactive `nc` session in terminal A, then a one-liner in terminal B with
a different nick — useful to see channel broadcasts and operator privileges.
