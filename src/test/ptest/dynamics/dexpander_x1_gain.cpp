/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-lib
 * Created on: 19 окт. 2023 г.
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

#include <lsp-plug.in/common/alloc.h>
#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/test-fw/ptest.h>

#define MIN_RANK 8
#define MAX_RANK 16

namespace lsp
{
    namespace generic
    {
        void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
    }

    IF_ARCH_X86(
        namespace sse2
        {
            void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }

        namespace avx2
        {
            void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
            void dexpander_x1_gain_fma3(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }

        namespace avx512
        {
            void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }
    )

    IF_ARCH_X86_64(
        namespace avx2
        {
            void x64_dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
            void x64_dexpander_x1_gain_fma3(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }
    )

    IF_ARCH_ARM(
        namespace neon_d32
        {
            void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }
    )

    IF_ARCH_AARCH64(
        namespace asimd
        {
            void dexpander_x1_gain(float *dst, const float *src, const dsp::expander_knee_t *c, size_t count);
        }
    )
}

typedef void (* expander_x1_func_t)(float *dst, const float *src, const lsp::dsp::expander_knee_t *c, size_t count);

//-----------------------------------------------------------------------------
// Performance test for logarithmic axis calculation
PTEST_BEGIN("dsp.dynamics", dexpander_x1_gain, 5, 1000)

    void call(const char *label, float *dst, const float *src, const dsp::expander_knee_t *gate, size_t count, expander_x1_func_t func)
    {
        if (!PTEST_SUPPORTED(func))
            return;

        char buf[80];
        sprintf(buf, "%s x %d", label, int(count));
        printf("Testing %s points...\n", buf);

        PTEST_LOOP(buf,
            func(dst, src, gate, count);
        );
    }

    PTEST_MAIN
    {
        size_t buf_size     = 1 << MAX_RANK;
        uint8_t *data       = NULL;
        float *ptr          = alloc_aligned<float>(data, buf_size * 2, 64);

        dsp::expander_knee_t exp;
        exp = {
           0.0316227823f,
           0.12589255f,
           0.0f,
           { -0.361912072f, -1.49999988f, -1.55424464f },
           { 1.0f, 2.76310205f }};

        float *src          = ptr;
        float *dst          = &src[buf_size];
        float k             = 72.0f / (1 << MIN_RANK);

        for (size_t i=0; i<buf_size; ++i)
        {
            float db        = -72.0f + (i % (1 << MIN_RANK)) * k;
            src[i]          = expf(db * M_LN10 * 0.05f);
        }

        #define CALL(func) \
            call(#func, dst, src, &exp, count, func)

        for (size_t i=MIN_RANK; i <= MAX_RANK; ++i)
        {
            size_t count = 1 << i;

            CALL(generic::dexpander_x1_gain);
            IF_ARCH_X86(CALL(sse2::dexpander_x1_gain));
            IF_ARCH_X86(CALL(avx2::dexpander_x1_gain));
            IF_ARCH_X86_64(CALL(avx2::x64_dexpander_x1_gain));
            IF_ARCH_X86(CALL(avx2::dexpander_x1_gain_fma3));
            IF_ARCH_X86_64(CALL(avx2::x64_dexpander_x1_gain_fma3));
            IF_ARCH_X86(CALL(avx512::dexpander_x1_gain));
            IF_ARCH_ARM(CALL(neon_d32::dexpander_x1_gain));
            IF_ARCH_AARCH64(CALL(asimd::dexpander_x1_gain));
            PTEST_SEPARATOR;
        }

        free_aligned(data);
    }
PTEST_END



