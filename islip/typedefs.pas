// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// C-like type declarations for convenience

unit typedefs;

interface

type
    int     = integer;  // short-hand for integer
    float   = single;   // 32-bit IEEE float
    pfloat  = ^float;
    //double  = double; // redundancy
    size_t  = cardinal; // basically an unsigned int
    ushort  = word;     // unsigned short

    // these don't have anything to do with C, but they're here for convenience
    cfile   = file of char;
    pcfile  = ^cfile;
    pbool   = ^boolean;

implementation

// nothing to see here, move along

end.