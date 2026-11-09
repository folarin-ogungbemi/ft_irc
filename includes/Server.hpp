#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
# include <set>
# include <poll.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <arpa/inet.h>
# include <fcntl.h>
# include <unistd.h>
# include <csignal>
# include <cerrno>
# include <cstring>
# include <stdexcept>
# include <iostream>
# include <sstream>
# include "Client.hpp"
# include "Channel.hpp"
# include "Parser.hpp"

class Server
{
private:
	struct Handshake
	{
		bool		passwordAccepted;
		bool		nicknameSet;
		bool		usernameSet;
		bool		isRegistered;
		std::string	hostname;
		std::string	realName;
		std::string	quitReason;

		Handshake(void)
		: passwordAccepted(false)
		, nicknameSet(false)
		, usernameSet(false)
		, isRegistered(false)
		, quitReason("Connection closed")
		{}
	};

	typedef void (Server::*CommandHandler)(int clientFd, const IrcMessage& message);
	struct CommandEntry
	{
		CommandHandler	handler;
		bool			requiresRegistration;
	};

	int									_listenSocketFd;
	int									_port;
	std::string							_password;
	std::vector<struct pollfd>			_pollFds;
	std::map<int, Client*>				_clientsByFd;
	std::map<int, std::string>			_receiveBuffers;
	std::map<int, std::string>			_sendBuffers;
	std::map<int, Handshake>			_handshakes;
	std::set<int>						_clientsToClose;
	std::map<std::string, Channel*>		_channelsByName;
	std::map<std::string, CommandEntry>	_commandTable;
	std::vector<std::string>			_motdLines;

	static volatile sig_atomic_t		_stopRequested;
	static void							_signalHandler(int signalNumber);

	Server(void);
	Server(const Server& other);
	Server&	operator=(const Server& other);

	void		_setupListenSocket(void);
	void		_setNonBlocking(int socketFd);
	void		_updatePollEvents(void);
	void		_acceptNewClient(void);
	bool		_handleClientRead(size_t pollIndex);
	bool		_handleClientWrite(size_t pollIndex);
	void		_disconnectClient(size_t pollIndex);
	void		_cleanup(void);
	void		_loadMotd(const std::string& path);
	void		_sendMotd(int clientFd);
	void		_processReceiveBuffer(int clientFd);
	void		_handleLine(int clientFd, const std::string& line);
	void		_queueMessage(int clientFd, const std::string& text);
	void		_sendNumeric(int clientFd, int code, const std::string& text);
	void		_broadcastToChannel(const Channel& channel, const std::string& text, int exceptFd = -1);
	std::string	_nicknameOf(int clientFd) const;
	std::string	_hostmaskOf(int clientFd) const;
	void		_scheduleDisconnect(int clientFd, const std::string& reason);
	void		_disconnectScheduledClients(void);

	void		_registerCommands(void);
	void		_registerCommand(const std::string& name, CommandHandler handler, bool requiresRegistration);
	void		_cmdCap(int clientFd, const IrcMessage& message);
	void		_cmdPass(int clientFd, const IrcMessage& message);
	void		_cmdNick(int clientFd, const IrcMessage& message);
	void		_cmdUser(int clientFd, const IrcMessage& message);
	void		_cmdPing(int clientFd, const IrcMessage& message);
	void		_cmdPong(int clientFd, const IrcMessage& message);
	void		_cmdQuit(int clientFd, const IrcMessage& message);
	void		_completeRegistrationIfReady(int clientFd);
	bool		_isNicknameInUse(const std::string& nickname, int exceptFd) const;
	void		_announceNickChange(int clientFd, const std::string& oldHostmask, const std::string& newNickname);
	void		_cmdJoin(int clientFd, const IrcMessage& message);
	void		_joinChannel(int clientFd, const std::string& channelName, const std::string& key);
	Channel*	_findChannel(const std::string& channelName) const;
	Channel*	_createChannel(const std::string& channelName);
	void		_removeChannel(const std::string& channelName);
	void		_leaveAllChannels(int clientFd, const std::string& quitReason);
	void		_cmdPrivmsg(int clientFd, const IrcMessage& message);
	int			_findClientByNickname(const std::string& nickname) const;	/* fd or -1 */
//	ADD new commands here
	void		_cmdTopic(int clientFd, const IrcMessage& message);
	void		_cmdKick(int clientFd, const IrcMessage& message);
	void		_cmdInvite(int clientFd, const IrcMessage& message);
public:
	Server(int port, const std::string& password);
	~Server();

	static void			setupSignals(void);
	void				run(void);
	int					getPort(void) const;
	const std::string&	getPassword(void) const;
};

#endif
