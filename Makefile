all: picc1101 


clean:
	rm -f *.o picc1101
	 

picc1101: main.o serial.o pi_cc_spi.o radio.o util.o
	$(CCPREFIX)gcc $(LDFLAGS) -s -lm -lwiringPi -o picc1101 main.o serial.o pi_cc_spi.o radio.o util.o

main.o: main.h main.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o main.o main.c

serial.o: main.h serial.h serial.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o serial.o serial.c

pi_cc_spi.o: main.h pi_cc_spi.h pi_cc_spi.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o pi_cc_spi.o pi_cc_spi.c

radio.o: main.h radio.h radio.c
	$(CCPREFIX)gcc $(CFLAGS) -c -o radio.o radio.c

util.o: util.h util.c
	$(CCPREFIX)gcc $(CFLAGS) -DMAX_VERBOSE_LEVEL=3 -c -o util.o util.c
