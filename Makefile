all: picc1101


clean:
	rm -f *.o picc1101
	 

picc1101: main.o
	$(CROSS_COMPILE)gcc $(LDFLAGS) -s -lm -o picc1101 main.o

main.o: main.h main.c
	$(CROSS_COMPILE)gcc $(CFLAGS) -c -o main.o main.c

