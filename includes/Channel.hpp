#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <string>
# include <map>
# include <set>
# include "Client.hpp"

class Channel
{
private:
	std::string				_name;
	std::string				_topic;
	std::string				_key;
	size_t					_limit;
	bool					_inviteOnly;
	bool					_topicRestricted;
	std::map<int, Client*>	_members;
	std::set<int>			_operators;
	std::set<int>			_invited;

	Channel(void);
	Channel(const Channel& rhs);
	Channel&	operator=(const Channel& rhs);

public:
	explicit Channel(const std::string& name);
	~Channel();

	const std::string&	getName(void) const;
	const std::string&	getTopic(void) const;
	void				setTopic(const std::string& topic);
	const std::string&	getKey(void) const;
	void				setKey(const std::string& key);
	bool				hasKey(void) const;
	size_t				getLimit(void) const;
	void				setLimit(size_t limit);
	bool				isInviteOnly(void) const;
	void				setInviteOnly(bool value);
	bool				isTopicRestricted(void) const;
	void				setTopicRestricted(bool value);

	bool				addMember(Client* client);
	bool				removeMember(int fd);
	bool				isMember(int fd) const;
	size_t				size(void) const;
	bool				isEmpty(void) const;
	bool				isFull(void) const;
	Client*				findMember(const std::string& nick) const;
	const std::map<int, Client*>&	getMembers(void) const;

	void				addOperator(int fd);
	void				removeOperator(int fd);
	bool				isOperator(int fd) const;

	void				invite(int fd);
	bool				isInvited(int fd) const;
	int					canJoin(int fd, const std::string& key) const;

	std::string			getNamesList(void) const;
	std::string			getModeString(void) const;
	static bool			isValidName(const std::string& name);
};

#endif
