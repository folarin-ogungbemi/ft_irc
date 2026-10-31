#ifndef PARSER_HPP
# define PARSER_HPP

# include <string>
# include <vector>

struct IrcMessage
{
	std::string	prefix;
	std::string	command;
	std::vector<std::string>	params;
};

class Parser
{
private:
	Parser(void);
	Parser(const Parser& rhs);
	Parser&	operator=(const Parser& rhs);
	static bool	is_valid_prefix( const std::string&, size_t&, size_t& );
	static bool	is_valid_command( const std::string&, size_t&, size_t& );
	static bool	is_valid_params( const std::string&, size_t&, size_t&, std::vector<std::string>& );
public:
	static IrcMessage	parse( const std::string& );
};

#endif
