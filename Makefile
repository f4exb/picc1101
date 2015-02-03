all: picc1101 serial.o


clean:
	rm -f *.o picc1101
	 

picc1101: main.o serial.o
	$(CCPREFIX)gcc $(LDFLAGS) -s -lm -o picc1101 main.o serial.o

main.o: main.h main.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o main.o main.c

serial.o: serial.h serial.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o serial.o serial.c
