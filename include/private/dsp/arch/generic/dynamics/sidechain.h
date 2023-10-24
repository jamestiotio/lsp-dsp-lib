/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-lib
 * Created on: 23 окт. 2023 г.
 *
 * lsp-dsp-lib is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * lsp-dsp-lib is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with lsp-dsp-lib. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef PRIVATE_DSP_ARCH_GENERIC_DYNAMICS_SIDECHAIN_H_
#define PRIVATE_DSP_ARCH_GENERIC_DYNAMICS_SIDECHAIN_H_

#ifndef PRIVATE_DSP_ARCH_GENERIC_IMPL
    #error "This header should not be included directly"
#endif /* PRIVATE_DSP_ARCH_GENERIC_IMPL */

namespace lsp
{
    namespace generic
    {

        float sidechain_rms(float *dst, float *head, const float *tail, float rms, float k, size_t count)
        {
            dsp::copy(head, dst, count);

            for (size_t i=0; i<count; ++i)
            {
                float s         = dst[i];
                float t         = tail[i];
                rms            += s*s - t*t;
                dst[i]          = rms * k;      // ssqrt1 will fix the negative values
            }

            dsp::ssqrt1(dst, count);
            return rms;
        }

    } /* namespace generic */
} /* namespace lsp */

#endif /* PRIVATE_DSP_ARCH_GENERIC_DYNAMICS_SIDECHAIN_H_ */
