// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Parsing helper unit

unit parser;

{$IFDEF fpc}
    {$MODE objfpc}
{$ENDIF}

interface

uses typedefs;

const
    TOKEN_HISTORY_SIZE  =   5;  // how many previous tokens the parser is
                                // supposed to remember; must be >= 1

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
            // NOTE: can only roll TOKEN_HISTORY_SIZE tokens back
            procedure unget_token;
            // retrieves current token starting position in input file (for
            // error reporting)
            procedure get_pos(var row : size_t; var col : size_t);

        private
            m_reader    : islip_reader;

            m_row       : size_t;
            m_col       : size_t;

            m_undepth   : size_t;
            m_tokens    : array[0..TOKEN_HISTORY_SIZE] of string;
            m_types     : array[0..TOKEN_HISTORY_SIZE] of islip_parser_token_type;

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
var
    i   : size_t;
begin
    m_reader := islip_reader.create(input);
    m_undepth := 0;
    for i := 0 to TOKEN_HISTORY_SIZE do begin
        m_tokens[i] := '';
        m_types[i] := TT_WS;
    end;
end;

function islip_parser.comment : boolean;
var
    c   : char;
begin
    comment := false;
    if m_tokens[0] = 'BTW' then begin
        // skip characters until we read a newline and
        // start a new token
        while m_reader.get_char(c) do
            if c in [chr(10), chr(13)] then begin
                m_tokens[0] := '';
                break;
            end;
    end else if m_tokens[0] = 'OBTW' then begin
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
                            m_tokens[0] := '';
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
    i       : size_t;
begin
    get_token := true;

    esc := false;

    if m_undepth > 0 then begin
        s := m_tokens[m_undepth];
        t := m_types[m_undepth];
        dec(m_undepth);
        exit;
    // if we still have something to throw, do it
    end else if length(m_tokens[0]) > 0 then begin
        s := m_tokens[0];
        t := m_types[0];
        for i := TOKEN_HISTORY_SIZE downto 1 do begin
            m_tokens[i] := m_tokens[i - 1];
            m_types[i] := m_types[i - 1];
        end;
        m_tokens[0] := chr(0);  // HACK HACK HACK! somehow necessary for the
        m_tokens[0] := '';      // string to be zeroed in FPC
        m_types[0] := TT_WS;
        exit;
    end;

    // initial state
    m_types[0] := TT_WS;

    while m_reader.get_char(c) do begin
        case m_types[0] of
            TT_WS:
                begin
                    if c = '"' then begin
                        m_types[0] := TT_STRING;
                        // keep track of where we started the token
                        m_reader.get_pos(m_row, m_col);
                    end else if c in [chr(10), chr(13), ','] then begin
                        if c = ',' then
                            c := chr(10);
                        m_tokens[0] := c;
                        s := m_tokens[0];
                        t := m_types[0];
                        for i := TOKEN_HISTORY_SIZE downto 1 do begin
                            m_tokens[i] := m_tokens[i - 1];
                            m_types[i] := m_types[i - 1];
                        end;
                        m_tokens[0] := chr(0);  // HACK HACK HACK! somehow
                        m_tokens[0] := '';      // necessary for the string to
                        m_types[0] := TT_WS;    // be zeroed in FPC
                        exit;
                    end else if ord(c) > 32 then begin
                        m_tokens[0] := m_tokens[0] + c;
                        m_types[0] := TT_EXPR;
                        // keep track of where we started the token
                        m_reader.get_pos(m_row, m_col);
                    end;
                end;
            TT_EXPR:
                begin
                    if c in [chr(10), chr(13), ','] then begin
                        // handle comments
                        if comment then begin
                            if m_tokens[0] = '' then
                                continue;
                        end else begin
                            writeln('ERROR: Unterminated multi-line comment');
                            get_token := false;
                            exit;
                        end;
                        // throw what we've got so far and keep that newline in
                        // mind
                        s := m_tokens[0];
                        t := m_types[0];
                        for i := TOKEN_HISTORY_SIZE downto 1 do begin
                            m_tokens[i] := m_tokens[i - 1];
                            m_types[i] := m_types[i - 1];
                        end;
                        if c = ',' then
                            c := chr(10);
                        m_tokens[0] := c;
                        exit;
                    end else if ord(c) > 32 then
                        m_tokens[0] := m_tokens[0] + c
                    else begin
                        // handle comments
                        if comment then begin
                            if m_tokens[0] = '' then
                                continue;
                        end else begin
                            writeln('ERROR: Unterminated multi-line comment');
                            get_token := false;
                            exit;
                        end;
                        s := m_tokens[0];
                        t := m_types[0];
                        for i := TOKEN_HISTORY_SIZE downto 1 do begin
                            m_tokens[i] := m_tokens[i - 1];
                            m_types[i] := m_types[i - 1];
                        end;
                        m_tokens[0] := chr(0);  // HACK HACK HACK! somehow
                        m_tokens[0] := '';      // necessary for the string to
                        m_types[0] := TT_WS;    // be zeroed in FPC
                        exit;
                    end;
                end;
            TT_STRING:
                begin
                    if c = ':' then begin
                        if esc then begin
                            m_tokens[0] := m_tokens[0] + c;
                            esc := false;
                        end else
                            esc := true;
                    end else if esc then begin
                        if c = ')' then
                            m_tokens[0] := m_tokens[0] + chr(10)
                        else if c = '>' then
                            m_tokens[0] := m_tokens[0] + chr(9)
                        else if c = 'o' then
                            m_tokens[0] := m_tokens[0] + chr(7)
                        else if c = '"' then
                            m_tokens[0] := m_tokens[0] + c
                        // FIXME: hex, var, unicode name
                        else    // invalid escape sequence, handle the error
                            break;
                        esc := false;
                    end else if c <> '"' then
                        m_tokens[0] := m_tokens[0] + c
                    else begin
                        s := m_tokens[0];
                        t := m_types[0];
                        for i := TOKEN_HISTORY_SIZE downto 1 do begin
                            m_tokens[i] := m_tokens[i - 1];
                            m_types[i] := m_types[i - 1];
                        end;
                        m_tokens[0] := chr(0);  // HACK HACK HACK! somehow
                        m_tokens[0] := '';      // necessary for the string to
                        m_types[0] := TT_WS;    // be zeroed in FPC
                        exit;
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
        s := m_tokens[0];
        for i := TOKEN_HISTORY_SIZE downto 1 do begin
            m_tokens[i] := m_tokens[i - 1];
            m_types[i] := m_types[i - 1];
        end;
        m_tokens[0] := chr(0);  // HACK HACK HACK! somehow
        m_tokens[0] := '';      // necessary for the string to
        m_types[0] := TT_WS;    // be zeroed in FPC
    end;
end;

procedure islip_parser.unget_token;
var
    i   : size_t;
begin
    if m_undepth < TOKEN_HISTORY_SIZE then
        inc(m_undepth)
    else
        writeln('WARNING: Attempted to unget tokens beyond token history size');
end;

procedure islip_parser.get_pos(var row : size_t; var col : size_t);
begin
    row := m_row + 1;
    col := m_col - 1;
end;

end.