#include <cerrno>
#include <cstdlib>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <fcntl.h>
#include <unistd.h>
#include "includes/Client.hpp"

#define REQUIRE(cond) \
	do { if (!(cond)) { std::cerr << "FAIL: " #cond << " (line " \
		<< __LINE__ << ")" << std::endl; return (false); } } while (0)

typedef bool	(*TestFunction)(void);

static bool	createSocketPair(int sockets[2])
{
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == -1)
	{
		std::cerr << "FAIL: socketpair() failed" << std::endl;
		return (false);
	}
	return (true);
}

static bool	testClientInitialStateAndIdentity(void)
{
	int	sockets[2];

	if (!createSocketPair(sockets))
		return (false);
	{
		Client	client(sockets[0]);

		REQUIRE(client.getFD() == sockets[0]);
		REQUIRE(client.getUsername().empty());
		REQUIRE(client.getNickname().empty());
		REQUIRE(!client.isPasswordAccepted());
		REQUIRE(!client.isRegistered());
		REQUIRE(client.setUsername("alice"));
		REQUIRE(client.getUsername() == "alice");
		REQUIRE(client.setNickname("Alice"));
		REQUIRE(client.getNickname() == "Alice");
	}
	close(sockets[1]);
	return (true);
}

static bool	testUsernameValidation(void)
{
	int	sockets[2];

	if (!createSocketPair(sockets))
		return (false);
	{
		Client	client(sockets[0]);

		REQUIRE(!client.setUsername(std::string("", 0)));
		REQUIRE(client.getUsername().empty());
		REQUIRE(!client.setUsername(std::string("user name", 9)));
		REQUIRE(!client.setUsername(std::string("user\0name", 9)));
		REQUIRE(!client.setUsername(std::string("user\rname", 9)));
		REQUIRE(!client.setUsername(std::string("user\nname", 9)));
		REQUIRE(client.setUsername(std::string("a", 1)));
		REQUIRE(client.getUsername() == "a");
		REQUIRE(client.setUsername(std::string("user@name", 9)));
		REQUIRE(client.getUsername() == "user@name");
	}
	close(sockets[1]);
	return (true);
}

static bool	testNicknameValidation(void)
{
	int			sockets[2];
	std::string	maxNickname(9, 'a');
	std::string	tooLongNickname(10, 'a');

	if (!createSocketPair(sockets))
		return (false);
	{
		Client	client(sockets[0]);

		REQUIRE(client.setNickname("A"));
		REQUIRE(client.setNickname("valid[]"));
		REQUIRE(client.setNickname("valid-2"));
		REQUIRE(client.setNickname(maxNickname));
		REQUIRE(client.getNickname() == maxNickname);
		REQUIRE(!client.setNickname(""));
		REQUIRE(!client.setNickname("1invalid"));
		REQUIRE(!client.setNickname("-invalid"));
		REQUIRE(!client.setNickname("invalid nickname"));
		REQUIRE(!client.setNickname("invalid!"));
		REQUIRE(!client.setNickname(tooLongNickname));
		REQUIRE(client.getNickname() == maxNickname);
		REQUIRE(client.setNickname("A^{}z09-"));
		REQUIRE(client.setNickname("A[]\\`z09"));
		REQUIRE(client.getNickname() == "A[]\\`z09");
	}
	close(sockets[1]);
	return (true);
}

static bool	testRegistration(void)
{
	int	sockets[2];

	if (!createSocketPair(sockets))
		return (false);
	{
		Client	client(sockets[0]);

		client.setPasswordAccepted(true);
		REQUIRE(client.isPasswordAccepted());
		client.setPasswordAccepted(false);
		REQUIRE(!client.isPasswordAccepted());
		client.setRegistered(true);
		REQUIRE(client.isRegistered());
		client.setRegistered(false);
		REQUIRE(!client.isRegistered());
	}
	close(sockets[1]);
	return (true);
}

static bool	testClientClosesOwnedSocket(void)
{
	int	sockets[2];
	int	clientFd;

	if (!createSocketPair(sockets))
		return (false);
	clientFd = sockets[0];
	{
		Client	client(clientFd);

		REQUIRE(client.getFD() == clientFd);
	}
	REQUIRE(fcntl(clientFd, F_GETFD) == -1);
	REQUIRE(errno == EBADF);
	close(sockets[1]);
	return (true);
}

static bool	runTest(const char *msg, TestFunction test)
{
	if (!test())
	{

		std::cerr << "\033[31m" << "FAIL: " <<  "\033[0m" << msg << std::endl;
		return (false);
	}
	std::cerr << "\033[32m" << "PASS: " << "\033[0m" << msg << std::endl;
	return (true);
}

int	main(void)
{
	bool	passed;

	std::cout << "==================Client tests==================" << std::endl;
	passed = true;
	if (!runTest("INITIAL STATE AND IDENTITY SETTERS/GETTERS",
			testClientInitialStateAndIdentity))
		passed = false;
	if (!runTest("USERNAME CHARACTER VALIDATION CHECKS",
			testUsernameValidation))
		passed = false;
	if (!runTest("NICKNAME CHARACTER VALIDATION CHECKS",
			testNicknameValidation))
		passed = false;
	if (!runTest("CLIENT REGISTRATION", testRegistration))
		passed = false;
	if (!runTest("OWNED SOCKETS CLOSES ON REGISTRATION",
			testClientClosesOwnedSocket))
		passed = false;
	if (!passed)
	{
		std::cerr << "\033[31m" << "Client test suite: FAIL" << "\033[0m" << std::endl;
		std::cout << "==================Client tests==================" << std::endl;
		return (EXIT_FAILURE);
	}
	std::cerr << "\033[32m" << "Client test suite: PASS" << "\033[0m" << std::endl;
	std::cout << "==================Client tests==================" << std::endl;
	return (EXIT_SUCCESS);
}
