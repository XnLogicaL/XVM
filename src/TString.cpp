// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/TString.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

String::String(const char* str)
  : data((char*)str /* TODO: dupe string */),
    data_size(std::strlen(str)),
    hash(0 /* TODO: hash string */) {}

String::~String() {
    delete[] data;
}

String::String(const String& other)
  : data(other.data /* TODO: dupe string */),
    data_size(other.data_size),
    hash(other.hash) {}

String::String(String&& other)
  : data(other.data),
    data_size(other.data_size),
    hash(other.hash) {
    other.data = NULL;
    other.data_size = 0;
    other.hash = 0;
}

String& String::operator=(const String& other) {
    if (this != &other) {
        delete[] data;

        // TODO: dup string
        data = other.data;
        data_size = other.data_size;
        hash = other.hash;
    }

    return *this;
}

String& String::operator=(String&& other) {
    if (this != &other) {
        delete[] data;

        data = other.data;
        data_size = other.data_size;
        hash = other.hash;

        other.data = NULL;
        other.data_size = 0;
        other.hash = 0;
    }

    return *this;
}

void String::set(size_t position, const String& value) {
    XVM_ASSERT(position < data_size, "String index position out of bounds");
    XVM_ASSERT(value.data_size == 1, "Setting String index to non-character String");

    data[position] = value.data[0];
}

String String::get(size_t position) {
    XVM_ASSERT(position < data_size, "String index position out of bounds");
    char chr = data[position];
    char str[] = {chr, '\0'};
    return String(str);
}

} // namespace xvm
