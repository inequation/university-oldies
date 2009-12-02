// islip - IneQuation's Scripting Language Interpreter in Pascal
// Written by Leszek "IneQuation" Godlewski <leszgod081@student.polsl.pl>
// Bytecode definition

unit bytecode;

interface

const
    // ====================================================
    // instructions
    // ====================================================
    
    // NOP: do nothing
    // args: n/a
    // result: n/a
    BI_NOP          =   $00;

    // PUSH: push variable onto the stack
    // args: index of the variable to read from and put on the stack
    // result: the variable is on the top of the stack
    BI_PUSH         =   $01;

    // POP: pop variable off the stack
    // args: index of the variable to write the value off the stack to
    // result: the top-most variable is removed from the stack
    BI_POP          =   $02;

    // ADD: addition of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    BI_ADD          =   $03;

    // ADD: subtraction of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    BI_SUB          =   $04;

    // MUL: multiplication of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    BI_MUL          =   $05;

    // DIV: division of the two top-most variables on the stack
    // args: n/a
    // result: pops the two top-most variables and pushes the operation result
    BI_DIV          =   $06;

    // JMP: unconditional jump
    // args: instruction offset to apply
    // result: corresponding pointer arithmetic on the interpreter's instruction pointer
    BI_JMP          =   $07;

    // CNDJMP: conditional jump (if top of the stack < 0)
    // args: instruction offset to apply if condition is true
    // result: corresponding pointer arithmetic on the interpreter's instruction pointer
    BI_CNDJMP       =   $08;

    // TRAP: call an interpreter trap
    // args: ID of the routine to call
    // result: n/a
    BI_TRAP         =   $09;

    // ====================================================
    // interpreter traps
    // ====================================================

    // PRINT: prints the top of the stack to stdio
    TRAP_PRINT      =   $01;

type
    islip_bytecode  = array of byte;

implementation

end.