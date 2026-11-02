#include "Channel.hpp"
#include <sstream>

Channel::Channel(const std::string& name)
: _name(name)
, _topic("")
, _key("")
, _limit(0)
, _inviteOnly(false)
, _topicRestricted(false)
{
}

Channel::~Channel()
{
}

const std::string&	Channel::getName(void) const 
{
	return (_name);
}

const std::string&	Channel::getTopic(void) const
{
	return (_topic);
}

void	Channel::setTopic(const std::string& topic)
{
	_topic = topic;
}

const std::string&	Channel::getKey(void) const
{
	return (_key);
}

void	Channel::setKey(const std::string& key)
{
	_key = key;
}

bool	Channel::hasKey(void) const 
{
	return (!_key.empty());
}

size_t	Channel::getLimit(void) const 
{
	return (_limit);
}

void	Channel::setLimit(size_t limit)
{
	_limit = limit;
}

bool	Channel::isInviteOnly(void) const
{
	return (_inviteOnly);
}

void	Channel::setInviteOnly(bool value)
{
	_inviteOnly = value;
}

bool	Channel::isTopicRestricted(void) const
{
	return (_topicRestricted);
}

void	Channel::setTopicRestricted(bool value)
{
	_topicRestricted = value;
}

bool	Channel::addMember(Client* client)
{
	int	fd;

	if (!client)
		return (false);
	fd = client->getFD();
	if (_members.find(fd) != _members.end())
		return (false);
	_members[fd] = client;
	if (_members.size() == 1)
		_operators.insert(fd);
	_invited.erase(fd);
	return (true);
}

bool	Channel::removeMember(int fd)
{
	_operators.erase(fd);
	_invited.erase(fd);
	return (_members.erase(fd) > 0);
}

bool	Channel::isMember(int fd) const
{
	return (_members.find(fd) != _members.end());
}

size_t	Channel::size(void) const
{
	return (_members.size());
}

bool	Channel::isEmpty(void) const
{
	return (_members.empty());
}

bool	Channel::isFull(void) const
{
	return (_limit > 0 && _members.size() >= _limit);
}

Client*	Channel::findMember(const std::string& nick) const
{
	std::map<int, Client*>::const_iterator	it;

	for (it = _members.begin(); it != _members.end(); ++it)
		if (it->second->getNickname() == nick)
			return (it->second);
	return (NULL);
}

const std::map<int, Client*>&	Channel::getMembers(void) const
{
	return (_members);
}

void	Channel::addOperator(int fd)
{
	if (isMember(fd))
		_operators.insert(fd);
}

void	Channel::removeOperator(int fd)
{
	_operators.erase(fd);
}

bool	Channel::isOperator(int fd) const
{
	return (_operators.find(fd) != _operators.end());
}

void	Channel::invite(int fd)
{
	_invited.insert(fd);
}

bool	Channel::isInvited(int fd) const
{
	return (_invited.find(fd) != _invited.end());
}

int	Channel::canJoin(int fd, const std::string& key) const
{
	if (_inviteOnly && !isInvited(fd))
		return (473);
	if (hasKey() && key != _key)
		return (475);
	if (isFull())
		return (471);
	return (0);
}

std::string	Channel::getNamesList(void) const
{
	std::map<int, Client*>::const_iterator	it;
	std::string								list;

	for (it = _members.begin(); it != _members.end(); ++it)
	{
		if (!list.empty())
			list = list + " ";
		if (isOperator(it->first))
			list = list + "@";
		list = list + it->second->getNickname();
	}
	return (list);
}

std::string	Channel::getModeString(void) const
{
	std::string			modes("+");
	std::string			params;
	std::ostringstream	oss;

	if (_inviteOnly)
		modes = modes + "i";
	if (_topicRestricted)
		modes = modes + "t";
	if (hasKey())
	{
		modes = modes + "k";
		params = params + " " + _key;
	}
	if (_limit > 0)
	{
		modes = modes + "l";
		oss << _limit;
		params = params + " " + oss.str();
	}
	return (modes + params);
}

bool	Channel::isValidName(const std::string& name)
{
	if (name.size() < 2 || name.size() > 50)
		return (false);
	if (name[0] != '#' && name[0] != '&')
		return (false);
	for (size_t i = 1; i < name.size(); ++i)
		if (name[i] == ' ' || name[i] == ',' || name[i] == '\a'
			|| name[i] == '\r' || name[i] == '\n' || name[i] == '\0')
			return (false);
	return (true);
}
