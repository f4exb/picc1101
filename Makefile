all: picc1101


clean:
	rm -f *.o picc1101
	 

picc1101: main.o
	$(CCPREFIX)gcc $(LDFLAGS) -s -lm -o picc1101 main.o

main.o: main.h main.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o main.o main.c

