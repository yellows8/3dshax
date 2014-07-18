.PHONY: clean

all:	3dshax_arm9.bin 3dshax_arm11.bin

clean:
	make clean -f Makefile.arm9
	make clean -f Makefile.arm11

3dshax_arm9.bin:
	make -f Makefile.arm9 OUTPATH=$(OUTPATH)

3dshax_arm11.bin:
	make -f Makefile.arm11 OUTPATH=$(OUTPATH)
