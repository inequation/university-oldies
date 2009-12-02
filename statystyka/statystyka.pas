program statystyka;

{$APPTYPE CONSOLE}

uses
  SysUtils, crt;

const
     wersja       = '0.2';

type
     stat         = record
        c         : char;
        n         : cardinal;
        len       : cardinal;
     end;

var
     n_freq       : cardinal;
     plot         : boolean;
     query        : integer;
     fname        : string;
     input        : text;
     stats        : array[0..255] of stat;
     total        : cardinal;
     ws_filter    : boolean;
     maxlen       : cardinal;   // ilosc miejsca zajmowana przez najdluzszy zapis statystyki
     sort         : boolean;
     alphanum     : boolean;

procedure print_help;
begin
     writeln('Uzycie: statystyka [opcje] <plik_wejsciowy>');
     writeln('Opcje:');
     writeln('  -a, --alphanumeric        pokazuje tylko statystyki znaków alfanumerycznych');
     writeln('  -f <n>, --freq=<n>        pokazuje n najczęściej występujących znaków');
     writeln('                            posortowanych wg częstotliwości wystąpień');
     writeln('  -p, --plot                rysuje wykres częstotliwości wystąpień');
     writeln('                            znaków');
     writeln('  -q <znak>, --query=<znak> podaje szczegółowe statystyki dotyczące danego');
     writeln('                            znaku');
     writeln('  -s, --sort                sortuje wyniki rosnąco');
     writeln('  -w, --whitespace          nie pomija znaków niedrukowanych przy wypisywaniu statystyk');
     writeln;
     writeln('Autor: Leszek Godlewski <leszgod081@student.polsl.pl>');
end;

function parse_args : boolean;
var
   i : integer;
begin
     // inicjalizacja stanu opcji programu
     parse_args := true;
     n_freq := 0;
     plot := false;
     query := -1;
     fname := '';
     ws_filter := true;
     sort := false;
     alphanum := false;

     i := 1;
     while i <= ParamCount do begin
         if ParamStr(i)[1] <> '-' then begin
            fname := ParamStr(i);
            exit;
         end;
         if ParamStr(i)[2] = '-' then begin
             if pos('--freq=', ParamStr(i)) = 1 then begin
                n_freq := strtoint(copy(ParamStr(i), 8, length(ParamStr(i)) - 7));
                sort := true;   // zadanie n najczesciej wystepujacych implikuje sortowanie
             end else if pos('--plot', ParamStr(i)) = 1 then
                plot := true
             else if pos('--query=', ParamStr(i)) = 1 then
                query := ord(ParamStr(i)[9])
             else if pos('--whitespace', ParamStr(i)) = 1 then
                ws_filter := false
             else if pos('--sort', ParamStr(i)) = 1 then
                sort := true
             else if pos('--alphanumeric', ParamStr(i)) = 1 then
                alphanum := true
             else begin
                parse_args := false;
                exit;
             end;
         end else begin
             case ParamStr(i)[2] of
                  'f' : begin
                      inc(i);
                      n_freq := strtoint(ParamStr(i));
                      sort := true;   // zadanie n najczesciej wystepujacych implikuje sortowanie
                      end;
                  'p' : plot := true;
                  'q' : begin
                      inc(i);
                      query := ord(ParamStr(i)[1]);
                      end;
                  'w' : ws_filter := false;
                  's' : sort := true;
                  'a' : alphanum := true;
              else begin
                   parse_args := false;
                   exit;
              end; end;
         end;
         inc(i);
     end;
end;

function open_file : boolean;
begin
     open_file := true;
     {$I-}
     assign(input, fname);
     reset(input);
     {$I+}
     if IOResult <> 0 then
        open_file := false;
end;

procedure close_file;
begin
     close(input);
end;

procedure count_char(c : char);
begin
     if ws_filter and (ord(c) <= ord(' ')) then
        exit;
     if alphanum and not (c in ['A'..'Z', 'a'..'z', '0'..'9']) then
        exit;
     inc(stats[ord(c)].n);
     inc(total);
end;

procedure parse_text;
var
   line : string;
   i    : integer;
begin
     // inicjalizujemy tablice
     for i := 0 to 255 do begin
        stats[i].c := chr(i);
        stats[i].n := 0;
     end;
     total := 0;

     while not eof(input) do begin
        readln(input, line);
        for i := 1 to length(line) do
           count_char(line[i]);
     end;
end;

procedure quicksort(var t : array of stat; l, r : integer);
var
   pivot, i, j  : integer;
   b            : stat;
begin
     if l < r then begin
        pivot := t[random(r - l) + l + 1].n; // losowanie elementu dzielacego
        //pivot := t[(l + r) div 2].n;
        i := l - 1;
        j := r + 1;
        repeat
           repeat
              i := i + 1
           until pivot <= t[i].n;
           repeat
              j := j - 1
           until pivot >= t[j].n;
           b := t[i];
           t[i] := t[j];
           t[j] := b;
        until i >= j;
        t[j] := t[i];
        t[i] := b;
        quicksort(t, l, i - 1);
        quicksort(t, i, r);
    end;
end;

procedure calc_space(i : integer);
var
     temp : integer;
     frac : real;
begin
     stats[i].len := 12; // stale elementy

     // dlugosc liczby wystapien
     temp := stats[i].n;
     while temp div 10 > 0 do begin
        temp := temp div 10;
        inc(stats[i].len);
     end;
     if temp mod 10 > 0 then
        inc(stats[i].len);

     // dlugosc procentow
     frac := stats[i].n / total;
     if frac = 1.0 then
        stats[i].len := stats[i].len + 3
     else if frac >= 0.1 then
        stats[i].len := stats[i].len + 2
     else
        stats[i].len := stats[i].len + 1;

     // dlugosc znaku
     if (ord(stats[i].c) > ord(' ')) and (ord(stats[i].c) <= 127) then
        inc(stats[i].len) // 1 znak
     else begin
        // #XXX - kod ASCII
        if ord(stats[i].c) > 99 then
           stats[i].len := stats[i].len + 4
        else if ord(stats[i].c) > 9 then
           stats[i].len := stats[i].len + 3
        else
           stats[i].len := stats[i].len + 2;
     end;
end;

procedure sort_stats;
var
   i    : integer;
begin
     if sort then begin
        randomize;
        quicksort(stats, 0, 255);
     end;

     // przy okazji sprawdz, ile miejsca mamy do wykorzystania na wykres
     maxlen := 0;
     for i := 0 to 255 do begin
        calc_space(i);
        if stats[i].len > maxlen then
           maxlen := stats[i].len
     end;
end;

procedure print_single_stat(i : integer);
var
   frac : real;
   j, x : integer;
begin
     frac := stats[i].n / total;
     if (ord(stats[i].c) <= ord(' ')) or (ord(stats[i].c) > 127) then
        write('#', ord(stats[i].c), ': ', stats[i].n, ' (', (frac * 100.0):0:2, '%) ')
     else
        write(stats[i].c, ': ', stats[i].n, ' (', (frac * 100.0):0:2, '%) ');

     if not plot then begin
        writeln;
        exit;
     end;

     // rysujemy wykres
     x := maxlen - stats[i].len;
     for i := 1 to x do
        write(' ');
     write('[');
     x := round(real(ScreenWidth - 1 - maxlen) * frac);
     for j := 1 to x do
        write('*');
     x := round(real(ScreenWidth - 1 - maxlen) * (1.0 - frac));
     for j := 1 to x do
        write(' ');
     writeln(']');
{     if x < (ScreenWidth - maxlen) then
        writeln; // w przeciwnym wypadku kursor i tak przejdzie do nastepnej linii}
end;

procedure print_stats;
var
   i, lim       : integer;
   percent      : real;
begin
     writeln('Długość tekstu: ', total, ' znaków');
     if sort then begin
        if n_freq > 0 then
           lim := 255 - n_freq + 1
        else
           lim := 0;
        for i := 255 downto lim do
           if (stats[i].n > 0) and ((not alphanum) or (stats[i].c in ['A'..'Z', 'a'..'z', '0'..'9'])) then
              print_single_stat(i);
     end else for i := 0 to 255 do
        if (stats[i].n > 0) and ((not alphanum) or (stats[i].c in ['A'..'Z', 'a'..'z', '0'..'9'])) then
           print_single_stat(i);
end;

procedure print_query;
var
   i    : integer;
begin
     for i := 0 to 255 do
        if stats[i].c = chr(query) then begin
           print_single_stat(i);
           exit;
        end;
end;

begin
     writeln('Statystyka tekstów v', wersja);
     if (ParamCount < 1) or not parse_args then begin
        print_help;
        exit;
     end;
     if not open_file then begin
        writeln('Błąd przy otwieraniu pliku!');
        exit;
     end;
     parse_text;
     if total < 1 then begin
        writeln('Plik wejściowy jest pusty.');
        exit;
     end;
     close_file;
     if query >= 0 then
        print_query
     else begin
        sort_stats;
        print_stats; begin
                writeln;
     end;
end;
     writeln;
end.

