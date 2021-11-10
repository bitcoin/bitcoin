// Copyright 2004 The Trustees of Indiana University.

// Use, modification and distribution is subject to the Boost Software
// License, Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

//  Authors: Douglas Gregor
//           Peter Gottschling
//           Andrew Lumsdaine
#ifndef BOOST_PARALLEL_DISTRIBUTION_HPP
#define BOOST_PARALLEL_DISTRIBUTION_HPP

#ifndef BOOST_GRAPH_USE_MPI
#error "Parallel BGL files should not be included unless <boost/graph/use_mpi.hpp> has been included"
#endif

#include <cstddef>
#include <vector>
#include <algorithm>
#include <numeric>
#include <boost/assert.hpp>
#include <boost/iterator/counting_iterator.hpp>
#include <boost/random/uniform_int.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/config.hpp>
#include <typeinfo>

namespace boost { namespace parallel {

template<typename ProcessGroup, typename SizeType = std::size_t>
class variant_distribution
{
public:
  typedef typename ProcessGroup::process_id_type process_id_type;
  typedef typename ProcessGroup::process_size_type process_size_type;
  typedef SizeType size_type;

private:
  struct basic_distribution
  {
    virtual ~basic_distribution() {}
    virtual size_type block_size(process_id_type, size_type) const = 0;
    virtual process_id_type in_process(size_type) const = 0;
    virtual size_type local(size_type) const = 0;
    virtual size_type global(size_type) const = 0;
    virtual size_type global(process_id_type, size_type) const = 0;
    virtual void* address() = 0;
    virtual const void* address() const = 0;
    virtual const std::type_info& type() const = 0;
  };

  template<typename Distribution>
  struct poly_distribution : public basic_distribution
  {
    explicit poly_distribution(const Distribution& distribution)
      : distribution_(distribution) { }

    virtual size_type block_size(process_id_type id, size_type n) const
    { return distribution_.block_size(id, n); }

    virtual process_id_type in_process(size_type i) const
    { return distribution_(i); }

    virtual size_type local(size_type i) const
    { return distribution_.local(i); }

    virtual size_type global(size_type n) const
    { return distribution_.global(n); }

    virtual size_type global(process_id_type id, size_type n) const
    { return distribution_.global(id, n); }

    virtual void* address() { return &distribution_; }
    virtual const void* address() const { return &distribution_; }
    virtual const std::type_info& type() const { return typeid(Distribution); }

  private:
    Distribution distribution_;
  };

public:
  variant_distribution() { }

  template<typename Distribution>
  variant_distribution(const Distribution& distribution)
    : distribution_(new poly_distribution<Distribution>(distribution)) { }

  size_type block_size(process_id_type id, size_type n) const
  { return distribution_->block_size(id, n); }
  
  process_id_type operator()(size_type i) const
  { return distribution_->in_process(i); }
  
  size_type local(size_type i) const
  { return distribution_->local(i); }
  
  size_type global(size_type n) const
  { return distribution_->global(n); }

  size_type global(process_id_type id, size_type n) const
  { return distribution_->global(id, n); }

  operator bool() const { return distribution_; }

  void clear() { distribution_.reset(); }

  template<typename T>
  T* as()
  {
    if (distribution_->type() == typeid(T))
      return static_cast<T*>(distribution_->address());
    else
      return 0;
  }

  template<typename T>
  const T* as() const
  {
    if (distribution_->type() == typeid(T))
      return static_cast<T*>(distribution_->address());
    else
      return 0;
  }

private:
  shared_ptr<basic_distribution> distribution_;
};

struct block
{
  template<typename LinearProcessGroup>
  explicit block(const LinearProcessGroup& pg, std::size_t n) 
    : id(process_id(pg)), p(num_processes(pg)), n(n) { }

  // If there are n elements in the distributed data structure, returns the number of elements stored locally.
  template<typename SizeType>
  SizeType block_size(SizeType n) const
  { return (n / p) + ((std::size_t)(n % p) > id? 1 : 0); }

  // If there are n elements in the distributed data structure, returns the number of elements stored on processor ID
  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType n) const
  { return (n / p) + ((ProcessID)(n % p) > id? 1 : 0); }

  // Returns the processor on which element with global index i is stored
  template<typename SizeType>
  SizeType operator()(SizeType i) const
  { 
    SizeType cutoff_processor = n % p;
    SizeType cutoff = cutoff_processor * (n / p + 1);

    if (i < cutoff) return i / (n / p + 1);
    else return cutoff_processor + (i - cutoff) / (n / p);
  }

  // Find the starting index for processor with the given id
  template<typename ID>
  std::size_t start(ID id) const
  {
    std::size_t estimate = id * (n / p + 1);
    ID cutoff_processor = n % p;
    if (id < cutoff_processor) return estimate;
    else return estimate - (id - cutoff_processor);
  }

  // Find the local index for the ith global element
  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    SizeType owner = (*this)(i);
    return i - start(owner);
  }

  // Returns the global index of local element i
  template<typename SizeType>
  SizeType global(SizeType i) const
  { return global(id, i); }

  // Returns the global index of the ith local element on processor id
  template<typename ProcessID, typename SizeType>
  SizeType global(ProcessID id, SizeType i) const
  { return i + start(id); }

 private:
  std::size_t id; //< The ID number of this processor
  std::size_t p;  //< The number of processors
  std::size_t n;  //< The size of the problem space
};

// Block distribution with arbitrary block sizes
struct uneven_block
{
  typedef std::vector<std::size_t>    size_vector;

  template<typename LinearProcessGroup>
  explicit uneven_block(const LinearProcessGroup& pg, const std::vector<std::size_t>& local_sizes) 
    : id(process_id(pg)), p(num_processes(pg)), local_sizes(local_sizes)
  { 
    BOOST_ASSERT(local_sizes.size() == p);
    local_starts.resize(p + 1);
    local_starts[0] = 0;
    std::partial_sum(local_sizes.begin(), local_sizes.end(), &local_starts[1]);
    n = local_starts[p];
  }

  // To do maybe: enter local size in each process and gather in constructor (much handier)
  // template<typename LinearProcessGroup>
  // explicit uneven_block(const LinearProcessGroup& pg, std::size_t my_local_size) 

  // If there are n elements in the distributed data structure, returns the number of elements stored locally.
  template<typename SizeType>
  SizeType block_size(SizeType) const
  { return local_sizes[id]; }

  // If there are n elements in the distributed data structure, returns the number of elements stored on processor ID
  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType) const
  { return local_sizes[id]; }

  // Returns the processor on which element with global index i is stored
  template<typename SizeType>
  SizeType operator()(SizeType i) const
  {
    BOOST_ASSERT (i >= (SizeType) 0 && i < (SizeType) n); // check for valid range
    size_vector::const_iterator lb = std::lower_bound(local_starts.begin(), local_starts.end(), (std::size_t) i);
    return ((SizeType)(*lb) == i ? lb : --lb) - local_starts.begin();
  }

  // Find the starting index for processor with the given id
  template<typename ID>
  std::size_t start(ID id) const 
  {
    return local_starts[id];
  }

  // Find the local index for the ith global element
  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    SizeType owner = (*this)(i);
    return i - start(owner);
  }

  // Returns the global index of local element i
  template<typename SizeType>
  SizeType global(SizeType i) const
  { return global(id, i); }

  // Returns the global index of the ith local element on processor id
  template<typename ProcessID, typename SizeType>
  SizeType global(ProcessID id, SizeType i) const
  { return i + start(id); }

 private:
  std::size_t              id;           //< The ID number of this processor
  std::size_t              p;            //< The number of processors
  std::size_t              n;            //< The size of the problem space
  std::vector<std::size_t> local_sizes;  //< The sizes of all blocks
  std::vector<std::size_t> local_starts; //< Lowest global index of each block
};


struct oned_block_cyclic
{
  template<typename LinearProcessGroup>
  explicit oned_block_cyclic(const LinearProcessGroup& pg, std::size_t size)
    : id(process_id(pg)), p(num_processes(pg)), size(size) { }
      
  template<typename SizeType>
  SizeType block_size(SizeType n) const
  { 
    return block_size(id, n);
  }

  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType n) const
  {
    SizeType all_blocks = n / size;
    SizeType extra_elements = n % size;
    SizeType everyone_gets = all_blocks / p;
    SizeType extra_blocks = all_blocks % p;
    SizeType my_blocks = everyone_gets + (p < extra_blocks? 1 : 0);
    SizeType my_elements = my_blocks * size 
                         + (p == extra_blocks? extra_elements : 0);
    return my_elements;
  }

  template<typename SizeType>
  SizeType operator()(SizeType i) const
  { 
    return (i / size) % p;
  }

  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    return ((i / size) / p) * size + i % size;
  }

  template<typename SizeType>
  SizeType global(SizeType i) const
  { return global(id, i); }

  template<typename ProcessID, typename SizeType>
  SizeType global(ProcessID id, SizeType i) const
  { 
    return ((i / size) * p + id) * size + i % size;
  }

 private:
  std::size_t id;                   //< The ID number of this processor
  std::size_t p;                    //< The number of processors
  std::size_t size;                 //< Block size
};

struct twod_block_cyclic
{
  template<typename LinearProcessGroup>
  explicit twod_block_cyclic(const LinearProcessGroup& pg,
                             std::size_t block_rows, std::size_t block_columns,
                             std::size_t data_columns_per_row)
    : id(process_id(pg)), p(num_processes(pg)), 
      block_rows(block_rows), block_columns(block_columns), 
      data_columns_per_row(data_columns_per_row)
  { }
      
  template<typename SizeType>
  SizeType block_size(SizeType n) const
  { 
    return block_size(id, n);
  }

  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType n) const
  {
    // TBD: This is really lame :)
    int result = -1;
    while (n > 0) {
      --n;
      if ((*this)(n) == id && (int)local(n) > result) result = local(n);
    }
    ++result;

    //    std::cerr << "Block size of id " << id << " is " << result << std::endl;
    return result;
  }

  template<typename SizeType>
  SizeType operator()(SizeType i) const
  { 
    SizeType result = get_block_num(i) % p;
    //    std::cerr << "Item " << i << " goes on processor " << result << std::endl;
    return result;
  }

  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    // Compute the start of the block
    std::size_t block_num = get_block_num(i);
    //    std::cerr << "Item " << i << " is in block #" << block_num << std::endl;

    std::size_t local_block_num = block_num / p;
    std::size_t block_start = local_block_num * block_rows * block_columns;

    // Compute the offset into the block 
    std::size_t data_row = i / data_columns_per_row;
    std::size_t data_col = i % data_columns_per_row;
    std::size_t block_offset = (data_row % block_rows) * block_columns 
                             + (data_col % block_columns);    

    //    std::cerr << "Item " << i << " maps to local index " << block_start+block_offset << std::endl;
    return block_start + block_offset;
  }

  template<typename SizeType>
  SizeType global(SizeType i) const
  { 
    // Compute the (global) block in which this element resides
    SizeType local_block_num = i / (block_rows * block_columns);
    SizeType block_offset = i % (block_rows * block_columns);
    SizeType block_num = local_block_num * p + id;

    // Compute the position of the start of the block (globally)
    SizeType block_start = block_num * block_rows * block_columns;

    std::cerr << "Block " << block_num << " starts at index " << block_start
              << std::endl;

    // Compute the row and column of this block
    SizeType block_row = block_num / (data_columns_per_row / block_columns);
    SizeType block_col = block_num % (data_columns_per_row / block_columns);

    SizeType row_in_block = block_offset / block_columns;
    SizeType col_in_block = block_offset % block_columns;

    std::cerr << "Local index " << i << " is in block at row " << block_row
              << ", column " << block_col << ", in-block row " << row_in_block
              << ", in-block col " << col_in_block << std::endl;

    SizeType result = block_row * block_rows + block_col * block_columns
                    + row_in_block * block_rows + col_in_block;


    std::cerr << "global(" << i << "@" << id << ") = " << result 
              << " =? " << local(result) << std::endl;
    BOOST_ASSERT(i == local(result));
    return result;
  }

 private:
  template<typename SizeType>
  std::size_t get_block_num(SizeType i) const
  {
    std::size_t data_row = i / data_columns_per_row;
    std::size_t data_col = i % data_columns_per_row;
    std::size_t block_row = data_row / block_rows;
    std::size_t block_col = data_col / block_columns;
    std::size_t blocks_in_row = data_columns_per_row / block_columns;
    std::size_t block_num = block_col * blocks_in_row + block_row;
    return block_num;
  }

  std::size_t id;                   //< The ID number of this processor
  std::size_t p;                    //< The number of processors
  std::size_t block_rows;           //< The # of rows in each block
  std::size_t block_columns;        //< The # of columns in each block
  std::size_t data_columns_per_row; //< The # of columns per row of data
};

class twod_random
{
  template<typename RandomNumberGen>
  struct random_int
  {
    explicit random_int(RandomNumberGen& gen) : gen(gen) { }

    template<typename T>
    T operator()(T n) const
    {
      uniform_int<T> distrib(0, n-1);
      return distrib(gen);
    }

  private:
    RandomNumberGen& gen;
  };
  
 public:
  template<typename LinearProcessGroup, typename RandomNumberGen>
  explicit twod_random(const LinearProcessGroup& pg,
                       std::size_t block_rows, std::size_t block_columns,
                       std::size_t data_columns_per_row,
                       std::size_t n,
                       RandomNumberGen& gen)
    : id(process_id(pg)), p(num_processes(pg)), 
      block_rows(block_rows), block_columns(block_columns), 
      data_columns_per_row(data_columns_per_row),
      global_to_local(n / (block_rows * block_columns))
  { 
    std::copy(make_counting_iterator(std::size_t(0)),
              make_counting_iterator(global_to_local.size()),
              global_to_local.begin());

#if defined(BOOST_NO_CXX98_RANDOM_SHUFFLE)
    std::shuffle(global_to_local.begin(), global_to_local.end(), gen);
#else
    random_int<RandomNumberGen> rand(gen);
    std::random_shuffle(global_to_local.begin(), global_to_local.end(), rand);
#endif
  }
      
  template<typename SizeType>
  SizeType block_size(SizeType n) const
  { 
    return block_size(id, n);
  }

  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType n) const
  {
    // TBD: This is really lame :)
    int result = -1;
    while (n > 0) {
      --n;
      if ((*this)(n) == id && (int)local(n) > result) result = local(n);
    }
    ++result;

    //    std::cerr << "Block size of id " << id << " is " << result << std::endl;
    return result;
  }

  template<typename SizeType>
  SizeType operator()(SizeType i) const
  { 
    SizeType result = get_block_num(i) % p;
    //    std::cerr << "Item " << i << " goes on processor " << result << std::endl;
    return result;
  }

  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    // Compute the start of the block
    std::size_t block_num = get_block_num(i);
    //    std::cerr << "Item " << i << " is in block #" << block_num << std::endl;

    std::size_t local_block_num = block_num / p;
    std::size_t block_start = local_block_num * block_rows * block_columns;

    // Compute the offset into the block 
    std::size_t data_row = i / data_columns_per_row;
    std::size_t data_col = i % data_columns_per_row;
    std::size_t block_offset = (data_row % block_rows) * block_columns 
                             + (data_col % block_columns);    

    //    std::cerr << "Item " << i << " maps to local index " << block_start+block_offset << std::endl;
    return block_start + block_offset;
  }

 private:
  template<typename SizeType>
  std::size_t get_block_num(SizeType i) const
  {
    std::size_t data_row = i / data_columns_per_row;
    std::size_t data_col = i % data_columns_per_row;
    std::size_t block_row = data_row / block_rows;
    std::size_t block_col = data_col / block_columns;
    std::size_t blocks_in_row = data_columns_per_row / block_columns;
    std::size_t block_num = block_col * blocks_in_row + block_row;
    return global_to_local[block_num];
  }

  std::size_t id;                   //< The ID number of this processor
  std::size_t p;                    //< The number of processors
  std::size_t block_rows;           //< The # of rows in each block
  std::size_t block_columns;        //< The # of columns in each block
  std::size_t data_columns_per_row; //< The # of columns per row of data
  std::vector<std::size_t> global_to_local;
};

class random_distribution
{
  template<typename RandomNumberGen>
  struct random_int
  {
    explicit random_int(RandomNumberGen& gen) : gen(gen) { }

    template<typename T>
    T operator()(T n) const
    {
      uniform_int<T> distrib(0, n-1);
      return distrib(gen);
    }

  private:
    RandomNumberGen& gen;
  };
  
 public:
  template<typename LinearProcessGroup, typename RandomNumberGen>
  random_distribution(const LinearProcessGroup& pg, RandomNumberGen& gen,
                      std::size_t n)
    : base(pg, n), local_to_global(n), global_to_local(n)
  {
    std::copy(make_counting_iterator(std::size_t(0)),
              make_counting_iterator(n),
              local_to_global.begin());

#if defined(BOOST_NO_CXX98_RANDOM_SHUFFLE)
    std::shuffle(local_to_global.begin(), local_to_global.end(), gen);
#else
    random_int<RandomNumberGen> rand(gen);
    std::random_shuffle(local_to_global.begin(), local_to_global.end(), rand);
#endif

    for (std::vector<std::size_t>::size_type i = 0; i < n; ++i)
      global_to_local[local_to_global[i]] = i;
  }

  template<typename SizeType>
  SizeType block_size(SizeType n) const
  { return base.block_size(n); }

  template<typename SizeType, typename ProcessID>
  SizeType block_size(ProcessID id, SizeType n) const
  { return base.block_size(id, n); }

  template<typename SizeType>
  SizeType operator()(SizeType i) const
  {
    return base(global_to_local[i]);
  }

  template<typename SizeType>
  SizeType local(SizeType i) const
  { 
    return base.local(global_to_local[i]);
  }

  template<typename ProcessID, typename SizeType>
  SizeType global(ProcessID p, SizeType i) const
  { 
    return local_to_global[base.global(p, i)];
  }

  template<typename SizeType>
  SizeType global(SizeType i) const
  { 
    return local_to_global[base.global(i)];
  }

 private:
  block base;
  std::vector<std::size_t> local_to_global;
  std::vector<std::size_t> global_to_local;
};

} } // end namespace boost::parallel

#endif // BOOST_PARALLEL_DISTRIBUTION_HPP

