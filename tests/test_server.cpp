#include <cstdlib>
#include <iostream>
#include "Server.hpp"

int	main(int argc, char** argv)
{
	char*		end;
	long		port;
	std::string	password;

	if (argc != 3)
	{
		std::cerr << "usage: ./ircserv <port> <password>\n";
		return (1);
	}

	port = std::strtol(argv[1], &end, 10);
	if (*argv[1] == '\0' || *end != '\0' || port < 1 || port > 65535)
	{
		std::cerr << "error: invalid port (1-65535)\n";
		return (1);
	}
	password = argv[2];
	if (password.empty())
	{
		std::cerr << "error: empty password\n";
		return (1);
	}

	Server::setupSignals();

	try
	{
		Server	server(static_cast<int>(port), password);
		server.run();
	}
	catch (const std::exception& e)
	{
		std::cerr << "fatal: " << e.what() << "\n";
		return (1);
	}
	return (0);
}
