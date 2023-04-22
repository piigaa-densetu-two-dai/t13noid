CC=avr-gcc
AS=$(CC)
MCU_TARGET=attiny13a
CFLAGS=-g -Wall -Os -mmcu=$(MCU_TARGET) -DF_CPU=4800000 # -flto
ASFLAGS=$(CFLAGS)
LDFLAGS=-Wl,-Map,main.map
AVRDUDE=avrdude -c usbasp -B 100 -p $(MCU_TARGET)

OBJS=main.o

all: main.hex

clean:
	$(RM) main.elf main.hex main.map main.lst $(OBJS)

main.elf: $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJS)

main.hex: main.elf
	avr-size --format=avr --mcu=$(MCU_TARGET) $<
	avr-objdump -h -S $< > main.lst
	avr-objcopy -j .data -j .text -O ihex $< $@

flash: main.hex
	$(AVRDUDE) -U flash:w:$<

fuse:
	$(AVRDUDE) -U lfuse:w:0x79:m -U hfuse:w:0xFF:m # 4.8MHz CKDIV8非プログラム
