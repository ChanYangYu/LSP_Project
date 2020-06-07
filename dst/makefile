ssu_mntr : main.o foreground.o
	gcc main.o foreground.o -o ssu_mntr
main.o : main.c
	gcc -c main.c
foreground.o : foreground.c
	gcc -c foreground.c

clean : 
	rm *.o
	rm ssu_mntr
