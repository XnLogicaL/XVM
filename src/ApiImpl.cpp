// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/ApiImpl.h>
#include <Interpreter/TString.h>
#include <cmath>

namespace xvm {

namespace impl {

static std::unordered_map<NativeFn, std::string> native_fn_ids{};

const InstructionData& __getAddressData(const State* state, const Instruction* const pc) {
    size_t offset = state->pc - pc;
    return state->holder.insnData.at(offset);
}

static std::string nativeId(NativeFn fn) {
    auto it = native_fn_ids.find(fn);
    if (it != native_fn_ids.end()) {
        return std::format("function {}", it->second);
    }
    else {
        return std::format("function <native@0x{:x}>", reinterpret_cast<uintptr_t>(fn));
    }
}

std::string __getFuncSig(const Callable& func) {
    return func.type == CallableKind::Function ? std::format("function {}", func.u.fn.id)
                                               : nativeId(func.u.ntv);
}

void __setError(State* state, const std::string& message) {
    const Callable& func = __callframe((State*)state)->closure->callee;
    state->err->has_error = true;
    state->err->funcsig = __getFuncSig(func);
    state->err->message = message;
}

void __clearError(State* state) {
    state->err->has_error = false;
}

bool __hasError(const State* state) {
    return state->err->has_error;
}

template<typename T>
static bool unwindStackUntilGuardFrame(State* state, T callback) {
    auto& callstack = state->callstack;

    for (int i = callstack->sp - 1; i >= 0; i--) {
        CallFrame* frame = __callframe(state);
        callback(frame);

        if (frame->protect) {
            return true;
        }

        __popCallframe(state);
    }

    return false;
}

bool __handleError(State* state) {
    std::vector<std::string> sigs;

    bool guardFrameTouched = unwindStackUntilGuardFrame(state, [&sigs, &state](CallFrame* frame) {
        if (frame->protect) {
            const auto& err = state->err;
            const char* raw = err->message.c_str();

            __clearError(state);
            __return(state, Value(new String(raw)));
            return;
        }

        auto sig = __getFuncSig(frame->closure->callee);
        sigs.push_back(sig);
    });

    if (guardFrameTouched) {
        return true;
    }

    std::ostringstream oss;
    oss << state->err->funcsig << ": " << state->err->message << "\n";

    for (size_t i = 0; const std::string& funcsig : sigs) {
        oss << " #" << i++ << ' ' << funcsig << "\n";
    }

    std::cout << oss.str();
    return false;
}

Value __getConstant(const State* state, size_t index) {
    // TODO
    return XVM_NIL;
}

Value __type(const Value& val) {
    using enum ValueKind;
    const char* type = NULL;

    // clang-format off
    switch (val.type) {
    case Nil:      type = "nil"; break;
    case Int:      type = "int"; break;
    case Float:    type = "float"; break;
    case Bool:     type = "bool"; break;
    case String:   type = "string"; break;
    case Function: type = "function"; break;
    case Array:    type = "array"; break;
    case Dict:     type = "dict"; break;
    } // clang-format on

    return Value(new struct String(type));
}

std::string __type_cxx_string(const Value& val) {
    Value _Type = __type(val);
    return std::string(_Type.u.str->data);
}

void* __toPtr(const Value& val) {
    switch (val.type) {
    case ValueKind::Function:
    case ValueKind::Array:
    case ValueKind::Dict:
    case ValueKind::String:
        // This is technically UB... too bad!
        return reinterpret_cast<void*>(val.u.str);
    default:
        return NULL;
    }
}

CallFrame* __callframe(State* state) {
    auto& callstack = state->callstack;
    return &callstack->frames[callstack->sp - 1];
}

void __pushCallframe(State* state, CallFrame&& frame) {
    if (state->callstack->sp >= 200) {
        __setError(state, "Stack overflow");
        return;
    }

    auto& callstack = state->callstack;
    callstack->frames[callstack->sp++] = std::move(frame);
}

void __popCallframe(State* state) {
    auto& callstack = state->callstack;
    callstack->sp--;
}

template<const bool IsProtected>
void __call_base(State* state, Closure* closure) {
    CallFrame cf;
    cf.protect = IsProtected;
    cf.closure = new Closure(*closure);

    if (closure->callee.type == CallableKind::Function) {
        // Functions are automatically positioned by RET instructions; no need to increment saved
        // program counter.
        cf.savedpc = state->pc;
        __pushCallframe(state, std::move(cf));

        state->pc = closure->callee.u.fn.code;
    }
    else if (closure->callee.type == CallableKind::Native) {
        // Native functions require manual positioning as they don't increment program counter with
        // a RET instruction or similar.
        cf.savedpc = state->pc + 1;
        __pushCallframe(state, std::move(cf));
        __return(state, closure->callee.u.ntv(state));
    }
}

void __call(State* state, Closure* closure) {
    __call_base<false>(state, closure);
}

void __pcall(State* state, Closure* closure) {
    __call_base<true>(state, closure);
}

void __return(State* XVM_RESTRICT state, Value&& retv) {
    CallFrame* current_frame = __callframe(state);
    state->pc = current_frame->savedpc;

    __setRegister(state, state->ret, std::move(retv));
    __popCallframe(state);
}

Value __length(Value& val) {
    if (val.is_string())
        return Value(static_cast<int>(val.u.str->size));
    else if (val.is_array() || val.is_dict()) {
        size_t len = val.is_array() ? __getArraySize(val.u.arr) : __getDictSize(val.u.dict);
        return Value(static_cast<int>(len));
    }

    return XVM_NIL;
}

int __lengthCxx(Value& val) {
    Value len = __length(val);
    return len.is_nil() ? -1 : len.u.i;
}

Value __toString(const Value& val) {
    using enum ValueKind;

    if (val.is_string()) {
        return val.clone();
    }

    switch (val.type) {
    case Int: {
        std::string str = std::to_string(val.u.i);
        return Value(new struct String(str.c_str()));
    }
    case Float: {
        std::string str = std::to_string(val.u.f);
        return Value(new struct String(str.c_str()));
    }
    case Bool:
        return Value(new struct String(val.u.b ? "true" : "false"));
    case Array:
    case Dict: {
        auto type_str = __type_cxx_string(val);
        auto final_str = std::format("<{}@0x{:x}>", type_str, (uintptr_t)__toPtr(val));

        return Value(new struct String(final_str.c_str()));
    }

    case Function: {
        std::string fnty = "native";
        std::string fnn = "";

        if (val.u.clsr->callee.type == CallableKind::Function) {
            fnty = "function ";
            fnn = val.u.clsr->callee.u.fn.id;
        }

        std::string final_str = std::format("<{}{}@0x{:x}>", fnty, fnn, (uintptr_t)val.u.clsr);
        return Value(new struct String(final_str.c_str()));
    }
    default:
        return Value(new struct String("nil"));
    }

    XVM_UNREACHABLE();
    return XVM_NIL;
}

std::string __toCxxString(const Value& val) {
    Value str = __toString(val);
    return std::string(str.u.str->data);
}

std::string __toLitCxxString(const Value& val) {
    Value str = __toString(val);
    return xvm::stresc(str.u.str->data);
}

Value __toBool(const Value& val) {
    if (val.is_bool()) {
        return val.clone();
    }

    return Value(val.type != ValueKind::Nil);
}

bool __toCxxBool(const Value& val) {
    return __toBool(val).u.b;
}

Value __toInt(State* state, const Value& val) {
    using enum ValueKind;

    if (val.is_number()) {
        return val.clone();
    }

    switch (val.type) {
    case String: {
        const std::string& str = val.u.str->data;
        if (str.empty()) {
            return XVM_NIL;
        }

        int int_result;
        auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), int_result);
        if (ec == std::errc() && ptr == str.data() + str.size()) {
            return Value(int_result);
        }

        __setError(state, "Failed to cast String into Int");
        return XVM_NIL;
    }
    case Bool:
        return Value(static_cast<int>(val.u.b));
    default:
        break;
    }

    auto type = __toCxxString(val);
    __setError(state, std::format("Failed to cast {} into Int", type));
    return XVM_NIL;
}

Value __toFloat(State* state, const Value& val) {
    using enum ValueKind;

    if (val.is_number()) {
        return val.clone();
    }

    switch (val.type) {
    case String: {
        const std::string& str = val.u.str->data;
        if (str.empty()) {
            return XVM_NIL;
        }

        float float_result;
        auto [ptr_f, ec_f] = std::from_chars(str.data(), str.data() + str.size(), float_result);
        if (ec_f == std::errc() && ptr_f == str.data() + str.size()) {
            return Value(float_result);
        }

        __setError(state, "Failed to cast string to float");
        return XVM_NIL;
    }
    case Bool:
        return Value(static_cast<float>(val.u.b));
    default:
        break;
    }

    auto type = __type_cxx_string(val);
    __setError(state, std::format("Failed to cast {} to float", type));
    return XVM_NIL;
}

bool __compare(const Value& val0, const Value& val1) {
    using enum ValueKind;

    if (val0.type != val1.type) {
        return false;
    }

    switch (val0.type) {
    case Int:
        return val0.u.i == val1.u.i;
    case Float:
        return val0.u.f == val1.u.f;
    case Bool:
        return val0.u.b == val1.u.b;
    case Nil:
        return true;
    case String:
        return !std::strcmp(val0.u.str->data, val1.u.str->data);
    default:
        return false;
    }

    XVM_UNREACHABLE();
    return false;
};


bool __compareDeep(const Value& val0, const Value& val1) {
    using enum ValueKind;

    if (val0.type != val1.type) {
        return false;
    }

    switch (val0.type) {
    case Int:
        return val0.u.i == val1.u.i;
    case Float:
        return val0.u.f == val1.u.f;
    case Bool:
        return val0.u.b == val1.u.b;
    case Nil:
        return true;
    case String:
        return !std::strcmp(val0.u.str->data, val1.u.str->data);
    case Array: {
        if (__getArraySize(val0.u.arr) != __getArraySize(val1.u.arr)) {
            return false;
        }

        for (size_t i = 0; i < __getArraySize(val0.u.arr); i++) {
            Value* val = __getArrayField(val0.u.arr, i);
            Value* other = __getArrayField(val1.u.arr, i);

            if (!val->compare(*other)) {
                return false;
            }
        }

        return true;
    }
    // TODO: IMPLEMENT OTHER DATA TYPES
    default:
        return false;
    }

    XVM_UNREACHABLE();
    return false;
}

// Automatically resizes UpValue vector of closure by XVM_UPV_RESIZE_FACTOR.
void __resizeClosureUpvs(Closure* closure) {
    uint32_t current_size = closure->count;
    uint32_t new_size = current_size == 0 ? 8 : (current_size * 2);
    UpValue* new_location = new UpValue[new_size];

    // Check if upvalues are initialized
    if (current_size != 0) {
        for (UpValue* ptr = closure->upvs; ptr < closure->upvs + current_size; ptr++) {
            uint32_t offset = ptr - closure->upvs;
            new_location[offset] = std::move(*ptr);
        }

        delete[] closure->upvs;
    }

    // Update closure
    closure->upvs = new_location;
    closure->count = new_size;
}

// Checks if a given index is within the bounds of the UpValue vector of the closure.
// Used for resizing.
bool __rangeCheckClosureUpvs(Closure* closure, size_t index) {
    return closure->count >= index;
}

// Attempts to retrieve UpValue at index <upv_id>.
// Returns NULL if <upv_id> is out of UpValue vector bounds.
UpValue* __getClosureUpv(Closure* closure, size_t upv_id) {
    if (!__rangeCheckClosureUpvs(closure, upv_id)) {
        return NULL;
    }

    return &closure->upvs[upv_id];
}

// Dynamically reassigns UpValue at index <upv_id> the value <val>.
void __setClosureUpv(Closure* closure, size_t upv_id, Value& val) {
    UpValue* _Upv = __getClosureUpv(closure, upv_id);
    if (_Upv != NULL) {
        if (_Upv->value != NULL) {
            *_Upv->value = val.clone();
        }
        else {
            _Upv->value = &val;
        }

        _Upv->valid = true;
    }
}

static void handleCapture(State* state, Closure* closure, size_t* upvalues) {
    if (__rangeCheckClosureUpvs(closure, *upvalues)) {
        __resizeClosureUpvs(closure);
    }

    uint16_t   idx = state->pc->b;
    Value*     value;
    CallFrame* frame = __callframe(state);

    if (state->pc->a == 0) {
        value = &frame->locals[idx];
    }
    else { // Upvalue is captured twice; automatically close it.
        UpValue* upv = &frame->closure->upvs[idx];
        if (upv->valid && upv->open) {
            upv->heap = upv->value->clone();
            upv->value = &upv->heap;
            upv->open = false;
        }
        value = upv->value;
    }

    closure->upvs[(*upvalues)++] = {
      .open = true,
      .valid = true,
      .value = value,
      .heap = XVM_NIL,
    };
}

// Loads closure bytecode by iterating over the Instruction pipeline.
// Handles sentinel/special opcodes like RET or CAPTURE while assembling closure.
void __initClosure(State* state, Closure* closure, size_t len) {
    size_t upvalues = 0;
    for (size_t i = 0; i < len; ++i) {
        if ((state->pc++)->op == Opcode::CAPTURE) {
            handleCapture(state, closure, &upvalues);
        }
    }
}

static void closeUpvalue(UpValue* upv) {
    upv->heap = upv->value->clone();
    upv->value = &upv->heap;
    upv->open = false;
}

// Moves upvalues of the current closure into the heap, "closing" them.
void __closeClosureUpvs(const Closure* closure) {
    // C Function replica compliance
    if (closure->upvs == NULL) {
        return;
    }

    for (UpValue* upv = closure->upvs; upv < closure->upvs + closure->count; upv++) {
        if (upv->valid && upv->open) {
            closeUpvalue(upv);
        }
    }
}

//  ================
// [ Table handling ]
//  ================

// Hashes a dictionary key using the FNV-1a hashing algorithm.
size_t __hashDictKey(const Dict* dict, const char* key) {
    size_t hash = 2166136261u;
    while (*key) {
        hash = (hash ^ *key++) * 16777619;
    }

    return hash % dict->capacity;
}

// Inserts a key-value pair into the hash table component of a given table_obj object.
void __setDictField(const Dict* dict, const char* key, Value val) {
    size_t index = __hashDictKey(dict, key);
    if (index > dict->capacity) {
        // Handle relocation
    }

    Dict::HNode& node = dict->data[index];
    node.key = key;
    node.value = std::move(val);
    dict->csize.valid = false;
}

// Performs a look-up on the given table with a given key. Returns NULL upon lookup failure.
Value* __getDictField(const Dict* dict, const char* key) {
    size_t index = __hashDictKey(dict, key);
    if (index > dict->capacity) {
        return NULL;
    }

    return &dict->data[index].value;
}

// Returns the real size_t of the hashtable component of the given table object.
size_t __getDictSize(const Dict* dict) {
    if (dict->csize.valid) {
        return dict->csize.cache;
    }

    size_t index = 0;
    for (; index < dict->capacity; index++) {
        Dict::HNode& obj = dict->data[index];
        if (obj.value.is_nil()) {
            break;
        }
    }

    dict->csize.cache = index;
    dict->csize.valid = true;

    return index;
}

// Checks if the given index is out of bounds of a given tables array component.
bool __rangeCheckArray(const Array* array, size_t index) {
    return array->capacity > index;
}

// Dynamically grows and relocates the array component of a given table_obj object.
void __resizeArray(Array* array) {
    size_t oldcap = array->capacity;
    size_t newcap = oldcap * 2;

    Value* old_location = array->data;
    Value* new_location = new Value[newcap];

    for (Value* ptr = old_location; ptr < old_location + oldcap; ptr++) {
        size_t position = ptr - old_location;
        new_location[position] = std::move(*ptr);
    }

    array->data = new_location;
    array->capacity = newcap;

    delete[] old_location;
}

// Sets the given index of a table to a given value. Resizes the array component of the table_obj
// object if necessary.
void __setArrayField(Array* array, size_t index, Value val) {
    if (!__rangeCheckArray(array, index)) {
        __resizeArray(array);
    }

    array->csize.valid = false;
    array->data[index] = std::move(val);
}

// Attempts to get the value at the given index of the array component of the table. Returns NULL
// if the index is out of array capacity range.
Value* __getArrayField(const Array* array, size_t index) {
    if (!__rangeCheckArray(array, index)) {
        return NULL;
    }

    return &array->data[index];
}

// Returns the real size_t of the given tables array component.
size_t __getArraySize(const Array* array) {
    if (array->csize.valid) {
        return array->csize.cache;
    }

    size_t size = 0;
    for (Value* ptr = array->data; ptr < array->data + array->capacity; ptr++) {
        if (!ptr->is_nil()) {
            size++;
        }
    }

    array->csize.cache = size;
    array->csize.valid = true;

    return size;
}

void __setString(String* string, size_t index, char chr) {
    string->data[index] = chr;
    string->hash = xvm::strhash(string->data);
}

char __getString(String* string, size_t index) {
    return string->data[index];
}

String* __concatString(String* left, String* right) {
    TempBuf<char> buf(left->size + right->size + 1);

    std::memcpy(buf.data, left->data, left->size);
    std::memcpy(buf.data + left->size, right->data, right->size);
    // Set nullbyte
    std::memset(buf.data + left->size + right->size, 0, 1);

    return new String(buf.data);
}

Instruction* __getLabelAddress(const State* state, size_t index) {
    return state->lat.data[index];
}

void __linearScanLabelsInBytecode(const State* state) {
    using enum Opcode;

    size_t index = 0;

    for (Instruction* pc = state->holder.insns.data(); 1; pc++) {
        if (pc->op == Opcode::LBL) {
            state->lat.data[index++] = pc;
        }
        else if (pc->op == RET || pc->op == RETBF || pc->op == RETBT) {
            break;
        }
    }
}

void __push(State* state, Value&& val) {
    CallFrame* frame = __callframe(state);
    frame->locals[frame->capacity++] = std::move(val);
}

void __drop(State* state) {
    CallFrame* frame = __callframe(state);
    Value      dropped = std::move(frame->locals[frame->capacity--]);
    dropped.reset();
}

Value* __getLocal(State* XVM_RESTRICT state, size_t offset) {
    CallFrame* frame = __callframe(state);
    return &frame->locals[offset];
}

void __setLocal(State* XVM_RESTRICT state, size_t offset, Value&& val) {
    CallFrame* frame = __callframe(state);
    frame->locals[offset] = std::move(val);
}

// ==========================================================
// Register handling
void __initRegisterFile(State* state) {
    state->heapRegs = new SpillRegFile();
}

void __deinitRegisterFile(const State* state) {
    delete state->heapRegs;
}

void __setRegister(const State* state, uint16_t reg, Value&& val) {
    if XVM_LIKELY (reg < kStkRegCount) {
        state->stkRegs.registers[reg] = std::move(val);
    }
    else {
        const uint16_t offset = reg - kStkRegCount;
        state->heapRegs->registers[offset] = std::move(val);
    }
}

Value* __getRegister(const State* state, uint16_t reg) {
    if XVM_LIKELY ((reg & 0xFF) == reg) {
        return &state->stkRegs.registers[reg];
    }
    else {
        const uint16_t offset = reg - kStkRegCount;
        return &state->heapRegs->registers[offset];
    }
}

void __initMainFunction(State* state) {
    Function fn;
    fn.id = "main";
    fn.line = 0;
    fn.code = state->holder.insns.data();
    fn.size = state->holder.insns.size();

    Callable c;
    c.type = CallableKind::Function;
    c.u = {.fn = fn};
    c.arity = 1;

    Closure* cl = new Closure(std::move(c));
    state->main = Value(cl);
}

static Callable makeNativeCallable(NativeFn ptr, size_t arity) {
    Callable c;
    c.type = CallableKind::Native;
    c.u.ntv = ptr;
    c.arity = arity;
    return c;
}

static void declareCoreFunction(State* state, const char* id, NativeFn ptr, size_t arity) {
    Callable c = makeNativeCallable(ptr, arity);
    Closure* cl = new Closure(std::move(c));

    __setDictField(state->globals, id, Value(cl));
    native_fn_ids[ptr] = id;
}

void __initCoreInterpLib(State* state) {
    static NativeFn core_print = [](State* state) -> Value {
        Value* arg0 = __getRegister(state, state->args);
        std::cout << arg0->to_cxx_string() << "\n";
        return XVM_NIL;
    };

    static NativeFn core_error = [](State* state) -> Value {
        Value* arg0 = __getRegister(state, state->args);
        __setError(state, arg0->to_cxx_string());
        return XVM_NIL;
    };

    declareCoreFunction(state, "print", core_print, 1);
    declareCoreFunction(state, "error", core_error, 1);
}

} // namespace impl

} // namespace xvm
