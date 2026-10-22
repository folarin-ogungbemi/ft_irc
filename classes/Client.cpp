#include <unistd.h>
#include <iostream>
#include "Client.hpp"

Client::Client(){}
Client::Client(int fd)
{
	_fd = fd;
}

Client::~Client()
{
	if (_fd >= 0)
		close(_fd);
}

Client::Client( const Client& rhs )
{
	*this = rhs;
}

Client&	Client::operator=( const Client& rhs )
{
	if (this != &rhs)
		this->_fd = rhs._fd;
	return (*this);
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
