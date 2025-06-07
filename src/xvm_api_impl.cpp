// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_api_impl.h"
#include "xvm_string.h"
#include <cmath>

namespace xvm {

namespace impl {

using enum Opcode;

const InstructionData& __getAddressData( const State* state, const Instruction* const pc ) {
  size_t offset = state->pc - pc;
  return state->bcInfoHolder.at( offset );
}

static std::string nativeId( NativeFn fn ) {
  static std::unordered_map<NativeFn, std::string> id_map;

  auto it = id_map.find( fn );
  if ( it != id_map.end() ) {
    return std::format( "function {}", it->second );
  }
  else {
    return std::format( "function <native@0x{:x}>", (uintptr_t)fn );
  }
}

std::string __getFuncSig( const Callable& func ) {
  return func.type == CallableKind::Function ? std::format( "function {}", func.u.fn.id )
                                             : nativeId( func.u.ntv );
}

void __ethrow( State* state, const std::string& message ) {
  const Callable& c = ( state->callInfoTop - 1 )->closure->callee;
  const std::string func = __getFuncSig( c );

  state->errorInfo->error = true;
  state->errorInfo->func = state->stringAtor.fromArray( func.c_str() );
  state->errorInfo->msg = state->stringAtor.fromArray( message.c_str() );
}

void __ethrowf( State* state, const std::string& fmt, std::string args... ) {
  try {
    auto str = std::vformat( fmt, std::make_format_args( args ) );
    __ethrow( state, str );
  }
  catch ( const std::format_error& ) {
    // This function is the only exception to non-assertive implementation functions.
    XVM_ASSERT( false, "__ethrowf: Invalid format string" )
  }
}

void __eclear( State* state ) {
  state->errorInfo->error = false;
}

bool __echeck( const State* state ) {
  return state->errorInfo->error;
}

template<typename T>
static bool unwindStackUntilGuardFrame( State* state, T callback ) {
  for ( CallInfo* ci = state->callInfoTop - 1; ci >= state->callInfoStack.data; ci-- ) {
    callback( ci );

    if ( ci->protect ) {
      return true;
    }

    __popCallInfo( state );
  }

  return false;
}

bool __ehandle( State* state ) {
  std::vector<std::string> sigs;

  bool guardFrameTouched = unwindStackUntilGuardFrame( state, [&sigs, &state]( CallInfo* frame ) {
    if ( frame->protect ) {
      const auto& errorInfo = state->errorInfo;
      const String* msg = new String( errorInfo->msg );

      __eclear( state );
      __return( state, Value( msg ) );
      return;
    }

    auto sig = __getFuncSig( frame->closure->callee );
    sigs.push_back( sig );
  } );

  if ( guardFrameTouched ) {
    return true;
  }

  const ErrorInfo* errorInfo = state->errorInfo.obj;

  std::ostringstream oss;
  oss << errorInfo->func << ": " << errorInfo->msg << "\n";

  for ( size_t i = 0; const std::string& func : sigs ) {
    oss << " #" << i++ << ' ' << func << "\n";
  }

  std::cout << oss.str();
  return false;
}

Value __getConstant( const State* state, size_t index ) {
  const Value& k = state->kHolder.at( index );
  return __cloneValue( &k );
}

std::string __getValueType( const Value* val ) {
  using enum ValueKind;

  // clang-format off
    switch (val->type) {
    case Nil:      return "nil";
    case Int:      return "int";
    case Float:    return "float";
    case Bool:     return "bool";
    case String:   return "string";
    case Function: return "function";
    case Array:    return "array";
    case Dict:     return "dict";
    } // clang-format on

  XVM_UNREACHABLE();
}

void* __toPointer( const Value* val ) {
  switch ( val->type ) {
  case ValueKind::Function:
  case ValueKind::Array:
  case ValueKind::Dict:
  case ValueKind::String:
    // This is technically UB... too bad!
    return reinterpret_cast<void*>( val->u.str );
  default:
    return NULL;
  }
}

void __pushCallInfo( State* state, CallInfo&& ci ) {
  if ( state->stackTop - state->stack.data >= 200 ) {
    __ethrow( state, "Stack overflow" );
    return;
  }

  *( state->callInfoTop++ ) = std::move( ci );
}

void __popCallInfo( State* state ) {
  state->callInfoTop--;
}

template<const bool IsProtected>
static void callBase( State* state, Closure* closure ) {
  CallInfo cf;
  cf.protect = IsProtected;
  cf.closure = new Closure( *closure );

  if ( closure->callee.type == CallableKind::Function ) {
    // Functions are automatically positioned by RET instructions; no need to increment saved
    // program counter.
    cf.pc = state->pc;
    cf.stackTop = state->stackTop;

    __pushCallInfo( state, std::move( cf ) );

    state->pc = closure->callee.u.fn.code;
    state->stackBase = state->stackTop;
  }
  else if ( closure->callee.type == CallableKind::Native ) {
    // Native functions require manual positioning as they don't increment program counter with
    // a RET instruction or similar.
    cf.pc = state->pc + 1;
    cf.stackTop = state->stackTop;

    __pushCallInfo( state, std::move( cf ) );
    __return( state, closure->callee.u.ntv( state ) );
  }
}

void __call( State* state, Closure* closure ) {
  callBase<false>( state, closure );
}

void __pcall( State* state, Closure* closure ) {
  callBase<true>( state, closure );
}

void __return( State* XVM_RESTRICT state, Value&& retv ) {
  state->pc = ( state->callInfoTop - 1 )->pc;
  state->stackTop = ( state->callInfoTop - 1 )->stackTop + 1;

  __pushStack( state, std::move( retv ) );
  __popCallInfo( state );
}

int __getValueLength( const Value* val ) {
  using enum ValueKind;

  if ( val->type == String )
    return (int)val->u.str->size;
  else if ( val->type == Array )
    return (int)__getArraySize( val->u.arr );
  else if ( val->type == Dict )
    return (int)__getDictSize( val->u.dict );

  return -1;
}

std::string __toString( const Value* val ) {
  using enum ValueKind;

  if ( val->type == String ) {
    return val->u.str->data;
  }

  switch ( val->type ) {
  case Int:
    return std::to_string( val->u.i );
  case Float:
    return std::to_string( val->u.f );
  case Bool:
    return val->u.b ? "true" : "false";
  case Array:
  case Dict:
    return std::format( "<{}@0x{:x}>", __getValueType( val ), (uintptr_t)__toPointer( val ) );
  case Function: {
    std::string type = "native";
    std::string id = "";

    if ( val->u.clsr->callee.type == CallableKind::Function ) {
      type = "function ";
      id = val->u.clsr->callee.u.fn.id;
    }

    return std::format( "<{}{}@0x{:x}>", type, id, (uintptr_t)val->u.clsr );
  }
  default:
    return "nil";
  }

  XVM_UNREACHABLE();
}

bool __toBool( const Value* val ) {
  if ( val->type == ValueKind::Bool ) {
    return val->u.b;
  }

  return val->type != ValueKind::Nil;
}

int __toInt( const Value* val, bool* fail ) {
  using enum ValueKind;

  if ( fail != NULL ) {
    *fail = false;
  }

  if ( val->type == Int ) {
    return val->u.i;
  }

  switch ( val->type ) {
  case String: {
    const std::string& str = val->u.str->data;
    if ( str.empty() ) {
      break;
    }

    int int_result;
    auto [ptr, ec] = std::from_chars( str.data(), str.data() + str.size(), int_result );
    if ( ec == std::errc() && ptr == str.data() + str.size() ) {
      return int_result;
    }

    break;
  }
  case Bool:
    return (int)val->u.b;
  default:
    break;
  }

  if ( fail != NULL )
    *fail = true;
  return -0x0FFFFFFF;
}

float __toFloat( const Value* val, bool* fail ) {
  using enum ValueKind;

  if ( fail != NULL ) {
    *fail = false;
  }

  if ( val->type == Float ) {
    return val->u.f;
  }

  switch ( val->type ) {
  case String: {
    const std::string& str = val->u.str->data;
    if ( str.empty() ) {
      break;
    }

    float float_result;
    auto [ptr_f, ec_f] = std::from_chars( str.data(), str.data() + str.size(), float_result );
    if ( ec_f == std::errc() && ptr_f == str.data() + str.size() ) {
      return float_result;
    }

    break;
  }
  case Bool:
    return (float)val->u.b;
  default:
    break;
  }

  if ( fail != NULL )
    *fail = true;
  return std::nanf( "" );
}

bool __compareValue( const Value* val0, const Value* val1 ) {
  using enum ValueKind;

  if ( val0->type != val1->type ) {
    return false;
  }

  switch ( val0->type ) {
  case Int:
    return val0->u.i == val1->u.i;
  case Float:
    return val0->u.f == val1->u.f;
  case Bool:
    return val0->u.b == val1->u.b;
  case Nil:
    return true;
  case String:
    return !std::strcmp( val0->u.str->data, val1->u.str->data );
  default:
    return false;
  }

  XVM_UNREACHABLE();
};

bool __deepCompareValue( const Value* val0, const Value* val1 ) {
  using enum ValueKind;

  if ( val0->type != val1->type ) {
    return false;
  }

  switch ( val0->type ) {
  case Int:
    return val0->u.i == val1->u.i;
  case Float:
    return val0->u.f == val1->u.f;
  case Bool:
    return val0->u.b == val1->u.b;
  case Nil:
    return true;
  case String:
    return !std::strcmp( val0->u.str->data, val1->u.str->data );
  case Array: {
    if ( __getArraySize( val0->u.arr ) != __getArraySize( val1->u.arr ) ) {
      return false;
    }

    for ( size_t i = 0; i < __getArraySize( val0->u.arr ); i++ ) {
      Value* val = __getArrayField( val0->u.arr, i );
      Value* other = __getArrayField( val1->u.arr, i );

      if ( !__deepCompareValue( val, other ) ) {
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

Value __cloneValue( const Value* val ) {
  using enum ValueKind;

  // clang-format off
    switch (val->type) {
    case Nil:       return Value();
    case Int:       return Value(val->u.i);
    case Float:     return Value(val->u.f);
    case Bool:      return Value(val->u.b);
    case String:    return Value(new struct String(*val->u.str));
    case Array:     return Value(new struct Array(*val->u.arr));
    case Dict:      return Value(new struct Dict(*val->u.dict));
    case Function:  return Value(new struct Closure(*val->u.clsr));
    } // clang-format on

  XVM_UNREACHABLE();
}

void __resetValue( Value* val ) {
  using enum ValueKind;

  // clang-format off
    switch (val->type) {
    case Nil:
    case Int:
    case Float:
    case Bool:      val->u = {}; break;
    case String:    delete val->u.str; break;
    case Array:     delete val->u.arr; break;
    case Dict:      delete val->u.dict; break;
    case Function:  delete val->u.clsr; break;
    } // clang-format on

  val->type = Nil;
}

// Checks if a given index is within the bounds of the UpValue vector of the closure.
// Used for resizing.
bool __rangeCheckClosureUpvs( Closure* closure, size_t index ) {
  return closure->upvs.size >= index;
}

// Attempts to retrieve UpValue at index <upv_id>.
// Returns NULL if <upv_id> is out of UpValue vector bounds.
UpValue* __getClosureUpv( Closure* closure, size_t upv_id ) {
  if ( !__rangeCheckClosureUpvs( closure, upv_id ) ) {
    return NULL;
  }

  return &closure->upvs.data[upv_id];
}

// Dynamically reassigns UpValue at index <upv_id> the value <val>.
void __setClosureUpv( Closure* closure, size_t upv_id, Value* val ) {
  UpValue* upv = __getClosureUpv( closure, upv_id );
  if ( upv != NULL ) {
    if ( upv->value != NULL )
      *upv->value = __cloneValue( val );
    else
      upv->value = val;
    upv->valid = true;
  }
}

static void handleCapture( State* state, Closure* closure, size_t* upvalues ) {
  if ( __rangeCheckClosureUpvs( closure, *upvalues ) ) {
    __resizeClosureUpvs( closure );
  }

  uint16_t idx = state->pc->b;
  Value* value;

  if ( state->pc->a == 0 ) {
    value = state->stackBase + idx - 1;
  }
  else { // Upvalue is captured twice; automatically close it.
    UpValue* upv = &( state->callInfoTop - 1 )->closure->upvs.data[idx];
    if ( upv->valid && upv->open ) {
      upv->heap = __cloneValue( upv->value );
      upv->value = &upv->heap;
      upv->open = false;
    }
    value = upv->value;
  }

  closure->upvs.data[( *upvalues )++] = {
    .open = true,
    .valid = true,
    .value = value,
    .heap = XVM_NIL,
  };
}

// Loads closure bytecode by iterating over the Instruction pipeline.
// Handles sentinel/special opcodes like RET or CAPTURE while assembling closure.
void __initClosure( State* state, Closure* closure, size_t len ) {
  size_t upvalues = 0;
  for ( size_t i = 0; i < len; ++i ) {
    if ( ( state->pc++ )->op == CAPTURE ) {
      handleCapture( state, closure, &upvalues );
    }
  }
}

static void closeUpvalue( UpValue* upv ) {
  upv->heap = __cloneValue( upv->value );
  upv->value = &upv->heap;
  upv->open = false;
}

// Moves upvalues of the current closure into the heap, "closing" them.
void __closeClosureUpvs( const Closure* closure ) {
  for ( UpValue* upv = closure->upvs.data; upv < closure->upvs.data + closure->upvs.size; upv++ ) {
    if ( upv->valid && upv->open ) {
      closeUpvalue( upv );
    }
  }
}

// Hashes a dictionary key using the FNV-1a hashing algorithm.
size_t __hashDictKey( const Dict* dict, const char* key ) {
  size_t hash = 2166136261u;
  while ( *key ) {
    hash = ( hash ^ *key++ ) * 16777619;
  }

  return hash % dict->cap;
}

// Inserts a key-value pair into the hash table component of a given table_obj object.
void __setDictField( Dict* dict, const char* key, Value val ) {
  size_t index = __hashDictKey( dict, key );
  if ( index > dict->cap ) {
    // Handle relocation
  }

  Dict::HNode& node = dict->data[index];
  node.key = key;
  node.value = std::move( val );
  dict->cvalid = false;
}

// Performs a look-up on the given table with a given key. Returns NULL upon lookup failure.
Value* __getDictField( const Dict* dict, const char* key ) {
  size_t index = __hashDictKey( dict, key );
  if ( index > dict->cap ) {
    return NULL;
  }

  return &dict->data[index].value;
}

// Returns the real size_t of the hashtable component of the given table object.
size_t __getDictSize( Dict* dict ) {
  if ( dict->cvalid ) {
    return dict->csize;
  }

  size_t index = 0;
  for ( ; index < dict->cap; index++ ) {
    Dict::HNode& obj = dict->data[index];
    if ( obj.value.type == ValueKind::Nil ) {
      break;
    }
  }

  dict->csize = index;
  dict->cvalid = true;

  return index;
}

// Checks if the given index is out of bounds of a given tables array component.
bool __rangeCheckArray( const Array* array, size_t index ) {
  return array->cap > index;
}

// Dynamically grows and relocates the array component of a given table_obj object.
void __resizeArray( Array* array ) {
  size_t oldcap = array->cap;
  size_t newcap = oldcap * 2;

  Value* old_location = array->data;
  Value* new_location = new Value[newcap];

  for ( Value* ptr = old_location; ptr < old_location + oldcap; ptr++ ) {
    size_t position = ptr - old_location;
    new_location[position] = std::move( *ptr );
  }

  array->data = new_location;
  array->cap = newcap;

  delete[] old_location;
}

// Sets the given index of a table to a given value. Resizes the array component of the table_obj
// object if necessary.
void __setArrayField( Array* array, size_t index, Value val ) {
  if ( !__rangeCheckArray( array, index ) ) {
    __resizeArray( array );
  }

  array->cvalid = false;
  array->data[index] = std::move( val );
}

// Attempts to get the value at the given index of the array component of the table. Returns NULL
// if the index is out of array cap range.
Value* __getArrayField( const Array* array, size_t index ) {
  if ( !__rangeCheckArray( array, index ) ) {
    return NULL;
  }

  return &array->data[index];
}

// Returns the real size_t of the given tables array component.
size_t __getArraySize( Array* array ) {
  if ( array->cvalid ) {
    return array->csize;
  }

  size_t size = 0;
  for ( Value* ptr = array->data; ptr < array->data + array->cap; ptr++ ) {
    if ( ptr->type != ValueKind::Nil ) {
      size++;
    }
  }

  array->csize = size;
  array->cvalid = true;

  return size;
}

char __getString( const String* str, size_t pos, bool* fail ) {
  if ( fail != NULL ) {
    *fail = false;
  }

  if ( pos < str->size ) {
    return str->data[pos];
  }

  if ( fail != NULL )
    *fail = true;
  return 0x00;
}

void __setString( String* str, size_t pos, char chr, bool* fail ) {
  if ( fail != NULL ) {
    *fail = false;
  }

  if ( pos < str->size ) {
    str->data[pos] = chr;
    return;
  }

  if ( fail != NULL ) {
    *fail = true;
  }
}

String* __concatString( String* left, String* right ) {
  TempBuf<char> buf( left->size + right->size + 1 );

  std::memcpy( buf.data, left->data, left->size );
  std::memcpy( buf.data + left->size, right->data, right->size );
  // Set nullbyte
  std::memset( buf.data + left->size + right->size, 0, 1 );

  return new String( buf.data );
}

void __pushStack( State* state, Value&& val ) {
  *( state->stackTop++ ) = std::move( val );
}

void __dropStack( State* state ) {
  state->stackTop--;
  __resetValue( state->stackTop );
}

Value* __getGlobal( State* state, const char* name ) {
  return __getDictField( state->globalEnv, name );
}

const Value* __getGlobal( const State* state, const char* name ) {
  return __getDictField( state->globalEnv, name );
}

void __setGlobal( State* state, const char* name, Value&& val ) {
  __setDictField( state->globalEnv, name, std::move( val ) );
}

Value* __getLocal( State* state, size_t offset ) {
  return state->stackBase + offset - 1;
}

const Value* __getLocal( const State* state, size_t offset ) {
  return state->stackBase + offset + 1;
}

Value* __getArgument( State* state, size_t offset ) {
  return state->stackBase - offset;
}

const Value* __getArgument( const State* state, size_t offset ) {
  return state->stackBase - offset;
}

void __setLocal( State* XVM_RESTRICT state, size_t offset, Value&& val ) {
  *( state->stackBase + offset - 1 ) = std::move( val );
}

void __setRegister( State* state, uint16_t reg, Value&& val ) {
  state->registers.data[reg] = std::move( val );
}

Value* __getRegister( State* state, uint16_t reg ) {
  return &state->registers.data[reg];
}

const Value* __getRegister( const State* state, uint16_t reg ) {
  return &state->registers.data[reg];
}

} // namespace impl

} // namespace xvm
