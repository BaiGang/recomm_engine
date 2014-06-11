#ifndef _RECOMM_ENGINE_THRIFT_SERVICE_H_
#define _RECOMM_ENGINE_THRIFT_SERVICE_H_
#include <boost/thread.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/type_traits/is_same.hpp>
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TNonblockingServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/server/TThreadPoolServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include "gflags/gflags.h"

DECLARE_int32(num_connection_threads);

namespace recomm_engine {

/**
 * A templated class that encapsulates thrift service's basic creating 
 * and invoking interfaces.
 *
 */

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
class ThriftService {
 public:
  explicit ThriftService(boost::shared_ptr<Handler_t> handler,
                         boost::shared_ptr<ProcessorFactory_t> processor_factory,
                         int port);
  virtual ~ThriftService();

  //! start the server, blocking
  void Serve();
  
  //!  start the server non-blockingly, by creating a thread
  void ServeDetached();

  //! Finish the serving, join threads, etc.
  void End();

 protected:

  boost::shared_ptr<Handler_t> handler_;
  boost::shared_ptr<ProcessorFactory_t> processor_factory_;
  boost::shared_ptr<Server_t> server_;
  boost::shared_ptr<boost::thread> serving_thread_;
};

//////////////////////////////////////////////////

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
ThriftService<Handler_t, ProcessorFactory_t, Server_t>::ThriftService(
    boost::shared_ptr<Handler_t> handler,
    boost::shared_ptr<ProcessorFactory_t> processor_factory,
    int port) 
    : handler_(handler),
      processor_factory_(processor_factory) {
 
  using namespace ::apache::thrift;
  using namespace ::apache::thrift::protocol;
  using namespace ::apache::thrift::transport;
  using namespace ::apache::thrift::server;
  ::boost::shared_ptr<TServerTransport> server_transport(new TServerSocket(port));
  ::boost::shared_ptr<TTransportFactory> transport_factory(new TBufferedTransportFactory());
  ::boost::shared_ptr<TProtocolFactory> protocol_factory(new TBinaryProtocolFactory());

  server_.reset(new Server_t(processor_factory_, server_transport, transport_factory, protocol_factory));
  /*
  if (::boost::is_same<Server_t, ::apache::thrift::server::TNonblockingServer>) {
    server_->setNumIOThreads(FLAGS_num_connection_threads);
  }
  */
}

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
ThriftService<Handler_t, ProcessorFactory_t, Server_t>::~ThriftService() {
}

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
void ThriftService<Handler_t, ProcessorFactory_t, Server_t>::Serve() {
  server_->serve();
}

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
void ThriftService<Handler_t, ProcessorFactory_t, Server_t>::ServeDetached() {
  serving_thread_.reset(new boost::thread(&(ThriftService<Handler_t, ProcessorFactory_t,
                                                Server_t>::Serve),
                                          this));
}

template <class Handler_t,
          class ProcessorFactory_t,
          class Server_t>
void ThriftService<Handler_t, ProcessorFactory_t, Server_t>::End() {
  if (serving_thread_)
    serving_thread_->join();
  server_->stop();
}

}  // namespace recomm_engine

#endif // _RECOMM_ENGINE_THRIFT_SERVICE_H_
