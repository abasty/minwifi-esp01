basic : basic.c keywords.c
	gcc -g -O0 -o basic basic.c

keywords.c : create-keywords.sh
	./create-keywords.sh >keywords.c

clean:
	rm -f *.o
	rm -f keywords.c
	rm -f basic
