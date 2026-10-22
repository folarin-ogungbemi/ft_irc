# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: aielo <aielo@student.42berlin.de>          +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2026/01/05 13:32:20 by aielo             #+#    #+#              #
#    Updated: 2026/10/09 13:56:31 by foogungb         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

# Name
NAME		= ircserv
TEST_NAME	= irctest

# Directories

OBJ_DIR		= obj
CLASS_DIR	= classes
INCL_DIR	= includes
TEST_DIR	= tests

# Sources
CLASS_SRC	= $(CLASS_DIR)/
				
SOURCES		= 	main.cpp \
			classes/Utils.cpp \
			classes/Client.cpp\

TEST_SOURCES	= 	tests/test_client.cpp\
			classes/Utils.cpp \
			classes/Client.cpp\

# Objects
OBJECTS		= $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)
TEST_OBJECTS    = $(TEST_SOURCES:%.cpp=$(OBJ_DIR)/%.o)
DEP		= $(OBJECTS:.o=.d) $(TEST_OBJECTS:.o=.d)

# Compiler
CC		= c++
CFLAGS		= -Wall -Wextra -Werror -Wshadow -fsanitize=undefined
CEXTRA_VER	= -std=c++98
CEXTRA_INC 	= -I$(INCL_DIR) -I. -MMD -MP
CPP_FLAGS	= $(CFLAGS) $(CEXTRA_VER) $(CEXTRA_INC)

# Rules
all: $(OBJ_DIR) $(NAME)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(NAME): $(OBJECTS)
	$(CC) $(CPP_FLAGS) $(OBJECTS) -o $@

$(TEST_NAME): $(TEST_OBJECTS)
	$(CC) $(CPP_FLAGS) $(TEST_OBJECTS) -o $@

test: $(TEST_NAME)
	./$(TEST_NAME)

$(OBJ_DIR)/%.o: %.cpp
	mkdir -p $(dir $@)
	$(CC) $(CPP_FLAGS) -c $< -o $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re: fclean all

# Valgrind rules
val: $(NAME)
	valgrind \
		--leak-check=full \
		--show-leak-kinds=all \
		--track-origins=yes \
		./$(NAME) DEBUG

valre: re val

.PHONY: all clean fclean re val valre

-include $(DEP)
