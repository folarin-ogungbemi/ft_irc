#include "Parser.hpp"
#include <stdexcept>

bool	Parser::is_valid_prefix( const std::string& prefix, size_t& pos, size_t& end )
{
	if (prefix.empty() || pos >= prefix.size() || prefix[pos] != ':')
		return (false);
	end = prefix.find(' ', pos);
	if (end == std::string::npos || end == pos + 1)
		return (false);
	return (true);
}

bool	Parser::is_valid_command(const std::string& command, size_t& pos, size_t& end)
{
	if (command.empty() || pos >= command.size())
		return false;
	end = command.find(' ', pos);
	if (end == std::string::npos)
		end = command.size();
	if (end == pos)
		return false;

	bool is_alpha = true;
	bool is_numeric = true;

	for (size_t i = pos; i < end; ++i)
	{
		if (!((command[i] >= 'A' && command[i] <= 'Z')
			|| (command[i] >= 'a' && command[i] <= 'z')))
			is_alpha = false;
		if (!(command[i] >= '0' && command[i] <= '9'))
			is_numeric = false;
	}
	if (!(is_alpha || (is_numeric && end - pos == 3)))
		return (false);
    return true;
}

bool	Parser::is_valid_params(const std::string& line, size_t& pos, size_t& end, std::vector<std::string>& params)
{
	while (pos < line.size())
	{
		if (line[pos] != ' ')
			return (false);
		while (pos < line.size() && line[pos] == ' ')
			++pos;
		if (pos >= line.size())
			return (false);
		if (line[pos] == ':')
		{
			++pos;
			std::string trailing = line.substr(pos);
			for (size_t i = 0; i < trailing.size(); ++i)
			{
				if (trailing[i] == '\0' || trailing[i] == '\r' ||
					trailing[i] == '\n')
					return false;
			}
			params.push_back(trailing);
			if (params.size() > 15)
				return false;
			pos = line.size();
			break;
		}
		end = line.find(' ', pos);
		if (end == std::string::npos)
			end = line.size();
		std::string middle = line.substr(pos, end - pos);
		for (size_t i = 0; i < middle.size(); ++i)
		{
			if (middle[i] == '\0' || middle[i] == '\r' ||
				middle[i] == '\n' || middle[i] == ':')
				return false;
		}
		params.push_back(middle);
		pos = end;
		if (params.size() > 15)
			return false;
	}
	return true;
}

IrcMessage	Parser::parse(const std::string& line)
{
	IrcMessage msg;
	size_t pos = 0;
	size_t end = 0;

	if (line.empty() || line[0] == ' ')
		throw std::invalid_argument("invalid IRC message");
	if (line[pos] == ':')
	{
		if (!is_valid_prefix(line, pos, end))
			throw std::invalid_argument("invalid prefix");
		msg.prefix = line.substr(pos + 1, end - pos - 1);
		pos = end;
		while (pos < line.size() && line[pos] == ' ')
			++pos;
	}
	if (!is_valid_command(line, pos, end))
		throw std::invalid_argument("invalid command");
	msg.command = line.substr(pos, end - pos);
	pos = end;
	if (!is_valid_params(line, pos, end, msg.params))
		throw std::invalid_argument("invalid params");
	return msg;
}
