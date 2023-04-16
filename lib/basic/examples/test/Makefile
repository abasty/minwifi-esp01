test : test.c ../../token.h ../../token.c ../../keywords.h ../../keywords.inc
	g++ -Wall -g -O0 -o test test.c ../../token.c -I ../..

clean:
	rm -f *.o
	rm -f test
