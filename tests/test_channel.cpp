#include "Channel.hpp"
#include "Client.hpp"
#include <iostream>
#include <string>
#include <cstdlib>
#include <fcntl.h>

static int	g_total = 0;
static int	g_failed = 0;

static void	check(bool ok, const char* expr, int line)
{
	++g_total;
	if (!ok)
	{
		++g_failed;
		std::cerr << "  [FAIL] line " << line << ": " << expr << "\n";
	}
}

# define CHECK(cond) check((cond), #cond, __LINE__)

static Client*	mk(const std::string& nick)
{
	int		fd;
	Client*	c;

	fd = open("/dev/null", O_RDONLY);
	if (fd < 0)
	{
		std::cerr << "open(/dev/null) failed\n";
		std::exit(2);
	}
	c = new Client(fd);
	c->setNickname(nick);
	return (c);
}

static void	title(const char* name)
{
	std::cout << "- " << name << "\n";
}

static void	test_defaults(void)
{
	title("defaults");
	Channel	ch("#test");

	CHECK(ch.getName() == "#test");
	CHECK(ch.getTopic() == "");
	CHECK(ch.getKey() == "");
	CHECK(!ch.hasKey());
	CHECK(ch.getLimit() == 0);
	CHECK(!ch.isInviteOnly());
	CHECK(!ch.isTopicRestricted());
	CHECK(ch.isEmpty());
	CHECK(ch.size() == 0);
	CHECK(!ch.isFull());
	CHECK(ch.getModeString() == "+");
	CHECK(ch.getNamesList() == "");
}

static void	test_valid_names(void)
{
	title("isValidName");

	CHECK(Channel::isValidName("#a"));
	CHECK(Channel::isValidName("&local"));
	CHECK(Channel::isValidName("#general"));
	CHECK(Channel::isValidName("#" + std::string(49, 'x')));

	CHECK(!Channel::isValidName(""));
	CHECK(!Channel::isValidName("#"));
	CHECK(!Channel::isValidName("general"));
	CHECK(!Channel::isValidName("#a b"));
	CHECK(!Channel::isValidName("#a,b"));
	CHECK(!Channel::isValidName("#a\ab"));
	CHECK(!Channel::isValidName("#a\r\n"));
	CHECK(!Channel::isValidName("#" + std::string(50, 'x')));
}

static void	test_membership(void)
{
	title("addMember / removeMember / isMember / size");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Channel	ch("#test");

	CHECK(!ch.addMember(NULL));
	CHECK(ch.isEmpty());

	CHECK(ch.addMember(a));
	CHECK(ch.size() == 1);
	CHECK(ch.isMember(a->getFD()));
	CHECK(!ch.isMember(b->getFD()));

	CHECK(!ch.addMember(a));
	CHECK(ch.size() == 1);

	CHECK(ch.addMember(b));
	CHECK(ch.size() == 2);
	CHECK(!ch.isEmpty());

	CHECK(ch.removeMember(a->getFD()));
	CHECK(!ch.isMember(a->getFD()));
	CHECK(ch.size() == 1);
	CHECK(!ch.removeMember(a->getFD()));

	CHECK(ch.removeMember(b->getFD()));
	CHECK(ch.isEmpty());

	delete a;
	delete b;
}

static void	test_operators(void)
{
	title("operators");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Client*	c = mk("giorgio");
	Channel	ch("#test");

	ch.addMember(a);
	ch.addMember(b);

	CHECK(ch.isOperator(a->getFD()));
	CHECK(!ch.isOperator(b->getFD()));

	ch.addOperator(b->getFD());
	CHECK(ch.isOperator(b->getFD()));

	ch.removeOperator(b->getFD());
	CHECK(!ch.isOperator(b->getFD()));

	ch.addOperator(c->getFD());
	CHECK(!ch.isOperator(c->getFD()));

	ch.removeMember(a->getFD());
	CHECK(!ch.isOperator(a->getFD()));

	delete a;
	delete b;
	delete c;
}

static void	test_find_member(void)
{
	title("findMember");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Channel	ch("#test");

	ch.addMember(a);
	ch.addMember(b);

	CHECK(ch.findMember("ale") == a);
	CHECK(ch.findMember("folarin") == b);
	CHECK(ch.findMember("nobody") == NULL);
	CHECK(ch.findMember("Ale") == NULL);

	ch.removeMember(b->getFD());
	CHECK(ch.findMember("giorgio") == NULL);

	delete a;
	delete b;
}

static void	test_topic_and_flags(void)
{
	title("topic and simple setters");
	Channel	ch("#test");

	ch.setTopic("hello world");
	CHECK(ch.getTopic() == "hello world");
	ch.setTopic("");
	CHECK(ch.getTopic() == "");

	ch.setInviteOnly(true);
	CHECK(ch.isInviteOnly());
	ch.setInviteOnly(false);
	CHECK(!ch.isInviteOnly());

	ch.setTopicRestricted(true);
	CHECK(ch.isTopicRestricted());
	ch.setTopicRestricted(false);
	CHECK(!ch.isTopicRestricted());

	ch.setKey("secret");
	CHECK(ch.hasKey());
	CHECK(ch.getKey() == "secret");
	ch.setKey("");
	CHECK(!ch.hasKey());

	ch.setLimit(5);
	CHECK(ch.getLimit() == 5);
	ch.setLimit(0);
	CHECK(ch.getLimit() == 0);
}

static void	test_limit(void)
{
	title("isFull / limit");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Client*	c = mk("giorgio");
	Channel	ch("#test");

	ch.setLimit(2);
	CHECK(!ch.isFull());
	ch.addMember(a);
	CHECK(!ch.isFull());
	ch.addMember(b);
	CHECK(ch.isFull());
	CHECK(ch.canJoin(c->getFD(), "") == 471);

	ch.setLimit(3);
	CHECK(!ch.isFull());
	CHECK(ch.canJoin(c->getFD(), "") == 0);

	ch.setLimit(0);
	ch.addMember(c);
	CHECK(!ch.isFull());

	delete a;
	delete b;
	delete c;
}

static void	test_can_join(void)
{
	title("canJoin (+i, +k, +l and their priority)");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Channel	ch("#test");

	ch.addMember(a);
	CHECK(ch.canJoin(b->getFD(), "") == 0);

	ch.setInviteOnly(true);
	CHECK(ch.canJoin(b->getFD(), "") == 473);
	ch.invite(b->getFD());
	CHECK(ch.isInvited(b->getFD()));
	CHECK(ch.canJoin(b->getFD(), "") == 0);

	ch.setKey("secret");
	CHECK(ch.canJoin(b->getFD(), "") == 475);
	CHECK(ch.canJoin(b->getFD(), "wrong") == 475);
	CHECK(ch.canJoin(b->getFD(), "secret") == 0);

	ch.setLimit(1);
	CHECK(ch.canJoin(b->getFD(), "secret") == 471);

	Client*	c = mk("giorgio");
	CHECK(ch.canJoin(c->getFD(), "wrong") == 473);
	ch.invite(c->getFD());
	CHECK(ch.canJoin(c->getFD(), "wrong") == 475);
	CHECK(ch.canJoin(c->getFD(), "secret") == 471);

	delete a;
	delete b;
	delete c;
}

static void	test_invite_lifecycle(void)
{
	title("invitation lifecycle");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Channel	ch("#test");

	ch.addMember(a);
	ch.setInviteOnly(true);

	ch.invite(b->getFD());
	CHECK(ch.isInvited(b->getFD()));

	ch.addMember(b);
	CHECK(!ch.isInvited(b->getFD()));

	ch.removeMember(b->getFD());
	CHECK(ch.canJoin(b->getFD(), "") == 473);

	ch.invite(b->getFD());
	ch.removeMember(b->getFD());
	CHECK(!ch.isInvited(b->getFD()));

	delete a;
	delete b;
}

static void	test_names_list(void)
{
	title("getNamesList");
	Client*	a = mk("ale");
	Client*	b = mk("folarin");
	Client*	c = mk("giorgio");
	Channel	ch("#test");

	ch.addMember(a);
	ch.addMember(b);
	ch.addMember(c);

	CHECK(ch.getNamesList() == "@ale folarin giorgio");

	ch.addOperator(c->getFD());
	CHECK(ch.getNamesList() == "@ale folarin @giorgio");

	ch.removeMember(b->getFD());
	CHECK(ch.getNamesList() == "@ale @giorgio");

	delete a;
	delete b;
	delete c;
}

static void	test_mode_string(void)
{
	title("getModeString");
	Channel	ch("#test");

	CHECK(ch.getModeString() == "+");

	ch.setInviteOnly(true);
	CHECK(ch.getModeString() == "+i");

	ch.setTopicRestricted(true);
	CHECK(ch.getModeString() == "+it");

	ch.setKey("secret");
	CHECK(ch.getModeString() == "+itk secret");

	ch.setLimit(10);
	CHECK(ch.getModeString() == "+itkl secret 10");

	ch.setInviteOnly(false);
	CHECK(ch.getModeString() == "+tkl secret 10");

	ch.setKey("");
	CHECK(ch.getModeString() == "+tl 10");

	ch.setLimit(0);
	ch.setTopicRestricted(false);
	CHECK(ch.getModeString() == "+");
}

int	main(void)
{
	std::cout << "Channel tests\n";

	test_defaults();
	test_valid_names();
	test_membership();
	test_operators();
	test_find_member();
	test_topic_and_flags();
	test_limit();
	test_can_join();
	test_invite_lifecycle();
	test_names_list();
	test_mode_string();

	std::cout << "\n" << (g_total - g_failed) << "/" << g_total
		<< " checks passed\n";
	if (g_failed)
	{
		std::cout << g_failed << " FAILED\n";
		return (1);
	}
	std::cout << "ALL OK\n";
	return (0);
}
