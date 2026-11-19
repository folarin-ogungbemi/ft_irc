# Rules
stresstest_low:
	chmod +x ./test/StressTest.sh
	./test/StressTest.sh 6667 pass 5 5

stresstest_high:
	chmod +x ./test/StressTest.sh
	./test/StressTest.sh 6667 pass 50 50

.PHONY: stresstest_low stresstest_high
