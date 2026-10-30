#include <unistd.h>
#include <iostream>
#include "Client.hpp"

Client::Client(int fd) : _fd(fd), _passwordAccepted(false), _registered(false)
{}

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

bool	Client::setUsername( const std::string& username )
{
	if (username.empty())
		return (false);
	for (size_t i = 0; i < username.size(); ++i)
	{
		unsigned char c = username[i];
		if (c == ' ' || c == '\0' || c == '\r' || c == '\n')
			return (false);
	}
	_username = username;
	return (true);
}

bool	Client::setNickname( const std::string& nickname )
{
	if (nickname.empty() || nickname.size() > 9)
		return (false);
	for (size_t i = 0; i < nickname.size(); ++i)
	{
		unsigned char c = nickname[i];
		bool letter = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
		bool number = c >= '0' && c <= '9';
		bool special = c == '-' || c == '[' || c == ']' || c == '\\'
				|| c == '`' || c == '^' || c == '{' || c == '}';	
		if (i == 0)
		{
			if (!letter)
				return (false);
		}
		else if (!letter && !number && !special)
			return (false);
	}
	_nickname = nickname;
	return (true);
}

void	Client::setPasswordAccepted( bool accepted )
{
	_passwordAccepted = accepted;
}

void	Client::setRegistered( bool registered )
{
	_registered = registered;
}

const std::string&	Client::getUsername( void ) const
{
	return (_username);
}

const std::string&	Client::getNickname( void ) const
{
	return (_nickname);
}

bool	Client::isPasswordAccepted( void ) const
{
	return (_passwordAccepted);
}

bool	Client::isRegistered( void ) const
{
	return (_registered);
}
