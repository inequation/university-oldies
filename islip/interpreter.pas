// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Bytecode interpreter unit

unit interpreter;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs, variable, bytecode, SysUtils;

type
    islip_stack         = class
        public
            constructor create(size : size_t);
            destructor destroy; override;

            procedure push(pv : pislip_var);
            procedure pop(pv : pislip_var);
            function peek : pislip_var;
        private
            m_stack     : array of islip_var;
            m_top       : integer;
    end;

    islip_interpreter   = class
        public
            constructor create(stack_size : size_t; var c : islip_bytecode;
                var d : islip_data);
            destructor destroy; override;

            procedure run;
        private
            m_stack     : islip_stack;
            m_code      : pislip_bytecode;
            m_data      : pislip_data;
    end;

implementation

// ====================================================
// interpreter implementation
// ====================================================

constructor islip_interpreter.create(stack_size : size_t;
    var c : islip_bytecode; var d : islip_data);
begin
    m_stack := islip_stack.create(stack_size);
    m_code := @c;
    m_data := @d;
end;

destructor islip_interpreter.destroy;
begin
    m_stack.destroy;
end;

procedure islip_interpreter.run;
var
    i   : size_t;
    pv  : pislip_var;
    v   : islip_var;
    s   : string;
begin
    i := 1;
    v := islip_var.create;
    while i < size_t(length(m_code^)) do begin
        case m_code^[i].inst of
            OP_STOP:
                break;
            OP_PUSH:
                if m_code^[i].arg = ARG_NULL then
                    m_stack.push(nil)
                else
                    m_stack.push(@m_data^[m_code^[i].arg]);
            OP_POP:
                if m_code^[i].arg = ARG_NULL then
                    m_stack.pop(nil)
                else
                    m_stack.pop(@m_data^[m_code^[i].arg]);
            OP_ADD,
            OP_SUB,
            OP_MUL,
            OP_DIV,
            OP_MOD,
            OP_MIN,
            OP_MAX:
                // pop the top-most value off the stack, write the result
                // into the second one
                begin
                    m_stack.pop(@v);
                    pv := m_stack.peek;
                    if not pv^.math(@v, m_code^[i].inst) then begin
                        writeln('RUNTIME ERROR: Invalid operands to ',
                            'arithmetic operator at 0x', IntToHex(i, 8));
                        v.destroy;
                        exit;
                    end;
                end;
            OP_NEG:
                begin
                    pv := m_stack.peek;
                    pv^.logic(nil, OP_NEG);
                end;
            OP_AND,
            OP_OR,
            OP_XOR,
            OP_EQ,
            OP_NEQ:
                // pop the top-most value off the stack, write the result
                // into the second one
                begin
                    m_stack.pop(@v);
                    pv := m_stack.peek;
                    if not pv^.logic(@v, m_code^[i].inst) then begin
                        writeln('RUNTIME ERROR: Invalid operands to ',
                            'logical operator at 0x', IntToHex(i, 8));
                        v.destroy;
                        exit;
                    end;
                end;
            OP_CONCAT:
                begin
                    m_stack.pop(@v);
                    pv := m_stack.peek;
                    pv^.concat(@v);
                end;
            OP_PRINT:
                begin
                    m_stack.pop(@v);
                    if m_code^[i].arg = 1 then begin
                        v.echo(true);
                        writeln(stderr);
                    end else begin
                        v.echo(false);
                        writeln;
                    end;
                end;
            OP_READ:
                begin
                    readln(s);
                    new(pv);
                    pv^ := islip_var.create(s);
                    m_stack.push(pv);
                end;
            OP_CAST:
                (m_stack.peek)^.cast(islip_type(m_code^[i].arg));
            OP_JMP:
                begin
                    i := m_code^[i].arg;
                    continue;
                end;
            OP_CNDJMP:
                begin
                    m_stack.pop(@v);
                    // NOTE: jump occurs if the top of the stack variable is
                    // FALSE! this is because the success block follows
                    // immediately, while the "else" block (or further code, if
                    // there is no "else") appears after the success block
                    if not v.get_bool then begin
                        i := m_code^[i].arg;
                        continue;
                    end;
                end;
            OP_INCR:
                begin
                    pv := m_stack.peek;
                    v.destroy;
                    if m_code^[i].arg = 0 then
                        v := islip_var.create(-1)
                    else
                        v := islip_var.create(1);
                    pv^.math(@v, OP_ADD);
                    v.reset_value;
                end;
            else begin
                writeln('ERROR: Invalid instruction 0x',
                    IntToHex(m_code^[i].inst, 2), ', possibly corrupt bytecode ',
                    'or unimplemented instruction');
                v.destroy;
                exit;
            end;
        end;
        inc(i);
    end;
    v.destroy;
end;

// ====================================================
// stack implementation
// ====================================================

constructor islip_stack.create(size : size_t);
var
    i   : size_t;
begin
    setlength(m_stack, size);
    for i := 1 to size do
        m_stack[i] := islip_var.create;
    m_top := 0;
end;

destructor islip_stack.destroy;
var
    i   : size_t;
begin
    for i := 1 to length(m_stack) do
        m_stack[i].destroy;
end;

procedure islip_stack.push(pv : pislip_var);
var
    i   : size_t;
begin
    if m_top = length(m_stack) then begin
        //writeln('ERROR: Stack overflow');
        //exit;
        setlength(m_stack, length(m_stack) * 2);
        // initialize the new part of the stack
        for i := length(m_stack) div 2 + 1 to length(m_stack) do
            m_stack[i] := islip_var.create;
    end;
    inc(m_top);
    if pv <> nil then
        m_stack[m_top].copy(pv)
    else
        m_stack[m_top] := islip_var.create;
end;

procedure islip_stack.pop(pv : pislip_var);
begin
    if m_top <= 0 then begin
        writeln('RUNTIME ERROR: Stack underflow');
        exit;
    end;
    if pv <> nil then begin
        pv^.copy(@m_stack[m_top]);
    end;
    dec(m_top);
end;

function islip_stack.peek : pislip_var;
begin
    if m_top <= 0 then begin
        writeln('RUNTIME ERROR: Peeking into an empty stack');
        exit;
    end;
    peek := @m_stack[m_top];
end;

end.