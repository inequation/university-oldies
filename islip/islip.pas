// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Main interpreter executable

program islip;

// the console apptype is only supported on win32, gives warnings in Linux
// since any app is a console one by default
{$IFDEF WIN32}
    {$APPTYPE CONSOLE}
{$ENDIF}

uses
    SysUtils,
{$IFNDEF WIN32} // only use crt in Linux
    crt,
{$ENDIF}
    typedefs,       // C-like types
    compiler,       // language parser and bytecode compiler
    bytecode,       // bytecode definitions
    interpreter,    // bytecode interpreter
    variable;

type
    // container for command line options
    islip_options   = record
        script_fname    : string;
        pseudoasm       : boolean;
    end;

var
    g_options       : islip_options;
    
    g_script        : cfile;

    g_compiler      : islip_compiler;
    g_interpreter   : islip_interpreter;

    // the raw instruction list and data to be fed to the interpreter
    g_bytecode      : islip_bytecode;
    g_data          : islip_data;

// parse command line arguments, return false if the user's done something wrong
function islip_parse_args : boolean;
var
    i   : int;
begin
    // initialization
    islip_parse_args := false;
    with g_options do begin
        script_fname := '';
        pseudoasm := false;
    end;

    if ParamCount < 1 then
        exit;

    i := 1;
    while i <= ParamCount do begin
        if ParamStr(i)[1] <> '-' then
            break;
        if length(ParamStr(i)) = 2 then
            case ParamStr(i)[2] of
                'g':
                    g_options.pseudoasm := true;
                else
                    exit;
            end;
        inc(i);
    end;

    if i <> ParamCount then
        exit;

    g_options.script_fname := ParamStr(ParamCount);
    islip_parse_args := true;
end;

// print instructions
procedure islip_print_help;
begin
    writeln('islip - IneQuation''s Simple LOLCODE Interpreter in Pascal');
    writeln('Written by Leszek "IneQuation" Godlewski ',
        '<leszgod081@student.polsl.pl>');
    writeln('Usage: ', ParamStr(0), ' [OPTIONS] <SCRIPT FILE>');
    writeln('Options:');
    writeln('  -g      Output pseudo-assembly code for debugging');
end;

// open the script file, return false on failure
function islip_file_open : boolean;
begin
    islip_file_open := true;
    assign(g_script, g_options.script_fname);
    {$I-}
    reset(g_script);
    {$I+}
    if IOResult <> 0 then
        islip_file_open := false;
end;

// close the script file
procedure islip_file_close;
begin
    close(g_script);
end;

// output pseudo-assembly code
procedure islip_asm_print;
var
    i   : integer;
begin
    writeln('segment .data');
    for i := 1 to length(g_data) do begin
        write('  0x', IntToHex(i, 8), ' = ');
        if g_data[i].get_type = VT_STRING then
            write('"');
        g_data[i].echo(false);
        if g_data[i].get_type = VT_STRING then
            write('"');
        writeln;
    end;
    writeln;
    writeln('segment .text');
    for i := 1 to length(g_bytecode) do begin
        case g_bytecode[i].inst of
            OP_STOP:
                writeln('  stop');
            OP_PUSH:
                begin
                    if g_bytecode[i].arg = ARG_NULL then
                        writeln('  push   NULL')
                    else
                        writeln('  push   0x', IntToHex(g_bytecode[i].arg, 8));
                end;
            OP_POP:
                begin
                    if g_bytecode[i].arg = ARG_NULL then
                        writeln('  pop    NULL')
                    else
                        writeln('  pop    0x', IntToHex(g_bytecode[i].arg, 8));
                end;
            OP_ADD:
                writeln('  add');
            OP_SUB:
                writeln('  sub');
            OP_MUL:
                writeln('  mul');
            OP_DIV:
                writeln('  div');
            OP_MOD:
                writeln('  mod');
            OP_MIN:
                writeln('  min');
            OP_MAX:
                writeln('  max');
            OP_AND:
                writeln('  and');
            OP_OR:
                writeln('  or');
            OP_XOR:
                writeln('  xor');
            OP_EQ:
                writeln('  eq');
            OP_NEQ:
                writeln('  neq');
            OP_NEG:
                writeln('  neg');
            OP_CONCAT:
                writeln('  concat');
            OP_JMP:
                writeln('  jmp    0x', IntToHex(g_bytecode[i].arg, 8));
            OP_CNDJMP:
                writeln('  cndjmp 0x', IntToHex(g_bytecode[i].arg, 8));
            OP_PRINT:
                writeln('  print');
            OP_READ:
                writeln('  read');
            OP_CALL:
                writeln('  call   0x', IntToHex(g_bytecode[i].arg, 8));
        end;
    end;
end;

// main routine
begin
    if not islip_parse_args then begin
        islip_print_help;
        exit;
    end;

    if not islip_file_open then begin
        writeln('ERROR: Failed to read script file "', g_options.script_fname,
            '"');
        exit;
    end;

    // compile the script
    g_compiler := islip_compiler.create(g_script);
    if not g_compiler.compile then begin
        // we rely on the compiler and the parser to print errors/warnings
        g_compiler.destroy;
        islip_file_close;
        exit;
    end;
    islip_file_close;
    // retrieve compilation products
    g_compiler.get_products(g_bytecode, g_data);
    g_compiler.destroy;

    // output pseudo-asm, if requested
    if g_options.pseudoasm then
        islip_asm_print
    else begin
        // run the bytecode
        g_interpreter := islip_interpreter.create(32, g_bytecode, g_data);
        g_interpreter.run;
        g_interpreter.destroy;
    end;

    // kthxbai!
end.