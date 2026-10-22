#ifndef	CLIENT_HPP
# define	CLIENT_HPP

#include <string>

class	Client{
private:
	std::string	_username;
	std::string	_nickname;
	int	_fd;
	Client( const Client& rhs );	
	Client&	operator=( const Client& rhs );	
public:
	Client( void );
	Client(int fd);
	~Client();
	int	getFD( void ) const;
	void	setUsername( const std::string& username );
	void	setNickname( const std::string& nickname );
	const std::string&	getUsername( void ) const;
	const std::string&	getNickname( void ) const;
};

#endif // CLIENT.HPP
