#include <unistd.h>
#include <iostream>
#include "Client.hpp"

Client::Client(int fd)
{
	_fd = fd;
}

Client::~Client()
{
	if (_fd >= 0)
		close(_fd);
	_fd = -1;
}

int	Client::getFD( void ) const
{
	return (_fd);
}

void	Client::setUsername( const std::string& username )
{
	_username = username;
}

void	Client::setNickname( const std::string& nickname )
{
	_nickname = nickname;
}

const std::string&	Client::getUsername( void ) const
{
	return (_username);
}

const std::string&	Client::getNickname( void ) const
{
	return (_nickname);
}
