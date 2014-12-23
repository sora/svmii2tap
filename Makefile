default: tapdev_stdio
	gcc -O -o tapdev_stdio tapdev_stdio.c

clean:
	rm -f *.o tapdev_stdio
