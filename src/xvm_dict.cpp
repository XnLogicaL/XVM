// This file is a part of the XVM project
// Copyright (C) 2025 XnLogical - Licensed under GNU GPL v3.0

#include "xvm_dict.h"
#include "xvm_api_impl.h"

namespace xvm {

Dict::Dict( const Dict& other )
  : data( new Dict::HNode[kDictCapacity] ),
    cap( other.cap ),
    csize( other.csize ) {
  for ( size_t i = 0; i < cap; ++i ) {
    HNode& src = other.data[i];
    HNode* dst = &data[i];
    dst->key = src.key;
    dst->value = impl::__cloneValue( &src.value );
  }
}

Dict::Dict( Dict&& other )
  : data( other.data ),
    cap( other.cap ),
    csize( other.csize ) {
  other.data = NULL;
  other.cap = 0;
  other.csize = {};
}

Dict& Dict::operator=( const Dict& other ) {
  if ( this != &other ) {
    data = new HNode[other.cap];
    cap = other.cap;
    csize = other.csize;

    for ( size_t i = 0; i < cap; ++i ) {
      Dict::HNode& src = other.data[i];
      Dict::HNode* dst = &data[i];
      dst->key = src.key;
      dst->value = impl::__cloneValue( &src.value );
    }
  }

  return *this;
}

Dict& Dict::operator=( Dict&& other ) {
  if ( this != &other ) {
    data = other.data;
    cap = other.cap;
    csize = other.csize;

    other.data = NULL;
    other.cap = 0;
    other.csize = {};
  }

  return *this;
}

Dict::Dict()
  : data( new HNode[kDictCapacity] ) {}

Dict::~Dict() {
  delete[] data;
}

} // namespace xvm
