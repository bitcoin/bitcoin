//---------------------------------------------------------------------------//
// Copyright (c) 2013 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_HPP
#define BOOST_COMPUTE_HPP

#include <boost/compute/algorithm.hpp>
#include <boost/compute/allocator.hpp>
#include <boost/compute/async.hpp>
#include <boost/compute/buffer.hpp>
#include <boost/compute/cl.hpp>
#include <boost/compute/command_queue.hpp>
#include <boost/compute/config.hpp>
#include <boost/compute/container.hpp>
#include <boost/compute/context.hpp>
#include <boost/compute/device.hpp>
#include <boost/compute/functional.hpp>
#include <boost/compute/image.hpp>
#include <boost/compute/iterator.hpp>
#include <boost/compute/kernel.hpp>
#include <boost/compute/lambda.hpp>
#include <boost/compute/pipe.hpp>
#include <boost/compute/platform.hpp>
#include <boost/compute/program.hpp>
#include <boost/compute/random.hpp>
#include <boost/compute/svm.hpp>
#include <boost/compute/system.hpp>
#include <boost/compute/types.hpp>
#include <boost/compute/user_event.hpp>
#include <boost/compute/utility.hpp>
#include <boost/compute/version.hpp>

#ifdef BOOST_COMPUTE_HAVE_HDR_CL_EXT
#include <boost/compute/cl_ext.hpp>
#endif

#endif // BOOST_COMPUTE_HPP
