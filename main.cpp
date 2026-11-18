#include <cstdlib>
#include <iostream>
#include <new>
#include <string>
#include "Server.hpp"
#include "Utils.hpp"

int	main(int argc, char** argv)
{
	int			port;
	std::string	password;

	if (argc != 3)
	{
		error(INPUT, "Usage: ./ircserv <port> <password>");
		return (EXIT_FAILURE);
	}

	try
	{
		port = convert_to_int(argv[1]);
	}
	catch (const std::exception& e)
	{
		error(VALIDATION, e.what());
		return (EXIT_FAILURE);
	}
	if (port < 1 || port > 65535)
	{
		error(VALIDATION, "Port must be between 1 and 65535");
		return (EXIT_FAILURE);
	}

	password = argv[2];
	if (password.empty())
	{
		error(VALIDATION, "Password cannot be empty");
		return (EXIT_FAILURE);
	}

	Server::setupSignals();
	try
	{
		Server	server(port, password);
		server.run();
	}
	catch (const std::bad_alloc&)
	{
		error(SYSTEM, "Out of memory");
		return (EXIT_FAILURE);
	}
	catch (const std::exception& e)
	{
		error(NETWORK, e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
