// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/State.h>
#include <Interpreter/ApiImpl.h>
#include <Interpreter/TString.h>
#include <cmath>

#define VM_ERROR(message)                                                                          \
    do {                                                                                           \
        __setError(state, message);                                                                \
        VM_NEXT();                                                                                 \
    } while (0)

#define VM_FATAL(message)                                                                          \
    do {                                                                                           \
        std::cerr << "VM terminated with message: " << message << '\n';                            \
        std::abort();                                                                              \
    } while (0)

#define VM_NEXT()                                                                                  \
    do {                                                                                           \
        if constexpr (SingleStep) {                                                                \
            if constexpr (OverrideProgramCounter)                                                  \
                state->pc = savedpc;                                                               \
            else                                                                                   \
                state->pc++;                                                                       \
            goto exit;                                                                             \
        }                                                                                          \
        state->pc++;                                                                               \
        goto dispatch;                                                                             \
    } while (0)

#define VM_CHECK_RETURN()                                                                          \
    if XVM_UNLIKELY (state->callstack->sp == 0) {                                                  \
        goto exit;                                                                                 \
    }

// Stolen from Luau :)
// Whether to use a dispatch table for instruction loading.
#define VM_USE_CGOTO XVMC == CGCC || XVMC == CCLANG

#if VM_USE_CGOTO
#define VM_CASE(op) CASE_##op:
#else
#define VM_CASE(op) case op:
#endif

#define VM_DISPATCH_OP(op) &&CASE_##op
#define VM_DISPATCH_TABLE()                                                                        \
    VM_DISPATCH_OP(NOP), VM_DISPATCH_OP(LBL), VM_DISPATCH_OP(EXIT), VM_DISPATCH_OP(ADD),           \
      VM_DISPATCH_OP(IADD), VM_DISPATCH_OP(FADD), VM_DISPATCH_OP(SUB), VM_DISPATCH_OP(ISUB),       \
      VM_DISPATCH_OP(FSUB), VM_DISPATCH_OP(MUL), VM_DISPATCH_OP(IMUL), VM_DISPATCH_OP(FMUL),       \
      VM_DISPATCH_OP(DIV), VM_DISPATCH_OP(IDIV), VM_DISPATCH_OP(FDIV), VM_DISPATCH_OP(MOD),        \
      VM_DISPATCH_OP(IMOD), VM_DISPATCH_OP(FMOD), VM_DISPATCH_OP(POW), VM_DISPATCH_OP(IPOW),       \
      VM_DISPATCH_OP(FPOW), VM_DISPATCH_OP(NEG), VM_DISPATCH_OP(MOV), VM_DISPATCH_OP(LOADK),       \
      VM_DISPATCH_OP(LOADNIL), VM_DISPATCH_OP(LOADI), VM_DISPATCH_OP(LOADF),                       \
      VM_DISPATCH_OP(LOADBT), VM_DISPATCH_OP(LOADBF), VM_DISPATCH_OP(LOADARR),                     \
      VM_DISPATCH_OP(LOADDICT), VM_DISPATCH_OP(CLOSURE), VM_DISPATCH_OP(PUSH),                     \
      VM_DISPATCH_OP(PUSHK), VM_DISPATCH_OP(PUSHNIL), VM_DISPATCH_OP(PUSHI),                       \
      VM_DISPATCH_OP(PUSHF), VM_DISPATCH_OP(PUSHBT), VM_DISPATCH_OP(PUSHBF), VM_DISPATCH_OP(DROP), \
      VM_DISPATCH_OP(GETGLOBAL), VM_DISPATCH_OP(SETGLOBAL), VM_DISPATCH_OP(SETUPV),                \
      VM_DISPATCH_OP(GETUPV), VM_DISPATCH_OP(GETLOCAL), VM_DISPATCH_OP(SETLOCAL),                  \
      VM_DISPATCH_OP(GETARG), VM_DISPATCH_OP(CAPTURE), VM_DISPATCH_OP(INC), VM_DISPATCH_OP(DEC),   \
      VM_DISPATCH_OP(EQ), VM_DISPATCH_OP(DEQ), VM_DISPATCH_OP(NEQ), VM_DISPATCH_OP(AND),           \
      VM_DISPATCH_OP(OR), VM_DISPATCH_OP(NOT), VM_DISPATCH_OP(LT), VM_DISPATCH_OP(GT),             \
      VM_DISPATCH_OP(LTEQ), VM_DISPATCH_OP(GTEQ), VM_DISPATCH_OP(JMP), VM_DISPATCH_OP(JMPIF),      \
      VM_DISPATCH_OP(JMPIFN), VM_DISPATCH_OP(JMPIFEQ), VM_DISPATCH_OP(JMPIFNEQ),                   \
      VM_DISPATCH_OP(JMPIFLT), VM_DISPATCH_OP(JMPIFGT), VM_DISPATCH_OP(JMPIFLTEQ),                 \
      VM_DISPATCH_OP(JMPIFGTEQ), VM_DISPATCH_OP(LJMP), VM_DISPATCH_OP(LJMPIF),                     \
      VM_DISPATCH_OP(LJMPIFN), VM_DISPATCH_OP(LJMPIFEQ), VM_DISPATCH_OP(LJMPIFNEQ),                \
      VM_DISPATCH_OP(LJMPIFLT), VM_DISPATCH_OP(LJMPIFGT), VM_DISPATCH_OP(LJMPIFLTEQ),              \
      VM_DISPATCH_OP(LJMPIFGTEQ), VM_DISPATCH_OP(CALL), VM_DISPATCH_OP(PCALL),                     \
      VM_DISPATCH_OP(RET), VM_DISPATCH_OP(RETBT), VM_DISPATCH_OP(RETBF), VM_DISPATCH_OP(RETNIL),   \
      VM_DISPATCH_OP(GETARR), VM_DISPATCH_OP(SETARR), VM_DISPATCH_OP(NEXTARR),                     \
      VM_DISPATCH_OP(LENARR), VM_DISPATCH_OP(GETDICT), VM_DISPATCH_OP(SETDICT),                    \
      VM_DISPATCH_OP(NEXTDICT), VM_DISPATCH_OP(LENDICT), VM_DISPATCH_OP(CONSTR),                   \
      VM_DISPATCH_OP(GETSTR), VM_DISPATCH_OP(SETSTR), VM_DISPATCH_OP(LENSTR),                      \
      VM_DISPATCH_OP(ICAST), VM_DISPATCH_OP(FCAST), VM_DISPATCH_OP(STRCAST), VM_DISPATCH_OP(BCAST)

namespace xvm {

using enum ValueKind;
using enum Opcode;

// We use implementation functions only in this file.
using namespace impl;

template<const bool SingleStep = false, const bool OverrideProgramCounter = false>
void __execute(State* state, Instruction insn = Instruction()) {
#if VM_USE_CGOTO
    static constexpr void* dispatch_table[0xFF] = {VM_DISPATCH_TABLE()};
#endif

dispatch:
    Instruction* savedpc = state->pc;

    // Check for errors and attempt handling them.
    // The __handleError function works by unwinding the stack until
    // either hitting a stack frame flagged as error handler, or, the root
    // stack frame, and the root stack frame cannot be an error handler
    // under any circumstances. Therefore the error will act as a fatal
    // error, being automatically thrown by __handleError, along with a
    // callstack and debug information.
    if (__hasError(state) && !__handleError(state)) {
        goto exit;
    }

    if constexpr (SingleStep && OverrideProgramCounter) {
        state->pc = &insn;
    }

#if VM_USE_CGOTO
    goto* dispatch_table[(uint16_t)state->pc->op];
#else
    switch (state->pc->op)
#endif
    {
        // Handle special/opcodes
        VM_CASE(NOP)
        VM_CASE(GETDICT)
        VM_CASE(SETDICT)
        VM_CASE(LENDICT)
        VM_CASE(NEXTDICT)
        VM_CASE(CAPTURE)
        VM_CASE(LBL) {
            VM_NEXT();
        }

        VM_CASE(ADD) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.i += rhs->u.i;
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = static_cast<float>(lhs->u.i) + rhs->u.f;
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.f += static_cast<float>(rhs->u.i);
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f += rhs->u.f;
                }
            }

            VM_NEXT();
        }
        VM_CASE(IADD) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i += imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f += imm;
            }

            VM_NEXT();
        }
        VM_CASE(FADD) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            float  imm = ((uint32_t)fc << 16) | fb;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i += imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f += imm;
            }

            VM_NEXT();
        }

        VM_CASE(SUB) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.i -= rhs->u.i;
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = static_cast<float>(lhs->u.i) - rhs->u.f;
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.f -= static_cast<float>(rhs->u.i);
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f -= rhs->u.f;
                }
            }

            VM_NEXT();
        }
        VM_CASE(ISUB) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i -= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f -= imm;
            }

            VM_NEXT();
        }
        VM_CASE(FSUB) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            float  imm = ((uint32_t)fc << 16) | fb;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i -= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f -= imm;
            }

            VM_NEXT();
        }

        VM_CASE(MUL) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.i *= rhs->u.i;
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = static_cast<float>(lhs->u.i) * rhs->u.f;
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.f *= static_cast<float>(rhs->u.i);
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f *= rhs->u.f;
                }
            }

            VM_NEXT();
        }
        VM_CASE(IMUL) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i *= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f *= imm;
            }

            VM_NEXT();
        }
        VM_CASE(FMUL) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            float  imm = ((uint32_t)fc << 16) | fb;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i *= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f *= imm;
            }

            VM_NEXT();
        }

        VM_CASE(DIV) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (rhs->u.i == 0) {
                        VM_ERROR("Division by zero");
                    }

                    lhs->u.i /= rhs->u.i;
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (rhs->u.f == 0.0f) {
                        VM_ERROR("Division by zero");
                    }

                    lhs->u.f = static_cast<float>(lhs->u.i) / rhs->u.f;
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (rhs->u.i == 0) {
                        VM_ERROR("Division by zero");
                    }

                    lhs->u.f /= static_cast<float>(rhs->u.i);
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (rhs->u.f == 0.0f) {
                        VM_ERROR("Division by zero");
                    }

                    lhs->u.f /= rhs->u.f;
                }
            }

            VM_NEXT();
        }
        VM_CASE(IDIV) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;
            if (imm == 0) {
                VM_ERROR("Division by zero");
            }

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i /= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f /= imm;
            }

            VM_NEXT();
        }
        VM_CASE(FDIV) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            float imm = ((uint32_t)fc << 16) | fb;
            if (imm == 0.0f) {
                VM_ERROR("Division by zero");
            }

            Value* lhs = __getRegister(state, ra);

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i /= imm;
            }
            else if (lhs->is_float()) {
                lhs->u.f /= imm;
            }

            VM_NEXT();
        }

        VM_CASE(POW) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.i = std::pow(lhs->u.i, rhs->u.i);
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = std::pow(static_cast<float>(lhs->u.i), rhs->u.f);
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.f = std::pow(lhs->u.f, static_cast<float>(rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = std::pow(lhs->u.f, rhs->u.f);
                }
            }

            VM_NEXT();
        }
        VM_CASE(IPOW) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i = std::pow(lhs->u.i, imm);
            }
            else if (lhs->is_float()) {
                lhs->u.f = std::pow(lhs->u.f, imm);
            }

            VM_NEXT();
        }
        VM_CASE(FPOW) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            float  imm = ((uint32_t)fc << 16) | fb;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i = std::pow(lhs->u.i, imm);
            }
            else if (lhs->is_float()) {
                lhs->u.f = std::pow(lhs->u.f, imm);
            }

            VM_NEXT();
        }

        VM_CASE(MOD) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.i %= rhs->u.i;
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = std::fmod(static_cast<float>(lhs->u.i), rhs->u.f);
                    lhs->type = Float;
                }
            }
            else if (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    lhs->u.f = std::fmod(lhs->u.f, static_cast<float>(rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    lhs->u.f = std::fmod(lhs->u.f, rhs->u.f);
                }
            }

            VM_NEXT();
        }
        VM_CASE(IMOD) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            int    imm = ((uint32_t)ic << 16) | ib;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i = std::fmod(lhs->u.i, imm);
            }
            else if (lhs->is_float()) {
                lhs->u.f = std::fmod(lhs->u.f, imm);
            }

            VM_NEXT();
        }
        VM_CASE(FMOD) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            Value* lhs = __getRegister(state, ra);
            float  imm = ((uint32_t)fc << 16) | fb;

            if XVM_LIKELY (lhs->is_int()) {
                lhs->u.i = std::fmod(lhs->u.i, imm);
            }
            else if (lhs->is_float()) {
                lhs->u.f = std::fmod(lhs->u.f, imm);
            }

            VM_NEXT();
        }

        VM_CASE(NEG) {
            uint16_t  dst = state->pc->a;
            Value*    val = __getRegister(state, dst);
            ValueKind type = val->type;

            if (type == Int) {
                val->u.i = -val->u.i;
            }
            else if (type == Float) {
                val->u.f = -val->u.f;
            }

            VM_NEXT();
        }

        VM_CASE(MOV) {
            uint16_t rdst = state->pc->a;
            uint16_t rsrc = state->pc->b;
            Value*   src_val = __getRegister(state, rsrc);

            __setRegister(state, rdst, src_val->clone());
            VM_NEXT();
        }

        VM_CASE(INC) {
            uint16_t rdst = state->pc->a;
            Value*   dst_val = __getRegister(state, rdst);

            if XVM_LIKELY (dst_val->is_int()) {
                dst_val->u.i++;
            }
            else if XVM_UNLIKELY (dst_val->is_float()) {
                dst_val->u.f++;
            }

            VM_NEXT();
        }

        VM_CASE(DEC) {
            uint16_t rdst = state->pc->a;
            Value*   dst_val = __getRegister(state, rdst);

            if XVM_LIKELY (dst_val->is_int()) {
                dst_val->u.i--;
            }
            else if XVM_UNLIKELY (dst_val->is_float()) {
                dst_val->u.f--;
            }

            VM_NEXT();
        }

        VM_CASE(LOADK) {
            uint16_t dst = state->pc->a;
            uint16_t idx = state->pc->b;

            const Value& kval = __getK(state, idx);

            __setRegister(state, dst, kval.clone());
            VM_NEXT();
        }

        VM_CASE(LOADNIL) {
            uint16_t dst = state->pc->a;

            __setRegister(state, dst, Value());
            VM_NEXT();
        }

        VM_CASE(LOADI) {
            uint16_t dst = state->pc->a;
            int      imm = ((uint32_t)state->pc->b << 16) | state->pc->a;

            __setRegister(state, dst, Value(imm));
            VM_NEXT();
        }

        VM_CASE(LOADF) {
            uint16_t dst = state->pc->a;
            float    imm = ((uint32_t)state->pc->b << 16) | state->pc->a;

            __setRegister(state, dst, Value(imm));
            VM_NEXT();
        }

        VM_CASE(LOADBT) {
            uint16_t dst = state->pc->a;
            __setRegister(state, dst, Value(true));
            VM_NEXT();
        }

        VM_CASE(LOADBF) {
            uint16_t dst = state->pc->a;
            __setRegister(state, dst, Value(false));
            VM_NEXT();
        }

        VM_CASE(LOADARR) {
            uint16_t dst = state->pc->a;
            Value    arr(new struct Array());

            __setRegister(state, dst, std::move(arr));
            VM_NEXT();
        }

        VM_CASE(LOADDICT) {
            uint16_t dst = state->pc->a;
            Value    dict(new struct Dict());

            __setRegister(state, dst, std::move(dict));
            VM_NEXT();
        }

        VM_CASE(CLOSURE) {
            uint16_t dst = state->pc->a;
            uint16_t len = state->pc->b;
            uint16_t argc = state->pc->c;

            const InstructionData& data = __getAddressData(state, state->pc);
            const std::string&     comment = data.comment;

            struct Function f;
            f.code = ++state->pc;
            f.size = len;

            Callable c;
            c.arity = argc;
            c.type = CallableKind::Function;
            c.u = {.fn = std::move(f)};

            Closure* closure = new Closure();
            closure->callee = std::move(c);

            __initClosure(state, closure, len);
            __setRegister(state, dst, Value(closure));

            // Do not increment program counter, as __initClosure automatically positions it
            // to the correct instruction.
            if constexpr (SingleStep)
                goto exit;
            else
                goto dispatch;
        }

        VM_CASE(GETUPV) {
            uint16_t dst = state->pc->a;
            uint16_t upv_id = state->pc->b;
            UpValue* upv = __getClosureUpv(__callframe(state)->closure, upv_id);

            __setRegister(state, dst, upv->value->clone());
            VM_NEXT();
        }

        VM_CASE(SETUPV) {
            uint16_t src = state->pc->a;
            uint16_t upv_id = state->pc->b;
            Value*   val = __getRegister(state, src);

            __setClosureUpv(__callframe(state)->closure, upv_id, *val);
            VM_NEXT();
        }

        VM_CASE(PUSH) {
            uint16_t src = state->pc->a;
            Value*   val = __getRegister(state, src);

            __push(state, std::move(*val));
            VM_NEXT();
        }

        VM_CASE(PUSHK) {
            uint16_t const_idx = state->pc->a;
            Value    constant = __getK(state, const_idx);

            __push(state, std::move(constant));
            VM_NEXT();
        }

        VM_CASE(PUSHNIL) {
            __push(state, Value());
            VM_NEXT();
        }

        VM_CASE(PUSHI) {
            int imm = ((uint32_t)state->pc->b << 16) | state->pc->a;
            __push(state, Value(imm));
            VM_NEXT();
        }

        VM_CASE(PUSHF) {
            float imm = ((uint32_t)state->pc->b << 16) | state->pc->a;
            __push(state, Value(imm));
            VM_NEXT();
        }

        VM_CASE(PUSHBT) {
            __push(state, Value(true));
            VM_NEXT();
        }

        VM_CASE(PUSHBF) {
            __push(state, Value(false));
            VM_NEXT();
        }

        VM_CASE(DROP) {
            __drop(state);
            VM_NEXT();
        }

        VM_CASE(GETLOCAL) {
            uint16_t dst = state->pc->a;
            uint16_t off = state->pc->b;
            Value*   val = __getLocal(state, off);

            __setRegister(state, dst, val->clone());
            VM_NEXT();
        }

        VM_CASE(SETLOCAL) {
            uint16_t src = state->pc->a;
            uint16_t off = state->pc->b;
            Value*   val = __getRegister(state, src);

            __setLocal(state, off, std::move(*val));
            VM_NEXT();
        }

        VM_CASE(GETARG) {
            uint16_t dst = state->pc->a;
            uint16_t off = state->pc->b;
            Value*   val = __getRegister(state, state->args + off);

            __setRegister(state, dst, val->clone());
            VM_NEXT();
        }

        VM_CASE(GETGLOBAL) {
            uint16_t dst = state->pc->a;
            uint16_t key = state->pc->b;

            Value*         key_obj = __getRegister(state, key);
            struct String* key_str = key_obj->u.str;
            const Value&   global = state->globals->get(key_str->data);

            __setRegister(state, dst, global.clone());
            VM_NEXT();
        }

        VM_CASE(SETGLOBAL) {
            uint16_t src = state->pc->a;
            uint16_t key = state->pc->b;

            Value*         key_obj = __getRegister(state, key);
            struct String* key_str = key_obj->u.str;
            Value*         global = __getRegister(state, src);

            state->globals->set(key_str->data, std::move(*global));
            VM_NEXT();
        }

        VM_CASE(EQ) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            if XVM_UNLIKELY (rb == rc) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_UNLIKELY (lhs == rhs) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            bool result = __compare(*lhs, *rhs);
            __setRegister(state, ra, Value(result));

            VM_NEXT();
        }

        VM_CASE(DEQ) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            if XVM_UNLIKELY (rb == rc) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_UNLIKELY (lhs == rhs) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            bool result = __compareDeep(*lhs, *rhs);
            __setRegister(state, ra, Value(result));

            VM_NEXT();
        }

        VM_CASE(NEQ) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            if XVM_LIKELY (rb != rc) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs != rhs) {
                __setRegister(state, ra, Value(true));
                VM_NEXT();
            }

            bool result = __compare(*lhs, *rhs);
            __setRegister(state, ra, Value(result));

            VM_NEXT();
        }

        VM_CASE(AND) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);
            bool   cond = __toCxxBool(*lhs) && __toCxxBool(*rhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(OR) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);
            bool   cond = __toCxxBool(*lhs) || __toCxxBool(*rhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(NOT) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, rb);
            bool   cond = !__toCxxBool(*lhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(LT) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.i < rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) < rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.f < static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(lhs->u.f < rhs->u.f));
                }
            }

            VM_NEXT();
        }

        VM_CASE(GT) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.i > rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) > rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.f > static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(lhs->u.f > rhs->u.f));
                }
            }

            VM_NEXT();
        }

        VM_CASE(LTEQ) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.i <= rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) <= rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.f <= static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(lhs->u.f <= rhs->u.f));
                }
            }

            VM_NEXT();
        }

        VM_CASE(GTEQ) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.i >= rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) >= rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    __setRegister(state, ra, Value(lhs->u.f >= static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    __setRegister(state, ra, Value(lhs->u.f >= rhs->u.f));
                }
            }

            VM_NEXT();
        }

        VM_CASE(EXIT) {
            goto exit;
        }

        VM_CASE(JMP) {
            int16_t offset = state->pc->a;
            state->pc += offset;
            goto dispatch;
        }

        VM_CASE(JMPIF) {
            uint16_t cond = state->pc->a;
            int16_t  offset = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (__toCxxBool(*cond_val)) {
                state->pc += offset;
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFN) {
            uint16_t cond = state->pc->a;
            int16_t  offset = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (!__toCxxBool(*cond_val)) {
                state->pc += offset;
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            if XVM_UNLIKELY (cond_lhs == cond_rhs) {
                state->pc += offset;
                goto dispatch;
            }
            else {
                Value* lhs = __getRegister(state, cond_lhs);
                Value* rhs = __getRegister(state, cond_rhs);

                if XVM_UNLIKELY (lhs == rhs || __compare(*lhs, *rhs)) {
                    state->pc += offset;
                    goto dispatch;
                }
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFNEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            if XVM_LIKELY (cond_lhs != cond_rhs) {
                state->pc += offset;
                goto dispatch;
            }
            else {
                Value* lhs = __getRegister(state, cond_lhs);
                Value* rhs = __getRegister(state, cond_rhs);

                if XVM_LIKELY (lhs != rhs || !__compare(*lhs, *rhs)) {
                    state->pc += offset;
                    goto dispatch;
                }
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFLT) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i < rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) < rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f < static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f < rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFGT) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i > rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) > rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f > static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f > rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFLTEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i <= rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) <= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f <= static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f <= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFGTEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            int16_t  offset = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i >= rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) >= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f >= static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f >= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMP) {
            uint16_t label = state->pc->a;

            state->pc = __getLabelAddress(state, label);

            goto dispatch;
        }

        VM_CASE(LJMPIF) {
            uint16_t cond = state->pc->a;
            uint16_t label = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (__toCxxBool(*cond_val)) {
                state->pc = __getLabelAddress(state, label);
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFN) {
            uint16_t cond = state->pc->a;
            uint16_t label = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (!__toCxxBool(*cond_val)) {
                state->pc = __getLabelAddress(state, label);
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            if XVM_UNLIKELY (cond_lhs == cond_rhs) {
                state->pc = __getLabelAddress(state, label);
                goto dispatch;
            }
            else {
                Value* lhs = __getRegister(state, cond_lhs);
                Value* rhs = __getRegister(state, cond_rhs);

                if XVM_UNLIKELY (lhs == rhs || __compare(*lhs, *rhs)) {
                    state->pc = __getLabelAddress(state, label);
                    goto dispatch;
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFNEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            if XVM_LIKELY (cond_lhs != cond_rhs) {
                state->pc = __getLabelAddress(state, label);
                goto dispatch;
            }
            else {
                Value* lhs = __getRegister(state, cond_lhs);
                Value* rhs = __getRegister(state, cond_rhs);

                if XVM_LIKELY (lhs != rhs || !__compare(*lhs, *rhs)) {
                    state->pc = __getLabelAddress(state, label);
                    goto dispatch;
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFLT) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i < rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) < rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f < static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f < rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFGT) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i > rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) > rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f > static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f > rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFLTEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i <= rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) <= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f <= static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f <= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFGTEQ) {
            uint16_t cond_lhs = state->pc->a;
            uint16_t cond_rhs = state->pc->b;
            uint16_t label = state->pc->c;

            Value* lhs = __getRegister(state, cond_lhs);
            Value* rhs = __getRegister(state, cond_rhs);

            if XVM_LIKELY (lhs->is_int()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.i >= rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (static_cast<float>(lhs->u.i) >= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->is_float()) {
                if XVM_LIKELY (rhs->is_int()) {
                    if (lhs->u.f >= static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->is_float()) {
                    if (lhs->u.f >= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }

            VM_NEXT();
        }

        VM_CASE(CALL) {
            uint16_t fn = state->pc->a;
            uint16_t ap = state->pc->b;
            uint16_t rr = state->pc->c;

            Value* fn_val = __getRegister(state, fn);

            state->args = ap;
            state->ret = rr;

            __call(state, fn_val->u.clsr);

            if constexpr (SingleStep)
                goto exit;
            else
                goto dispatch;
        }

        VM_CASE(PCALL) {
            uint16_t fn = state->pc->a;
            uint16_t ap = state->pc->b;
            uint16_t rr = state->pc->c;

            Value* fn_val = __getRegister(state, fn);

            state->args = ap;
            state->ret = rr;

            __pcall(state, fn_val->u.clsr);

            if constexpr (SingleStep)
                goto exit;
            else
                goto dispatch;
        }

        VM_CASE(RETNIL) {
            __closeClosureUpvs(__callframe(state)->closure);
            __return(state, Value());

            VM_CHECK_RETURN();
            VM_NEXT();
        }

        VM_CASE(RETBT) {
            __return(state, Value(true));

            VM_CHECK_RETURN();
            VM_NEXT();
        }

        VM_CASE(RETBF) {
            __return(state, Value(false));

            VM_CHECK_RETURN();
            VM_NEXT();
        }

        VM_CASE(RET) {
            uint16_t src = state->pc->a;
            Value*   val = __getRegister(state, src);

            __return(state, std::move(*val));

            VM_CHECK_RETURN();
            VM_NEXT();
        }

        VM_CASE(GETARR) {
            uint16_t dst = state->pc->a;
            uint16_t tbl = state->pc->b;
            uint16_t key = state->pc->c;

            Value* value = __getRegister(state, tbl);
            Value* index = __getRegister(state, key);
            Value* result = __getArrayField(value->u.arr, index->u.i);

            __setRegister(state, dst, result->clone());
            VM_NEXT();
        }

        VM_CASE(SETARR) {
            uint16_t src = state->pc->a;
            uint16_t tbl = state->pc->b;
            uint16_t key = state->pc->c;

            Value* array = __getRegister(state, tbl);
            Value* index = __getRegister(state, key);
            Value* value = __getRegister(state, src);

            __setArrayField(array->u.arr, index->u.i, std::move(*value));
            VM_NEXT();
        }

        VM_CASE(NEXTARR) {
            static std::unordered_map<void*, uint16_t> next_table;

            uint16_t dst = state->pc->a;
            uint16_t valr = state->pc->b;

            Value*   val = __getRegister(state, valr);
            void*    ptr = __toPtr(*val);
            uint16_t key = 0;

            auto it = next_table.find(ptr);
            if (it != next_table.end()) {
                key = ++it->second;
            }
            else {
                next_table[ptr] = 0;
            }

            Value* field = __getArrayField(val->u.arr, key);
            __setRegister(state, dst, field->clone());
            VM_NEXT();
        }

        VM_CASE(LENARR) {
            uint16_t dst = state->pc->a;
            uint16_t tbl = state->pc->b;

            Value* val = __getRegister(state, tbl);
            int    size = __getArraySize(val->u.arr);

            __setRegister(state, dst, Value(size));
            VM_NEXT();
        }

        VM_CASE(LENSTR) {
            uint16_t rdst = state->pc->a;
            uint16_t objr = state->pc->b;

            Value* val = __getRegister(state, objr);
            int    len = val->u.str->size;

            __setRegister(state, rdst, Value(len));
            VM_NEXT();
        }

        VM_CASE(CONSTR) {
            uint16_t left = state->pc->a;
            uint16_t right = state->pc->b;

            Value* left_val = __getRegister(state, left);
            Value* right_val = __getRegister(state, right);

            struct String* left_str = left_val->u.str;
            struct String* right_str = right_val->u.str;

            size_t new_length = left_str->size + right_str->size;
            char*  new_string = new char[new_length + 1];

            std::memcpy(new_string, left_str->data, left_str->size);
            std::memcpy(new_string + left_str->size, right_str->data, right_str->size);

            __setRegister(state, left, Value(new_string));

            delete[] new_string;

            VM_NEXT();
        }

        VM_CASE(GETSTR) {
            uint16_t dst = state->pc->a;
            uint16_t str = state->pc->b;
            uint16_t idx = state->pc->c;

            Value*         str_val = __getRegister(state, str);
            struct String* tstr = str_val->u.str;
            char           chr = tstr->data[idx];

            __setRegister(state, dst, Value(new struct String(&chr)));
            VM_NEXT();
        }

        VM_CASE(SETSTR) {
            uint16_t str = state->pc->a;
            uint16_t src = state->pc->b;
            uint16_t idx = state->pc->c;

            Value*         str_val = __getRegister(state, str);
            struct String* tstr = str_val->u.str;

            char chr = static_cast<char>(src);
            VM_NEXT();
        }

        VM_CASE(ICAST) {
            uint16_t dst = state->pc->a;
            uint16_t src = state->pc->b;

            Value* target = __getRegister(state, src);
            Value  result = __toInt(state, *target);

            __setRegister(state, dst, std::move(result));
            VM_NEXT();
        }

        VM_CASE(FCAST) {
            uint16_t dst = state->pc->a;
            uint16_t src = state->pc->b;

            Value* target = __getRegister(state, src);
            Value  result = __toFloat(state, *target);

            __setRegister(state, dst, std::move(result));
            VM_NEXT();
        }

        VM_CASE(STRCAST) {
            uint16_t dst = state->pc->a;
            uint16_t src = state->pc->b;

            Value* target = __getRegister(state, src);
            Value  result = __toString(*target);

            __setRegister(state, dst, std::move(result));
            VM_NEXT();
        }

        VM_CASE(BCAST) {
            uint16_t dst = state->pc->a;
            uint16_t src = state->pc->b;

            Value* target = __getRegister(state, src);
            Value  result = __toBool(*target);

            __setRegister(state, dst, std::move(result));
            VM_NEXT();
        }
    }

exit:;
}

void State::execute() {
    __execute<false, false>(this);
}

void State::execute_step(std::optional<Instruction> insn) {
    if (insn.has_value()) {
        __execute<true, true>(this, *insn);
    }
    else {
        __execute<true, false>(this, *insn);
    }
}

} // namespace xvm
