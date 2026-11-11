#include "Server.hpp"
#include <iomanip>
#include <cctype>
#include <fstream>
#include <limits>

namespace
{
	const size_t		kMaxLineLength = 512;
	const int			kPollTimeoutMs = 1000;
	const std::string	kServerName("42_ircserv");

	struct ChannelModeChange
	{
		char		mode;
		bool		adding;
		std::string	parameter;
		int			targetFd;
		size_t		limit;

		ChannelModeChange(void)
		: mode('\0'), adding(true), targetFd(-1), limit(0)
		{}
	};

	std::string	toUpper(std::string text)
	{
		for (size_t i = 0; i < text.size(); ++i)
			text[i] = static_cast<char>(std::toupper(static_cast<unsigned char>(text[i])));
		return (text);
	}

	std::string	toLower(std::string text)
	{
		for (size_t i = 0; i < text.size(); ++i)
			text[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(text[i])));
		return (text);
	}

	bool	isNicknameSpecialChar(char c)
	{
		return (c == '[' || c == ']' || c == '\\' || c == '`'
			|| c == '_' || c == '^' || c == '{' || c == '|' || c == '}');
	}

	bool	isValidNickname(const std::string& nickname)
	{
		if (nickname.empty() || nickname.size() > 30)
			return (false);
		if (!std::isalpha(static_cast<unsigned char>(nickname[0]))
			&& !isNicknameSpecialChar(nickname[0]))
			return (false);
		for (size_t i = 1; i < nickname.size(); ++i)
			if (!std::isalnum(static_cast<unsigned char>(nickname[i]))
				&& !isNicknameSpecialChar(nickname[i]) && nickname[i] != '-')
				return (false);
		return (true);
	}

	std::string	escapeForLog(const std::string& text)
	{
		std::ostringstream	escaped;
		unsigned char		character;

		for (size_t i = 0; i < text.size(); ++i)
		{
			character = static_cast<unsigned char>(text[i]);
			if (character >= 32 && character < 127)
				escaped << text[i];
			else
				escaped << "\\x" << std::hex << std::setw(2) << std::setfill('0')
					<< static_cast<int>(character) << std::dec;
		}
		return (escaped.str());
	}

	std::vector<std::string>	splitOnCommas(const std::string& text)
	{
		std::vector<std::string>	items;
		size_t						start = 0;
		size_t						commaPos;

		while (start <= text.size())
		{
			commaPos = text.find(',', start);
			if (commaPos == std::string::npos)
				commaPos = text.size();
			if (commaPos > start)
				items.push_back(text.substr(start, commaPos - start));
			start = commaPos + 1;
		}
		return (items);
	}
}

volatile sig_atomic_t	Server::_stopRequested = 0;

Server::Server(int port, const std::string& password)
: _listenSocketFd(-1)
, _port(port)
, _password(password)
{
	this->_registerCommands();
	this->_loadMotd("motd.txt");
	try
	{
		this->_setupListenSocket();
	}
	catch (...)
	{
		this->_cleanup();
		throw;
	}
}

Server::~Server()
{
	this->_cleanup();
}

void	Server::_signalHandler(int signalNumber)
{
	(void)signalNumber;
	_stopRequested = 1;
}

void	Server::setupSignals(void)
{
	struct sigaction	action;

	std::memset(&action, 0, sizeof(action));
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	action.sa_handler = &Server::_signalHandler;
	sigaction(SIGINT, &action, NULL);
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGQUIT, &action, NULL);

	action.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &action, NULL);
}

void	Server::_setupListenSocket(void)
{
	struct sockaddr_in	serverAddr;
	struct pollfd		listenPollFd;
	int					reuseAddress;

	if (this->_port < 1 || this->_port > 65535)
		throw std::runtime_error("invalid port");

	this->_listenSocketFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listenSocketFd == -1)
		throw std::runtime_error("socket() failed");

	reuseAddress = 1;
	if (setsockopt(this->_listenSocketFd, SOL_SOCKET, SO_REUSEADDR,
			&reuseAddress, sizeof(reuseAddress)) == -1)
		throw std::runtime_error("setsockopt() failed");

	std::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(static_cast<uint16_t>(this->_port));

	if (bind(this->_listenSocketFd,
			reinterpret_cast<struct sockaddr*>(&serverAddr),
			sizeof(serverAddr)) == -1)
		throw std::runtime_error("bind() failed");

	if (listen(this->_listenSocketFd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() failed");

	this->_setNonBlocking(this->_listenSocketFd);

	listenPollFd.fd = this->_listenSocketFd;
	listenPollFd.events = POLLIN;
	listenPollFd.revents = 0;
	this->_pollFds.push_back(listenPollFd);

	std::cout << "server listening on port " << this->_port << "\n";
}

void	Server::_setNonBlocking(int socketFd)
{
	if (fcntl(socketFd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl() failed");
}

void	Server::_updatePollEvents(void)
{
	std::map<int, std::string>::const_iterator	sendBufferIt;

	for (size_t i = 0; i < this->_pollFds.size(); ++i)
	{
		if (this->_pollFds[i].fd == this->_listenSocketFd)
			continue ;
		this->_pollFds[i].events = POLLIN;
		sendBufferIt = this->_sendBuffers.find(this->_pollFds[i].fd);
		if (sendBufferIt != this->_sendBuffers.end()
			&& !sendBufferIt->second.empty())
			this->_pollFds[i].events |= POLLOUT;
	}
}

void	Server::run(void)
{
	int		readyCount;
	short	events;
	bool	stillConnected;

	while (!_stopRequested)
	{
		this->_disconnectScheduledClients();
		this->_updatePollEvents();

		readyCount = poll(&this->_pollFds[0],
				static_cast<nfds_t>(this->_pollFds.size()), kPollTimeoutMs);
		if (readyCount == -1)
		{
			if (errno == EINTR)
				continue ;
			throw std::runtime_error("poll() failed");
		}
		if (readyCount == 0)
			continue ;

		for (size_t index = 0; index < this->_pollFds.size(); )
		{
			events = this->_pollFds[index].revents;
			if (events == 0)
			{
				++index;
				continue ;
			}
			if (this->_pollFds[index].fd == this->_listenSocketFd)
			{
				this->_acceptNewClient();
				++index;
				continue ;
			}

			stillConnected = true;
			if (events & (POLLERR | POLLNVAL))
			{
				this->_disconnectClient(index);
				stillConnected = false;
			}
			else
			{
				if (events & (POLLIN | POLLHUP))
					stillConnected = this->_handleClientRead(index);
				if (stillConnected && (events & POLLOUT))
					stillConnected = this->_handleClientWrite(index);
			}
			if (stillConnected)
				++index;
		}
	}
	std::cout << "server shutting down\n";
}

void	Server::_acceptNewClient(void)
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientAddrLen;
	int					clientFd;
	struct pollfd		clientPollFd;
	Client*				client;
	Handshake			handshake;

	clientAddrLen = sizeof(clientAddr);
	clientFd = accept(this->_listenSocketFd,
			reinterpret_cast<struct sockaddr*>(&clientAddr), &clientAddrLen);
	if (clientFd == -1)
		return ;

	client = NULL;
	try
	{
		this->_setNonBlocking(clientFd);
		client = new Client(clientFd);
		this->_clientsByFd[clientFd] = client;

		handshake.hostname = inet_ntoa(clientAddr.sin_addr);
		this->_handshakes[clientFd] = handshake;

		clientPollFd.fd = clientFd;
		clientPollFd.events = POLLIN;
		clientPollFd.revents = 0;
		this->_pollFds.push_back(clientPollFd);
	}
	catch (...)
	{
		this->_handshakes.erase(clientFd);
		if (client)
		{
			this->_clientsByFd.erase(clientFd);
			delete client;
		}
		else
			close(clientFd);
		std::cerr << "could not register new client\n";
		return ;
	}
	std::cout << "client connected (fd " << clientFd << ")" << "\n";
}

bool	Server::_handleClientRead(size_t pollIndex)
{
	char		chunk[kMaxLineLength];
	int			clientFd;
	ssize_t		bytesRead;

	clientFd = this->_pollFds[pollIndex].fd;
	std::string&	receiveBuffer = this->_receiveBuffers[clientFd];

	bytesRead = recv(clientFd, chunk, sizeof(chunk), 0);
	if (bytesRead <= 0)
	{
		this->_disconnectClient(pollIndex);
		return (false);
	}

	receiveBuffer.append(chunk, static_cast<size_t>(bytesRead));
	this->_processReceiveBuffer(clientFd);

	if (receiveBuffer.size() > kMaxLineLength)
	{
		this->_disconnectClient(pollIndex);
		return (false);
	}
	return (true);
}

bool	Server::_handleClientWrite(size_t pollIndex)
{
	int			clientFd;
	ssize_t		bytesSent;

	clientFd = this->_pollFds[pollIndex].fd;
	std::string&	sendBuffer = this->_sendBuffers[clientFd];

	if (sendBuffer.empty())
		return (true);

	bytesSent = send(clientFd, sendBuffer.data(), sendBuffer.size(), 0);
	if (bytesSent > 0)
		sendBuffer.erase(0, static_cast<size_t>(bytesSent));
	return (true);
}

void	Server::_disconnectClient(size_t pollIndex)
{
	int											clientFd;
	std::string									quitReason("Connection closed");
	std::map<int, Client*>::iterator			clientIt;
	std::map<int, Handshake>::const_iterator	handshakeIt;

	clientFd = this->_pollFds[pollIndex].fd;

	handshakeIt = this->_handshakes.find(clientFd);
	if (handshakeIt != this->_handshakes.end())
		quitReason = handshakeIt->second.quitReason;
	this->_leaveAllChannels(clientFd, quitReason);

	clientIt = this->_clientsByFd.find(clientFd);
	if (clientIt != this->_clientsByFd.end())
	{
		delete clientIt->second;
		this->_clientsByFd.erase(clientIt);
	}
	else
		close(clientFd);

	this->_receiveBuffers.erase(clientFd);
	this->_sendBuffers.erase(clientFd);
	this->_handshakes.erase(clientFd);
	this->_clientsToClose.erase(clientFd);
	this->_pollFds.erase(this->_pollFds.begin() + pollIndex);
	std::cout << "client disconnected (fd " << clientFd << ")" << "\n";
}

void	Server::_scheduleDisconnect(int clientFd, const std::string& reason)
{
	if (!this->_clientsToClose.insert(clientFd).second)
		return ;
	this->_handshakes[clientFd].quitReason = reason;
	this->_queueMessage(clientFd, "ERROR :Closing Link: "
		+ this->_handshakes[clientFd].hostname + " (" + reason + ")");
}

void	Server::_disconnectScheduledClients(void)
{
	std::set<int>::iterator						nextIt;
	std::set<int>::iterator						currentIt;
	std::map<int, std::string>::const_iterator	sendBufferIt;
	int											clientFd;

	nextIt = this->_clientsToClose.begin();
	while (nextIt != this->_clientsToClose.end())
	{
		currentIt = nextIt++;
		clientFd = *currentIt;
		sendBufferIt = this->_sendBuffers.find(clientFd);
		if (sendBufferIt != this->_sendBuffers.end()
			&& !sendBufferIt->second.empty())
			continue ;		/* still flushing */
		for (size_t i = 0; i < this->_pollFds.size(); ++i)
		{
			if (this->_pollFds[i].fd == clientFd)
			{
				this->_disconnectClient(i);
				break ;
			}
		}
		this->_clientsToClose.erase(clientFd);
	}
}

void	Server::_processReceiveBuffer(int clientFd)
{
	std::string&	receiveBuffer = this->_receiveBuffers[clientFd];
	std::string		line;
	size_t			newlinePos;

	while (!this->_clientsToClose.count(clientFd)
		&& (newlinePos = receiveBuffer.find('\n')) != std::string::npos)
	{
		line = receiveBuffer.substr(0, newlinePos);
		receiveBuffer.erase(0, newlinePos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (!line.empty())
			this->_handleLine(clientFd, line);
	}
}

void	Server::_handleLine(int clientFd, const std::string& line)
{
	IrcMessage											message;
	std::map<std::string, CommandEntry>::const_iterator	commandIt;
	bool												isRegistered;

	try
	{
		message = Parser::parse(line);
	}
	catch (const std::invalid_argument&)
	{
		if (toUpper(line.substr(0, 4)) == "PASS")
			std::cout << "[fd " << clientFd << "] malformed line ignored: PASS ****\n";
		else
			std::cout << "[fd " << clientFd << "] malformed line ignored: \""
				<< escapeForLog(line) << "\"\n";
		return ;
	}
	message.command = toUpper(message.command);

	if (message.command == "PASS")
		std::cout << "[fd " << clientFd << "] PASS ****\n";
	else
		std::cout << "[fd " << clientFd << "] " << line << "\n";

	isRegistered = this->_handshakes[clientFd].isRegistered;
	commandIt = this->_commandTable.find(message.command);
	if (commandIt == this->_commandTable.end())
	{
		if (isRegistered)
			this->_sendNumeric(clientFd, 421, message.command + " :Unknown command");
		else
			this->_sendNumeric(clientFd, 451, ":You have not registered");
		return ;
	}
	if (commandIt->second.requiresRegistration && !isRegistered)
	{
		this->_sendNumeric(clientFd, 451, ":You have not registered");
		return ;
	}
	(this->*(commandIt->second.handler))(clientFd, message);
}

void	Server::_queueMessage(int clientFd, const std::string& text)
{
	this->_sendBuffers[clientFd] += text + "\r\n";
}

void	Server::_sendNumeric(int clientFd, int code, const std::string& text)
{
	std::ostringstream	line;

	line << ":" << kServerName << " " << std::setw(3) << std::setfill('0')
		<< code << " " << this->_nicknameOf(clientFd) << " " << text;
	this->_queueMessage(clientFd, line.str());
}

void	Server::_broadcastToChannel(const Channel& channel, const std::string& text, int exceptFd)
{
	std::map<int, Client*>::const_iterator	memberIt;

	for (memberIt = channel.getMembers().begin();
		memberIt != channel.getMembers().end(); ++memberIt)
		if (memberIt->first != exceptFd)
			this->_queueMessage(memberIt->first, text);
}

std::string	Server::_nicknameOf(int clientFd) const
{
	std::map<int, Client*>::const_iterator	clientIt;

	clientIt = this->_clientsByFd.find(clientFd);
	if (clientIt == this->_clientsByFd.end()
		|| clientIt->second->getNickname().empty())
		return ("*");
	return (clientIt->second->getNickname());
}

std::string	Server::_hostmaskOf(int clientFd) const
{
	std::map<int, Client*>::const_iterator		clientIt;
	std::map<int, Handshake>::const_iterator	handshakeIt;

	clientIt = this->_clientsByFd.find(clientFd);
	handshakeIt = this->_handshakes.find(clientFd);
	if (clientIt == this->_clientsByFd.end()
		|| handshakeIt == this->_handshakes.end())
		return (kServerName);
	return (clientIt->second->getNickname() + "!"
		+ clientIt->second->getUsername() + "@" + handshakeIt->second.hostname);
}

void	Server::_registerCommands(void)
{
	this->_registerCommand("CAP", &Server::_cmdCap, false);
	this->_registerCommand("PASS", &Server::_cmdPass, false);
	this->_registerCommand("NICK", &Server::_cmdNick, false);
	this->_registerCommand("USER", &Server::_cmdUser, false);
	this->_registerCommand("PING", &Server::_cmdPing, false);
	this->_registerCommand("PONG", &Server::_cmdPong, false);
	this->_registerCommand("QUIT", &Server::_cmdQuit, false);
	this->_registerCommand("JOIN", &Server::_cmdJoin, true);
	this->_registerCommand("PRIVMSG", &Server::_cmdPrivmsg, true);
	this->_registerCommand("MODE", &Server::_cmdMode, true);
	this->_registerCommand("TOPIC", &Server::_cmdTopic, true);
	this->_registerCommand("KICK", &Server::_cmdKick, true);
	this->_registerCommand("INVITE", &Server::_cmdInvite, true);
}

void	Server::_registerCommand(const std::string& name, CommandHandler handler, bool requiresRegistration)
{
	CommandEntry	entry;

	entry.handler = handler;
	entry.requiresRegistration = requiresRegistration;
	this->_commandTable[name] = entry;
}

void	Server::_cmdCap(int clientFd, const IrcMessage& message)
{
	std::string	subCommand;

	if (message.params.empty())
		return ;
	subCommand = toUpper(message.params[0]);
	if (subCommand == "LS" || subCommand == "LIST")
		this->_queueMessage(clientFd, ":" + kServerName + " CAP "
			+ this->_nicknameOf(clientFd) + " " + subCommand + " :");
	else if (subCommand == "REQ")
		this->_queueMessage(clientFd, ":" + kServerName + " CAP "
			+ this->_nicknameOf(clientFd) + " NAK :"
			+ (message.params.size() > 1 ? message.params[1] : ""));
}

void	Server::_cmdPass(int clientFd, const IrcMessage& message)
{
	Handshake&	handshake = this->_handshakes[clientFd];

	if (handshake.isRegistered)
		this->_sendNumeric(clientFd, 462, ":You may not reregister");
	else if (message.params.empty())
		this->_sendNumeric(clientFd, 461, "PASS :Not enough parameters");
	else if (message.params[0] != this->_password)
	{
		this->_sendNumeric(clientFd, 464, ":Password incorrect");
		this->_scheduleDisconnect(clientFd, "Bad password");
	}
	else
	{
		handshake.passwordAccepted = true;
		this->_completeRegistrationIfReady(clientFd);
	}
}

void	Server::_cmdNick(int clientFd, const IrcMessage& message)
{
	Handshake&	handshake = this->_handshakes[clientFd];
	Client*		client = this->_clientsByFd[clientFd];
	std::string	oldHostmask;

	if (message.params.empty() || message.params[0].empty())
	{
		this->_sendNumeric(clientFd, 431, ":No nickname given");
		return ;
	}
	if (!isValidNickname(message.params[0]))
	{
		this->_sendNumeric(clientFd, 432, message.params[0] + " :Erroneous nickname");
		return ;
	}
	if (this->_isNicknameInUse(message.params[0], clientFd))
	{
		this->_sendNumeric(clientFd, 433,
			message.params[0] + " :Nickname is already in use");
		return ;
	}

	oldHostmask = this->_hostmaskOf(clientFd);
	client->setNickname(message.params[0]);
	handshake.nicknameSet = true;
	if (handshake.isRegistered)
		this->_announceNickChange(clientFd, oldHostmask, message.params[0]);
	else
		this->_completeRegistrationIfReady(clientFd);
}

void	Server::_cmdUser(int clientFd, const IrcMessage& message)
{
	Handshake&	handshake = this->_handshakes[clientFd];

	if (handshake.isRegistered)
		this->_sendNumeric(clientFd, 462, ":You may not reregister");
	else if (message.params.size() < 4)
		this->_sendNumeric(clientFd, 461, "USER :Not enough parameters");
	else
	{
		this->_clientsByFd[clientFd]->setUsername(message.params[0]);
		handshake.realName = message.params[3];
		handshake.usernameSet = true;
		this->_completeRegistrationIfReady(clientFd);
	}
}

void	Server::_cmdPing(int clientFd, const IrcMessage& message)
{
	if (message.params.empty())
		this->_sendNumeric(clientFd, 409, ":No origin specified");
	else
		this->_queueMessage(clientFd, ":" + kServerName + " PONG "
			+ kServerName + " :" + message.params[0]);
}

void	Server::_cmdPong(int clientFd, const IrcMessage& message)
{
	(void)clientFd;
	(void)message;
}

void	Server::_cmdQuit(int clientFd, const IrcMessage& message)
{
	this->_scheduleDisconnect(clientFd, "Quit: "
		+ (message.params.empty() ? std::string("Client quit") : message.params[0]));
}

void	Server::_loadMotd(const std::string& path)
{
	std::ifstream	file(path.c_str());
	std::string		line;

	while (std::getline(file, line))
	{
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (line.size() > 400)
			line.erase(400);
		this->_motdLines.push_back(line);
	}
}

void	Server::_sendMotd(int clientFd)
{
	if (this->_motdLines.empty())
	{
		this->_sendNumeric(clientFd, 422, ":MOTD File is missing");
		return ;
	}
	this->_sendNumeric(clientFd, 375, ":- " + kServerName + " Message of the day -");
	for (size_t i = 0; i < this->_motdLines.size(); ++i)
		this->_sendNumeric(clientFd, 372, ":- " + this->_motdLines[i]);
	this->_sendNumeric(clientFd, 376, ":End of /MOTD command");
}

void	Server::_completeRegistrationIfReady(int clientFd)
{
	Handshake&	handshake = this->_handshakes[clientFd];
	Client*		client = this->_clientsByFd[clientFd];

	if (handshake.isRegistered || !handshake.nicknameSet || !handshake.usernameSet)
		return ;
	if (!handshake.passwordAccepted)
	{
		this->_sendNumeric(clientFd, 464, ":Password incorrect");
		this->_scheduleDisconnect(clientFd, "Bad password");
		return ;
	}

	handshake.isRegistered = true;
	this->_sendNumeric(clientFd, 1, ":Welcome to the ft_irc Network, "
		+ client->getNickname() + "!" + client->getUsername()
		+ "@" + handshake.hostname);
	this->_sendNumeric(clientFd, 2, ":Your host is " + kServerName
		+ ", running version 1.0");
	this->_sendNumeric(clientFd, 3, ":This server was created today");
	this->_sendNumeric(clientFd, 4, kServerName + " 1.0 o itkol");
	this->_sendMotd(clientFd);
}

bool	Server::_isNicknameInUse(const std::string& nickname, int exceptFd) const
{
	std::map<int, Client*>::const_iterator	clientIt;
	const std::string						lowered = toLower(nickname);

	for (clientIt = this->_clientsByFd.begin();
		clientIt != this->_clientsByFd.end(); ++clientIt)
		if (clientIt->first != exceptFd
			&& toLower(clientIt->second->getNickname()) == lowered)
			return (true);
	return (false);
}

void	Server::_announceNickChange(int clientFd, const std::string& oldHostmask, const std::string& newNickname)
{
	std::set<int>									recipients;
	std::set<int>::const_iterator					recipientIt;
	std::map<std::string, Channel*>::const_iterator	channelIt;
	std::map<int, Client*>::const_iterator			memberIt;

	recipients.insert(clientFd);
	for (channelIt = this->_channelsByName.begin();
		channelIt != this->_channelsByName.end(); ++channelIt)
	{
		if (!channelIt->second->isMember(clientFd))
			continue ;
		for (memberIt = channelIt->second->getMembers().begin();
			memberIt != channelIt->second->getMembers().end(); ++memberIt)
			recipients.insert(memberIt->first);
	}
	for (recipientIt = recipients.begin(); recipientIt != recipients.end();
		++recipientIt)
		this->_queueMessage(*recipientIt,
			":" + oldHostmask + " NICK :" + newNickname);
}

void	Server::_cmdJoin(int clientFd, const IrcMessage& message)
{
	std::vector<std::string>	channelNames;
	std::vector<std::string>	keys;

	if (message.params.empty())
	{
		this->_sendNumeric(clientFd, 461, "JOIN :Not enough parameters");
		return ;
	}
	channelNames = splitOnCommas(message.params[0]);
	if (message.params.size() > 1)
		keys = splitOnCommas(message.params[1]);
	for (size_t i = 0; i < channelNames.size(); ++i)
		this->_joinChannel(clientFd, channelNames[i],
			i < keys.size() ? keys[i] : "");
}

void	Server::_cmdMode(int clientFd, const IrcMessage& message)
{
	Channel*					channel;
	std::vector<ChannelModeChange>	changes;
	ChannelModeChange				change;
	std::string						modeString;
	std::string						changedModes;
	std::string						changedParams;
	std::string						currentSign;
	std::string						modeParam;
	std::string						modeChar;
	size_t							paramIndex;
	size_t							limit;
	size_t							digit;
	bool							adding;
	bool							modeChanged;

	if (message.params.empty())
	{
		this->_sendNumeric(clientFd, 461, "MODE :Not enough parameters");
		return ;
	}
	channel = this->_findChannel(message.params[0]);
	if (!channel)
	{
		this->_sendNumeric(clientFd, 403,
			message.params[0] + " :No such channel");
		return ;
	}
	if (message.params.size() == 1)
	{
		this->_sendNumeric(clientFd, 324,
			channel->getName() + " " + channel->getModeString());
		return ;
	}
	if (!channel->isMember(clientFd))
	{
		this->_sendNumeric(clientFd, 442,
			channel->getName() + " :You're not on that channel");
		return ;
	}
	if (!channel->isOperator(clientFd))
	{
		this->_sendNumeric(clientFd, 482,
			channel->getName() + " :You're not channel operator");
		return ;
	}

	modeString = message.params[1];
	paramIndex = 2;
	adding = true;
	for (size_t i = 0; i < modeString.size(); ++i)
	{
		if (modeString[i] == '+')
		{
			adding = true;
			continue ;
		}
		if (modeString[i] == '-')
		{
			adding = false;
			continue ;
		}
		modeChar.assign(1, modeString[i]);
		if (modeString[i] != 'i' && modeString[i] != 't'
			&& modeString[i] != 'k' && modeString[i] != 'o'
			&& modeString[i] != 'l')
		{
			this->_sendNumeric(clientFd, 472,
				modeChar + " :is unknown mode char to me");
			return ;
		}

		change = ChannelModeChange();
		change.mode = modeString[i];
		change.adding = adding;
		modeParam.clear();
		if ((modeString[i] == 'k' || modeString[i] == 'o'
				|| (modeString[i] == 'l' && adding))
			&& paramIndex < message.params.size())
		{
			modeParam = message.params[paramIndex];
			change.parameter = modeParam;
			++paramIndex;
		}
		else if (modeString[i] == 'k' || modeString[i] == 'o'
			|| (modeString[i] == 'l' && adding))
		{
			this->_sendNumeric(clientFd, 461, "MODE :Not enough parameters");
			return ;
		}
		if (modeString[i] == 'k')
		{
			if (modeParam.empty())
			{
				this->_sendNumeric(clientFd, 461,
					"MODE :Not enough parameters");
				return ;
			}
		}
		else if (modeString[i] == 'o')
		{
			if (modeParam.empty())
			{
				this->_sendNumeric(clientFd, 461,
					"MODE :Not enough parameters");
				return ;
			}
			change.targetFd = this->_findClientByNickname(modeParam);
			if (change.targetFd == -1)
			{
				this->_sendNumeric(clientFd, 401,
					modeParam + " :No such nick");
				return ;
			}
			if (!channel->isMember(change.targetFd))
			{
				this->_sendNumeric(clientFd, 441, modeParam + " "
					+ channel->getName() + " :They aren't on that channel");
				return ;
			}
		}
		else if (modeString[i] == 'l' && adding)
		{
			if (modeParam.empty())
			{
				this->_sendNumeric(clientFd, 461,
					"MODE :Invalid channel limit");
				return ;
			}
			limit = 0;
			for (size_t j = 0; j < modeParam.size(); ++j)
			{
				if (!std::isdigit(static_cast<unsigned char>(modeParam[j])))
				{
					this->_sendNumeric(clientFd, 461,
						"MODE :Invalid channel limit");
					return ;
				}
				digit = static_cast<size_t>(modeParam[j] - '0');
				if (limit > (std::numeric_limits<size_t>::max() - digit) / 10)
				{
					this->_sendNumeric(clientFd, 461,
						"MODE :Invalid channel limit");
					return ;
				}
				limit = limit * 10 + digit;
			}
			if (limit == 0)
			{
				this->_sendNumeric(clientFd, 461,
					"MODE :Invalid channel limit");
				return ;
			}
			change.limit = limit;
		}
		changes.push_back(change);
	}

	currentSign.clear();
	changedModes.clear();
	changedParams.clear();
	for (size_t i = 0; i < changes.size(); ++i)
	{
		change = changes[i];
		modeChanged = false;
		if (change.mode == 'i')
		{
			modeChanged = (channel->isInviteOnly() != change.adding);
			if (modeChanged)
				channel->setInviteOnly(change.adding);
		}
		else if (change.mode == 't')
		{
			modeChanged = (channel->isTopicRestricted() != change.adding);
			if (modeChanged)
				channel->setTopicRestricted(change.adding);
		}
		else if (change.mode == 'k')
		{
			if (change.adding)
			{
				modeChanged = (!channel->hasKey()
					|| channel->getKey() != change.parameter);
				if (modeChanged)
					channel->setKey(change.parameter);
			}
			else
			{
				modeChanged = channel->hasKey();
				if (modeChanged)
					channel->setKey("");
			}
		}
		else if (change.mode == 'o')
		{
			modeChanged = (channel->isOperator(change.targetFd) != change.adding);
			if (modeChanged)
			{
				if (change.adding)
					channel->addOperator(change.targetFd);
				else
					channel->removeOperator(change.targetFd);
			}
		}
		else if (change.mode == 'l')
		{
			if (change.adding)
			{
				modeChanged = (channel->getLimit() != change.limit);
				if (modeChanged)
					channel->setLimit(change.limit);
			}
			else
			{
				modeChanged = (channel->getLimit() != 0);
				if (modeChanged)
					channel->setLimit(0);
			}
		}
		if (modeChanged)
		{
			if (currentSign != (change.adding ? "+" : "-"))
			{
				currentSign = change.adding ? "+" : "-";
				changedModes += currentSign;
			}
			changedModes += change.mode;
			if (change.mode == 'k' || change.mode == 'o'
				|| (change.mode == 'l' && change.adding))
				changedParams += " " + change.parameter;
		}
	}
	if (!changedModes.empty())
		this->_broadcastToChannel(*channel, ":" + this->_hostmaskOf(clientFd)
			+ " MODE " + channel->getName() + " " + changedModes
			+ changedParams);
}

void	Server::_cmdTopic(int clientFd, const IrcMessage& message)
{
	Channel*	channel;

	if (message.params.empty())
	{
		this->_sendNumeric(clientFd, 461, "TOPIC :Not enough parameters");
		return ;
	}
	channel = this->_findChannel(message.params[0]);
	if (!channel)
	{
		this->_sendNumeric(clientFd, 403,
			message.params[0] + " :No such channel");
		return ;
	}
	if (message.params.size() == 1)
	{
		if (channel->getTopic().empty())
			this->_sendNumeric(clientFd, 331,
				channel->getName() + " :No topic is set");
		else
			this->_sendNumeric(clientFd, 332,
				channel->getName() + " :" + channel->getTopic());
		return ;
	}
	if (!channel->isMember(clientFd))
	{
		this->_sendNumeric(clientFd, 442,
			channel->getName() + " :You're not on that channel");
		return ;
	}
	if (channel->isTopicRestricted() && !channel->isOperator(clientFd))
	{
		this->_sendNumeric(clientFd, 482,
			channel->getName() + " :You're not channel operator");
		return ;
	}

	channel->setTopic(message.params[1]);
	this->_broadcastToChannel(*channel, ":" + this->_hostmaskOf(clientFd)
		+ " TOPIC " + channel->getName() + " :" + channel->getTopic());
}

void	Server::_cmdKick(int clientFd, const IrcMessage& message)
{
	Channel*	channel;
	int			targetFd;
	std::string	kickMessage;

	if (message.params.size() < 2)
	{
		this->_sendNumeric(clientFd, 461, "KICK :Not enough parameters");
		return ;
	}
	channel = this->_findChannel(message.params[0]);
	if (!channel)
	{
		this->_sendNumeric(clientFd, 403,
			message.params[0] + " :No such channel");
		return ;
	}
	if (!channel->isMember(clientFd))
	{
		this->_sendNumeric(clientFd, 442,
			channel->getName() + " :You're not on that channel");
		return ;
	}
	if (!channel->isOperator(clientFd))
	{
		this->_sendNumeric(clientFd, 482,
			channel->getName() + " :You're not channel operator");
		return ;
	}
	targetFd = this->_findClientByNickname(message.params[1]);
	if (targetFd == -1 || !channel->isMember(targetFd))
	{
		this->_sendNumeric(clientFd, 441, message.params[1] + " "
			+ channel->getName() + " :They aren't on that channel");
		return ;
	}

	kickMessage = ":" + this->_hostmaskOf(clientFd) + " KICK "
		+ channel->getName() + " " + this->_nicknameOf(targetFd);
	if (message.params.size() > 2)
		kickMessage += " :" + message.params[2];
	this->_broadcastToChannel(*channel, kickMessage);
	channel->removeMember(targetFd);
	if (channel->isEmpty())
		this->_removeChannel(channel->getName());
}

void	Server::_cmdInvite(int clientFd, const IrcMessage& message)
{
	Channel*	channel;
	int			targetFd;

	if (message.params.size() < 2)
	{
		this->_sendNumeric(clientFd, 461, "INVITE :Not enough parameters");
		return ;
	}
	targetFd = this->_findClientByNickname(message.params[0]);
	if (targetFd == -1)
	{
		this->_sendNumeric(clientFd, 401,
			message.params[0] + " :No such nick");
		return ;
	}
	channel = this->_findChannel(message.params[1]);
	if (channel && !channel->isMember(clientFd))
	{
		this->_sendNumeric(clientFd, 442,
			channel->getName() + " :You're not on that channel");
		return ;
	}
	if (channel && channel->isMember(targetFd))
	{
		this->_sendNumeric(clientFd, 443, this->_nicknameOf(targetFd) + " "
			+ channel->getName() + " :is already on channel");
		return ;
	}
	if (channel && channel->isInviteOnly() && !channel->isOperator(clientFd))
	{
		this->_sendNumeric(clientFd, 482,
			channel->getName() + " :You're not channel operator");
		return ;
	}

	if (channel)
		channel->invite(targetFd);
	this->_sendNumeric(clientFd, 341, this->_nicknameOf(targetFd) + " "
		+ message.params[1]);
	this->_queueMessage(targetFd, ":" + this->_hostmaskOf(clientFd)
		+ " INVITE " + this->_nicknameOf(targetFd) + " :"
		+ message.params[1]);
}

void	Server::_joinChannel(int clientFd, const std::string& channelName,
			const std::string& key)
{
	Channel*	channel;
	bool		channelWasCreated;
	int			joinError;

	if (!Channel::isValidName(channelName))
	{
		this->_sendNumeric(clientFd, 403, channelName + " :No such channel");
		return ;
	}

	channelWasCreated = false;
	channel = this->_findChannel(channelName);
	if (channel)
	{
		if (channel->isMember(clientFd))
			return ;
		joinError = channel->canJoin(clientFd, key);
		if (joinError == 473)
			this->_sendNumeric(clientFd, 473, channelName + " :Cannot join channel (+i)");
		else if (joinError == 475)
			this->_sendNumeric(clientFd, 475, channelName + " :Cannot join channel (+k)");
		else if (joinError == 471)
			this->_sendNumeric(clientFd, 471, channelName + " :Cannot join channel (+l)");
		if (joinError)
			return ;
	}
	else
	{
		channel = this->_createChannel(channelName);
		channelWasCreated = true;
	}

	if (!channel->addMember(this->_clientsByFd[clientFd]))
	{
		if (channelWasCreated)
			this->_removeChannel(channelName);
		return ;
	}

	this->_broadcastToChannel(*channel, ":" + this->_hostmaskOf(clientFd)
		+ " JOIN :" + channel->getName());
	if (channel->getTopic().empty())
		this->_sendNumeric(clientFd, 331, channel->getName() + " :No topic is set");
	else
		this->_sendNumeric(clientFd, 332,
			channel->getName() + " :" + channel->getTopic());
	this->_sendNumeric(clientFd, 353,
		"= " + channel->getName() + " :" + channel->getNamesList());
	this->_sendNumeric(clientFd, 366, channel->getName() + " :End of /NAMES list.");
}

Channel*	Server::_findChannel(const std::string& channelName) const
{
	std::map<std::string, Channel*>::const_iterator	channelIt;

	channelIt = this->_channelsByName.find(toLower(channelName));
	if (channelIt == this->_channelsByName.end())
		return (NULL);
	return (channelIt->second);
}

Channel*	Server::_createChannel(const std::string& channelName)
{
	Channel*	channel;

	channel = new Channel(channelName);
	try
	{
		this->_channelsByName[toLower(channelName)] = channel;
	}
	catch (...)
	{
		delete channel;
		throw;
	}
	return (channel);
}

void	Server::_removeChannel(const std::string& channelName)
{
	std::map<std::string, Channel*>::iterator	channelIt;

	channelIt = this->_channelsByName.find(toLower(channelName));
	if (channelIt == this->_channelsByName.end())
		return ;
	delete channelIt->second;
	this->_channelsByName.erase(channelIt);
}

void	Server::_leaveAllChannels(int clientFd, const std::string& quitReason)
{
	std::set<int>									peers;
	std::set<int>::const_iterator					peerIt;
	std::vector<std::string>						emptyChannelNames;
	std::map<std::string, Channel*>::iterator		channelIt;
	std::map<int, Client*>::const_iterator			memberIt;
	const std::string								hostmask = this->_hostmaskOf(clientFd);

	for (channelIt = this->_channelsByName.begin();
		channelIt != this->_channelsByName.end(); ++channelIt)
	{
		if (!channelIt->second->isMember(clientFd))
			continue ;
		for (memberIt = channelIt->second->getMembers().begin();
			memberIt != channelIt->second->getMembers().end(); ++memberIt)
			if (memberIt->first != clientFd)
				peers.insert(memberIt->first);
		channelIt->second->removeMember(clientFd);
		if (channelIt->second->isEmpty())
			emptyChannelNames.push_back(channelIt->first);
	}
	for (peerIt = peers.begin(); peerIt != peers.end(); ++peerIt)
		this->_queueMessage(*peerIt, ":" + hostmask + " QUIT :" + quitReason);
	for (size_t i = 0; i < emptyChannelNames.size(); ++i)
		this->_removeChannel(emptyChannelNames[i]);
}

int	Server::_findClientByNickname(const std::string& nickname) const
{
	std::map<int, Client*>::const_iterator		clientIt;
	std::map<int, Handshake>::const_iterator	handshakeIt;
	const std::string							lowered = toLower(nickname);

	for (clientIt = this->_clientsByFd.begin();
		clientIt != this->_clientsByFd.end(); ++clientIt)
	{
		handshakeIt = this->_handshakes.find(clientIt->first);
		if (handshakeIt != this->_handshakes.end()
			&& handshakeIt->second.isRegistered
			&& toLower(clientIt->second->getNickname()) == lowered)
			return (clientIt->first);
	}
	return (-1);
}

void	Server::_cmdPrivmsg(int clientFd, const IrcMessage& message)
{
	std::vector<std::string>	targets;
	Channel*					channel;
	int							targetFd;

	if (!message.params.empty())
		targets = splitOnCommas(message.params[0]);
	if (targets.empty())
	{
		this->_sendNumeric(clientFd, 411, ":No recipient given (PRIVMSG)");
		return ;
	}
	if (message.params.size() < 2 || message.params[1].empty())
	{
		this->_sendNumeric(clientFd, 412, ":No text to send");
		return ;
	}

	for (size_t i = 0; i < targets.size(); ++i)
	{
		if (targets[i][0] == '#' || targets[i][0] == '&')
		{
			channel = this->_findChannel(targets[i]);
			if (!channel)
				this->_sendNumeric(clientFd, 403, targets[i] + " :No such channel");
			else if (!channel->isMember(clientFd))		/* no external messages */
				this->_sendNumeric(clientFd, 404,
					channel->getName() + " :Cannot send to channel");
			else
				this->_broadcastToChannel(*channel,
					":" + this->_hostmaskOf(clientFd) + " PRIVMSG "
					+ channel->getName() + " :" + message.params[1], clientFd);
		}
		else
		{
			targetFd = this->_findClientByNickname(targets[i]);
			if (targetFd == -1)
				this->_sendNumeric(clientFd, 401, targets[i] + " :No such nick/channel");
			else
				this->_queueMessage(targetFd, ":" + this->_hostmaskOf(clientFd)
					+ " PRIVMSG " + this->_nicknameOf(targetFd)
					+ " :" + message.params[1]);
		}
	}
}

void	Server::_cleanup(void)
{
	std::map<int, Client*>::iterator			clientIt;
	std::map<std::string, Channel*>::iterator	channelIt;

	for (channelIt = this->_channelsByName.begin();
		channelIt != this->_channelsByName.end(); ++channelIt)
		delete channelIt->second;
	this->_channelsByName.clear();

	/* clients close their own fd in ~Client() */
	for (clientIt = this->_clientsByFd.begin();
		clientIt != this->_clientsByFd.end(); ++clientIt)
		delete clientIt->second;
	this->_clientsByFd.clear();
	this->_receiveBuffers.clear();
	this->_sendBuffers.clear();
	this->_handshakes.clear();
	this->_clientsToClose.clear();

	if (this->_listenSocketFd >= 0)
		close(this->_listenSocketFd);
	this->_listenSocketFd = -1;
	this->_pollFds.clear();
}

int	Server::getPort(void) const
{
	return (this->_port);
}

const std::string&	Server::getPassword(void) const
{
	return (this->_password);
}
