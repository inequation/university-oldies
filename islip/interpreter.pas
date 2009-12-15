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
            //destructor destroy; override;

            procedure push(pv : pislip_var);
            procedure pop(pv : pislip_var);
            function peek : pislip_var;
        private
            m_stack     : array of islip_var;
            m_top       : size_t;
    end;

    islip_interpreter   = class
        public
            constructor create(stack_size : size_t; var c : islip_bytecode;
                var d : islip_data);
            //destructor destroy; override;

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

procedure islip_interpreter.run;
var
    i   : int;
    pv  : pislip_var;
begin
    for i := 1 to length(m_code^) do begin
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
            OP_TRAP:
                case m_code^[i].arg of
                    TRAP_PRINT:
                        begin
                            pv := m_stack.peek;
                            pv^.echo;
                        end;
                    TRAP_LINEFEED:
                        writeln;
                end;
            else begin
                writeln('ERROR: Invalid instruction 0x',
                    IntToHex(m_code^[i].inst, 2), ', possibly corrupt bytecode ',
                    'or unimplemented instruction');
                exit;
            end;
        end;
    end;
end;

// ====================================================
// stack implementation
// ====================================================

constructor islip_stack.create(size : size_t);
begin
    setlength(m_stack, size);
    m_top := 0;
end;

procedure islip_stack.push(pv : pislip_var);
begin
    if m_top = length(m_stack) then begin
        writeln('ERROR: Stack overflow');
        exit;
    end;
    inc(m_top);
    if pv <> nil then
        m_stack[m_top] := pv^
    else // FIXME!!!
        m_stack[m_top] := islip_var.create('');
end;

procedure islip_stack.pop(pv : pislip_var);
begin
    if m_top <= 0 then begin
        writeln('ERROR: Stack underflow');
        exit;
    end;
    if pv <> nil then
        pv^ := m_stack[m_top];
    dec(m_top);
end;

function islip_stack.peek : pislip_var;
begin
    peek := @m_stack[m_top];
end;

end.