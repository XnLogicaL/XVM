// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_state.h"
#include "xvm_api_impl.h"
#include "xvm_string.h"
#include <cmath>

#define VM_ERROR(message)                                                                          \
    {                                                                                              \
        __ethrow(state, message);                                                                  \
        VM_NEXT();                                                                                 \
    }

#define VM_ERRORF(message, ...)                                                                    \
    {                                                                                              \
        __ethrowf(state, message, __VA_ARGS__);                                                    \
        VM_NEXT();                                                                                 \
    }


#define VM_DISPATCH()                                                                              \
    if constexpr (SingleStep) {                                                                    \
        goto exit;                                                                                 \
    }                                                                                              \
    else {                                                                                         \
        goto dispatch;                                                                             \
    }

#define VM_NEXT()                                                                                  \
    {                                                                                              \
        if constexpr (SingleStep) {                                                                \
            if constexpr (OverrideProgramCounter)                                                  \
                state->pc = pc;                                                                    \
            else                                                                                   \
                state->pc++;                                                                       \
            goto exit;                                                                             \
        }                                                                                          \
        state->pc++;                                                                               \
        goto dispatch;                                                                             \
    }

#define VM_CHECK_RETURN()                                                                          \
    if XVM_UNLIKELY (state->ci_top == state->cis.data) {                                           \
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

using enum Opcode;

// We use implementation functions only in this file.
using namespace impl;

static bool isArithOpcode(Opcode op) {
    return (uint16_t)op >= (uint16_t)ADD && (uint16_t)op <= (uint16_t)FPOW;
}

template<typename A, typename B = A>
static XVM_FORCEINLINE void performArith(Opcode op, A& a, B b) {
    switch (op) {
    case ADD:
    case IADD:
    case FADD:
        a += b;
        break;
    case SUB:
    case ISUB:
    case FSUB:
        a -= b;
        break;
    case MUL:
    case IMUL:
    case FMUL:
        a *= b;
        break;
    case DIV:
    case IDIV:
    case FDIV:
        a /= b;
        break;
    case MOD:
    case IMOD:
    case FMOD:
        a = std::fmod(a, b);
        break;
    case POW:
    case IPOW:
    case FPOW:
        a = std::pow(a, b);
        break;
    default:
        break;
    }
}

static XVM_FORCEINLINE int arith(State* state, Opcode op, Value* lhs, Value* rhs) {
    using enum ValueKind;

    if (!isArithOpcode(op)) {
        return 1;
    }

    if (lhs->type == Int && rhs->type == Int) {
        performArith(op, lhs->u.i, rhs->u.i);
    }
    else {
        auto as_float = [](const Value& v) -> float {
            return v.type == Int ? static_cast<float>(v.u.i) : v.u.f;
        };

        float a = as_float(*lhs);
        float b = as_float(*rhs);

        performArith(op, a, b);

        lhs->u.f = a;
        lhs->type = ValueKind::Float;
    }

    return 0;
}

static XVM_FORCEINLINE void iarith(State* state, Opcode op, Value* lhs, int i) {
    using enum ValueKind;

    if XVM_LIKELY (lhs->type == Int) {
        performArith(op, lhs->u.i, i);
    }
    else if (lhs->type == Float) {
        performArith(op, lhs->u.f, i);
    }
}

static XVM_FORCEINLINE void farith(State* state, Opcode op, Value* lhs, float f) {
    using enum ValueKind;

    if XVM_LIKELY (lhs->type == Int) {
        performArith(op, lhs->u.i, f);
    }
    else if (lhs->type == Float) {
        performArith(op, lhs->u.f, f);
    }
}

template<const bool SingleStep = false, const bool OverrideProgramCounter = false>
static void execute(State* state, Instruction insn = Instruction()) {
#if VM_USE_CGOTO
    static constexpr void* dispatch_table[0xFF] = {VM_DISPATCH_TABLE()};
#endif

dispatch:
    const Instruction* pc = state->pc;

    // Check for errors and attempt handling them.
    // The __ehandle function works by unwinding the stack until
    // either hitting a stack frame flagged as error handler, or, the root
    // stack frame, and the root stack frame cannot be an error handler
    // under any circumstances. Therefore the error will act as a fatal
    // error, being automatically thrown by __ehandle, along with a
    // cstk and debug information.
    if (__echeck(state) && !__ehandle(state)) {
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

        VM_CASE(ADD)
        VM_CASE(SUB)
        VM_CASE(MUL)
        VM_CASE(DIV)
        VM_CASE(MOD)
        VM_CASE(POW) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            arith(state, pc->op, lhs, rhs);
            VM_NEXT();
        }

        VM_CASE(IADD)
        VM_CASE(ISUB)
        VM_CASE(IMUL)
        VM_CASE(IDIV)
        VM_CASE(IMOD)
        VM_CASE(IPOW) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;
            uint16_t ic = state->pc->c;

            int    imm = ((uint32_t)ic << 16) | ib;
            Value* lhs = __getRegister(state, ra);

            iarith(state, pc->op, lhs, imm);
            VM_NEXT();
        }

        VM_CASE(FADD)
        VM_CASE(FSUB)
        VM_CASE(FMUL)
        VM_CASE(FDIV)
        VM_CASE(FMOD)
        VM_CASE(FPOW) {
            uint16_t ra = state->pc->a;
            uint16_t fb = state->pc->b;
            uint16_t fc = state->pc->c;

            float  imm = ((uint32_t)fc << 16) | fb;
            Value* lhs = __getRegister(state, ra);

            farith(state, pc->op, lhs, imm);
            VM_NEXT();
        }

        VM_CASE(NEG) {
            uint16_t  ra = state->pc->a;
            Value*    val = __getRegister(state, ra);
            ValueKind type = val->type;

            if (type == ValueKind::Int) {
                val->u.i = -val->u.i;
            }
            else if (type == ValueKind::Float) {
                val->u.f = -val->u.f;
            }

            VM_NEXT();
        }

        VM_CASE(MOV) {
            uint16_t rdst = state->pc->a;
            uint16_t rsrc = state->pc->b;
            Value*   src_val = __getRegister(state, rsrc);

            __setRegister(state, rdst, __clone(src_val));
            VM_NEXT();
        }

        VM_CASE(INC) {
            uint16_t rdst = state->pc->a;
            Value*   dst_val = __getRegister(state, rdst);

            if XVM_LIKELY (dst_val->type == ValueKind::Int) {
                dst_val->u.i++;
            }
            else if XVM_UNLIKELY (dst_val->type == ValueKind::Float) {
                dst_val->u.f++;
            }

            VM_NEXT();
        }

        VM_CASE(DEC) {
            uint16_t rdst = state->pc->a;
            Value*   dst_val = __getRegister(state, rdst);

            if XVM_LIKELY (dst_val->type == ValueKind::Int) {
                dst_val->u.i--;
            }
            else if XVM_UNLIKELY (dst_val->type == ValueKind::Float) {
                dst_val->u.f--;
            }

            VM_NEXT();
        }

        VM_CASE(LOADK) {
            uint16_t ra = state->pc->a;
            uint16_t idx = state->pc->b;

            const Value& kval = __getConstant(state, idx);

            __setRegister(state, ra, __clone(&kval));
            VM_NEXT();
        }

        VM_CASE(LOADNIL) {
            uint16_t ra = state->pc->a;

            __setRegister(state, ra, XVM_NIL);
            VM_NEXT();
        }

        VM_CASE(LOADI) {
            uint16_t ra = state->pc->a;
            int      imm = ((uint32_t)state->pc->b << 16) | state->pc->a;

            __setRegister(state, ra, Value(imm));
            VM_NEXT();
        }

        VM_CASE(LOADF) {
            uint16_t ra = state->pc->a;
            float    imm = ((uint32_t)state->pc->b << 16) | state->pc->a;

            __setRegister(state, ra, Value(imm));
            VM_NEXT();
        }

        VM_CASE(LOADBT) {
            uint16_t ra = state->pc->a;

            __setRegister(state, ra, Value(true));
            VM_NEXT();
        }

        VM_CASE(LOADBF) {
            uint16_t ra = state->pc->a;

            __setRegister(state, ra, Value(false));
            VM_NEXT();
        }

        VM_CASE(LOADARR) {
            uint16_t ra = state->pc->a;

            Value arr(new Array());

            __setRegister(state, ra, std::move(arr));
            VM_NEXT();
        }

        VM_CASE(LOADDICT) {
            uint16_t ra = state->pc->a;

            Value dict(new Dict());

            __setRegister(state, ra, std::move(dict));
            VM_NEXT();
        }

        VM_CASE(CLOSURE) {
            uint16_t ra = state->pc->a;
            uint16_t lb = state->pc->b;
            uint16_t cc = state->pc->c;

            const auto&        data = __getAddressData(state, state->pc);
            const std::string& comment = data.comment;

            size_t idlen = comment.size();
            char*  idbuf = state->salloc.allocBytes(idlen + 1 /*for nullbyte*/);
            std::strcpy(idbuf, comment.c_str());

            Function f;
            f.id = idbuf;
            f.code = ++state->pc;
            f.size = lb;

            Callable c;
            c.arity = cc;
            c.type = CallableKind::Function;
            c.u = {.fn = std::move(f)};

            Closure* closure = new Closure();
            closure->callee = std::move(c);

            __initClosure(state, closure, lb);
            __setRegister(state, ra, Value(closure));

            // Do not increment program counter, as __initClosure automatically positions it
            // to the correct instruction.
            VM_DISPATCH();
        }

        VM_CASE(GETUPV) {
            uint16_t ra = state->pc->a;
            uint16_t ib = state->pc->b;

            UpValue* upv = __getClosureUpv((state->ci_top - 1)->closure, ib);

            __setRegister(state, ra, __clone(upv->value));
            VM_NEXT();
        }

        VM_CASE(SETUPV) {
            uint16_t ra = state->pc->a;
            uint16_t upv_id = state->pc->b;

            Value* val = __getRegister(state, ra);

            __setClosureUpv((state->ci_top - 1)->closure, upv_id, val);
            VM_NEXT();
        }

        VM_CASE(PUSH) {
            uint16_t ra = state->pc->a;
            Value*   val = __getRegister(state, ra);

            __push(state, std::move(*val));
            VM_NEXT();
        }

        VM_CASE(PUSHK) {
            uint16_t const_idx = state->pc->a;
            Value    constant = __getConstant(state, const_idx);

            __push(state, std::move(constant));
            VM_NEXT();
        }

        VM_CASE(PUSHNIL) {
            __push(state, XVM_NIL);
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
            uint16_t ra = state->pc->a;
            uint16_t off = state->pc->b;
            Value*   val = __getLocal(state, off);

            __setRegister(state, ra, __clone(val));
            VM_NEXT();
        }

        VM_CASE(SETLOCAL) {
            uint16_t ra = state->pc->a;
            uint16_t off = state->pc->b;
            Value*   val = __getRegister(state, ra);

            __setLocal(state, off, std::move(*val));
            VM_NEXT();
        }

        VM_CASE(GETARG) {
            uint16_t ra = state->pc->a;
            uint16_t off = state->pc->b;

            StkId val = state->stk_base - off - 1;

            __setRegister(state, ra, __clone(val));
            VM_NEXT();
        }

        VM_CASE(GETGLOBAL) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* key = __getRegister(state, rb);
            Value* global = __getGlobal(state, key->u.str->data);

            __setRegister(state, ra, __clone(global));
            VM_NEXT();
        }

        VM_CASE(SETGLOBAL) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* key = __getRegister(state, rb);
            Value* global = __getRegister(state, ra);

            __setGlobal(state, key->u.str->data, std::move(*global));
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

            bool result = __compare(lhs, rhs);
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

            bool result = __compareDeep(lhs, rhs);
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

            bool result = __compare(lhs, rhs);
            __setRegister(state, ra, Value(result));

            VM_NEXT();
        }

        VM_CASE(AND) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);
            bool   cond = __toBool(lhs) && __toBool(rhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(OR) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);
            bool   cond = __toBool(lhs) || __toBool(rhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(NOT) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, rb);
            bool   cond = !__toBool(lhs);

            __setRegister(state, ra, Value(cond));
            VM_NEXT();
        }

        VM_CASE(LT) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t rc = state->pc->c;

            Value* lhs = __getRegister(state, rb);
            Value* rhs = __getRegister(state, rc);

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.i < rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) < rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.f < static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.i > rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) > rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.f > static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.i <= rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) <= rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.f <= static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.i >= rhs->u.i));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    __setRegister(state, ra, Value(static_cast<float>(lhs->u.i) >= rhs->u.f));
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    __setRegister(state, ra, Value(lhs->u.f >= static_cast<float>(rhs->u.i)));
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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
            if (__toBool(cond_val)) {
                state->pc += offset;
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(JMPIFN) {
            uint16_t cond = state->pc->a;
            int16_t  offset = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (!__toBool(cond_val)) {
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

                if XVM_UNLIKELY (lhs == rhs || __compare(lhs, rhs)) {
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

                if XVM_LIKELY (lhs != rhs || !__compare(lhs, rhs)) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i < rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) < rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f < static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i > rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) > rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f > static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i <= rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) <= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f <= static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i >= rhs->u.i) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) >= rhs->u.f) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f >= static_cast<float>(rhs->u.i)) {
                        state->pc += offset;
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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
            if (__toBool(cond_val)) {
                state->pc = __getLabelAddress(state, label);
                goto dispatch;
            }

            VM_NEXT();
        }

        VM_CASE(LJMPIFN) {
            uint16_t cond = state->pc->a;
            uint16_t label = state->pc->b;

            Value* cond_val = __getRegister(state, cond);
            if (!__toBool(cond_val)) {
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

                if XVM_UNLIKELY (lhs == rhs || __compare(lhs, rhs)) {
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

                if XVM_LIKELY (lhs != rhs || !__compare(lhs, rhs)) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i < rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) < rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f < static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i > rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) > rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f > static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i <= rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) <= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f <= static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            if XVM_LIKELY (lhs->type == ValueKind::Int) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.i >= rhs->u.i) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
                    if (static_cast<float>(lhs->u.i) >= rhs->u.f) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
            }
            else if XVM_UNLIKELY (lhs->type == ValueKind::Float) {
                if XVM_LIKELY (rhs->type == ValueKind::Int) {
                    if (lhs->u.f >= static_cast<float>(rhs->u.i)) {
                        state->pc = __getLabelAddress(state, label);
                        goto dispatch;
                    }
                }
                else if XVM_UNLIKELY (rhs->type == ValueKind::Float) {
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

            Value* fn_val = __getRegister(state, fn);

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

            __pcall(state, fn_val->u.clsr);

            if constexpr (SingleStep)
                goto exit;
            else
                goto dispatch;
        }

        VM_CASE(RETNIL) {
            __closeClosureUpvs((state->ci_top - 1)->closure);
            __return(state, XVM_NIL);

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
            uint16_t ra = state->pc->a;
            Value*   val = __getRegister(state, ra);

            __return(state, std::move(*val));

            VM_CHECK_RETURN();
            VM_NEXT();
        }

        VM_CASE(GETARR) {
            uint16_t ra = state->pc->a;
            uint16_t tbl = state->pc->b;
            uint16_t key = state->pc->c;

            Value* value = __getRegister(state, tbl);
            Value* index = __getRegister(state, key);
            Value* result = __getArrayField(value->u.arr, index->u.i);

            __setRegister(state, ra, __clone(result));
            VM_NEXT();
        }

        VM_CASE(SETARR) {
            uint16_t ra = state->pc->a;
            uint16_t tbl = state->pc->b;
            uint16_t key = state->pc->c;

            Value* array = __getRegister(state, tbl);
            Value* index = __getRegister(state, key);
            Value* value = __getRegister(state, ra);

            __setArrayField(array->u.arr, index->u.i, std::move(*value));
            VM_NEXT();
        }

        VM_CASE(NEXTARR) {
            static std::unordered_map<void*, uint16_t> next_table;

            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value*   val = __getRegister(state, rb);
            void*    ptr = __toPointer(val);
            uint16_t key = 0;

            auto it = next_table.find(ptr);
            if (it != next_table.end()) {
                key = ++it->second;
            }
            else {
                next_table[ptr] = 0;
            }

            Value* field = __getArrayField(val->u.arr, key);
            __setRegister(state, ra, __clone(field));
            VM_NEXT();
        }

        VM_CASE(LENARR) {
            uint16_t ra = state->pc->a;
            uint16_t tbl = state->pc->b;

            Value* val = __getRegister(state, tbl);
            int    size = __getArraySize(val->u.arr);

            __setRegister(state, ra, Value(size));
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
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* lhs = __getRegister(state, ra);
            Value* rhs = __getRegister(state, rb);

            String* lstr = lhs->u.str;
            String* rstr = rhs->u.str;
            String* str = __concatString(lstr, rstr);

            __setRegister(state, ra, Value(str));
            VM_NEXT();
        }

        VM_CASE(GETSTR) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;
            uint16_t ic = state->pc->c;

            Value*  val = __getRegister(state, ra);
            String* str = val->u.str;
            if (ic + 1 > str->size) {
                VM_ERROR("string index out of range");
            }

            char chr = __getString(str, ic);

            TempBuf<char> buf(2);
            buf.data[0] = chr;
            buf.data[1] = '\0';

            __setRegister(state, rb, Value(buf.data));
            VM_NEXT();
        }

        VM_CASE(SETSTR) {
            uint16_t ra = state->pc->a;
            uint16_t cb = state->pc->b;
            uint16_t ic = state->pc->c;

            Value*  val = __getRegister(state, ra);
            String* str = val->u.str;
            if (ic + 1 > str->size) {
                VM_ERROR("string index out of range");
            }

            __setString(str, ic, (char)cb);
            VM_NEXT();
        }

        VM_CASE(ICAST) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* target = __getRegister(state, rb);

            bool fail;
            int  result = __toInt(target, &fail);

            if (fail) {
                VM_ERROR("Integer cast failed");
            }

            __setRegister(state, ra, Value(result));
            VM_NEXT();
        }

        VM_CASE(FCAST) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* target = __getRegister(state, rb);

            bool  fail;
            float result = __toFloat(target, &fail);

            if (fail) {
                VM_ERROR("Float cast failed");
            }

            __setRegister(state, ra, Value(result));
            VM_NEXT();
        }

        VM_CASE(STRCAST) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* target = __getRegister(state, rb);
            auto   result = __toString(target);

            __setRegister(state, ra, Value(new String(result.c_str())));
            VM_NEXT();
        }

        VM_CASE(BCAST) {
            uint16_t ra = state->pc->a;
            uint16_t rb = state->pc->b;

            Value* target = __getRegister(state, rb);
            auto   result = __toString(target);

            __setRegister(state, ra, Value(new String(result.c_str())));
            VM_NEXT();
        }
    }

exit:;
}

void execute(State& state) {
    execute<false, false>(&state);
}

void executeStep(State& state, std::optional<Instruction> insn) {
    insn.has_value() ? execute<true, true>(&state, *insn) : execute<true, false>(&state);
}

} // namespace xvm
