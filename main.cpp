#include <iostream>
#include <cstdlib>
#include <sys/socket.h>
#include "includes/Client.hpp"
#include "includes/Utils.hpp"

int	main(int ac, char **av)
{
	if (ac != 3)
	{
		error(INPUT, "Usage: ./ircserv <port> <password>");
		return (EXIT_FAILURE);
	}
	try
	{
		const int	port = convert_to_int(av[1]);
		const std::string	passwd = av[2];
		if (port < 1 || port > 65535)
		{
			error(VALIDATION, "Port must be between 1 - 65535");
			return (EXIT_FAILURE);
		}
		if (passwd.empty())
		{
			error(VALIDATION, "Password cannot be empty");
			return (EXIT_FAILURE);
		}
		std::cout << "Port: " << port <<  std::endl;
		Client	c(5);
		std::cout << "Socket FD: " << c.getFD() << std::endl;
	} catch (const std::exception& e) {
		error(VALIDATION, e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
