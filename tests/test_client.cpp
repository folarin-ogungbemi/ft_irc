#include <iostream>
#include <cassert>
#include <cstdlib>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include "includes/Client.hpp"
#include "includes/Utils.hpp"

#define REQUIRE(cond) \
    do { if (!(cond)) { error(TEST, "REQUIRE failed: " #cond); return EXIT_FAILURE; } } while(0)   

int	main()
{
	int	sockets[2];

	if (socketpair(AF_UNIX, SOCK_STREAM, 1, sockets) == -1)
	{
		error(SYSTEM, "socketpair() failed");
		return (EXIT_FAILURE);
	}
	int	client_fd = sockets[0];
	{
		// CLIENT CONSTRUCTOR AND FD OPEN TEST
		Client	client(client_fd);
		REQUIRE(client.getFD() == client_fd);
		std::cout << "Constructor and fd getter: PASS" << std::endl;
		const std::string	username = "testUsername";
		const std::string	nickname = "testNickname";
		client.setUsername(username);
		client.setNickname(nickname);
		// CLIENT USERNAME AND NICKNAME TEST
		REQUIRE(client.getUsername() == username);
		std::cout << "Username getter: PASS" << std::endl;
		REQUIRE(client.getNickname() == nickname);
		std::cout << "Nickname getter: PASS" << std::endl;
	}
	// DESTRUCTOR AND FD CLOSURE TEST
	REQUIRE(fcntl(client_fd, F_GETFD) == -1 && errno == EBADF);
	std::cout << "Destructor closes socket: PASS" << std::endl;
	close(sockets[1]);
	std::cout << "All Client tests passed!" << std::endl;
	return (EXIT_SUCCESS);
}

