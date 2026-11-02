# IRC protocol format. 
All IRC messages are text lines ending with CRLF (`\r\n`).

## Message Format

```
:[prefix] COMMAND :trailing parameter
```

**Components:**
- **Prefix** (optional, starts with `:`): identifies sender as `nick!user@host` or server name
- **Command** (required): uppercase word like `PRIVMSG`, `JOIN`, `NICK`, or a 3-digit number
- **Parameters**: space-separated values, with the last parameter optionally prefixed by `:` if it contains spaces

## Raw Line Examples

```
:nick!user@host PRIVMSG #channel :Hello world
:irc.server.net NOTICE nick :Server notice
:Alessandro JOIN #channel
NOTICE AUTH :*** Looking up your hostname...
nick: PASS secret
USER Alessandro 0 * :Alessandro
PING :irc.server.net
PONG :irc.server.net
```

The leading `:` on the prefix is optional when the client sends the message, but servers always include it. Trailing parameters starting with `:` can contain spaces; regular parameters cannot.

## Core Commands to Parse

| Command | Direction | Format | Description |
|---------|-----------|--------|-------------|
| `NICK` | client→server / broadcast | `NICK newname` | Change or announce nickname |
| `USER` | client→server | `USER user 0 * :Real Name` | Register username and real name |
| `JOIN` | client→server / broadcast | `JOIN #channel` or `JOIN #ch1,#ch2` | Join one or more channels |
| `PART` | client→server / broadcast | `PART #channel :Reason` | Leave channel |
| `PRIVMSG` | bidirectional | `PRIVMSG target :message` | Private or channel message |
| `NOTICE` | bidirectional | `NOTICE target :message` | Server/client notice (no auto-reply) |
| `QUIT` | broadcast | `QUIT :Reason` | Disconnect message |
| `KICK` | server→client | `KICK #channel nick :reason` | Remove user from channel |
| `TOPIC` | bidirectional | `TOPIC #channel :new topic` | Get/set channel topic |
| `MODE` | bidirectional | `MODE #channel +o nick` | Channel/user modes |
| `LIST` | client→server | `LIST #ch1,#ch2` | List channels |
| `INVITE` | server→client | `INVITE nick #channel` | Invite to channel |
| `PING` | bidirectional | `PING :server` | Keepalive probe |
| `PONG` | bidirectional | `PONG :server` | Keepalive response |
| `ERROR` | server→client | `ERROR :msg` | Terminate connection |

## Numeric Replies (Server Responses)

Numeric replies come as commands in the form `:server 001 nick :message`. Here are key ones:

| Code | Name | Meaning |
|------|------|---------|
| `001` | RPL_WELCOME | Welcome message |
| `002` | RPL_YOURHOST | Host info |
| `003` | RPL_CREATED | Server creation time |
| `004` | RPL_MYINFO | Server version/mode info |
| `251` | LUSERS | User count |
| `255` | LUSERS | Client count |
| `311` | WHOIS | WHOIS start (nick, user, host) |
| `332` | RPL_TOPIC | Current topic |
| `353` | RPL_NAMREPLY | Channel member list |
| `366` | END_OF_NAMES | End of NAMES list |
| `401` | ERR_NOSUCHNICK | No such nickname |
| `403` | ERR_NOSUCHCHANNEL | No such channel |
| `433` | ERR_NICKNAMEINUSE | Nickname in use |
| `464` | ERR_PASSWDMISMATCH | Wrong password |
| `471` | ERR_CHANNELISFULL | Channel full |
| `473` | INVITEONLY | Channel invite-only |
| `474` | BANNEDFROMCHAN | You're banned |
| `475` | BADCHANNELKEY | Wrong key |
| `530` | ERR_NOTLOGGEDIN | Not logged in |

Numbers below 400 are generally success/replies; 400+ are errors.

## Special Cases

1. **Trailing parameter**: When a parameter starts with `:`, everything after it is one string including spaces
2. **Empty messages**: Just `\r\n` means keep-alive, often ignored
3. **PING/PONG**: Servers send `PING :token`, clients respond with `PONG :token`
4. **Case insensitivity**: Commands are case-insensitive, nicknames may be case-sensitive depending on server config
5. **Line length limit**: Maximum 512 bytes per line (including CRLF). Long messages get truncated or rejected
6. **Whitespace**: Parameters are separated by ASCII 32 (space); no tabs allowed as separators

## Sample Connection Sequence

```
CLIENT → SERVER: NICK mybot
CLIENT → SERVER: USER bot 0 * :Bot Account
SERVER → CLIENT: :server 001 mybot :Welcome
SERVER → CLIENT: :server 002 mybot :Your host is server.net
CLIENT → SERVER: JOIN #channel
SERVER → CLIENT: :mybot JOIN #channel
SERVER → CLIENT: :server 353 mybot = #channel :Alessandro Folarin Giorgio
SERVER → CLIENT: :server 366 mybot #channel :End of /NAMES list
```
