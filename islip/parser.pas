// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Parsing helper unit

unit parser;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs;

type
    // helper types
    pstring = ^string;

    // retrieves characters from file, keeping track of the cursor position
    islip_reader = class
        public
            constructor create(var input : cfile);
            //destructor destroy; override;

            // retrieves a single character from input file and advances
            // counters and stuff
            function get_char(var c : char) : boolean;
            // retrieves cursor position in input file (for error reporting)
            procedure get_pos(var row : size_t; var col : size_t);
        private
            m_row   : size_t;
            m_col   : size_t;
        
            m_input : pcfile;
    end;

    islip_parser_token_type  = (
        TT_WS,      // whitespace
        TT_EXPR,    // expression
        TT_STRING   // string constant
    );

    // assembles read characters into proper tokens
    islip_parser = class
        public
            constructor create(var input : cfile);
            //destructor destroy; override;

            // retrieves a single token
            // puts token into s; empty string on eof
            // returns false on parsing errors            
            function get_token(var s : string; var t : islip_parser_token_type)
                : boolean;
            // returns to previous token for reinterpretation
            // NOTE: can only roll 1 token back
            procedure unget_token;
            // retrieves current token starting position in input file (for
            // error reporting)
            procedure get_pos(var row : size_t; var col : size_t);

        private
            m_reader    : islip_reader;

            m_row       : size_t;
            m_col       : size_t;
            m_token     : string;
            m_prevtoken : string;
            m_prevtype  : islip_parser_token_type;

            function comment : boolean;
    end;

implementation

// ========================================================
// islip_reader implementation
// ========================================================

constructor islip_reader.create(var input : cfile);
begin
    m_input := @input;
    m_row := 0;
    m_col := 0;
end;

function islip_reader.get_char(var c : char) : boolean;
begin
    get_char := true;

    // check if there's anything left to read
    if eof(m_input^) then begin
        get_char := false;
        exit;
    end;
    read(m_input^, c);
    if ord(c) = 10 then begin
        inc(m_row);
        m_col := 0;
    end else
        inc(m_col);
end;

procedure islip_reader.get_pos(var row : size_t; var col : size_t);
begin
    row := m_row;
    col := m_col - 1;
end;

// ========================================================
// islip_parser implementation
// ========================================================

constructor islip_parser.create(var input : cfile);
begin
    m_reader := islip_reader.create(input);
    m_token := '';
    m_prevtoken := '';
    m_prevtype := TT_WS;
end;

function islip_parser.comment : boolean;
var
    c   : char;
begin
    comment := false;
    if m_token = 'BTW' then begin
        // skip characters until we read a newline and
        // start a new token
        while m_reader.get_char(c) do begin
            if c in [chr(10), chr(13)] then begin
                m_token := '';
                break;
            end;
        end;
    end else if m_token = 'OBTW' then begin
        // skip characters until we read a "TLDR"
        while m_reader.get_char(c) do begin
            if c = 'T' then begin
                if not m_reader.get_char(c) then
                    exit;
                if c = 'L' then begin
                    if not m_reader.get_char(c) then
                        exit;
                    if c = 'D' then begin
                        if not m_reader.get_char(c) then
                            exit;
                        if c = 'R' then begin
                            if not m_reader.get_char(c) then
                                exit;
                            m_token := '';
                            break;
                        end;
                    end;
                end;
            end;
        end;
    end;
    comment := true;
end;

function islip_parser.get_token(var s : string; var t : islip_parser_token_type)
    : boolean;
var
    c       : char;
    esc     : boolean;
begin
    get_token := true;

    esc := false;

    // if we still have something to throw, do it
    if length(m_token) > 0 then begin
        s := m_token;
        t := m_prevtype;
        m_prevtoken := m_token;
        m_token[1] := chr(0);    // HACK HACK HACK! somehow necessary for the
        m_token := '';           // string to be zeroed in FPC
        exit;
    end;

    // initial state
    t := TT_WS;

    while m_reader.get_char(c) do begin
        case t of
            TT_WS:
                begin
                    if c = '"' then begin
                        t := TT_STRING;
                        // keep track of where we started the token
                        m_reader.get_pos(m_row, m_col);
                    end else if c in [chr(10), chr(13), ','] then begin
                        m_token := c;
                        s := m_token;
                        m_prevtoken := m_token;
                        m_prevtype := t;
                        m_token[1] := chr(0);    // HACK HACK HACK! somehow
                        m_token := '';           // necessary for the string to
                        exit;                    // be zeroed in FPC
                    end else if ord(c) > 32 then begin
                        m_token := m_token + c;
                        t := TT_EXPR;
                        // keep track of where we started the token
                        m_reader.get_pos(m_row, m_col);
                    end;
                end;
            TT_EXPR:
                begin
                    if c in [chr(10), chr(13), ','] then begin
                        // handle comments
                        if comment then begin
                            if m_token = '' then
                                continue;
                        end else begin
                            writeln('ERROR: Unterminated multi-line comment');
                            get_token := false;
                            exit;
                        end;
                        // throw what we've got so far and keep that newline in
                        // mind
                        s := m_token;
                        m_prevtoken := m_token;
                        m_prevtype := t;
                        m_token := c;
                        exit;
                    end else if ord(c) > 32 then
                        m_token := m_token + c
                    else begin
                        // handle comments
                        if comment then begin
                            if m_token = '' then
                                continue;
                        end else begin
                            writeln('ERROR: Unterminated multi-line comment');
                            get_token := false;
                            exit;
                        end;
                        s := m_token;
                        m_prevtoken := m_token;
                        m_prevtype := t;
                        m_token[1] := chr(0);    // HACK HACK HACK! somehow
                        m_token := '';           // necessary for the string to
                        exit;                    // be zeroed in FPC
                    end;
                end;
            TT_STRING:
                begin
                    if c = ':' then begin
                        if esc then begin
                            m_token := m_token + c;
                            esc := false;
                        end else
                            esc := true;
                    end else if esc then begin
                        if c = ')' then
                            m_token := m_token + chr(10)
                        else if c = '>' then
                            m_token := m_token + chr(9)
                        else if c = 'o' then
                            m_token := m_token + chr(7)
                        else if c = '"' then
                            m_token := m_token + c
                        // FIXME: hex, var, unicode name
                        else    // invalid escape sequence, handle the error
                            break;
                        esc := false;
                    end else if c <> '"' then
                        m_token := m_token + c
                    else begin
                        s := m_token;
                        m_prevtoken := m_token;
                        m_prevtype := t;
                        m_token[1] := chr(0);    // HACK HACK HACK! somehow
                        m_token := '';           // necessary for the string to
                        exit;                    // be zeroed in FPC
                    end;
                end;
        end;
    end;
    
    // the only proper (i.e. non-eof) way out of this function is by manual exit

    if esc then begin
        m_reader.get_pos(m_row, m_col);
        writeln('ERROR: Unknown escape sequence ":', c, '" at line ', m_row,
            ', column ', m_col);
        get_token := false;
        // make sure token is nonempty
        s := '!';
    end else if (t = TT_STRING) then begin
        writeln('ERROR: Unterminated string starting at line ', m_row,
            ', column ', m_col);
        get_token := false;
        // make sure token is nonempty
        s := '!';
    end else begin
        // return the last token on eof
        s := m_token;
        m_prevtoken := m_token;
        m_prevtype := t;
        m_token[1] := chr(0);    // HACK HACK HACK! somehow
        m_token := '';
    end;
end;

procedure islip_parser.unget_token;
begin
    if length(m_prevtoken) > 0 then
        m_token := m_prevtoken
    else
        writeln('WARNING: Attempted to unget twice in a row');
end;

procedure islip_parser.get_pos(var row : size_t; var col : size_t);
begin
    row := m_row;
    col := m_col - 1;
end;

end.