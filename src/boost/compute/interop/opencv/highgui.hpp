//---------------------------------------------------------------------------//
// Copyright (c) 2013-2014 Kyle Lutz <kyle.r.lutz@gmail.com>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//
// See http://boostorg.github.com/compute for more information.
//---------------------------------------------------------------------------//

#ifndef BOOST_COMPUTE_INTEROP_OPENCV_HIGHGUI_HPP
#define BOOST_COMPUTE_INTEROP_OPENCV_HIGHGUI_HPP

#include <opencv2/highgui/highgui.hpp>

#include <boost/compute/interop/opencv/core.hpp>

namespace boost {
namespace compute {

inline void opencv_imshow(const std::string &winname,
                          const image2d &image,
                          command_queue &queue = system::default_queue())
{
    const cv::Mat mat = opencv_create_mat_with_image2d(image, queue);

    cv::imshow(winname, mat);
}

} // end compute namespace
} // end boost namespace

#endif // BOOST_COMPUTE_INTEROP_OPENCV_HIGHGUI_HPP
