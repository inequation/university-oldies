// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Bytecode interpreter unit

unit interpreter;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs, variable, bytecode;

type
    islip_stack         = class
        public
            constructor create(size : size_t);
            //destructor destroy; override;

            procedure push(pv : pislip_var);
            procedure pop(pv : pislip_var);
        private
            m_stack     : array of islip_var;
            m_top       : size_t;
    end;

    islip_interpreter   = class
        public
            constructor create(stack_size : size_t; var c : islip_bytecode; var d : islip_data);
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

constructor islip_interpreter.create(stack_size : size_t; var c : islip_bytecode; var d : islip_data);
begin
    m_stack := islip_stack.create(stack_size);
    m_code := @c;
    m_data := @d;
end;

procedure islip_interpreter.run;
var
    i   : int;
begin
    for i := low(m_code^) to high(m_code^) do begin
        case m_code^[i].inst of
            BI_STOP:
                break;
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
    inc(m_top);
    m_stack[m_top] := pv^;
end;

procedure islip_stack.pop(pv : pislip_var);
begin
    pv^ := m_stack[m_top];
    dec(m_top);
end;

end.