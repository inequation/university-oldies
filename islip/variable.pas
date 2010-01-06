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
    islip_type      = (VT_UNTYPED, VT_INT, VT_FLOAT, VT_STRING, VT_BOOL, VT_ARRAY);

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
            // returns variable type
            function get_type : islip_type;
            // does a cast to target type
            function cast(target : islip_type) : boolean;

            // math operation; assumes "self" as the first operand and
            // destination of result; op is the opcode
            function math(other : pislip_var; op : byte) : boolean;
            // logical operation; assumes "self" as the first operand and
            // destination of result; op is the opcode
            function logic(other : pislip_var; op : byte) : boolean;
            // string concatenation; results in a cast to string
            procedure concat(other : pislip_var);
        private
            m_type      : islip_type;
            m_valptr    : pointer;

            // frees the value pointed to by m_valptr
            procedure reset_value;
    end;

implementation

uses convert, bytecode, SysUtils;

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
begin
    reset_value;
end;

procedure islip_var.reset_value;
var
    pi  : ^int;
    pf  : ^float;
    ps  : ^string;
    pb  : ^boolean;
    //pa  : ^islip_var;
begin
    if m_valptr = nil then
        exit;
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
                write(pf^:0:2);
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

function islip_var.get_type : islip_type;
begin
    get_type := m_type;
end;

function islip_var.cast(target : islip_type) : boolean;
var
    pi  : ^int;
    pf  : ^float;
    ps  : ^string;
    pb  : ^boolean;
begin
    if target = m_type then begin
        cast := true;
        exit;
    end;        
    cast := false;
    case m_type of
        VT_UNTYPED:
            case target of
                VT_INT:
                    begin
                        new(pi);
                        pi^ := 0;
                        m_valptr := pi;
                    end;
                VT_FLOAT:
                    begin
                        new(pf);
                        pf^ := 0;
                        m_valptr := pf;
                    end;
                VT_BOOL:
                    begin
                        new(pb);
                        pb^ := false;
                        m_valptr := pb;
                    end;
                VT_STRING:
                    begin
                        new(ps);
                        ps^ := '';
                        m_valptr := ps;
                    end;
            end;
        VT_INT:
            begin
                pi := m_valptr;
                case target of
                    VT_FLOAT:
                        begin
                            new(pf);
                            pf^ := float(pi^);
                            m_valptr := pf;
                        end;
                    VT_BOOL:
                        begin
                            new(pb);
                            pb^ := (pi^ <> 0);
                            m_valptr := pb;
                        end;
                    VT_STRING:
                        begin
                            new(ps);
                            ps^ := IntToStr(pi^);
                            m_valptr := ps;
                        end;
                end;
                if target <> VT_INT then
                    dispose(pi);
            end;
        VT_FLOAT:
            begin
                pf := m_valptr;
                case target of
                    VT_INT:
                        begin
                            new(pi);
                            pi^ := trunc(pf^);
                            m_valptr := pi;
                        end;
                    VT_BOOL:
                        begin
                            new(pb);
                            pb^ := (pf^ <> 0.0);
                            m_valptr := pb;
                        end;
                    VT_STRING:
                        begin
                            new(ps);
                            ps^ := FloatToStr(pi^);
                            m_valptr := ps;
                        end;
                end;
                if target <> VT_FLOAT then
                    dispose(pf);
            end;
        VT_BOOL:
            begin
                pb := m_valptr;
                case target of
                    VT_INT:
                        begin
                            new(pi);
                            pi^ := int(pb^);
                            m_valptr := pi;
                        end;
                    VT_FLOAT:
                        begin
                            new(pf);
                            if pb^ then
                                pf^ := 1.0
                            else
                                pf^ := 0.0;
                            m_valptr := pf;
                        end;
                    VT_STRING:
                        begin
                            new(ps);
                            if pb^ then
                                ps^ := 'WIN'
                            else
                                ps^ := 'FAIL';
                            m_valptr := ps;
                        end;
                end;
                if target <> VT_BOOL then
                    dispose(pb);
            end;
        VT_STRING:
            begin
                ps := m_valptr;
                case target of
                    VT_INT:
                        begin
                            new(pi);
                            try
                                pi^ := StrToInt(ps^);
                            except
                                on E : Exception do begin
                                    dispose(pi);
                                    exit;
                                end;
                            end;
                            m_valptr := pi;
                        end;
                    VT_FLOAT:
                        begin
                            new(pf);
                            if not atof(ps^, pf) then begin
                                dispose(pf);
                                exit;
                            end;
                            m_valptr := pf;
                        end;
                    VT_BOOL:
                        begin
                            new(pb);
                            pb^ := (length(ps^) > 0);
                            m_valptr := pb;
                        end;
                end;
                if target <> VT_STRING then
                    dispose(ps);
            end;
    end;
    m_type := target;
    cast := true;
end;

function islip_var.math(other : pislip_var; op : byte) : boolean;
var
    pi1, pi2    : ^int;
    pf1, pf2    : ^float;
begin
    math := false;
    if (m_type = VT_UNTYPED) or (other^.m_type = VT_UNTYPED)
        or (m_type = VT_STRING) or (other^.m_type = VT_STRING) then
        exit;
    if m_type = VT_INT then begin
        pi1 := m_valptr;
        // int + int
        if other^.m_type = VT_INT then begin
            pi2 := other^.m_valptr;
            case op of
                OP_ADD:
                    pi1^ := pi1^ + pi2^;
                OP_SUB:
                    pi1^ := pi1^ - pi2^;
                OP_MUL:
                    pi1^ := pi1^ * pi2^;
                OP_DIV:
                    pi1^ := pi1^ div pi2^;
                OP_MOD:
                    pi1^ := pi1^ mod pi2^;
                OP_MIN:
                    if pi1^ > pi2^ then
                        pi1^ := pi2^;
                OP_MAX:
                    if pi1^ < pi2^ then
                        pi1^ := pi2^;
                else
                    exit;
            end;
        // int + float
        end else begin
            pf2 := other^.m_valptr;
            // special case for modulo
            if op = OP_MOD then
                pi1^ := pi1^ mod trunc(pf2^)
            else begin
                self.cast(VT_FLOAT);
                pf1 := m_valptr;
                case op of
                    OP_ADD:
                        pf1^ := pf1^ + pf2^;
                    OP_SUB:
                        pf1^ := pf1^ - pf2^;
                    OP_MUL:
                        pf1^ := pf1^ * pf2^;
                    OP_DIV:
                        pf1^ := pf1^ / pf2^;
                    OP_MIN:
                        if pf1^ > pf2^ then
                            pf1^ := pf2^;
                    OP_MAX:
                        if pf1^ < pf2^ then
                            pf1^ := pf2^;
                    else
                        exit;
                end;
            end;
        end;
    end else begin
        pf1 := m_valptr;
        // float + int
        if other^.m_type = VT_INT then begin
            pi2 := other^.m_valptr;
            case op of
                OP_ADD:
                    pf1^ := pf1^ + float(pi2^);
                OP_SUB:
                    pf1^ := pf1^ - float(pi2^);
                OP_MUL:
                    pf1^ := pf1^ * float(pi2^);
                OP_DIV:
                    pf1^ := pf1^ / float(pi2^);
                OP_MOD:
                    begin
                        self.cast(VT_INT);
                        pi1 := m_valptr;
                        pi1^ := pi1^ mod pi2^;
                    end;
                OP_MIN:
                    if pf1^ > float(pi2^) then
                        pf1^ := float(pi2^);
                OP_MAX:
                    if pf1^ < float(pi2^) then
                        pf1^ := float(pi2^);
                else
                    exit;
            end;
        // float + float
        end else begin
            // special case for modulo
            if op = OP_MOD then begin
                self.cast(VT_INT);
                pi1 := m_valptr;
                pi1^ := pi1^ mod trunc(pf2^);
            end else begin
                pf2 := other^.m_valptr;
                case op of
                    OP_ADD:
                        pf1^ := pf1^ + pf2^;
                    OP_SUB:
                        pf1^ := pf1^ - pf2^;
                    OP_MUL:
                        pf1^ := pf1^ * pf2^;
                    OP_DIV:
                        pf1^ := pf1^ / pf2^;
                    OP_MIN:
                        if pf1^ > pf2^ then
                            pf1^ := pf2^;
                    OP_MAX:
                        if pf1^ < pf2^ then
                            pf1^ := pf2^;
                    else
                        exit;                    
                end;
            end;
        end;
    end;
    math := true;
end;

function islip_var.logic(other : pislip_var; op : byte) : boolean;
var
    pb1, pb2    : ^boolean;
    pi1, pi2    : ^int;
    pf1, pf2    : ^float;
    ps1, ps2    : ^string;
    comparison  : islip_type;
    temp        : boolean;
begin
    logic := false;
    case op of
        OP_NEG:
            begin
                self.cast(VT_BOOL);
                pb1 := m_valptr;
                pb1^ := not pb1^;
            end;
        OP_AND,
        OP_OR,
        OP_XOR:
            begin
                self.cast(VT_BOOL);
                other^.cast(VT_BOOL);
                pb1 := m_valptr;
                pb2 := other^.m_valptr;
                case op of
                    OP_AND:
                        pb1^ := pb1^ and pb2^;
                    OP_OR:
                        pb1^ := pb1^ or pb2^;
                    OP_XOR:
                        pb1^ := pb1^ xor pb2^;
                end;
            end;
        OP_EQ,
        OP_NEQ:
            begin
                // select comparison mode and perform necessary casts
                case m_type of
                    VT_INT:
                        case other^.m_type of
                            VT_INT:
                                comparison := VT_INT;
                            VT_FLOAT:
                                begin
                                    self.cast(VT_FLOAT);
                                    comparison := VT_FLOAT;
                                end;
                            VT_BOOL:
                                begin
                                    other^.cast(VT_INT);
                                    comparison := VT_INT;
                                end;
                            VT_STRING:
                                begin
                                    self.cast(VT_STRING);
                                    comparison := VT_STRING;
                                end;
                            else begin
                                self.cast(VT_BOOL);
                                other^.cast(VT_BOOL);
                                comparison := VT_BOOL;
                            end;
                        end;
                    VT_FLOAT:
                        case other^.m_type of
                            VT_INT,
                            VT_BOOL:
                                begin
                                    other^.cast(VT_FLOAT);
                                    comparison := VT_FLOAT;
                                end;
                            VT_FLOAT:
                                comparison := VT_FLOAT;
                            VT_STRING:
                                begin
                                    self.cast(VT_STRING);
                                    comparison := VT_STRING;
                                end;
                            else begin
                                self.cast(VT_BOOL);
                                other^.cast(VT_BOOL);
                                comparison := VT_BOOL;
                            end;
                        end;
                    VT_BOOL:
                        case other^.m_type of
                            VT_INT,
                            VT_FLOAT,
                            VT_STRING:
                                begin
                                    self.cast(other^.m_type);
                                    comparison := other^.m_type;
                                end;
                            VT_BOOL:
                                comparison := VT_BOOL;
                            else begin
                                other^.cast(VT_BOOL);
                                comparison := VT_BOOL;
                            end;
                        end;
                    VT_STRING:
                        case other^.m_type of
                            VT_INT,
                            VT_FLOAT,
                            VT_BOOL:
                                begin
                                    other^.cast(VT_STRING);
                                    comparison := VT_STRING;
                                end;
                            VT_STRING:
                                comparison := VT_STRING;
                            else begin
                                self.cast(VT_BOOL);
                                other^.cast(VT_BOOL);
                                comparison := VT_BOOL;
                            end;
                        end;
                    else begin
                        self.cast(VT_BOOL);
                        other^.cast(VT_BOOL);
                        comparison := VT_BOOL;
                    end;
                end;
                // now compare
                case comparison of
                    VT_INT:
                        begin
                            pi1 := m_valptr;
                            pi2 := other^.m_valptr;
                            temp := ((pi1^ = pi2^) and (op = OP_EQ))
                                or ((pi1^ <> pi2^) and (op = OP_NEQ));
                        end;
                    VT_FLOAT:
                        begin
                            pf1 := m_valptr;
                            pf2 := other^.m_valptr;
                            temp := ((pf1^ = pf2^) and (op = OP_EQ))
                                or ((pf1^ <> pf2^) and (op = OP_NEQ));
                        end;
                    VT_BOOL:
                        begin
                            pb1 := m_valptr;
                            pb2 := other^.m_valptr;
                            temp := ((pb1^ = pb2^) and (op = OP_EQ))
                                or ((pb1^ <> pb2^) and (op = OP_NEQ));
                        end;
                    VT_STRING:
                        begin
                            ps1 := m_valptr;
                            ps2 := other^.m_valptr;
                            temp := ((ps1^ = ps2^) and (op = OP_EQ))
                                or ((ps1^ <> ps2^) and (op = OP_NEQ));
                        end;
                end;
                // write down the result
                self.cast(VT_BOOL);
                pb1 := m_valptr;
                pb1^ := temp;
            end;
    end;
end;

procedure islip_var.concat(other : pislip_var);
var
    ps1, ps2    : ^string;
begin
    self.cast(VT_STRING);
    other^.cast(VT_STRING);
    ps1 := m_valptr;
    ps2 := other^.m_valptr;
    ps1^ := ps1^ + ps2^;
end;

end.