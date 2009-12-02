// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Variable class unit

unit variable;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs in 'typedefs.pas';

type
    islip_var_type  = (VT_INVALID, VT_INT, VT_FLOAT, VT_STRING, VT_ARRAY);

    pislip_var      = ^islip_var;
    islip_var       = class
        public
            constructor create(id : string; i : int); overload;
            constructor create(id : string; f : float); overload;
            constructor create(id : string; s : string); overload;
            //constructor create(id : string; arr_type : islip_var_type; s : size_t); overload;
            destructor destroy; override;

            // prints the variable to stdout
            procedure echo;
        private
            m_id        : string;
            m_type      : islip_var_type;
            m_valptr    : pointer;
    end;

implementation

// integer constructor
constructor islip_var.create(id : string; i : int);
var
    p   : ^int;
begin
    m_id := id;
    new(p);
    p^ := i;
    m_valptr := p;
    m_type := VT_INT;
end;

// float constructor
constructor islip_var.create(id : string; f : float);
var
    p   : ^float;
begin
    m_id := id;
    new(p);
    p^ := f;
    m_valptr := p;
    m_type := VT_FLOAT;
end;

// string constructor
constructor islip_var.create(id : string; s : string);
var
    p   : ^string;
begin
    m_id := id;
    new(p);
    p^ := s;
    m_valptr := p;
    m_type := VT_STRING;
end;

// array constructor
{constructor islip_var.create(id : string; arr_type : islip_var_type; s : size_t);
var
    p   : array of islip_var;
    i   : int;
begin
    m_id := id;
    setlength(p, s);
    for i := 1 to s do begin
        case arr_type of
            VT_INT:
                p[i].create(id + '[' + i + ']', int(0));
                break;
            VT_FLOAT:
                p[i].create(id + '[' + i + ']', float(0.0));
                break;
            VT_STRING:
                p[i].create(id + '[' + i + ']', '');
                break;
        end;
    end;
    p^ := f;
    m_valptr := p;
    m_type := VT_ARRAY;
end;}

// universal destructor
destructor islip_var.destroy;
var
    pi  : ^int;
    pf  : ^float;
    ps  : ^string;
    //pa  : ^islip_var;
begin
    case m_type of
        VT_INT:
            begin
                pi := m_valptr;
                dispose(pi);
            end;
        VT_FLOAT:
            begin
                pf := m_valptr;
                dispose(pf);
            end;
        VT_STRING:
            begin
                ps := m_valptr;
                dispose(ps);
            end;
        {VT_ARRAY:
            begin
            end;}
    end;
    m_valptr := nil;
end;

procedure islip_var.echo;
var
    pi  : ^int;
    pf  : ^float;
    ps  : ^string;
begin
    case m_type of
        VT_INT:
            begin
                pi := m_valptr;
                write(pi^);
            end;
        VT_FLOAT:
            begin
                pf := m_valptr;
                write(pf^);
            end;
        VT_STRING:
            begin
                ps := m_valptr;
                write(ps^);
            end;
    end;
end;

end.