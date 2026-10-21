#ifndef	CLIENT_HPP
# define	CLIENT_HPP

#include <string>

class	Client{
private:
	std::string	_username;
	int	_fd;
public:
	Client( void );
	Client(int fd);
	~Client();
	int	getFD( void ) const;
};

#endif // CLIENT.HPP
