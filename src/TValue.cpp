// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/TValue.h>
#include <Interpreter/TString.h>
#include <Interpreter/TArray.h>
#include <Interpreter/TDict.h>
#include <Interpreter/TClosure.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

using enum ValueKind;

Value::Value()
  : type(Nil) {}

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
        reset();

        this->type = other.type;
        this->u = other.u;

        other.type = Nil;
        other.u = {};
    }

    return *this;
}

Value::~Value() {
    reset();
}

// Return a clone of the Value based on its type
XVM_NODISCARD Value Value::clone() const {
    switch (type) {
    case Int:
        return Value(u.i);
    case Float:
        return Value(u.f);
    case Bool:
        return Value(u.b);
    case String:
        return Value(new struct String(*u.str));
    case Array:
        return Value(new struct Array(*u.arr));
    case Dict:
        return Value(new struct Dict(*u.dict));
    case Function:
        return Value(new Closure(*u.clsr));
    default:
        break;
    }

    return Value();
}

void Value::reset() {
    switch (type) {
    case String:
        delete u.str;
        u.str = NULL;
        break;
    case Array:
        delete u.arr;
        u.arr = NULL;
        break;
    case Dict:
        delete u.dict;
        u.dict = NULL;
        break;
    case Function:
        delete u.clsr;
        u.clsr = NULL;
        break;
    default:
        break;
    }

    type = Nil;
    u = {};
}

XVM_NODISCARD bool Value::compare(const Value& other) const {
    return impl::__compare(*this, other);
}

XVM_NODISCARD bool Value::deep_compare(const Value& other) const {
    return impl::__compare_deep(*this, other);
}

XVM_NODISCARD Value Value::to_string() const {
    return impl::__to_string(*this);
}

XVM_NODISCARD std::string Value::to_cxx_string() const {
    return impl::__to_cxx_string(*this);
}

XVM_NODISCARD std::string Value::to_literal_cxx_string() const {
    return impl::__to_literal_cxx_string(*this);
}

} // namespace xvm
