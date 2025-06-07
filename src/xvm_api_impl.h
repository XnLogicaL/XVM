// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

/**
 * @file api-impl.h
 * @brief Internal interpreter API implementation
 * @details Contains functions used by the xvm interpreter engine, including stack
 *          operations, value conversion, function calling, error propagation, closure management,
 *          dictionary/array manipulation, and register management.
 */
#ifndef XVM_API_IMPL_H
#define XVM_API_IMPL_H

#include "xvm_common.h"
#include "xvm_opcode.h"
#include "xvm_state.h"
#include "xvm_dict.h"
#include "xvm_array.h"
#include "xvm_closure.h"

/**
 * @namespace xvm
 * @ingroup xvm_namespace
 * @{
 */
namespace xvm {

/**
 * @namespace impl
 * @defgroup impl_namespace
 * @{
 */
namespace impl {

const InstructionData& __getAddressData( const State* state, const Instruction* const pc );

std::string __getFuncSig( const Callable& func );

void __ethrow( State* state, const std::string& message );
void __ethrowf( State* state, const std::string& fmt, std::string args... );
void __eclear( State* state );
bool __echeck( const State* state );
bool __ehandle( State* state );

Value __getConstant( const State* state, size_t index );

void __pushCallInfo( State* state, CallInfo&& ci );
void __popCallInfo( State* state );
void __call( State* state, Closure* callee );
void __pcall( State* state, Closure* callee );
void __return( State* XVM_RESTRICT state, Value&& retv );

void* __toPointer( const Value* val );
bool __toBool( const Value* val );
int __toInt( const Value* val, bool* fail = NULL );
float __toFloat( const Value* val, bool* fail = NULL );
std::string __toString( const Value* val );
std::string __getValueType( const Value* val );
int __getValueLength( const Value* val );
bool __compareValue( const Value* val0, const Value* val1 );
bool __deepCompareValue( const Value* val0, const Value* val1 );
Value __cloneValue( const Value* val );
void __resetValue( Value* val );

void __resizeClosureUpvs( Closure* closure );
bool __rangeCheckClosureUpvs( Closure* closure, size_t index );
UpValue* __getClosureUpv( Closure* closure, size_t upv_id );
void __resizeClosureUpvs( Closure* closure );
bool __rangeCheckClosureUpvs( Closure* closure, size_t index );
UpValue* __getClosureUpv( Closure* closure, size_t upv_id );
void __setClosureUpv( Closure* closure, size_t upv_id, Value* val );
void __initClosure( State* state, Closure* closure, size_t len );
void __closeClosureUpvs( const Closure* closure );

char __getString( const String* str, size_t pos, bool* fail = NULL );
void __setString( String* str, size_t pos, char chr, bool* fail = NULL );
String* __concatString( String* left, String* right );

size_t __hashDictKey( const Dict* dict, const char* key );
void __setDictField( Dict* dict, const char* key, Value val );
Value* __getDictField( const Dict* dict, const char* key );
size_t __getDictSize( Dict* dict );

bool __rangeCheckArray( const Array* array, size_t index );
void __resizeArray( Array* array );
void __setArrayField( Array* array, size_t index, Value val );
Value* __getArrayField( const Array* array, size_t index );
size_t __getArraySize( Array* array );

void __pushStack( State* state, Value&& val );
void __dropStack( State* state );

void __setGlobal( State* state, const char* name, Value&& val );
Value* __getGlobal( State* state, const char* name );
const Value* __getGlobal( const State* state, const char* name );

void __setLocal( State* XVM_RESTRICT state, size_t offset, Value&& val );
Value* __getLocal( State* state, size_t offset );
const Value* __getLocal( const State* state, size_t offset );

Value* __getArgument( State* state, size_t offset );
const Value* __getArgument( const State* state, size_t offset );

void __setRegister( State* state, uint16_t reg, Value&& val );
Value* __getRegister( State* state, uint16_t reg );
const Value* __getRegister( const State* state, uint16_t reg );

} // namespace impl

/** @} */

} // namespace xvm

/** @} */

#endif
