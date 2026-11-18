# Rules
stresstest_low:
	chmod +x ./tests/StressTest.sh
	./tests/StressTest.sh 6667 pass 5 5

stresstest_high:
	chmod +x ./tests/StressTest.sh
	./tests/StressTest.sh 6667 pass 50 50

.PHONY: stresstest_low stresstest_high
