// Copyright (C) 2018 Alain Miniussi <alain.miniussi@oca.eu>.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

// Request implementation dtails

// This header should be included only after the communicator and request 
// classes has been defined.
#ifndef BOOST_MPI_REQUEST_HANDLERS_HPP
#define BOOST_MPI_REQUEST_HANDLERS_HPP

#include <boost/mpi/skeleton_and_content_types.hpp>

namespace boost { namespace mpi {

namespace detail {
/**
 * Internal data structure that stores everything required to manage
 * the receipt of serialized data via a request object.
 */
template<typename T>
struct serialized_irecv_data {
  serialized_irecv_data(const communicator& comm, T& value)
    : m_ia(comm), m_value(value) {}

  void deserialize(status& stat) 
  { 
    m_ia >> m_value; 
    stat.m_count = 1;
  }

  std::size_t     m_count;
  packed_iarchive m_ia;
  T&              m_value;
};

template<>
struct serialized_irecv_data<packed_iarchive>
{
  serialized_irecv_data(communicator const&, packed_iarchive& ia) : m_ia(ia) { }

  void deserialize(status&) { /* Do nothing. */ }

  std::size_t      m_count;
  packed_iarchive& m_ia;
};

/**
 * Internal data structure that stores everything required to manage
 * the receipt of an array of serialized data via a request object.
 */
template<typename T>
struct serialized_array_irecv_data
{
  serialized_array_irecv_data(const communicator& comm, T* values, int n)
    : m_count(0), m_ia(comm), m_values(values), m_nb(n) {}

  void deserialize(status& stat);

  std::size_t     m_count;
  packed_iarchive m_ia;
  T*              m_values;
  int             m_nb;
};

template<typename T>
void serialized_array_irecv_data<T>::deserialize(status& stat)
{
  T* v = m_values;
  T* end =  m_values+m_nb;
  while (v < end) {
    m_ia >> *v++;
  }
  stat.m_count = m_nb;
}

/**
 * Internal data structure that stores everything required to manage
 * the receipt of an array of primitive data but unknown size.
 * Such an array can have been send with blocking operation and so must
 * be compatible with the (size_t,raw_data[]) format.
 */
template<typename T, class A>
struct dynamic_array_irecv_data
{
  BOOST_STATIC_ASSERT_MSG(is_mpi_datatype<T>::value, "Can only be specialized for MPI datatypes.");

  dynamic_array_irecv_data(std::vector<T,A>& values)
    : m_count(-1), m_values(values) {}

  std::size_t       m_count;
  std::vector<T,A>& m_values;
};

template<typename T>
struct serialized_irecv_data<const skeleton_proxy<T> >
{
  serialized_irecv_data(const communicator& comm, skeleton_proxy<T> proxy)
    : m_isa(comm), m_ia(m_isa.get_skeleton()), m_proxy(proxy) { }

  void deserialize(status& stat) 
  { 
    m_isa >> m_proxy.object;
    stat.m_count = 1;
  }

  std::size_t              m_count;
  packed_skeleton_iarchive m_isa;
  packed_iarchive&         m_ia;
  skeleton_proxy<T>        m_proxy;
};

template<typename T>
struct serialized_irecv_data<skeleton_proxy<T> >
  : public serialized_irecv_data<const skeleton_proxy<T> >
{
  typedef serialized_irecv_data<const skeleton_proxy<T> > inherited;

  serialized_irecv_data(const communicator& comm, const skeleton_proxy<T>& proxy)
    : inherited(comm, proxy) { }
};
}

#if BOOST_MPI_VERSION >= 3
template<class Data>
class request::probe_handler
  : public request::handler,
    protected Data {

protected:
  template<typename I1>
  probe_handler(communicator const& comm, int source, int tag, I1& i1)
    : Data(comm, i1),
      m_comm(comm),
      m_source(source),
      m_tag(tag) {}
  // no variadic template for now
  template<typename I1, typename I2>
  probe_handler(communicator const& comm, int source, int tag, I1& i1, I2& i2)
    : Data(comm, i1, i2),
      m_comm(comm),
      m_source(source),
      m_tag(tag) {}

public:
  bool active() const { return m_source != MPI_PROC_NULL; }
  optional<MPI_Request&> trivial() { return boost::none; }
  void cancel() { m_source = MPI_PROC_NULL; }

  status wait() {
    MPI_Message msg;
    status stat;
    BOOST_MPI_CHECK_RESULT(MPI_Mprobe, (m_source,m_tag,m_comm,&msg,&stat.m_status));
    return unpack(msg, stat);
  }
  
  optional<status> test() {
    status stat;
    int flag = 0;
    MPI_Message msg;
    BOOST_MPI_CHECK_RESULT(MPI_Improbe, (m_source,m_tag,m_comm,&flag,&msg,&stat.m_status));
    if (flag) {
      return unpack(msg, stat);
    } else {
      return optional<status>();
    } 
  }

protected:
  friend class request;

  status unpack(MPI_Message& msg, status& stat) {
    int count;
    MPI_Datatype datatype = this->Data::datatype();
    BOOST_MPI_CHECK_RESULT(MPI_Get_count, (&stat.m_status, datatype, &count));
    this->Data::resize(count);
    BOOST_MPI_CHECK_RESULT(MPI_Mrecv, (this->Data::buffer(), count, datatype, &msg, &stat.m_status));
    this->Data::deserialize();
    m_source = MPI_PROC_NULL;
    stat.m_count = 1;
    return stat;
  }
  
  communicator const& m_comm;
  int m_source;
  int m_tag;
};
#endif // BOOST_MPI_VERSION >= 3

namespace detail {
template<class A>
struct dynamic_primitive_array_data {
  dynamic_primitive_array_data(communicator const&, A& arr) : m_buffer(arr) {}
  
  void* buffer() { return m_buffer.data(); }
  void  resize(std::size_t sz) { m_buffer.resize(sz); }
  void  deserialize() {}
  MPI_Datatype datatype() { return get_mpi_datatype<typename A::value_type>(); }
  
  A& m_buffer;
};

template<typename T>
struct serialized_data {
  serialized_data(communicator const& comm, T& value) : m_archive(comm), m_value(value) {}

  void* buffer() { return m_archive.address(); }
  void  resize(std::size_t sz) { m_archive.resize(sz); }
  void  deserialize() { m_archive >> m_value; }
  MPI_Datatype datatype() { return MPI_PACKED; }

  packed_iarchive m_archive;
  T& m_value;
};

template<>
struct serialized_data<packed_iarchive> {
  serialized_data(communicator const& comm, packed_iarchive& ar) : m_archive(ar) {}
  
  void* buffer() { return m_archive.address(); }
  void  resize(std::size_t sz) { m_archive.resize(sz); }
  void  deserialize() {}
  MPI_Datatype datatype() { return MPI_PACKED; }

  packed_iarchive& m_archive;
};

template<typename T>
struct serialized_data<const skeleton_proxy<T> > {
  serialized_data(communicator const& comm, skeleton_proxy<T> skel)
    : m_proxy(skel),
      m_archive(comm) {}
  
  void* buffer() { return m_archive.get_skeleton().address(); }
  void  resize(std::size_t sz) { m_archive.get_skeleton().resize(sz); }
  void  deserialize() { m_archive >> m_proxy.object; }
  MPI_Datatype datatype() { return MPI_PACKED; }

  skeleton_proxy<T> m_proxy;
  packed_skeleton_iarchive m_archive;
};

template<typename T>
struct serialized_data<skeleton_proxy<T> >
  : public serialized_data<const skeleton_proxy<T> > {
  typedef serialized_data<const skeleton_proxy<T> > super;
  serialized_data(communicator const& comm, skeleton_proxy<T> skel)
    : super(comm, skel) {}
};

template<typename T>
struct serialized_array_data {
  serialized_array_data(communicator const& comm, T* values, int nb)
    : m_archive(comm), m_values(values), m_nb(nb) {}

  void* buffer() { return m_archive.address(); }
  void  resize(std::size_t sz) { m_archive.resize(sz); }
  void  deserialize() {
    T* end = m_values + m_nb;
    T* v = m_values;
    while (v != end) {
      m_archive >> *v++;
    }
  }
  MPI_Datatype datatype() { return MPI_PACKED; }

  packed_iarchive m_archive;
  T*  m_values;
  int m_nb;
};

}

class BOOST_MPI_DECL request::legacy_handler : public request::handler {
public:
  legacy_handler(communicator const& comm, int source, int tag);
  
  void cancel() {
    for (int i = 0; i < 2; ++i) {
      if (m_requests[i] != MPI_REQUEST_NULL) {
        BOOST_MPI_CHECK_RESULT(MPI_Cancel, (m_requests+i));
      }
    }
  }
  
  bool active() const;
  optional<MPI_Request&> trivial();
  
  MPI_Request      m_requests[2];
  communicator     m_comm;
  int              m_source;
  int              m_tag;
};

template<typename T>
class request::legacy_serialized_handler 
  : public request::legacy_handler, 
    protected detail::serialized_irecv_data<T> {
public:
  typedef detail::serialized_irecv_data<T> extra;
  legacy_serialized_handler(communicator const& comm, int source, int tag, T& value)
    : legacy_handler(comm, source, tag),
      extra(comm, value)  {
    BOOST_MPI_CHECK_RESULT(MPI_Irecv,
			   (&this->extra::m_count, 1, 
			    get_mpi_datatype(this->extra::m_count),
			    source, tag, comm, m_requests+0));
    
  }

  status wait() {
    status stat;
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Wait for the count message to complete
      BOOST_MPI_CHECK_RESULT(MPI_Wait,
                             (m_requests, &stat.m_status));
      // Resize our buffer and get ready to receive its data
      this->extra::m_ia.resize(this->extra::m_count);
      BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                             (this->extra::m_ia.address(), this->extra::m_ia.size(), MPI_PACKED,
                              stat.source(), stat.tag(), 
                              MPI_Comm(m_comm), m_requests + 1));
    }

    // Wait until we have received the entire message
    BOOST_MPI_CHECK_RESULT(MPI_Wait,
                           (m_requests + 1, &stat.m_status));

    this->deserialize(stat);
    return stat;    
  }
  
  optional<status> test() {
    status stat;
    int flag = 0;
    
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Check if the count message has completed
      BOOST_MPI_CHECK_RESULT(MPI_Test,
                             (m_requests, &flag, &stat.m_status));
      if (flag) {
        // Resize our buffer and get ready to receive its data
        this->extra::m_ia.resize(this->extra::m_count);
        BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                               (this->extra::m_ia.address(), this->extra::m_ia.size(),MPI_PACKED,
                                stat.source(), stat.tag(), 
                                MPI_Comm(m_comm), m_requests + 1));
      } else
        return optional<status>(); // We have not finished yet
    } 

    // Check if we have received the message data
    BOOST_MPI_CHECK_RESULT(MPI_Test,
                           (m_requests + 1, &flag, &stat.m_status));
    if (flag) {
      this->deserialize(stat);
      return stat;
    } else 
      return optional<status>();
  }
};

template<typename T>
class request::legacy_serialized_array_handler 
  : public    request::legacy_handler,
    protected detail::serialized_array_irecv_data<T> {
  typedef detail::serialized_array_irecv_data<T> extra;

public:
  legacy_serialized_array_handler(communicator const& comm, int source, int tag, T* values, int n)
    : legacy_handler(comm, source, tag),
      extra(comm, values, n) {
    BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                           (&this->extra::m_count, 1, 
                            get_mpi_datatype(this->extra::m_count),
                            source, tag, comm, m_requests+0));
  }

  status wait() {
    status stat;
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Wait for the count message to complete
      BOOST_MPI_CHECK_RESULT(MPI_Wait,
                             (m_requests, &stat.m_status));
      // Resize our buffer and get ready to receive its data
      this->extra::m_ia.resize(this->extra::m_count);
      BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                             (this->extra::m_ia.address(), this->extra::m_ia.size(), MPI_PACKED,
                              stat.source(), stat.tag(), 
                              MPI_Comm(m_comm), m_requests + 1));
    }

    // Wait until we have received the entire message
    BOOST_MPI_CHECK_RESULT(MPI_Wait,
                           (m_requests + 1, &stat.m_status));

    this->deserialize(stat);
    return stat;
  }
  
  optional<status> test() {
    status stat;
    int flag = 0;
    
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Check if the count message has completed
      BOOST_MPI_CHECK_RESULT(MPI_Test,
                             (m_requests, &flag, &stat.m_status));
      if (flag) {
        // Resize our buffer and get ready to receive its data
        this->extra::m_ia.resize(this->extra::m_count);
        BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                               (this->extra::m_ia.address(), this->extra::m_ia.size(),MPI_PACKED,
                                stat.source(), stat.tag(), 
                                MPI_Comm(m_comm), m_requests + 1));
      } else
        return optional<status>(); // We have not finished yet
    } 

    // Check if we have received the message data
    BOOST_MPI_CHECK_RESULT(MPI_Test,
                           (m_requests + 1, &flag, &stat.m_status));
    if (flag) {
      this->deserialize(stat);
      return stat;
    } else 
      return optional<status>();
  }
};

template<typename T, class A>
class request::legacy_dynamic_primitive_array_handler 
  : public request::legacy_handler,
    protected detail::dynamic_array_irecv_data<T,A>
{
  typedef detail::dynamic_array_irecv_data<T,A> extra;

public:
  legacy_dynamic_primitive_array_handler(communicator const& comm, int source, int tag, std::vector<T,A>& values)
    : legacy_handler(comm, source, tag),
      extra(values) {
    BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                           (&this->extra::m_count, 1, 
                            get_mpi_datatype(this->extra::m_count),
                            source, tag, comm, m_requests+0));
  }

  status wait() {
    status stat;
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Wait for the count message to complete
      BOOST_MPI_CHECK_RESULT(MPI_Wait,
                             (m_requests, &stat.m_status));
      // Resize our buffer and get ready to receive its data
      this->extra::m_values.resize(this->extra::m_count);
      BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                             (detail::c_data(this->extra::m_values), this->extra::m_values.size(), get_mpi_datatype<T>(),
                              stat.source(), stat.tag(), 
                              MPI_Comm(m_comm), m_requests + 1));
    }
    // Wait until we have received the entire message
    BOOST_MPI_CHECK_RESULT(MPI_Wait,
                           (m_requests + 1, &stat.m_status));
    return stat;    
  }

  optional<status> test() {
    status stat;
    int flag = 0;
    
    if (m_requests[1] == MPI_REQUEST_NULL) {
      // Check if the count message has completed
      BOOST_MPI_CHECK_RESULT(MPI_Test,
                             (m_requests, &flag, &stat.m_status));
      if (flag) {
        // Resize our buffer and get ready to receive its data
        this->extra::m_values.resize(this->extra::m_count);
        BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                               (detail::c_data(this->extra::m_values), this->extra::m_values.size(), get_mpi_datatype<T>(),
                                stat.source(), stat.tag(), 
                                MPI_Comm(m_comm), m_requests + 1));
      } else
        return optional<status>(); // We have not finished yet
    } 

    // Check if we have received the message data
    BOOST_MPI_CHECK_RESULT(MPI_Test,
                           (m_requests + 1, &flag, &stat.m_status));
    if (flag) {
      return stat;
    } else 
      return optional<status>();
  }
};

class BOOST_MPI_DECL request::trivial_handler : public request::handler {

public:
  trivial_handler();
  
  status wait();
  optional<status> test();
  void cancel();
  
  bool active() const;
  optional<MPI_Request&> trivial();

private:
  friend class request;
  MPI_Request      m_request;
};

class request::dynamic_handler : public request::handler {
  dynamic_handler();
  
  status wait();
  optional<status> test();
  void cancel();
  
  bool active() const;
  optional<MPI_Request&> trivial();

private:
  friend class request;
  MPI_Request      m_requests[2];
};

template<typename T> 
request request::make_serialized(communicator const& comm, int source, int tag, T& value) {
#if defined(BOOST_MPI_USE_IMPROBE)
  return request(new probe_handler<detail::serialized_data<T> >(comm, source, tag, value));
#else
  return request(new legacy_serialized_handler<T>(comm, source, tag, value));
#endif
}

template<typename T>
request request::make_serialized_array(communicator const& comm, int source, int tag, T* values, int n) {
#if defined(BOOST_MPI_USE_IMPROBE)
  return request(new probe_handler<detail::serialized_array_data<T> >(comm, source, tag, values, n));
#else
  return request(new legacy_serialized_array_handler<T>(comm, source, tag, values, n));
#endif
}

template<typename T, class A>
request request::make_dynamic_primitive_array_recv(communicator const& comm, int source, int tag, 
                                                   std::vector<T,A>& values) {
#if defined(BOOST_MPI_USE_IMPROBE)
  return request(new probe_handler<detail::dynamic_primitive_array_data<std::vector<T,A> > >(comm,source,tag,values));
#else
  return request(new legacy_dynamic_primitive_array_handler<T,A>(comm, source, tag, values));
#endif
}

template<typename T>
request
request::make_trivial_send(communicator const& comm, int dest, int tag, T const* values, int n) {
  trivial_handler* handler = new trivial_handler;
  BOOST_MPI_CHECK_RESULT(MPI_Isend,
                         (const_cast<T*>(values), n, 
                          get_mpi_datatype<T>(),
                          dest, tag, comm, &handler->m_request));
  return request(handler);
}

template<typename T>
request
request::make_trivial_send(communicator const& comm, int dest, int tag, T const& value) {
  return make_trivial_send(comm, dest, tag, &value, 1);
}

template<typename T>
request
request::make_trivial_recv(communicator const& comm, int dest, int tag, T* values, int n) {
  trivial_handler* handler = new trivial_handler;
  BOOST_MPI_CHECK_RESULT(MPI_Irecv,
                         (values, n, 
                          get_mpi_datatype<T>(),
                          dest, tag, comm, &handler->m_request));
  return request(handler);
}

template<typename T>
request
request::make_trivial_recv(communicator const& comm, int dest, int tag, T& value) {
  return make_trivial_recv(comm, dest, tag, &value, 1);
}

template<typename T, class A>
request request::make_dynamic_primitive_array_send(communicator const& comm, int dest, int tag, 
                                                   std::vector<T,A> const& values) {
#if defined(BOOST_MPI_USE_IMPROBE)
  return make_trivial_send(comm, dest, tag, values.data(), values.size());
#else
  {
    // non blocking recv by legacy_dynamic_primitive_array_handler
    // blocking recv by status recv_vector(source,tag,value,primitive)
    boost::shared_ptr<std::size_t> size(new std::size_t(values.size()));
    dynamic_handler* handler = new dynamic_handler;
    request req(handler);
    req.preserve(size);
    
    BOOST_MPI_CHECK_RESULT(MPI_Isend,
                           (size.get(), 1,
                            get_mpi_datatype(*size),
                            dest, tag, comm, handler->m_requests+0));
    BOOST_MPI_CHECK_RESULT(MPI_Isend,
                           (const_cast<T*>(values.data()), *size, 
                            get_mpi_datatype<T>(),
                            dest, tag, comm, handler->m_requests+1));
    return req;
  }
#endif
}

inline
request::legacy_handler::legacy_handler(communicator const& comm, int source, int tag)
  : m_comm(comm),
    m_source(source),
    m_tag(tag)
{
  m_requests[0] = MPI_REQUEST_NULL;
  m_requests[1] = MPI_REQUEST_NULL;
}
    
}}

#endif // BOOST_MPI_REQUEST_HANDLERS_HPP
