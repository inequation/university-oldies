// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Language parser and bytecode compiler unit

unit compiler;

// uncomment the following to enable debug printouts
//{$DEFINE DEBUG}

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs, parser, bytecode, variable, SysUtils;

type
    // variable linked list element
    pislip_cmp_var      = ^islip_cmp_var;
    islip_cmp_var       = record
        next            : pislip_cmp_var;
        id              : string;
        v               : pislip_var;
    end;

    // variable container
    islip_cmp_var_cont  = class
        public
            m_head      : pislip_cmp_var;
            m_tail      : pislip_cmp_var;
            m_index     : size_t;
            
            constructor create;
            destructor destroy; override;

            // appends the new variable to the list and returns its index
            function append(v : pislip_var; id : string) : size_t;
            // finds the index of the given variable by its ID; returns
            // ARG_NULL if not found
            function get_var_index(id : string) : size_t;
            // finds the given variable by its ID; returns nil on failure
            function get_var(id : string) : pislip_cmp_var;
    end;

    // instruction linked list element
    pislip_cmp_inst     = ^islip_cmp_inst;
    islip_cmp_inst      = record
        next            : pislip_cmp_inst;
        i               : byte;
        arg             : size_t;
    end;

    // instruction container
    islip_cmp_code_cont = class
        public
            m_head      : pislip_cmp_inst;
            m_tail      : pislip_cmp_inst;
            m_count     : size_t;

            constructor create;
            destructor destroy; override;

            // appends the new instruction to the list and returns its index
            procedure append(i : byte; arg : size_t);
            // removes the last element from the list
            procedure chop_tail;
    end;

    islip_compiler          = class
        public
            constructor create(var input : cfile);
            destructor destroy; override;

            // returns false on compilation failure
            function compile : boolean;
            procedure get_products(var c : islip_bytecode; var d : islip_data);
        private
            m_parser    : islip_parser;
            m_vars      : islip_cmp_var_cont;
            m_code      : islip_cmp_code_cont;
            m_done      : boolean;

            function eval_expr : islip_type;
    end;

implementation

const
    LOLCODE_VERSION         = '1.2';    // supported LOLCODE version

type
    islip_cmp_state = (
        CS_UNDEF,       // undefined state, before reading the first token
        CS_START,       // just had HAI, expecting version number or line break
        CS_STATEMENT    // program body, expecting a statement
    );

// ====================================================
// compiler implementation
// ====================================================

constructor islip_compiler.create(var input : cfile);
begin
    m_parser := islip_parser.create(input);
    m_vars := islip_cmp_var_cont.create;
    m_code := islip_cmp_code_cont.create;
    m_done := false;
end;

destructor islip_compiler.destroy;
begin
    m_parser.destroy;
    m_vars.destroy;
    m_code.destroy;
end;

function islip_compiler.eval_expr : islip_type;
var
    token       : string;
    toktype     : islip_parser_token_type;
    sr, sc, op  : size_t;
    temp        : islip_type;
    floatmath   : boolean;
    v           : pislip_var;
    f           : float;
    i           : int;
begin
    eval_expr := VT_UNTYPED;  // i.e. expression is invalid

    if not (m_parser.get_token(token, toktype) and (length(token) > 0)) then begin
        writeln('ERROR: Unexpected end of file');
        exit;
    end;
    
{$IFDEF DEBUG}
    writeln('DEBUG: eval_expr: Token: "', token, '", token type: ', integer(toktype));
{$ENDIF}
    floatmath := false;
    // basic math binary ops: +-*/% min max
    if (token = 'SUM') or (token = 'DIFF') or (token = 'PRODUKT')
        or (token = 'QUOSHUNT') or (token = 'MOD') or (token = 'BIGGR')
        or (token = 'SMALLR') then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: basic math op');
{$ENDIF}
        // choose the opcode
        if token = 'SUM' then
            op := OP_ADD
        else if token = 'DIFF' then
            op := OP_SUB
        else if token = 'PRODUKT' then
            op := OP_MUL
        else if token = 'QUOSHUNT' then
            op := OP_DIV
        else if token = 'MOD' then
            op := OP_MOD
        else if token = 'BIGGR' then
            op := OP_MAX
        else //if token = 'SMALLR' then
            op := OP_MIN;
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'OF' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "OF" expected, but got "', token,
                '" at line ', sr, ', column ', sc);
            exit;
        end;
        // evaluate the first operand
        temp := eval_expr();
        writeln('temp = ', integer(temp));
        if not ((temp = VT_INT) or (temp = VT_FLOAT)) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        floatmath := temp = VT_FLOAT;
        // read the "AN" keyword
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'AN' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "AN" expected, but got "', token,
                '" at line ', sr, ', column ', sc);
            exit;
        end;
        // evaluate the second operand
        temp := eval_expr();
        if not ((temp = VT_INT) or (temp = VT_FLOAT)) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        floatmath := floatmath or (temp = VT_FLOAT);
        // add the instruction
        m_code.append(op, ARG_NULL);
        if floatmath then
            eval_expr := VT_FLOAT
        else
            eval_expr := VT_INT;
        // basic boolean ops: && ==
        // oops! the BOTH keyword can start both an AND and an equality check,
        // thus the separate code block
    end else if token = 'BOTH' then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: BOTH');
{$ENDIF}
        
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        // boolean AND
        if token = 'OF' then begin
            // evaluate first operand
            if eval_expr() <> VT_BOOL then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Invalid operand type to binary math ',
                    'operator at line ', sr, ', column ', sc);
                exit;
            end;
            // read the "AN" keyword
            if not m_parser.get_token(token, toktype) then begin
                writeln('ERROR: Unexpected end of file');
                exit;
            end;
            if token <> 'AN' then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: "AN" expected, but got "', token,
                    '" at line ', sr, ', column ', sc);
                exit;
            end;
            // evaluate second operand
            if eval_expr() <> VT_BOOL then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Invalid operand type to binary math ',
                    'operator at line ', sr, ', column ', sc);
                exit;
            end;
            // add the instruction
            m_code.append(OP_AND, ARG_NULL);
            eval_expr := VT_BOOL;
        // equality check
        end else if token = 'SAEM' then begin
            // evaluate first operand
            if eval_expr() <> VT_BOOL then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Invalid operand type to binary math ',
                    'operator at line ', sr, ', column ', sc);
                exit;
            end;
            // read the "AN" keyword
            if not m_parser.get_token(token, toktype) then begin
                writeln('ERROR: Unexpected end of file');
                exit;
            end;
            if token <> 'AN' then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: "AN" expected, but got "', token,
                    '" at line ', sr, ', column ', sc);
                exit;
            end;
            // evaluate second operand
            if eval_expr() <> VT_BOOL then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Invalid operand type to binary math ',
                    'operator at line ', sr, ', column ', sc);
                exit;
            end;
            // add the instruction
            m_code.append(OP_EQ, ARG_NULL);
            eval_expr := VT_BOOL;
        end else begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "OF" or "SAEM" expected, but got "', token,
            '" at line ', sr, ', column ', sc);
            exit;
        end;
    // basic boolean binary ops: || ^
    end else if (token = 'EITHER') or (token = 'WON') then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: basic boolean op');
{$ENDIF}
        // choose the opcode
        if token = 'EITHER' then
            op := OP_OR
        else //if token = 'WON' then
            op := OP_XOR;
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'OF' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "OF" expected, but got "', token,
                '" at line ', sr, ', column ', sc);
            exit;
        end;
        // evaluate the first operand
        if eval_expr() <> VT_BOOL then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        // read the "AN" keyword
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'AN' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "AN" expected, but got "', token,
                '" at line ', sr, ', column ', sc);
            exit;
        end;
        // evaluate the second operand
        if eval_expr() <> VT_BOOL then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        // add the instruction
        m_code.append(op, ARG_NULL);
        eval_expr := VT_BOOL;
    // inequation
    end else if (token = 'DIFFRINT') then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: inequation');
{$ENDIF}
        // evaluate the first operand
        if eval_expr() <> VT_BOOL then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        // read the "AN" keyword
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'AN' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "AN" expected, but got "', token,
                '" at line ', sr, ', column ', sc);
            exit;
        end;
        // evaluate the second operand
        if eval_expr() <> VT_BOOL then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid operand type to binary math operator ',
                'at line ', sr, ', column ', sc);
            exit;
        end;
        // add the instruction
        m_code.append(OP_NEQ, ARG_NULL);
        eval_expr := VT_BOOL;
    // string constant
    end else if toktype = TT_STRING then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: string constant');
{$ENDIF}
        // create a new var and get it into the list
        new(v);
        v^ := islip_var.create(token);
        // generate the instructions
        m_code.append(OP_PUSH, m_vars.append(v, token));
        eval_expr := VT_STRING;
    // either a variable identifier or a numerical constant
    end else begin
        // we can detect number literals by the first character
        if (token[1] in ['0'..'9']) or (token[1] = '-') then begin
            floatmath := false;
            // check if this is a 
            // don't switch to floating point math unless we find a radix
            for i := 2 to length(token) do begin
                if token[i] = '.' then begin
                    if floatmath then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: Multiple radix characters in ',
                            'numerical constant at line ', sr,
                            ', column ', sc);
                        exit;
                    end;
                    floatmath := true;
                // be a smartass and use a built-in sysutils Pascal function
                end else if not IsDelimiter('0123456789', token, i) then begin
                    m_parser.get_pos(sr, sc);
                    writeln('ERROR: Invalid numerical constant at line ',
                        sr, ', column ', sc);
                    exit;
                end;
            end;
            new(v);
            if floatmath then begin
                try
                    f := StrToFloat(token);
                except
                    on E : Exception do begin
                        m_parser.get_pos(sr, sc);
                        Writeln ('ERROR: Invalid numerical constant at line ',
                            sr, ', column ', sc);
                        exit;
                    end;
                end;
                v^ := islip_var.create(f);
                eval_expr := VT_FLOAT;
            end else begin
                try
                    i := StrToInt(token);
                except
                    on E : Exception do begin
                        m_parser.get_pos(sr, sc);
                        Writeln ('ERROR: Invalid numerical constant at line ',
                            sr, ', column ', sc);
                        exit;
                    end;
                end;
                v^ := islip_var.create(i);
                eval_expr := VT_INT;
            end;
            // generate the instructions
            m_code.append(OP_PUSH, m_vars.append(v, token));
        end else begin
            // check for illegal characters
            if not IsValidIdent(token) then begin
                m_parser.get_pos(sr, sc);
                Writeln ('ERROR: Invalid characters in identifier at line ',
                    sr, ', column ', sc);
                exit;
            end;
        end;
    end;
end;

function islip_compiler.compile : boolean;
var
    token, id   : string;
    toktype     : islip_parser_token_type;
    state       : islip_cmp_state;
    sr, sc      : size_t;
    v           : pislip_cmp_var;
    index       : size_t;
begin
    compile := false;

    // initialization
    state := CS_UNDEF;

    while m_parser.get_token(token, toktype) and (length(token) > 0) do begin
        v := nil;
{$IFDEF DEBUG}
        writeln('DEBUG: Token: "', token, '", state: ', integer(state));
{$ENDIF}
        case state of
            CS_UNDEF:
                if token <> 'HAI' then begin
                    m_parser.get_pos(sr, sc);
                    writeln('ERROR: Unknown token "', token, '" at line ', sr,
                        ', column ', sc);
                    exit;
                end else
                    state := CS_START;
            CS_START:
                if (token <> LOLCODE_VERSION)
                    and not (token[1] in [chr(10), chr(13)]) then begin
                    m_parser.get_pos(sr, sc);
                    writeln('ERROR: Unsupported language version "', token,
                        '" at line ', sr, ', column ', sc);
                    exit;
                end else
                    state := CS_STATEMENT;
            CS_STATEMENT:
                // comment
                // FIXME: move comment handling to parser (ya, rly!)
                if token = 'BTW' then begin
                    // skip everything until a newline comes up
                    while true do begin
                        if not m_parser.get_token(token, toktype) then begin
                            writeln('ERROR: Unexpected end of file');
                            exit;
                        end;
                        if (length(token) = 1)
                            and (token[1] in [chr(10), chr(13)]) then
                            break;
                    end;
                    continue;
                // module inclusion
                end else if token = 'CAN' then begin
                    // fetch next token
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    if token <> 'HAS' then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: "HAS" expected, but got "', token,
                            '" at line ', sr, ', column ', sc);
                        exit;
                    end else begin
                        // fetch next token
                        if not m_parser.get_token(token, toktype) then begin
                            writeln('ERROR: Unexpected end of file');
                            exit;
                        end;
                        if token[length(token)] = '?' then begin
                            // remove the question mark
                            delete(token, length(token), 1);
                            // see if we recognize the library
                            // FIXME: allow inlcusion of third-party files
                            if (token <> 'STDIO') and (token <> 'MAHZ') then
                                begin
                                writeln('ERROR: Unknown module "', token, '"');
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
                    state := CS_COND}
                else if token = 'LOL' then begin
                    // fetch next token
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    // check if it's a valid identifier
                    if not IsValidIdent(token) then begin
                        m_parser.get_pos(sr, sc);
                        writeln ('ERROR: Invalid characters in identifier at ',
                            'line ', sr, ', column ', sc);
                        exit;
                    end;
                    // save off the ID
                    id := token;
                    // now see if it hasn't been used already
                    // try variables first
                    v := m_vars.get_var(id);
                    // now try functions - if we find anything, it's a misused
                    // identifier and therefore a syntax error
                    // TODO
                    {if m_blocks.get_func(token) <> nil then begin
                    end;}
                    // read the next part of the assignment statement
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    if token <> 'R' then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: "R" expected, but got "', token,
                            '" at line ', sr, ', column ', sc);
                        exit;
                    end;
                    // save off the instruction count
                    sc := m_code.m_count;
                    // evaluate the expression
                    if eval_expr = VT_UNTYPED then begin
                        // a variable must be assigned *something*
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: Unable to evaluate expression at ',
                            'line ', sr, ', column ', sc);
                        exit;
                    end;
                    // if the variable already exists, it's been initialized
                    // somewhere else already, so just pop the top of the stack
                    // into it
                    if v <> nil then
                        // FIXME: put index into islip_cmp_var
                        m_code.append(OP_POP, m_vars.get_var_index(v^.id))
                    else begin
                        // if the expression generates only 1 instruction, it
                        // must be a constant, so let's reduce instructions
                        if m_code.m_count - sc = 1 then begin
                            m_code.chop_tail;
                            m_vars.m_tail^.id := id;
                        end;
                    end;                    
                end else if token = 'VISIBLE' then begin
                    // put the value on the stack and print it
                    {if not m_parser.get_token(token, toktype)
                        or ((length(token) = 1)
                        and (token[1] in [chr(10), chr(13)])) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;}
{$IFDEF DEBUG}
                    writeln('DEBUG: Printing: "', token, '", state: ',
                        integer(state));
{$ENDIF}
                    // FIXME: rewrite this to use expression evaluation ONLY
                    //if toktype = TT_EXPR then begin
                        // TODO: print variables and evaluate expressions
                        // evaluate everything until a newline
                        // get the cursor position in case an error appears
                        m_parser.get_pos(sr, sc);
                        if eval_expr = VT_UNTYPED then begin
                            writeln('ERROR: Unable to evaluate expression at ',
                                'line ', sr, ', column ', sc);
                            exit;
                        end;
                        m_code.append(OP_TRAP, TRAP_PRINT);
                        m_code.append(OP_POP, ARG_NULL);
                    {end else begin
                        // shortcut - create a new var and get it into the list
                        new(v);
                        v^ := islip_var.create(token);
                        index := m_vars.append(v, token);
                        // generate the instructions
                        m_code.append(OP_PUSH, index);
                        m_code.append(OP_TRAP, TRAP_PRINT);
                        m_code.append(OP_POP, ARG_NULL);
                    end;}
                    // add a line feed
                    m_code.append(OP_TRAP, TRAP_LINEFEED);
                    continue;
                // program end
                end else if token = 'KTHXBYE' then begin
                    // add a STOP and end parsing
                    m_code.append(OP_STOP, ARG_NULL);
                    token := '';
                    break;
                // skip newlines
                end else if (length(token) = 1)
                    and (token[1] in [chr(10), chr(13)]) then
                    continue
                else begin
                    m_parser.get_pos(sr, sc);
                    writeln('ERROR: Unknown keyword "', token, '" at line ', sr,
                        ', column ', sc);
                    exit;
                end;
        end;
    end;
    if length(token) > 0 then begin
        writeln('ERROR: Unable to parse script');
        compile := false;
        exit;
    end;
    m_done := true;
    compile := true;
end;

procedure islip_compiler.get_products(var c : islip_bytecode;
    var d : islip_data);
var
    pi  : pislip_cmp_inst;
    pv  : pislip_cmp_var;
    i   : int;
begin
    if not m_done then begin
        writeln('ERROR: Attempted to retrieve compilation products before ',
            'compilation');
        exit;
    end;
    
    // resize the arrays accordingly
    setlength(c, integer(m_code.m_count));
    setlength(d, integer(m_vars.m_index));
    // copy the stuff from the lists to the arrays
    i := 1;
    pv := m_vars.m_head;
    while pv <> nil do begin
        d[i] := pv^.v^;
        pv := pv^.next;
        inc(i);
    end;
    i := 1;
    pi := m_code.m_head;
    while pi <> nil do begin
        c[i].inst := pi^.i;
        c[i].arg := pi^.arg;
        pi := pi^.next;
        inc(i);
    end;

end;

// ====================================================
// variable container implementation
// ====================================================

constructor islip_cmp_var_cont.create;
begin
    m_head := nil;
    m_tail := nil;
    m_index := 0;
end;

destructor islip_cmp_var_cont.destroy;
var
    p1, p2  : pislip_cmp_var;
begin
    p1 := m_head;
    while p1 <> nil do begin
        p2 := p1;
        p1 := p1^.next;
        dispose(p2^.v);
        dispose(p2);
    end;
end;

function islip_cmp_var_cont.append(v : pislip_var; id : string) : size_t;
var
    p   : pislip_cmp_var;
begin
    inc(m_index);
    append := m_index;
    new(p);
    p^.v := v;
    p^.id := id;
    p^.next := nil;
    if m_head = nil then begin
        m_head := p;
        m_tail := p;
    end else begin
        m_tail^.next := p;
        m_tail := p;
    end;
end;

function islip_cmp_var_cont.get_var_index(id : string) : size_t;
var
    p   : pislip_cmp_var;
    i   : size_t;
begin
    get_var_index := ARG_NULL;
    i := 1;
    p := m_head;
    while p <> nil do begin
        if p^.id = id then begin
            get_var_index := i;
            exit;
        end;
        p := p^.next;
        inc(i);
    end;
end;

function islip_cmp_var_cont.get_var(id : string) : pislip_cmp_var;
var
    p   : pislip_cmp_var;
    i   : size_t;
begin
    get_var := nil;
    i := 0;
    p := m_head;
    while p <> nil do begin
        if p^.id = id then begin
            get_var := p;
            exit;
        end;
        p := p^.next;
        inc(i);
    end;
end;

// ====================================================
// instruction container implementation
// ====================================================

constructor islip_cmp_code_cont.create;
begin
    m_head := nil;
    m_tail := nil;
    m_count := 0;
end;

destructor islip_cmp_code_cont.destroy;
var
    p1, p2  : pislip_cmp_inst;
begin
    p1 := m_head;
    while p1 <> nil do begin
        p2 := p1;
        p1 := p1^.next;
        dispose(p2);
    end;
end;

procedure islip_cmp_code_cont.append(i : byte; arg : size_t);
var
    p   : pislip_cmp_inst;
begin
    inc(m_count);
    new(p);
    p^.i := i;
    p^.arg := arg;
    p^.next := nil;
    if m_head = nil then begin
        m_head := p;
        m_tail := p;
    end else begin
        m_tail^.next := p;
        m_tail := p;
    end;
end;

procedure islip_cmp_code_cont.chop_tail;
var
    p   : pislip_cmp_inst;
begin
    if m_tail <> nil then begin
        p := m_head;
        while p^.next <> nil do
            p := p^.next;
        if p <> nil then
            p^.next := nil;
        dispose(m_tail);
        if m_head = m_tail then
            m_head := nil;
        m_tail := p;    // this would be nil anyway
        dec(m_count);
    end;
end;

end.