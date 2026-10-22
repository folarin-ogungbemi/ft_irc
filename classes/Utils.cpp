#include <iostream>
#include <string>
#include <climits>
#include <cerrno>
#include <cstdlib>
#include "Utils.hpp"

void	error(ErrorType type, const std::string& msg)
{
	const char* label;

	switch (type)
	{
	case INPUT:
	    label = "INPUT ERROR";
	    break;
	case VALIDATION:
	    label = "VALIDATION ERROR";
	    break;
	case NETWORK:
	    label = "NETWORK ERROR";
	    break;
	case SYSTEM:
	    label = "SYSTEM ERROR";
	    break;
	case TEST:
	    label = "TEST ERROR";
	    break;
	default:
	    label = "ERROR";
	    break;
	}
	std::cerr << label << ":\n" << msg << std::endl;
}

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


