// Copyright 2013 Sina Inc. All rights reserved.
// Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
// Description: Thrift util

#include <boost/shared_ptr.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/protocol/TDebugProtocol.h>
#include <thrift/transport/TBufferTransports.h>

namespace util {
// Serialization
template<class ThriftType>
std::string ThriftToString(const ThriftType* t) {
	using boost::shared_ptr;
	using ::apache::thrift::transport::TMemoryBuffer;
	using ::apache::thrift::protocol::TBinaryProtocol;
	using ::apache::thrift::protocol::TDebugProtocol;
  shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
	shared_ptr<TBinaryProtocol> bprotocol(new TBinaryProtocol(buffer));
	t->write(bprotocol.get());
	char* buf;
	uint32_t buf_size;
	buffer->getBuffer(reinterpret_cast<uint8_t**>(&buf), &buf_size);
	return std::string(buf, buf_size);
}


// Serialize to debug string
template<class ThriftType>
std::string ThriftToDebugString(const ThriftType* t) {
	using boost::shared_ptr;
	using ::apache::thrift::transport::TMemoryBuffer;
	using ::apache::thrift::protocol::TBinaryProtocol;
	using ::apache::thrift::protocol::TDebugProtocol;
	using ::apache::thrift::transport::TMemoryBuffer;
  shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());
	shared_ptr<TDebugProtocol> bprotocol(new TDebugProtocol(buffer));
	t->write(bprotocol.get());
	char* buf;
	uint32_t buf_size;
	buffer->getBuffer(reinterpret_cast<uint8_t**>(&buf), &buf_size);
	return std::string(buf, buf_size);
}
// Deserialization
template<class ThriftType>
bool StringToThrift(const std::string& str, ThriftType* t) {
	using boost::shared_ptr;
	using ::apache::thrift::transport::TMemoryBuffer;
	using ::apache::thrift::protocol::TBinaryProtocol;

  shared_ptr<TMemoryBuffer> mbuf(new TMemoryBuffer(
	  reinterpret_cast<uint8_t*>(const_cast<char*>(str.c_str())), str.size()));
	shared_ptr<TBinaryProtocol> bprotocol(new TBinaryProtocol(mbuf));
	try {
	  t->read(bprotocol.get());
	} catch(...) {
	  return false;
	}
	return true;
}

}  // namespace
