#ifndef SERVER_HPP
# define SERVER_HPP

# include <string>
# include <vector>
# include <map>
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

class Server
{
private:
	int							_listenFd;
	int							_port;
	std::string					_password;
	std::vector<struct pollfd>	_pfds;
	std::map<int, Client*>		_clients;
	std::map<int, std::string>	_recvBuf;
	std::map<int, std::string>	_sendBuf;

	static volatile sig_atomic_t	_stop;
	static void						_signalHandler(int sig);

	Server(const Server& rhs);
	Server&	operator=(const Server& rhs);

	void	_setupSocket(void);
	void	_setNonBlocking(int fd);
	void	_updatePollEvents(void);
	void	_acceptNewClient(void);
	bool	_handleClientRead(size_t index);
	bool	_handleClientWrite(size_t index);
	void	_disconnectClient(size_t index);
	void	_closeAll(void);
	void	_processBuffer(int fd);
	void	_processLine(int fd, const std::string& line);
	void	_relay(int senderFd, const std::string& msg);
	void	_queueMessage(int fd, const std::string& msg);

public:
	Server(void);
	Server(int port, const std::string& password);
	~Server();

	static void			setupSignals(void);
	void				run(void);
	int					getPort(void) const;
	const std::string&	getPassword(void) const;
};

#endif
