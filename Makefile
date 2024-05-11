yarnT: yarnT.c
	$(CC) yarnT.c -o yarnT -Wall -Wextra -pedantic -std=c99
clean:
	-rm *.out
	-rm *.o