#ifndef	CLIENT_HPP
# define	CLIENT_HPP

#include <string>

class	Client{
private:
	std::string	_username;
	std::string	_nickname;
	std::string	_realName;
	int	_fd;
	bool	_passwordAccepted;
	bool	_registered;

	Client( void );
	Client( const Client& rhs );	
	Client&	operator=( const Client& rhs );	
public:
	Client(int fd);
	~Client();
	int	getFD( void ) const;
	bool	setUsername( const std::string& username );
	bool	setNickname( const std::string& nickname );
	void	setRealName( const std::string& realName );
	void	setPasswordAccepted( bool accepted );
	void	setRegistered( bool registered );
	const std::string&	getUsername( void ) const;
	const std::string&	getNickname( void ) const;
	const std::string&	getRealName( void ) const;
	bool	isPasswordAccepted( void ) const;
	bool	isRegistered( void ) const;
};

#endif // CLIENT.HPP
