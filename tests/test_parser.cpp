#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include "includes/Parser.hpp"

#define REQUIRE(cond) \
	do { if (!(cond)) { std::cerr << "FAIL: " #cond << " (line " \
		<< __LINE__ << ")" << std::endl; return (false); } } while (0)

typedef bool	(*TestFunction)(void);

static bool	test_valid_messages(void)
{
	IrcMessage	message;

	message = Parser::parse("PING");
	REQUIRE(message.prefix.empty());
	REQUIRE(message.command == "PING");
	REQUIRE(message.params.empty());

	message = Parser::parse("PiNg");
	REQUIRE(message.command == "PiNg");
	REQUIRE(message.params.empty());

	message = Parser::parse("PASS secret");
	REQUIRE(message.command == "PASS");
	REQUIRE(message.params.size() == 1);
	REQUIRE(message.params[0] == "secret");

	message = Parser::parse("USER alice 0 * :Alice Example");
	REQUIRE(message.command == "USER");
	REQUIRE(message.params.size() == 4);
	REQUIRE(message.params[0] == "alice");
	REQUIRE(message.params[1] == "0");
	REQUIRE(message.params[2] == "*");
	REQUIRE(message.params[3] == "Alice Example");
	return (true);
}

static bool	test_optional_prefix_and_trailing_params(void)
{
	IrcMessage	message;

	message = Parser::parse(":alice!user@localhost PRIVMSG #room :hello there");
	REQUIRE(message.prefix == "alice!user@localhost");
	REQUIRE(message.command == "PRIVMSG");
	REQUIRE(message.params.size() == 2);
	REQUIRE(message.params[0] == "#room");
	REQUIRE(message.params[1] == "hello there");

	message = Parser::parse("PRIVMSG #room :hello   there");
	REQUIRE(message.params.size() == 2);
	REQUIRE(message.params[1] == "hello   there");

	message = Parser::parse("PRIVMSG #room :");
	REQUIRE(message.params.size() == 2);
	REQUIRE(message.params[1].empty());

	message = Parser::parse("PING :");
	REQUIRE(message.command == "PING");
	REQUIRE(message.params.size() == 1);
	REQUIRE(message.params[0].empty());
	return (true);
}

static bool	test_invalid_messages(void)
{
	std::string	inputs[] = {
		"",
		"   ",
		"  nick   alice  0",
		":alice!user@localhost ",
		"NICK#alice",
		"USER alice 0 * :bad\r\n",
		"JOIN #room :hello\n"
	};
	size_t	count = sizeof(inputs) / sizeof(inputs[0]);
	for (size_t i = 0; i < count; ++i)
	{
		try
		{
			Parser::parse(inputs[i]);
			std::cerr << "FAIL: parse accepted invalid input: " << inputs[i] << std::endl;
			return (false);
		}
		catch (const std::invalid_argument&){}
	}
	return (true);
}

static bool	runTest(const char *name, TestFunction test)
{
	if (!test())
	{
		std::cerr << "FAIL: " << name << std::endl;
		return (false);
	}
	std::cout << "PASS: " << name << std::endl;
	return (true);
}

int	main(void)
{
	bool	passed;

	std::cout << "==================Parser tests==================" << std::endl;
	passed = true;
	if (!runTest("VALID IRC MESSAGE PARSING", test_valid_messages))
		passed = false;
	if (!runTest("PREFIX AND TRAILING PARAMETER HANDLING",
			test_optional_prefix_and_trailing_params))
		passed = false;
	if (!runTest("INVALID IRC MESSAGE REJECTION", test_invalid_messages))
		passed = false;
	if (!passed)
	{
		std::cerr << "\033[31m" << "Parser test suite: FAIL" << "\033[0m" << std::endl;
		std::cout << "==================Parser tests==================" << std::endl;
		return (EXIT_FAILURE);
	}
	std::cerr << "\033[32m" << "Parser test suite: PASS" << "\033[0m" << std::endl;
	std::cout << "==================Parser tests==================" << std::endl;
	return (EXIT_SUCCESS);
}
