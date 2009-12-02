// islip - IneQuation's Scripting Language Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Language parser and bytecode compiler unit

unit compiler;

// uncomment the following to enable debug printouts
{$DEFINE DEBUG}

interface

uses typedefs, parser, bytecode;

function islip_compile(var input : file) : boolean;

implementation

const
    LOLCODE_VERSION         = '1.2';    // supported LOLCODE version

type
    islip_cmp_state = (
        CS_UNDEF,       // undefined state, before reading the first token
        CS_START,       // we've just had HAI, expecting version number or line break
        CS_STATEMENT    // program body, expecting a statement
    );

    // variable linked list element
    pislip_cmp_var  = ^islip_cmp_var;
    islip_cmp_var   = record
        next    : pislip_cmp_var;
        v       : islip_var;
    end;

    // instruction linked list element
    pislip_cmp_inst = ^islip_cmp_inst;
    islip_cmp_inst  = record
        next    : pislip_cmp_inst;
        i       : byte;
    end;

var
    g_parser    : islip_parser;
    g_var_head  : pislip_cmp_var;
    g_inst_head : pislip_cmp_inst;

function islip_compile(var input : file) : boolean;
var
    token   : string;
    toktype : islip_parser_token_type;
    state   : islip_cmp_state;
    sr, sc  : size_t;
    v       : pislip_cmp_var;
begin
    islip_compile := true;

    // initialization
    state := CS_UNDEF;
    g_parser := islip_parser.create(input);
    g_var_head := nil;
    g_inst_head := nil;
    
    while g_parser.get_token(token, toktype) and (length(token) > 0) do begin
{$IFDEF DEBUG}
        writeln('DEBUG: Token: "', token, '", state: ', integer(state));
{$ENDIF}
        case state of
            CS_UNDEF:
                if token <> 'HAI' then begin
                    g_parser.get_pos(sr, sc);
                    writeln('ERROR: Unknown token "', token, '" at line ', sr, ', column ', sc);
                    islip_compile := false;
                    exit;
                end else
                    state := CS_START;
            CS_START:
                if (token <> LOLCODE_VERSION) and not (token[1] in [chr(10), chr(13)]) then begin
                    g_parser.get_pos(sr, sc);
                    writeln('ERROR: Unsupported language version "', token, '" at line ', sr, ', column ', sc);
                    islip_compile := false;
                    exit;
                end else
                    state := CS_STATEMENT;
            CS_STATEMENT:
                // comment
                if token = 'BTW' then begin
                    // skip everything until a newline comes up
                    while true do begin
                        if not g_parser.get_token(token, toktype) then begin
                            writeln('ERROR: Unexpected end of file');
                            islip_compile := false;
                            exit;
                        end;
                        if (length(token) = 1) and (token[1] in [chr(10), chr(13)]) then
                            break;
                    end;
                    continue;
                // module inclusion
                end else if token = 'CAN' then begin
                    // fetch next token
                    if not g_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        islip_compile := false;
                        exit;
                    end;
                    if token <> 'HAS' then begin
                        g_parser.get_pos(sr, sc);
                        writeln('ERROR: "HAS" expected, but got "', token, '" at line ', sr, ', column ', sc);
                        islip_compile := false;
                        exit;
                    end else begin
                        // fetch next token
                        if not g_parser.get_token(token, toktype) then begin
                            writeln('ERROR: Unexpected end of file');
                            islip_compile := false;
                            exit;
                        end;
                        if pos('?', token) = length(token) then begin
                            // remove the question mark
                            delete(token, length(token), 1);
                            // see if we recognize the library
                            // FIXME: allow inlcusion of third-party files
                            if (token <> 'STDIO') and (token <> 'MAHZ') then begin
                                writeln('ERROR: Unknown module "', token, '"');
                                islip_compile := false;
                                exit;
                            end else
                                state := CS_STATEMENT;
{$IFDEF DEBUG}
                            writeln('DEBUG: Including module "', token, '"');
{$ENDIF}
                        end;
                    end;
                end {else if (token = 'BYES') or (token = 'DIAF') then
                    state := CS_DIE
                else if token = 'GIMMEH' then
                    state := CS_READ
                else if token = 'GTFO' then
                    state := CS_BREAK
                else if token = 'I' then
                    state := CS_VAR
                else if token = 'IM' then
                    state := CS_LOOP
                else if token = 'IZ' then
                    state := CS_COND
                else if token = 'LOL' then
                    state := CS_ASSIGN}
                else if token = 'VISIBLE' then begin
                    // put everything on the stack and print until a newline comes up
                    while true do begin
                        if not g_parser.get_token(token, toktype) then begin
                            writeln('ERROR: Unexpected end of file');
                            islip_compile := false;
                            exit;
                        end;
                        if (length(token) = 1) and (token[1] in [chr(10), chr(13)]) then
                            break;
                        if toktype = TT_EXPR then begin
                            // TODO: print variables and evaluate expressions
                        end else begin
                            new(v);
                            v^ := islip_var.create(token, token);
                            
                        end;
                    end;
                    continue;
                end else if (length(token) = 1) and (token[1] in [chr(10), chr(13)]) then
                    continue
                else begin
                    g_parser.get_pos(sr, sc);
                    writeln('ERROR: Unknown keyword "', token, '" at line ', sr, ', column ', sc);
                    islip_compile := false;
                    exit;
                end;
        end;
    end;
    if length(token) > 0 then begin
        writeln('ERROR: Unable to parse script');
        islip_compile := false;
    end;
end;

end.