voronoi1: main.o watchtower.o list.o
	gcc -Wall main.o watchtower.o list.o -o voronoi1 -g

list.o: list.c list.h
	gcc -Wall -o list.o list.c -c -g

watchtower.o: watchtower.c watchtower.h list.h
	gcc -Wall -o watchtower.o watchtower.c -c -g

main.o: main.c watchtower.h list.h
	gcc -Wall -o main.o main.c -c -g

clean:
	rm *.o voronoi1 main watchtower list
