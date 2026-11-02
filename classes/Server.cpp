#include "Server.hpp"

namespace
{
	const size_t	kMaxLine = 512;
	const int		kPollTimeoutMs = 1000;
}

volatile sig_atomic_t	Server::_stop = 0;

Server::Server(void)
: _listenFd(-1)
, _port(6667)
, _password("")
{
}

Server::Server(int port, const std::string& password)
: _listenFd(-1)
, _port(port)
, _password(password)
{
	try
	{
		this->_setupSocket();
	}
	catch (...)
	{
		this->_closeAll();
		throw;
	}
}

Server::~Server()
{
	this->_closeAll();
}

void	Server::_signalHandler(int sig)
{
	(void)sig;
	_stop = 1;
}

void	Server::setupSignals(void)
{
	struct sigaction	sa;

	std::memset(&sa, 0, sizeof(sa));
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_handler = &Server::_signalHandler;
	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGQUIT, &sa, NULL);

	sa.sa_handler = SIG_IGN;
	sigaction(SIGPIPE, &sa, NULL);
}

void	Server::_setupSocket(void)
{
	struct sockaddr_in	addr;
	struct pollfd		pfd;
	int					opt;

	if (this->_port < 1 || this->_port > 65535)
		throw std::runtime_error("invalid port");

	this->_listenFd = socket(AF_INET, SOCK_STREAM, 0);
	if (this->_listenFd == -1)
		throw std::runtime_error("socket() failed");

	opt = 1;
	if (setsockopt(this->_listenFd, SOL_SOCKET, SO_REUSEADDR,
			&opt, sizeof(opt)) == -1)
		throw std::runtime_error("setsockopt() failed");

	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(static_cast<uint16_t>(this->_port));

	if (bind(this->_listenFd, reinterpret_cast<struct sockaddr*>(&addr),
			sizeof(addr)) == -1)
		throw std::runtime_error("bind() failed");

	if (listen(this->_listenFd, SOMAXCONN) == -1)
		throw std::runtime_error("listen() failed");

	this->_setNonBlocking(this->_listenFd);

	pfd.fd = this->_listenFd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	this->_pfds.push_back(pfd);

	std::cout << "server listening on port " << this->_port << "\n";
}

void	Server::_setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		throw std::runtime_error("fcntl() failed");
}

void	Server::_updatePollEvents(void)
{
	std::map<int, std::string>::const_iterator	it;

	for (size_t i = 0; i < this->_pfds.size(); ++i)
	{
		if (this->_pfds[i].fd == this->_listenFd)
			continue ;
		this->_pfds[i].events = POLLIN;
		it = this->_sendBuf.find(this->_pfds[i].fd);
		if (it != this->_sendBuf.end() && !it->second.empty())
			this->_pfds[i].events |= POLLOUT;
	}
}

void	Server::run(void)
{
	int		ret;
	short	rev;
	bool	alive;

	if (this->_pfds.empty())
		throw std::runtime_error("server is not initialised");

	while (!_stop)
	{
		this->_updatePollEvents();

		ret = poll(&this->_pfds[0],
				static_cast<nfds_t>(this->_pfds.size()), kPollTimeoutMs);
		if (ret == -1)
		{
			if (errno == EINTR)
				continue ;
			throw std::runtime_error("poll() failed");
		}
		if (ret == 0)
			continue ;

		for (size_t i = 0; i < this->_pfds.size(); )
		{
			rev = this->_pfds[i].revents;
			if (rev == 0)
			{
				++i;
				continue ;
			}
			if (this->_pfds[i].fd == this->_listenFd)
			{
				this->_acceptNewClient();
				++i;
				continue ;
			}

			alive = true;
			if (rev & (POLLERR | POLLNVAL))
			{
				this->_disconnectClient(i);
				alive = false;
			}
			else
			{
				if (rev & (POLLIN | POLLHUP))
					alive = this->_handleClientRead(i);
				if (alive && (rev & POLLOUT))
					alive = this->_handleClientWrite(i);
			}
			if (alive)
				++i;
		}
	}
	std::cout << "server shutting down\n";
}

void	Server::_acceptNewClient(void)
{
	struct sockaddr_in	addr;
	socklen_t			len;
	int					fd;
	struct pollfd		pfd;
	Client*				client;

	len = sizeof(addr);
	fd = accept(this->_listenFd,
			reinterpret_cast<struct sockaddr*>(&addr), &len);
	if (fd == -1)
		return ;

	client = NULL;
	try
	{
		this->_setNonBlocking(fd);
		client = new Client(fd);
		this->_clients[fd] = client;

		pfd.fd = fd;
		pfd.events = POLLIN;
		pfd.revents = 0;
		this->_pfds.push_back(pfd);
	}
	catch (...)
	{
		if (client)
		{
			this->_clients.erase(fd);
			delete client;
		}
		else
			close(fd);
		std::cerr << "could not register new client\n";
		return ;
	}
	std::cout << "client connected (fd " << fd << ")" << "\n";
}

bool	Server::_handleClientRead(size_t index)
{
	char			buf[kMaxLine];
	int				fd;
	ssize_t			n;
	std::string&	in = this->_recvBuf[this->_pfds[index].fd];

	fd = this->_pfds[index].fd;

	n = recv(fd, buf, sizeof(buf), 0);
	if (n <= 0)
	{
		this->_disconnectClient(index);
		return (false);
	}

	in.append(buf, static_cast<size_t>(n));
	this->_processBuffer(fd);

	if (in.size() > kMaxLine)
	{
		this->_disconnectClient(index);
		return (false);
	}
	return (true);
}

bool	Server::_handleClientWrite(size_t index)
{
	int				fd;
	ssize_t			n;
	std::string&	out = this->_sendBuf[this->_pfds[index].fd];

	fd = this->_pfds[index].fd;
	if (out.empty())
		return (true);

	n = send(fd, out.data(), out.size(), 0);
	if (n > 0)
		out.erase(0, static_cast<size_t>(n));
	return (true);
}

void	Server::_disconnectClient(size_t index)
{
	int									fd;
	std::map<int, Client*>::iterator	it;

	fd = this->_pfds[index].fd;

	it = this->_clients.find(fd);
	if (it != this->_clients.end())
	{
		delete it->second;
		this->_clients.erase(it);
	}
	else
		close(fd);

	this->_recvBuf.erase(fd);
	this->_sendBuf.erase(fd);
	this->_pfds.erase(this->_pfds.begin() + index);
	std::cout << "client disconnected (fd " << fd << ")" << "\n";
}

void	Server::_processBuffer(int fd)
{
	std::string&	in = this->_recvBuf[fd];
	std::string		line;
	size_t			pos;

	while ((pos = in.find('\n')) != std::string::npos)
	{
		line = in.substr(0, pos);
		in.erase(0, pos + 1);
		if (!line.empty() && line[line.size() - 1] == '\r')
			line.erase(line.size() - 1);
		if (!line.empty())
			this->_processLine(fd, line);
	}
}

void	Server::_processLine(int fd, const std::string& line)
{
	std::map<int, Client*>::const_iterator	it;
	std::ostringstream						name;

	it = this->_clients.find(fd);
	if (it != this->_clients.end() && !it->second->getNickname().empty())
		name << it->second->getNickname();
	else
		name << "fd " << fd;

	std::cout << "[" << name.str() << "] " << line << "\n";
	/* 
		TODO: replace with the command parser (PASS/NICK/USER/JOIN...) 
	*/
	this->_relay(fd, "[" + name.str() + "] " + line);
}

void	Server::_relay(int senderFd, const std::string& msg)
{
	std::map<int, Client*>::const_iterator	it;

	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
		if (it->first != senderFd)
			this->_queueMessage(it->first, msg);
}

void	Server::_queueMessage(int fd, const std::string& msg)
{
	this->_sendBuf[fd] += msg + "\r\n";
}

void	Server::_closeAll(void)
{
	std::map<int, Client*>::iterator	it;

	for (it = this->_clients.begin(); it != this->_clients.end(); ++it)
		delete it->second;
	this->_clients.clear();
	this->_recvBuf.clear();
	this->_sendBuf.clear();

	if (this->_listenFd >= 0)
		close(this->_listenFd);
	this->_listenFd = -1;
	this->_pfds.clear();
}

int	Server::getPort( void ) const
{
	return (this->_port);
}

const std::string&	Server::getPassword( void ) const
{
	return (this->_password);
}
