PCY : PCY.o
	  	gcc -Os PCY.o -o PCY

PCY.o : PCY.c 
		gcc -c -Os PCY.c

clean:
		rm PCY.o PCY