
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2013 Francois Beaune, Jupiter Jazz Limited
// Copyright (c) 2014-2015 Francois Beaune, The appleseedhq Organization
// Copyright (c) 2014-2015 Esteban Tovagliari, The appleseedhq Organization
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/modeling/bsdf/bsdfsample.h"

// appleseed.foundation headers.
#include "foundation/math/basis.h"
#include "foundation/math/vector.h"

// Standard headers.
#include <algorithm>
#include <cmath>

namespace renderer
{

template <typename T>
class MicrofacetBRDFHelper
{
  public:
    typedef foundation::Vector<T, 3>    VectorType;
    typedef foundation::Basis3<T>       BasisType;

    template <typename MDF, typename FresnelFun>
    static void sample(
        const MDF&  mdf,
        const T     alpha_x,
        const T     alpha_y,
        const T     g_alpha_x,
        const T     g_alpha_y,
        FresnelFun  f,
        BSDFSample& sample)
    {
        const VectorType& n = sample.get_shading_normal();
        const T cos_on = std::min(foundation::dot(sample.get_outgoing_vector(), n), T(1.0));
        if (cos_on < T(0.0))
            return;

        // Compute the incoming direction by sampling the MDF.
        sample.get_sampling_context().split_in_place(3, 1);
        const VectorType s = sample.get_sampling_context().next_vector2<3>();
        const VectorType wo = sample.get_shading_basis().transform_to_local(sample.get_outgoing_vector());
        const VectorType m = mdf.sample(wo, s, alpha_x, alpha_y);
        const VectorType h = sample.get_shading_basis().transform_to_parent(m);

        const foundation::Vector3d incoming = foundation::reflect(sample.get_outgoing_vector(), h);
        const T cos_oh = foundation::dot(sample.get_outgoing_vector(), h);

        // No reflection below the shading surface.
        const T cos_in = foundation::dot(incoming, n);
        if (cos_in < T(0.0))
            return;

        const T D = mdf.D(m, alpha_x, alpha_y);

        const T G =
            mdf.G(
                sample.get_shading_basis().transform_to_local(incoming),
                wo,
                m,
                g_alpha_x,
                g_alpha_y);

        f(sample.get_outgoing_vector(), h, sample.get_shading_normal(), sample.value());
        sample.value() *= static_cast<float>(D * G / (T(4.0) * cos_on * cos_in));
        sample.set_probability(mdf.pdf(wo, m, alpha_x, alpha_y) / (T(4.0) * cos_oh));
        sample.set_mode(BSDFSample::Glossy);
        sample.set_incoming(incoming);
    }

    template <typename MDF, typename FresnelFun>
    static T evaluate(
        const MDF&          mdf,
        const T             alpha_x,
        const T             alpha_y,
        const T             g_alpha_x,
        const T             g_alpha_y,
        const BasisType&    shading_basis,
        const VectorType&   outgoing,
        const VectorType&   incoming,
        const int           modes,
        FresnelFun          f,
        Spectrum&           value)
    {
        if (!(modes & BSDFSample::Glossy))
            return 0.0;

        // No reflection below the shading surface.
        const VectorType& n = shading_basis.get_normal();
        const T cos_in = foundation::dot(incoming, n);
        const T cos_on = std::min(foundation::dot(outgoing, n), T(1.0));
        if (cos_in < T(0.0) || cos_on < T(0.0))
            return T(0.0);

        const VectorType h = foundation::normalize(incoming + outgoing);
        const VectorType m = shading_basis.transform_to_local(h);
        const T D = mdf.D(m, alpha_x, alpha_y);

        const VectorType wo = shading_basis.transform_to_local(outgoing);
        const T G =
            mdf.G(
                shading_basis.transform_to_local(incoming),
                wo,
                m,
                g_alpha_x,
                g_alpha_y);

        const T cos_oh = foundation::dot(outgoing, h);
        f(outgoing, h, shading_basis.get_normal(), value);
        value *= static_cast<float>(D * G / (T(4.0) * cos_on * cos_in));
        return mdf.pdf(wo, m, alpha_x, alpha_y) / (T(4.0) * cos_oh);
    }

    template <typename MDF>
    static T pdf(
        const MDF&          mdf,
        const T             alpha_x,
        const T             alpha_y,
        const BasisType&    shading_basis,
        const VectorType&   outgoing,
        const VectorType&   incoming,
        const int           modes)
    {
        if (!(modes & BSDFSample::Glossy))
            return T(0.0);

        // No reflection below the shading surface.
        const VectorType& n = shading_basis.get_normal();
        const T cos_in = foundation::dot(incoming, n);
        const T cos_on = std::min(foundation::dot(outgoing, n), T(1.0));
        if (cos_in < T(0.0) || cos_on < T(0.0))
            return T(0.0);

        const VectorType h = foundation::normalize(incoming + outgoing);
        const T cos_oh = foundation::dot(outgoing, h);
        return
            mdf.pdf(
                shading_basis.transform_to_local(outgoing),
                shading_basis.transform_to_local(h),
                alpha_x,
                alpha_y) / (T(4.0) * cos_oh);
    }
};

}   // namespace renderer
