test_client: tests/test_client.cpp $(CLASS_SRC)
	$(CC) $(CPP_FLAGS) $^ -o $@
	./$@

test_server: tests/test_server.cpp $(CLASS_SRC)
	$(CC) $(CPP_FLAGS) $^ -o $@
	./$@ 6667 pass

test_channel: tests/test_channel.cpp $(CLASS_SRC)
	$(CC) $(CPP_FLAGS) $^ -o $@
	./$@

fclean: fclean_tests

fclean_tests:
	rm -f test_client test_server test_channel test_*.d

.PHONY: test_client test_server test_channel fclean_tests
