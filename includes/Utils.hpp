#ifndef	UTILS_HPP
# define	UTILS_HPP

#include <string>

enum	ErrorType
{
	INPUT,
	VALIDATION,
	NETWORK,
	SYSTEM
};

void	error(ErrorType type, const std::string& msg);

#endif	//UTILS_HPP
