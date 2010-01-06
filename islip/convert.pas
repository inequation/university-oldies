// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Helper module for parsing floating point numbers out of strings, since
// Delphi's StrToFloat implementation appears to be locale-based

unit convert;

interface

uses typedefs;

function atof(s : string; f : pfloat) : boolean;

implementation

function atof(s : string; f : pfloat) : boolean;
var
    i       : int;
    neg     : boolean;
    frac    : boolean;
    scale   : float;
begin
    atof := false;
    neg := false;
    frac := false;
    scale := 0.1;
    f^ := 0;
    for i := 1 to length(s) do begin
        if (i = 1) and (s[1] = '-') then begin
            neg := true;
            continue;
        end;
        if s[i] = '.' then begin
            if frac then    // multiple radix chars, error
                exit;
            frac := true;
            continue;
        end;
        if not (s[i] in ['0'..'9']) then
            exit;
        if not frac then
            f^ := f^ * 10.0 + ord(s[i]) - ord('0')
        else begin
            f^ := f^ + (ord(s[i]) - ord('0')) * scale;
            scale := scale * 0.1;
        end;
    end;
    if neg then
        f^ := f^ * -1.0;
    atof := true;
end;

end.