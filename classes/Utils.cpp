#include <iostream>
#include <string>
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
	default:
	    label = "ERROR";
	    break;
	}
	std::cerr << label << ":\n" << msg << std::endl;
}
