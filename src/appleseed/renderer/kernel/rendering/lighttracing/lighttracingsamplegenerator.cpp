
//
// This source file is part of appleseed.
// Visit http://appleseedhq.net/ for additional information and resources.
//
// This software is released under the MIT license.
//
// Copyright (c) 2010-2011 Francois Beaune
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

// Interface header.
#include "lighttracingsamplegenerator.h"

// appleseed.renderer headers.
#include "renderer/global/globaltypes.h"
#include "renderer/kernel/rendering/sample.h"
#include "renderer/kernel/rendering/samplegeneratorbase.h"
#include "renderer/kernel/shading/shadingresult.h"
#include "renderer/modeling/frame/frame.h"

// appleseed.foundation headers.
#include "foundation/math/qmc.h"
#include "foundation/math/rng.h"
#include "foundation/math/vector.h"

// Forward declarations.
namespace foundation    { class LightingConditions; }

using namespace foundation;

namespace renderer
{

namespace
{
    //
    // LightTracingSampleGenerator class implementation.
    //

    class LightTracingSampleGenerator
      : public SampleGeneratorBase
    {
      public:
        LightTracingSampleGenerator(
            Frame&          frame,
            const size_t    generator_index,
            const size_t    generator_count)
          : SampleGeneratorBase(generator_index, generator_count)
          , m_frame(frame)
          , m_lighting_conditions(frame.get_lighting_conditions())
        {
        }

        virtual void release()
        {
            delete this;
        }

        virtual void reset()
        {
            SampleGeneratorBase::reset();
            m_rng = MersenneTwister();
        }

      private:
        Frame&                      m_frame;
        const LightingConditions&   m_lighting_conditions;
        MersenneTwister             m_rng;

        virtual void generate_sample(Sample& sample)
        {
            // Compute the sample coordinates in [0,1)^2.
            const size_t Bases[2] = { 2, 3 };
            const Vector2d s = halton_sequence<double, 2>(Bases, m_sequence_index);

            // Compute the sample position, in NDC.
            const Vector2d sample_position = m_frame.get_sample_position(s.x, s.y);

            // Create a sampling context.
            SamplingContext sampling_context(
                m_rng,
                2,                      // number of dimensions
                0,                      // number of samples
                m_sequence_index);      // initial instance number

            // Render the sample.
            ShadingResult shading_result;
/*
            m_sample_renderer->render_sample(
                sampling_context,
                sample_position,
                shading_result);
*/

            // Transform the sample to the linear RGB color space.
            shading_result.transform_to_linear_rgb(m_lighting_conditions);

            // Return the sample.
            sample.m_position = sample_position;
            sample.m_color[0] = shading_result.m_color[0];
            sample.m_color[1] = shading_result.m_color[1];
            sample.m_color[2] = shading_result.m_color[2];
            sample.m_color[3] = shading_result.m_alpha[0];
        }
    };
}


//
// LightTracingSampleGeneratorFactory class implementation.
//

LightTracingSampleGeneratorFactory::LightTracingSampleGeneratorFactory(Frame& frame)
  : m_frame(frame)
{
}

void LightTracingSampleGeneratorFactory::release()
{
    delete this;
}

ISampleGenerator* LightTracingSampleGeneratorFactory::create(
    const size_t    generator_index,
    const size_t    generator_count)
{
    return
        new LightTracingSampleGenerator(
            m_frame,
            generator_index,
            generator_count);
}

}   // namespace renderer
