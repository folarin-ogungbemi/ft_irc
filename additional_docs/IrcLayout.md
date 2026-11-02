# IRC Server Layout

A minimal reference layout: one IRC server, four connected clients, and two
server-side channels. Alessandro is in both channels, which is how a single client
can hold multiple memberships at once.

## Network & Channel Layout

```mermaid
flowchart LR
    subgraph CL["Clients"]
        direction TB
        C1["Alessandro — client 1"]
        C2["Davide — client 2"]
        C3["Giorgio — client 3"]
        C4["Folarin — client 4"]
    end

    SRV["IRC Server<br/>irc.example.net:6697<br/>TLS + IRCv3"]

    subgraph CH["Server-side channels"]
        direction TB
        G["#general<br/>members: Alessandro, Davide, Giorgio"]
        D["#dev<br/>members: Alessandro, Folarin"]
    end

    C1 ==>|"NICK / USER / JOIN #general / JOIN #dev"| SRV
    C2 ==>|"NICK / USER / JOIN #general"| SRV
    C3 ==>|"NICK / USER / JOIN #general"| SRV
    C4 ==>|"NICK / USER / JOIN #dev"| SRV

    SRV --- G
    SRV --- D

    classDef client stroke:#6d4aff,stroke-width:1px
    classDef server stroke:#4a2fd0,color:#ffffff,stroke-width:1px
    classDef chan stroke:#d4a017,stroke-width:1px

    class C1,C2,C3,C4 client
    class SRV server
    class G,D chan
```

Each client holds one TCP connection to the server. Channels
are not peer-to-peer: `#general` and `#dev` exist only as objects on the server,
and every message is relayed through it. That is why Alessandro can be in two
channels over a single socket.

## Message Flow

```mermaid
sequenceDiagram
    autonumber
    participant A as Alessandro
    participant S as IRC Server
    participant B as Davide
    participant D as Folarin

    A->>S: NICK Alessandro
    A->>S: USER Alessandro 0 * :Alessandro
    S-->>A: 001 Welcome to the network
    A->>S: JOIN general
    S-->>A: 353 / 366 names list
    S-->>B: :Alessandro!~a@host JOIN general

    A->>S: PRIVMSG general :hello everyone
    S-->>B: :Alessandro!~a@host PRIVMSG general :hello everyone

    A->>S: JOIN dev
    S-->>D: :Alessandro!~a@host JOIN dev

    D->>S: PRIVMSG dev :hi Alessandro
    S-->>A: :Folarin!~d@host PRIVMSG dev :hi Alessandro
```

## Notes

- **Ports:** 6667 is the classic plaintext port; 6697 is the conventional TLS
  port.
- **Prefixes:** outgoing messages carry a `:nick!user@host` prefix added by the
  server, not by the sending client.
- **Channel ops:** membership alone grants voice; `MODE #general +o Alessandro`
  grants operator privileges such as `KICK` and topic control.
