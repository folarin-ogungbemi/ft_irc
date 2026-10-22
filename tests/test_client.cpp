#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include "includes/Client.hpp"

int	main()
{
	int	sockets[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
	{
		std::cerr << "socketpair failed" << std::endl;
		return (EXIT_FAILURE);
	}
	int	client_fd = sockets[0];
	Client	client(client_fd);
	assert(client.getFD() == client_fd);
	std::cout << "Constructor and getter: PASS" << std::endl;
	errno = 0;
	assert(fcntl(client_fd, F_GETFD) == -1 && errno == EBADF);
	std::cout << "Destructor closes socket: PASS" << std::endl;
	close(sockets[1]);
	std::cout << "All Client tests passed!" << std::endl;
	return (EXIT_SUCCESS);
}

