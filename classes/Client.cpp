#include "Client.hpp"

Client::Client(int fd)
{
	_fd = fd;
}

Client::~Client(){}

int	Client::getFD( void ) const
{
	return (_fd);
}
