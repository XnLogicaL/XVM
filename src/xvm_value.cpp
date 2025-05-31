// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_value.h"
#include "xvm_string.h"
#include "xvm_array.h"
#include "xvm_dict.h"
#include "xvm_closure.h"
#include "xvm_api_impl.h"

namespace xvm {

using enum ValueKind;

Value::XVM_NIL : type(Nil) {}

Value::Value(bool b)
  : type(Bool),
    u({.b = b}) {}

Value::Value(int x)
  : type(Int),
    u({.i = x}) {}

Value::Value(float x)
  : type(Float),
    u({.f = x}) {}

Value::Value(struct String* ptr)
  : type(String),
    u({.str = ptr}) {}

Value::Value(struct Array* ptr)
  : type(Array),
    u({.arr = ptr}) {}

Value::Value(struct Dict* ptr)
  : type(Dict),
    u({.dict = ptr}) {}

Value::Value(Closure* ptr)
  : type(Function),
    u({.clsr = ptr}) {}

// Move constructor, transfer ownership based on type
Value::Value(Value&& other)
  : type(other.type),
    u(other.u) {
    other.type = Nil;
    other.u = {};
}

// Move-assignment operator, moves values from other object
Value& Value::operator=(Value&& other) {
    if (this != &other) {
        impl::__reset(this);

        this->type = other.type;
        this->u = other.u;

        other.type = Nil;
        other.u = {};
    }

    return *this;
}

Value::~XVM_NIL {
    impl::__reset(this);
}

} // namespace xvm
