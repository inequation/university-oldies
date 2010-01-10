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

uses
  typedefs,
  parser,
  bytecode,
  variable,
  convert,
  SysUtils;

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
        arg             : int;
    end;

    // instruction container
    islip_cmp_code_cont = class
        public
            m_head      : pislip_cmp_inst;
            m_tail      : pislip_cmp_inst;
            m_count     : size_t;

            constructor create;
            destructor destroy; override;

            // appends the new instruction to the list and returns its pointer
            function append(i : byte; arg : size_t) : pislip_cmp_inst;
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

            // evaluates the upcoming statement
            function eval_statement : boolean;
            // evaluates the upcoming expression
            function eval_expr : boolean;
            // parses a number or boolean literal
            function literal(token : string) : boolean;

            // optimizes some instruction combinations
            procedure optimize(start : pislip_cmp_inst);
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

    islip_flowstate = (
        FS_LINEAR,      // we're reading a normal, linear code block
        FS_IF,          // we're reading an "if" block
        FS_ELSEIF,      // we're reading an "else if" block
        FS_ELSE,        // we're reading an "else" block
        FS_SWITCH       // we're reading a switch block
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

function islip_compiler.literal(token : string) : boolean;
var
    floatmath   : boolean;
    sr, sc      : size_t;
    f           : float;
    i           : int;
    pv          : pislip_var;
begin
    literal := false;
    // we can detect number literals by the first character
    if not (token[1] in ['0'..'9']) or (token[1] = '-') then begin
        // it can only be either a boolean literal or a syntax error now
        if token = 'WIN' then begin
{$IFDEF DEBUG}
            writeln('DEBUG: boolean literal');
{$ENDIF}
            new(pv);
            pv^ := islip_var.create(true);
            m_code.append(OP_PUSH, m_vars.append(pv, token));
            literal := true;
        end else if token = 'FAIL' then begin
{$IFDEF DEBUG}
            writeln('DEBUG: boolean literal');
{$ENDIF}
            new(pv);
            pv^ := islip_var.create(false);
            m_code.append(OP_PUSH, m_vars.append(pv, token));
            literal := true;
        end;
        exit;
    end;
{$IFDEF DEBUG}
    writeln('DEBUG: number literal');
{$ENDIF}
    floatmath := false;
    // don't switch to floating point math unless we find a radix
    for i := 2 to length(token) do begin
        if token[i] = '.' then begin
            if floatmath then begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Multiple radix characters in number literal at',
                    ' line ', sr, ', column ', sc);
                exit;
            end;
            floatmath := true;
        end else if not (token[i] in ['0'..'9']) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid number literal at line ', sr,
                ', column ', sc);
            exit;
        end;
    end;
    new(pv);
    if floatmath then begin
        if not atof(token, @f) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid number literal at line ', sr,
                ', column ', sc);
            exit;
        end;
        pv^ := islip_var.create(f);
    end else begin
        try
            i := StrToInt(token);
        except
            on E : Exception do begin
                m_parser.get_pos(sr, sc);
                writeln('ERROR: Invalid number literal at line ', sr,
                    ', column ', sc);
                exit;
            end;
        end;
        pv^ := islip_var.create(i);
    end;    
    // generate the instructions
    m_code.append(OP_PUSH, m_vars.append(pv, token));
    literal := true;
end;

// ====================================================
// recursive expression evaluator
// ====================================================

function islip_compiler.eval_expr : boolean;
var
    token       : string;
    toktype     : islip_parser_token_type;
    sr, sc, op  : size_t;
    v           : pislip_var;
    mo          : boolean;
    i           : int;
begin
    eval_expr := false;  // i.e. expression is invalid

    if not (m_parser.get_token(token, toktype) and (length(token) > 0)) then
        begin
        writeln('ERROR: Unexpected end of file');
        exit;
    end;
    
{$IFDEF DEBUG}
    writeln('DEBUG: eval_expr: Token: "', token, '", token type: ', integer(toktype));
{$ENDIF}
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
        if not eval_expr() then
            exit;
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
        if not eval_expr() then
            exit;
        // add the instruction
        m_code.append(op, ARG_NULL);
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
            if not eval_expr() then
                exit;
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
            if not eval_expr() then
                exit;
            // add the instruction
            m_code.append(OP_AND, ARG_NULL);
        // equality check
        end else if token = 'SAEM' then begin
            // evaluate first operand
            if not eval_expr() then
                exit;
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
            if not eval_expr() then
                exit;
            // add the instruction
            m_code.append(OP_EQ, ARG_NULL);
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
        if not eval_expr() then
            exit;
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
        if not eval_expr() then
            exit;
        // add the instruction
        m_code.append(op, ARG_NULL);
    // inequation
    end else if token = 'DIFFRINT' then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: inequation');
{$ENDIF}
        // evaluate the first operand
        if not eval_expr() then
            exit;
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
        if not eval_expr() then
            exit;
        // add the instruction
        m_code.append(OP_NEQ, ARG_NULL);
    // negation
    end else if token = 'NOT' then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: negation');
{$ENDIF}
        // evaluate the operand
        if not eval_expr() then
            exit;
        m_code.append(OP_NEG, ARG_NULL);
    // string concatenation
    end else if token = 'SMOOSH' then begin
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: string concatenation');
{$ENDIF}
        // do we have more than 1 operand?
        mo := false;
        while true do begin
            if not m_parser.get_token(token, toktype) then begin
                writeln('ERROR: Unexpected end of file');
                exit;
            end;
            if ((toktype = TT_EXPR) and (token = 'MKAY'))
                or ((length(token) = 1) and (token[1] in [chr(10), chr(13)]))
                then
                break;
            m_parser.unget_token;
            if not eval_expr() then
                exit;
            // only append the instruction if have anything to concat
            if mo then
                m_code.append(OP_CONCAT, ARG_NULL)
            else
                mo := true;
        end;
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
    // either a variable identifier, or a number or boolean literal
    end else if not literal(token) then begin
        // check for illegal characters
        if not IsValidIdent(token) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid characters in identifier at line ',
                sr, ', column ', sc);
            exit;
        end;
{$IFDEF DEBUG}
        writeln('DEBUG: eval_expr: identifier');
{$ENDIF}
        // FIXME: search for functions, too
        i := m_vars.get_var_index(token);
        if i = 0 then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Unknown identifier "', token, '" at line ',
                sr, ', column ', sc);
            exit;
        end;
        // generate the instruction
        m_code.append(OP_PUSH, i);
    end;
    eval_expr := true;
end;

// ====================================================
// recursive statement evaluator
// ====================================================

function islip_compiler.eval_statement : boolean;
var
    token, id   : string;
    toktype     : islip_parser_token_type;
    sr, sc      : size_t;
    v           : pislip_cmp_var;
    pv          : pislip_var;
    i           : pislip_cmp_inst;
    ofs         : int;
    flowstate   : islip_flowstate;
begin
    eval_statement := false;

    if not (m_parser.get_token(token, toktype) and (length(token) > 0)) then begin
        writeln('ERROR: Unexpected end of file');
        exit;
    end;

    // skip newlines
    if (length(token) = 1)
        and (token[1] in [chr(10), chr(13)]) then begin
        eval_statement := true;
        exit;
    end;
    
    v := nil;
{$IFDEF DEBUG}
    writeln('DEBUG: eval_statement: Token: "', token, '", type: ', int(toktype));
{$ENDIF}
    if token = 'CAN' then begin
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
        end;
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
            if (token <> 'STDIO') and (token <> 'MAHZ') then begin
                writeln('ERROR: Unknown module "', token, '"');
                exit;
            end else
{$IFDEF DEBUG}
                writeln('DEBUG: Including module "', token, '"');
{$ENDIF}
        end;
    // conditional
    end else if token = 'O' then begin
        // fetch next token
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'RLY?' then begin
            m_parser.get_pos(sr, sc);
            writeln ('ERROR: "RLY?" expected, but got "', token, '" at ',
                 'line ', sr, ', column ', sc);
            exit;
        end;
        // fetch next token; there may be newlines before it
        while true do begin
            if not m_parser.get_token(token, toktype) then begin
                writeln('ERROR: Unexpected end of file');
                exit;
            end;
            if (length(token) = 1) and (token[1] in [chr(10), chr(13)]) then
                continue
            else if token <> 'YA' then begin
                m_parser.get_pos(sr, sc);
                writeln ('ERROR: "YA" expected, but got "', token, '" at ',
                    'line ', sr, ', column ', sc);
                exit;
            end else
                break;
        end;
        // fetch next token
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'RLY' then begin
            m_parser.get_pos(sr, sc);
            writeln ('ERROR: "RLY" expected, but got "', token, '" at ',
                'line ', sr, ', column ', sc);
            exit;
        end;
        // push IT onto the stack
        m_code.append(OP_PUSH, 1);
        // save off the instruction information; jump offset will be updated
        // later on
        i := m_code.append(OP_CNDJMP, ARG_NULL);
        ofs := int(m_code.m_count);
        // evaluate statements until we hit another flow control statement
        flowstate := FS_IF;
        while m_parser.get_token(token, toktype) do begin
            // check for the end of the block
            if token = 'OIC' then
                flowstate := FS_LINEAR;
            // this is intentional - there SHOULD NOT be an "else" here!
            if flowstate = FS_LINEAR then begin
                // optimize code and update jump offset
                optimize(i);
                i^.arg := int(m_code.m_count) - ofs;
                break;  // end of conditional block altogether
            end else if flowstate in [FS_IF, FS_ELSEIF] then begin
                // elseif
                if token = 'MEBBE' then begin
                    // optimize the code so far and update the jump offset
                    // FIXME: add an unconditional jump at the end of every
                    // success block so that we don't evaluate every damn
                    // single condition
                    optimize(i);
                    i^.arg := int(m_code.m_count) - ofs;
                    // set next state
                    flowstate := FS_ELSEIF;
                    // evaluate the condition
                    if not eval_expr() then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: Unable to evaluate expression at ',
                            'line ', sr, ', column ', sc);
                        exit;
                    end;
                    // save off the instruction information; jump offset will
                    // be updated later on
                    i := m_code.append(OP_CNDJMP, ARG_NULL);
                    ofs := int(m_code.m_count);
                    continue;
                // else
                end else if (toktype = TT_EXPR) and (token = 'NO') then
                    begin
                    // fetch next token
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    if token <> 'WAI' then begin
                        m_parser.get_pos(sr, sc);
                        writeln ('ERROR: "WAI" expected, but got "', token,
                            '" at line ', sr, ', column ', sc);
                        exit;
                    end;
                    // optimize the code so far and update the jump offset
                    // the +1 accounts for the unconditional jump we're about
                    // to add; it's there to skip the "else" block and we
                    // absolutely need it because there's no condition that can
                    // be checked
                    optimize(i);
                    i^.arg := int(m_code.m_count) - ofs + 1;
                    // set next state
                    flowstate := FS_ELSE;
                    // save off the instruction information; jump offset will
                    // be updated later on
                    i := m_code.append(OP_JMP, ARG_NULL);
                    ofs := int(m_code.m_count);
                    continue;
                end;
            end;
            // otherwise just parse the statement
            m_parser.unget_token;
            if not eval_statement() then
                exit;   // the recursive instance will have raised an error
        end;
        if flowstate <> FS_LINEAR then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
    // switch
    end else if token = 'WTF?' then begin
        flowstate := FS_LINEAR;
        while m_parser.get_token(token, toktype) do begin
            // check for the end of the block
            if token = 'OIC' then begin
                // if we haven't entered a switch statement yet, i and ofs will
                // be invalid
                if flowstate <> FS_LINEAR then begin
                    // optimize code and update jump offset
                    optimize(i);
                    i^.arg := int(m_code.m_count) - ofs;
                end;
                break; // end of block
            end;
            if flowstate in [FS_LINEAR, FS_SWITCH] then begin
                if token = 'OMG' then begin
                    // if we've come here from a switch block, finish it up
                    if flowstate = FS_SWITCH then begin
                        // optimize the code so far and update the jump offset
                        // FIXME: add an unconditional jump at the end of every
                        // switch block so that we don't evaluate every damn
                        // single condition
                        optimize(i);
                        i^.arg := int(m_code.m_count) - ofs;
                    end else
                        // set new state
                        flowstate := FS_SWITCH;
                    // push IT onto the stack
                    m_code.append(OP_PUSH, 1);
                    // read literal
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    if toktype = TT_STRING then begin
                        new(pv);
                        pv^ := islip_var.create(token);
                        m_code.append(OP_PUSH, m_vars.append(pv, token));
                    end else if not literal(token) then begin
                        m_parser.get_pos(sr, sc);
                        writeln ('ERROR: Literal expected, but got "', token,
                            '" at line ', sr, ', column ', sc);
                        exit;
                    end;
                    // condition check
                    m_code.append(OP_EQ, ARG_NULL);
                    // save off the instruction information; jump offset will
                    // be updated later on
                    i := m_code.append(OP_CNDJMP, ARG_NULL);
                    ofs := int(m_code.m_count);
                    continue;
                end else if (flowstate = FS_SWITCH) and (token = 'OMGWTF') then begin
                    // optimize the code so far and update the jump offset
                    // the +1 accounts for the unconditional jump we're about
                    // to add; it's there to skip the "else" block and we
                    // absolutely need it because there's no condition that can
                    // be checked
                    optimize(i);
                    i^.arg := int(m_code.m_count) - ofs + 1;
                    // set next state
                    flowstate := FS_ELSE;
                    // save off the instruction information; jump offset will
                    // be updated later on
                    i := m_code.append(OP_JMP, ARG_NULL);
                    ofs := int(m_code.m_count);
                    continue;
                end;
            end;
            // just parse the statement
            m_parser.unget_token;
            if not eval_statement() then
                exit;   // the recursive instance will have raised an error
        end;
    // variable assignation
    end else if token = 'LOL' then begin
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
        if not eval_expr() then begin
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
            // else we need to calculate the result, so allocate
            // a new data slot
            end else begin
                new(pv);
                pv^ := islip_var.create;
                m_code.append(OP_POP, m_vars.append(pv, id));
            end;
        end;
    end else if token = 'MAEK' then begin
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        // check for illegal characters
        if not IsValidIdent(token) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid characters in identifier at line ',
                sr, ', column ', sc);
            exit;
        end;
        // fail if the variable doesn't exist
        sr := m_vars.get_var_index(token);
        if sr = 0 then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Unknown identifier "', token, '" at line ',
                sr, ', column ', sc);
            exit;
        end;
        // check for "A"
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        if token <> 'A' then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: "A" expected, but got "', token, '" at line ',
                sr, ', column ', sc);
            exit;
        end;
        // check for type
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        m_code.append(OP_PUSH, sr);
        if token = 'NOOB' then
            m_code.append(OP_CAST, int(VT_UNTYPED))
        else if token = 'TROOF' then
            m_code.append(OP_CAST, int(VT_BOOL))
        else if token = 'NUMBR' then
            m_code.append(OP_CAST, int(VT_INT))
        else if token = 'NUMBAR' then
            m_code.append(OP_CAST, int(VT_FLOAT))
        else if token = 'YARN' then
            m_code.append(OP_CAST, int(VT_STRING))
        else begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Expected a type name, but got "', token, '" at line ',
                sr, ', column ', sc);
            exit;
        end;
        m_code.append(OP_POP, sr);
    end else if token = 'VISIBLE' then begin
        // put the value on the stack and print it
        if not eval_expr() then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Unable to evaluate expression at ',
                'line ', sr, ', column ', sc);
            exit;
        end;
        m_code.append(OP_PRINT, ARG_NULL);
    end else if token = 'GIMMEH' then begin
        if not m_parser.get_token(token, toktype) then begin
            writeln('ERROR: Unexpected end of file');
            exit;
        end;
        // check for illegal characters
        if not IsValidIdent(token) then begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Invalid characters in identifier at line ',
                sr, ', column ', sc);
            exit;
        end;
        // if the variable doesn't exist, create it
        sr := m_vars.get_var_index(token);
        if sr = ARG_NULL then begin
            new(pv);
            pv^ := islip_var.create;
            sr := m_vars.append(pv, token);
        end;
        m_code.append(OP_READ, ARG_NULL);
        m_code.append(OP_POP, sr);
    end else begin
        m_parser.unget_token;
        // try to evaluate the "IT" implied variable, which is
        // always on slot #1
        if eval_expr then
            m_code.append(OP_POP, 1)
        else begin
            m_parser.get_pos(sr, sc);
            writeln('ERROR: Unknown keyword "', token, '" at line ', sr,
                ', column ', sc);
            exit;
        end;
    end;
    eval_statement := true;
end;

function islip_compiler.compile : boolean;
var
    token   : string;
    toktype : islip_parser_token_type;
    state   : islip_cmp_state;
    sr, sc  : size_t;
    pv      : pislip_var;
    bye     : boolean;
begin
    compile := false;
    bye := false;

    // initialization
    state := CS_UNDEF;

    // create the implied "IT" variable
    new(pv);
    pv^ := islip_var.create;
    m_vars.append(pv, 'IT');

    while m_parser.get_token(token, toktype) and (length(token) > 0) do begin
{$IFDEF DEBUG}
        writeln('DEBUG: compile: Token: "', token, '" type ', int(toktype), ', state: ', int(state));
{$ENDIF}
            
        case state of
            CS_UNDEF:
                begin
                    if token <> 'HAI' then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: Unknown token "', token, '" at line ', sr,
                            ', column ', sc);
                        exit;
                    end;
                    if not m_parser.get_token(token, toktype) then begin
                        writeln('ERROR: Unexpected end of file');
                        exit;
                    end;
                    // the version number might've been omitted
                    if (length(token) = 1)
                        and (token[1] in [chr(10), chr(13)]) then begin
                        state := CS_STATEMENT;
                        continue;
                    end;
                    if token <> LOLCODE_VERSION then begin
                        m_parser.get_pos(sr, sc);
                        writeln('ERROR: Unsupported language version "', token,
                            '" at line ', sr, ', column ', sc);
                        exit;
                    end;
                    state := CS_STATEMENT;
                end;
            CS_STATEMENT:
                begin
                    // end of program
                    if token = 'KTHXBYE' then begin
                        m_code.append(OP_STOP, ARG_NULL);
                        token := '';
                        bye := true;
                        break;
                    end;
                    // unget token for reevaluation
                    m_parser.unget_token;
                    if not eval_statement then begin
                        //writeln('ERROR: Unable to parse script');
                        compile := false;
                        exit;
                    end;
                end;
        end;
    end;
    if not bye then begin
        writeln('ERROR: Script is missing termination ("KTHXBYE")');
        compile := false;
        exit;
    end;
    // run optimizations
    optimize(m_code.m_head);
    m_done := true;
    compile := true;
end;

// FIXME: try to make it context-sensitive so that it doesn't break code
procedure islip_compiler.optimize(start : pislip_cmp_inst);
{var
    prev, temp  : pislip_cmp_inst;}
begin
    {prev := nil;
    repeat
        // unlink contiguous pops and pushes of the same variable
        if (start^.next <> nil) and (start^.i = OP_POP)
            and (start^.next^.i = OP_PUSH) and (start^.arg = start^.next^.arg)
            then begin
            // unlink the instructions
            if prev <> nil then
                prev^.next := start^.next^.next
            else
                m_code.m_head := start^.next^.next;
            temp := start;
            start := start^.next^.next;
            dispose(temp^.next);
            dispose(temp);
            dec(m_code.m_count, 2);
        end else begin
            prev := start;
            start := start^.next;
        end;
    until start = nil;}
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
        //p2^.v^.destroy;
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

function islip_cmp_code_cont.append(i : byte; arg : size_t) : pislip_cmp_inst;
var
    p   : pislip_cmp_inst;
begin
    inc(m_count);
    new(p);
    //writeln('append ', i, ' ', arg, '! ', m_count, ' ', size_t(m_head), ' ', size_t(m_tail), ' ', size_t(p));
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
    append := p;
end;

procedure islip_cmp_code_cont.chop_tail;
var
    p   : pislip_cmp_inst;
begin
    //writeln('chop! ', m_count, ' ', size_t(m_head), ' ', size_t(m_tail));
    if m_tail = nil then
        exit;
    dec(m_count);
    if m_head = m_tail then begin
        m_head := nil;
        dispose(m_tail);
        m_tail := nil;
        exit;
    end;
    p := m_head;
    while p^.next <> m_tail do
        p := p^.next;
    dispose(p^.next);
    m_tail := p;
end;

end.