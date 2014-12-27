default: tapdev_stdio
	gcc -Wall -O -o tapdev_stdio tapdev_stdio.c

clean:
	rm -f *.o tapdev_stdio
