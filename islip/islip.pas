// islip - IneQuation's Scripting Language Interpreter in Pascal
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
    interpreter;    // bytecode interpreter

type
    // container for command line options
    islip_options   = record
        script_fname    : string;
    end;

var
    g_options       : islip_options;
    
    g_script        : file of byte;

// parse command line arguments, return false if the user's done something wrong
function islip_parse_args : boolean;
begin
    // initialization
    islip_parse_args := true;

    if ParamCount < 1 then
        islip_parse_args := false;

    g_options.script_fname := ParamStr(1);
end;

// print instructions
procedure islip_print_help;
begin
    writeln('islip - IneQuation''s Scripting Language Interpreter in Pascal');
    writeln('Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>');
    writeln('Usage: ', ParamStr(0), ' <SCRIPT FILE>');
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

// main routine
begin
    if not islip_parse_args then begin
        islip_print_help;
        exit;
    end;

    if not islip_file_open then begin
        writeln('ERROR: Failed to read script file "', g_options.script_fname, '"');
        exit;
    end;

    if not islip_compile(g_script) then begin
        // we rely on the parser to print errors/warnings
        islip_file_close;
        exit;
    end;
    islip_file_close;
end.