all: source release

release: source
	fpc islip.pas

debug: source
	fpc -g islip.pas

source: bytecode.pas compiler.pas convert.pas interpreter.pas islip.pas parser.pas typedefs.pas variable.pas

clean:
	rm -f islip
	rm -f bytecode.pas~ compiler.pas~ convert.pas~ interpreter.pas~ islip.pas~ parser.pas~ typedefs.pas~ variable.pas~
	rm -f bytecode.o compiler.o convert.o interpreter.o islip.o parser.o typedefs.o variable.o
	rm -f bytecode.ppu compiler.ppu convert.ppu interpreter.ppu islip.ppu parser.ppu typedefs.ppu variable.ppu
