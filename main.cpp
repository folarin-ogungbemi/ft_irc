#include <iostream>
#include <cstdlib>

int	main(int ac, char **av)
{
	if (ac != 2)
		return (EXIT_FAILURE);
	std::cout << av[1] << std::endl;
	return (EXIT_SUCCESS);
}
