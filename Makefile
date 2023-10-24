PCY : PCY.o
	  	gcc PCY.o -o PCY

PCY.o : PCY.c 
		gcc -c PCY.c

clean:
		rm PCY.o PCY