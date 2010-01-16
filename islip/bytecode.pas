// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Bytecode definition

unit bytecode;

interface

uses typedefs, variable;

const
    // ====================================================
    // opcodes
    // ====================================================
    
    // STOP: stop the execution of the program
    // args: n/a
    // result: n/a
    OP_STOP         =   $00;

    // PUSH: push variable onto the stack
    // args: index of the variable to read from and put on the stack
    // result: the variable is on the top of the stack
    OP_PUSH         =   $01;

    // POP: pop variable off the stack
    // args: index of the variable to write the value off the stack to
    // result: the top-most variable is removed from the stack
    OP_POP          =   $02;

    // ADD: addition of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_ADD          =   $03;

    // ADD: subtraction of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_SUB          =   $04;

    // MUL: multiplication of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_MUL          =   $05;

    // DIV: division of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_DIV          =   $06;

    // MOD: modulo of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_MOD          =   $07;

    // MIN: minimum of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_MIN          =   $08;

    // MAX: maximum of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_MAX          =   $09;

    // AND: boolean AND of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_AND          =   $0A;

    // OR: boolean OR of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_OR           =   $0B;

    // XOR: boolean XOR of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_XOR          =   $0C;

    // EQ: equality check of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_EQ           =   $0D;

    // NEQ: inequality check of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_NEQ          =   $0E;

    // NEG: boolean negation of the top-most stack variable
    // args: n/a
    // result: pops the top-most variable and pushes the operation result
    OP_NEG          =   $0F;

    // CONCAT: concatenates two top-most strings on the stack (will cast if
    // necessary)
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    OP_CONCAT       =   $10;

    // JMP: unconditional jump
    // args: address of the instruction to jump to
    // result: corresponding pointer arithmetic on the interpreter's instruction
    // pointer
    OP_JMP          =   $11;

    // CNDJMP: conditional jump (if the implicit "IT" variable is *false*)
    // args: address of the instruction to jump to
    // result: corresponding pointer arithmetic on the interpreter's instruction
    // pointer
    OP_CNDJMP       =   $12;

    // CALL: call a function (defined in LOLCODE); assumes that parameters are
    // on the stack (stdcall convention)
    // args: function index
    // result: function's return value on top of the stack
    OP_CALL         =   $13;

    // RETURN: return control from a function
    // args: n/a
    // result: control is returned to higher-level code block
    OP_RETURN       =   $14;

    // PRINT: prints the top of the stack to stdio
    // args: n/a
    // result: n/a
    OP_PRINT        =   $15;

    // READ: reads a string from stdin onto the top of the stack
    // args: n/a
    // result: n/a
    OP_READ         =   $16;

    // CAST: casts the value on the top of the stack to a given type
    // args: ID of the type to cast to
    // result: n/a
    OP_CAST         =   $17;

    // INCR: increments (arg = 1) or decrements (arg = 0) the value of the top
    // of the stack
    // args: 0 for decrementation, 1 for incrementation
    // result: n/a
    OP_INCR         =   $18;

    // ====================================================
    // special arguments
    // ====================================================

    // NULL argument
    // PUSH behaviour: push an empty variable onto the stack
    // POP behaviour: just pop the stack without writing the value to a variable
    ARG_NULL        =   $00000000;

type
    // single instruction with the argument
    islip_inst      = record
        inst        : byte;
        arg         : size_t;
    end;
    
    islip_bytecode  = array of islip_inst;
    pislip_bytecode = ^islip_bytecode;
    islip_data      = array of islip_var;
    pislip_data     = ^islip_data;

implementation

end.