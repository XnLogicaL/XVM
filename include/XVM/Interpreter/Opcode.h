// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#ifndef XVM_OPCODE_H
#define XVM_OPCODE_H

#include "common.h"

/**
 * @namespace xvm
 */
namespace xvm {

/**
 * @enum Opcode
 * @ingroup xvm_namespace
 * @brief Instruction operation code
 */
enum class Opcode : uint16_t {
  NOP,
  LBL,
  EXIT,
  ADD,
  ADDI,
  ADDF,
  SUB,
  SUBI,
  SUBF,
  MUL,
  MULI,
  MULF,
  DIV,
  DIVI,
  DIVF,
  MOD,
  MODI,
  MODF,
  POW,
  POWI,
  POWF,
  NEG,
  MOV,
  LOADK,
  LOADNIL,
  LOADI,
  LOADF,
  LOADBT,
  LOADBF,
  LOADARR,
  LOADDICT,
  CLOSURE,
  PUSH,
  PUSHK,
  PUSHNIL,
  PUSHI,
  PUSHF,
  PUSHBT,
  PUSHBF,
  DROP,
  GETGLOBAL,
  SETGLOBAL,
  SETUPV,
  GETUPV,
  GETLOCAL,
  SETLOCAL,
  GETARG,
  CAPTURE,
  INC,
  DEC,
  EQ,
  DEQ,
  NEQ,
  AND,
  OR,
  NOT,
  LT,
  GT,
  LTEQ,
  GTEQ,
  JMP,
  JMPIF,
  JMPIFN,
  JMPIFEQ,
  JMPIFNEQ,
  JMPIFLT,
  JMPIFGT,
  JMPIFLTEQ,
  JMPIFGTEQ,
  LJMP,
  LJMPIF,
  LJMPIFN,
  LJMPIFEQ,
  LJMPIFNEQ,
  LJMPIFLT,
  LJMPIFGT,
  LJMPIFLTEQ,
  LJMPIFGTEQ,
  CALL,
  PCALL,
  RET,
  RETBT,
  RETBF,
  RETNIL,
  GETARR,
  SETARR,
  NEXTARR,
  LENARR,
  GETDICT,
  SETDICT,
  NEXTDICT,
  LENDICT,
  CONSTR,
  GETSTR,
  SETSTR,
  LENSTR,
  ICAST,
  FCAST,
  STRCAST,
  BCAST,
};

} // namespace xvm

#endif
