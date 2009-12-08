// islip - IneQuation's Simple LOLCODE Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Bytecode definition

unit bytecode;

interface

uses typedefs, variable;

const
    // ====================================================
    // instructions
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

    // JMP: unconditional jump
    // args: instruction offset to apply
    // result: corresponding pointer arithmetic on the interpreter's instruction pointer
    OP_JMP          =   $07;

    // CNDJMP: conditional jump (if top of the stack < 0)
    // args: instruction offset to apply if condition is true
    // result: corresponding pointer arithmetic on the interpreter's instruction pointer
    OP_CNDJMP       =   $08;

    // TRAP: call an interpreter trap
    // args: ID of the routine to call
    // result: n/a
    OP_TRAP         =   $09;

    // CALL: call a function (defined in LOLCODE); assumes that parameters are on the stack (stdcall convention)
    // args: function index
    // result: function's return value on top of the stack
    OP_CALL         =   $0A;

    // ====================================================
    // interpreter traps
    // ====================================================

    // PRINT: prints the top of the stack to stdio
    TRAP_PRINT      =   $00;

    // LINEFEED: prints a line feed to stdout and flushes the buffer
    TRAP_LINEFEED   =   $01;

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