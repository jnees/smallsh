setup:
	gcc -std=gnu99 -g -Wall -o smallsh smallsh.c

clean:
	rm smallsh

test:
	gcc -std=gnu99 -g -Wall -o smallsh smallsh.c
	chmod +x ./p3testscript
	./p3testscript > mytestresults 2>&1