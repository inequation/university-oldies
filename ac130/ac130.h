// AC-130 shooter
// Written by Leszek Godlewski <leszgod081@student.polsl.pl>

// some includes shared by all the modules
#include <stdlib.h>
#include <SDL/SDL.h>

// type declarations

/// Boolean value.
typedef unsigned char	lbool_t;
/// Boolean false.
#define lfalse			0
/// Boolean true.
#define ltrue			(!lfalse)

// renderer function
lbool ac_rendrer_init(void);