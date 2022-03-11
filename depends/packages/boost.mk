package=boost
$(package)_version=1.82.0
$(package)_download_path=https://boostorg.jfrog.io/artifactory/main/release/$($(package)_version)/source/
$(package)_file_name=boost_$(subst .,_,$($(package)_version)).tar.bz2
$(package)_sha256_hash=a6e1ab9b0860e6a2881dd7b21fe9f737a095e5f33a3a874afc6a345228597ee6

define $(package)_stage_cmds
  mkdir -p $($(package)_staging_prefix_dir)/include/boost/algorithm/cxx11 && \
  cp -r boost/algorithm/cxx11/all_of.hpp $($(package)_staging_prefix_dir)/include/boost/algorithm/cxx11 && \
  cp boost/algorithm/string.hpp $($(package)_staging_prefix_dir)/include/boost/algorithm && \
  cp -r boost/algorithm/string $($(package)_staging_prefix_dir)/include/boost/algorithm && \
  cp boost/aligned_storage.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/align $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/asio $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/assert $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/array.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/assert.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/bind $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/blank.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/blank_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/call_traits.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/cerrno.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/concept $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/concept_check.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/container $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/container_hash $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/config.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/config $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/core $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/cstdint.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/cstdlib.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/current_function.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/date_time $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/describe $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/detail $($(package)_staging_prefix_dir)/include/boost && \
  rm -rf $($(package)_staging_prefix_dir)/include/boost/detail/winapi && \
  cp -r boost/exception $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/foreach_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/function.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/function_equal.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/function $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/functional $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/fusion $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/io_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/is_placeholder.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/get_pointer.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/io $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/integer $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/iterator $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/integer.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/integer_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/integer_traits.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/foreach_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/lexical_cast.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/lexical_cast $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/limits.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/make_shared.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/mem_fn.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/mp11 $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/multi_index $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/multi_index_container.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/multi_index_container_fwd.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/mpl $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/move $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/next_prior.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/noncopyable.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/none.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/none_t.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/numeric $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/operators.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/optional.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/optional $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/predef.h $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/predef $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/preprocessor $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/process.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/process $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/scoped_array.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/scoped_ptr.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/smart_ptr $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/swap.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/range $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/ref.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/shared_ptr.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/signals2 $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/system $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/static_assert.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/tokenizer.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/token_functions.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/token_iterator.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/tuple $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/type.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/type_index.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/type_index $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/type_traits $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/test $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/throw_exception.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/utility $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/utility.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/variant $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/version.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/visit_each.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp boost/weak_ptr.hpp $($(package)_staging_prefix_dir)/include/boost && \
  cp -r boost/winapi $($(package)_staging_prefix_dir)/include/boost
endef
