#include <iostream>
#include <cstdlib>
#include <climits>
#include <cerrno>
#include "includes/Client.hpp"
#include "includes/Utils.hpp"

int	convert_to_int( const std::string& s )
{
	char	*end;
	errno = 0;
	long	value = std::strtol(s.c_str(), &end, 10);
	if (s.empty() || s.c_str() == end || *end != '\0')
		throw std::invalid_argument("Port must me a valid integer.");
	else if (ERANGE == errno || (value > INT_MAX || value < INT_MIN)) 
		throw std::out_of_range("integer is out of Range");
	return (static_cast<int>(value));
}

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
		if (port < 1 || port > 65535)
		{
			error(VALIDATION, "Port must be between 1 - 65535");
			return (EXIT_FAILURE);
		}
		const std::string	passwd = av[2];
		if (passwd.empty())
		{
			error(VALIDATION, "Password cannot be empty");
			return (EXIT_FAILURE);
		}
		std::cout << "Port: " << port <<  std::endl;
		Client	c(port);
		std::cout << "Socket FD: " << c.getFD() << std::endl;
	} catch (const std::exception& e) {
		error(VALIDATION, e.what());
		return (EXIT_FAILURE);
	}
	return (EXIT_SUCCESS);
}
