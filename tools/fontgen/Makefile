FONTS = font_26.txt font_13.txt font_7.txt font_8.txt


all: font.c font.h

font.c: Makefile txt2cfont $(FONTS)
	./txt2cfont -i '<stdint.h>' ${FONTS} >$@

font.h: font.c
	sed -rn 's/ = .*/;/; s/uint8_t const /extern &/ p' font.c >$@

txt2cfont: Makefile txt2cfont.c
	$(CC) -g -Wall -Wextra -pedantic -o $@ $@.c

clean:
	rm -f txt2cfont *.o *.a *.elf
