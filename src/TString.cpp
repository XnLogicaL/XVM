// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include <Interpreter/TString.h>
#include <Interpreter/ApiImpl.h>

namespace xvm {

char* strdup(const char* str) {
    size_t len = std::strlen(str);
    char*  chars = new char[len + 1];
    std::strcpy(chars, str);
    return chars;
}

char* strdup(const std::string& str) {
    return strdup(str.c_str());
}

uint32_t strhash(const char* str) {
    static constexpr uint32_t BASE = 31u; // Prime number
    static constexpr uint32_t MOD = 0xFFFFFFFFu;

    uint32_t hash = 0;
    while (char chr = *str++) {
        hash = (hash * BASE + (uint32_t)chr) % MOD;
    }

    return hash;
}

uint32_t strhash(const std::string& str) {
    return strhash(str.c_str());
}

std::string stresc(const std::string& str) {
    std::ostringstream buf;

    for (unsigned char c : str) {
        switch (c) {
        // clang-format off
        case '\a': buf << "\\a"; break;
        case '\b': buf << "\\b"; break;
        case '\f': buf << "\\f"; break;
        case '\n': buf << "\\n"; break;
        case '\r': buf << "\\r"; break;
        case '\t': buf << "\\t"; break;
        case '\v': buf << "\\v"; break;
        case '\\': buf << "\\\\"; break;
        case '\"': buf << "\\\""; break;
        // clang-format on
        default:
            // If the character is printable, output it directly.
            // Otherwise, output it as a hex escape.
            if (std::isprint(c)) {
                buf << c;
            }
            else {
                buf << "\\x" << std::hex << std::setw(2) << std::setfill('0')
                    << static_cast<int>(c);
            }

            break;
        }
    }

    return buf.str();
}

String::String(const char* str)
  : data(xvm::strdup(str)),
    size(std::strlen(str)),
    hash(xvm::strhash(str)) {}

String::~String() {
    delete[] data;
}

String::String(const String& other)
  : data(xvm::strdup(other.data)),
    size(other.size),
    hash(other.hash) {}

String::String(String&& other)
  : data(other.data),
    size(other.size),
    hash(other.hash) {
    other.data = NULL;
    other.size = 0;
    other.hash = 0;
}

String& String::operator=(const String& other) {
    if (this != &other) {
        delete[] data;

        data = xvm::strdup(other.data);
        size = other.size;
        hash = other.hash;
    }

    return *this;
}

String& String::operator=(String&& other) {
    if (this != &other) {
        delete[] data;

        data = other.data;
        size = other.size;
        hash = other.hash;

        other.data = NULL;
        other.size = 0;
        other.hash = 0;
    }

    return *this;
}

void String::set(size_t position, const String& value) {
    XVM_ASSERT(position < size, "String index position out of bounds");
    XVM_ASSERT(value.size == 1, "Setting String index to non-character String");

    data[position] = value.data[0];
    // Refresh hash as the string was mutated
    hash = xvm::strhash(data);
}

String String::get(size_t position) {
    XVM_ASSERT(position < size, "String index position out of bounds");

    char chr = data[position];
    char str[] = {chr, '\0'};

    return String(str);
}

} // namespace xvm
