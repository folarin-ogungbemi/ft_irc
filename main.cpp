#include <iostream>
#include <cstdlib>
#include "includes/Server.hpp"
#include "includes/Utils.hpp"

int main(int ac, char **av)
{
	int port;
	std::string passwd;

	if (ac != 3)
	{
		error(INPUT, "Usage: ./ircserv <port> <password>");
		return (EXIT_FAILURE);
	}
	try
	{
		port = convert_to_int(av[1]);
		passwd = av[2];
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
	}
	catch (const std::exception &e)
	{
		error(VALIDATION, e.what());
		return (EXIT_FAILURE);
	}

	Server::setupSignals();
	try
	{
		Server server(port, passwd);
		server.run();
	}
	catch (const std::exception &e)
	{
		error(NETWORK, e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
