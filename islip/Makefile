all: source release

release: source
	fpc islip.pas

debug: source
	fpc -g islip.pas

source: islip.pas typedefs.pas bytecode.pas parser.pas compiler.pas variable.pas interpreter.pas convert.pas
