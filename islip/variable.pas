// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Variable class unit

unit variable;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs;

type
    islip_type  = (VT_UNTYPED, VT_INT, VT_FLOAT, VT_STRING, VT_BOOL, VT_ARRAY);

    pislip_var  = ^islip_var;
    islip_var   = class
        public
            constructor create; overload;
            constructor create(var v : islip_var); overload;
            constructor create(i : int); overload;
            constructor create(f : float); overload;
            constructor create(s : string); overload;
            constructor create(b : boolean); overload;
            {constructor create(id : string; arr_type : islip_var_type;
                s : size_t); overload;}
            destructor destroy; override;

            // prints the variable to stdout
            procedure echo;
        private
            m_type      : islip_type;
            m_valptr    : pointer;
    end;

implementation

// untyped constructor
constructor islip_var.create;
begin
    m_valptr := nil;
    m_type := VT_UNTYPED;
end;

// copy constructor
constructor islip_var.create(var v : islip_var);
var
    pi1, pi2    : ^int;
    pf1, pf2    : ^float;
    ps1, ps2    : ^string;
    pb1, pb2    : ^boolean;
begin
    m_type := v.m_type;
    case v.m_type of
        VT_UNTYPED:
            m_valptr := nil;
        VT_INT:
            begin
                pi1 := v.m_valptr;
                new(pi2);
                pi2^ := pi1^;
                m_valptr := pi2;
            end;
        VT_FLOAT:
            begin
                pf1 := v.m_valptr;
                new(pf2);
                pf2^ := pf1^;
                m_valptr := pf2;
            end;
        VT_STRING:
            begin
                ps1 := v.m_valptr;
                new(ps2);
                ps2^ := ps1^;
                m_valptr := ps2;
            end;
        VT_BOOL:
            begin
                pb1 := v.m_valptr;
                new(pb2);
                pb2^ := pb1^;
                m_valptr := pb2;
            end;
    end;
end;

// integer constructor
constructor islip_var.create(i : int);
var
    p   : ^int;
begin
    new(p);
    p^ := i;
    m_valptr := p;
    m_type := VT_INT;
end;

// float constructor
constructor islip_var.create(f : float);
var
    p   : ^float;
begin
    new(p);
    p^ := f;
    m_valptr := p;
    m_type := VT_FLOAT;
end;

// string constructor
constructor islip_var.create(s : string);
var
    p   : ^string;
begin
    new(p);
    p^ := s;
    m_valptr := p;
    m_type := VT_STRING;
end;

// boolean constructor
constructor islip_var.create(b : boolean);
var
    p   : ^boolean;
begin
    new(p);
    p^ := b;
    m_valptr := p;
    m_type := VT_BOOL;
end;

// array constructor
{constructor islip_var.create(id : string; arr_type : islip_var_type;
    s : size_t);
var
    p   : array of islip_var;
    i   : int;
begin
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
    pb  : ^boolean;
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
        VT_BOOL:
            begin
                pb := m_valptr;
                dispose(pb);
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
    pb  : ^boolean;
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
        VT_BOOL:
            begin
                pb := m_valptr;
                if pb^ then
                    write('WIN')
                else
                    write('FAIL');
            end;
    end;
end;

end.