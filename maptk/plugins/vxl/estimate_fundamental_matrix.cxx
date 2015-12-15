/*ckwg +29
 * Copyright 2015 by Kitware, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 *  * Neither name of Kitware, Inc. nor the names of any contributors may be used
 *    to endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


/**
 * \file
 * \brief vxl estimate fundamental matrix implementation
 */

#include "estimate_fundamental_matrix.h"

#include <vital/vital_foreach.h>

#include <vital/types/feature.h>
#include <maptk/plugins/vxl/camera.h>

#include <vgl/vgl_point_2d.h>
#include <Eigen/LU>

#include <vpgl/algo/vpgl_fm_compute_8_point.h>

using namespace kwiver::vital;

namespace kwiver {
namespace maptk {

namespace vxl
{


/// Private implementation class
class estimate_fundamental_matrix::priv
{
public:
  /// Constructor
  priv()
  : precondition(true)
  {
  }

  priv(const priv& other)
  : precondition(other.precondition)
  {
  }

  bool precondition;
};


/// Constructor
estimate_fundamental_matrix
::estimate_fundamental_matrix()
: d_(new priv)
{
}


/// Copy Constructor
estimate_fundamental_matrix
::estimate_fundamental_matrix(const estimate_fundamental_matrix& other)
: d_(new priv(*other.d_))
{
}


/// Destructor
estimate_fundamental_matrix
::~estimate_fundamental_matrix()
{
}



/// Get this algorithm's \link vital::config_block configuration block \endlink
vital::config_block_sptr
estimate_fundamental_matrix
::get_configuration() const
{
  // get base config from base class
  vital::config_block_sptr config =
      vital::algo::estimate_fundamental_matrix::get_configuration();

  config->set_value("precondition", d_->precondition,
                    "If true, precondition the data before estimating the "
                    "fundamental matrix");

  return config;
}


/// Set this algorithm's properties via a config block
void
estimate_fundamental_matrix
::set_configuration(vital::config_block_sptr config)
{

  d_->precondition = config->get_value<bool>("precondition",
                                             d_->precondition);
}


/// Check that the algorithm's currently configuration is valid
bool
estimate_fundamental_matrix
::check_configuration(vital::config_block_sptr config) const
{
  return true;
}


/// Estimate an essential matrix from corresponding points
fundamental_matrix_sptr
estimate_fundamental_matrix
::estimate(const std::vector<vector_2d>& pts1,
           const std::vector<vector_2d>& pts2,
           std::vector<bool>& inliers,
           double inlier_scale) const
{
  vcl_vector<vgl_homg_point_2d<double> > right_points, left_points;
  VITAL_FOREACH(const vector_2d& v, pts1)
  {
    right_points.push_back(vgl_homg_point_2d<double>(v.x(), v.y()));
  }
  VITAL_FOREACH(const vector_2d& v, pts2)
  {
    left_points.push_back(vgl_homg_point_2d<double>(v.x(), v.y()));
  }

  vpgl_fm_compute_8_point fm_compute(d_->precondition);

  vpgl_fundamental_matrix<double> fm;
  fm_compute.compute(right_points, left_points, fm);
  matrix_3x3d F(fm.get_matrix().data_block());
  F.transposeInPlace();

  matrix_3x3d Ft = F.transpose();

  inliers.resize(pts1.size());
  for(unsigned i=0; i<pts1.size(); ++i)
  {
    const vector_2d& p1 = pts1[i];
    const vector_2d& p2 = pts2[i];
    vector_3d v1(p1.x(), p1.y(), 1.0);
    vector_3d v2(p2.x(), p2.y(), 1.0);
    vector_3d l1 = F * v1;
    vector_3d l2 = Ft * v2;
    double s1 = 1.0 / sqrt(l1.x()*l1.x() + l1.y()*l1.y());
    double s2 = 1.0 / sqrt(l2.x()*l2.x() + l2.y()*l2.y());
    // sum of point to epipolar line distance in both images
    double d = v1.dot(l2) * (s1 + s2);
    inliers[i] = std::fabs(d) < inlier_scale;
  }

  return fundamental_matrix_sptr(new fundamental_matrix_d(F));
}


} // end namespace vxl

} // end namespace maptk
} // end namespace kwiver
