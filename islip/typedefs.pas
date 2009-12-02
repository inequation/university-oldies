// islip - IneQuation's Scripting Language Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// C-like type declarations for convenience

unit typedefs;

interface

type
    int     = integer;  // short-hand for integer
    float   = single;   // 32-bit IEEE float
    //double  = double; // redundancy
    size_t  = cardinal; // basically an unsigned int
    ushort  = word;     // unsigned short

implementation

// nothing to see here, move along

end.