/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-lib
 * Created on: 22 окт. 2023 г.
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

#include <lsp-plug.in/common/types.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>
#include <lsp-plug.in/test-fw/helpers.h>

#define MIN_RANK 8
#define MAX_RANK 16

namespace lsp
{
    namespace generic
    {
        void compressor_env(float *dst, const float *src, dsp::compressor_env_t *env, size_t count);
    }

    IF_ARCH_X86(
        namespace sse2
        {
            void compressor_env(float *dst, const float *src, dsp::compressor_env_t *env, size_t count);
        }
    )

    IF_ARCH_ARM(
        namespace neon_d32
        {
            void compressor_env(float *dst, const float *src, dsp::compressor_env_t *env, size_t count);
        }
    )

    IF_ARCH_AARCH64(
        namespace asimd
        {
            void compressor_env(float *dst, const float *src, dsp::compressor_env_t *env, size_t count);
        }
    )

    static void compressor_env_naive(float *dst, const float *src, dsp::compressor_env_t *env, size_t count)
    {
        for (size_t i=0; i<count; ++i)
        {
            float s         = src[i];
            if (env->env > env->rel_thresh)
                env->env        += (s > env->env) ? env->attack * (s - env->env) : env->release * (s - env->env);
            else
                env->env        += env->attack * (s - env->env);
            dst[i]          = env->env;
        }
    }
}

typedef void (* compressor_env_func_t)(float *dst, const float *src, lsp::dsp::compressor_env_t *env, size_t count);

//-----------------------------------------------------------------------------
// Unit test for simple operations
UTEST_BEGIN("dsp.dynamics", compressor_env)

    void call(const char *label, compressor_env_func_t func)
    {
        if (!UTEST_SUPPORTED(func))
            return;

        dsp::compressor_env_t env, env1, env2;

        env.env         = 0.0f;
        env.rel_thresh  = 0.25f;
        env.attack      = 0.55f;
        env.release     = 0.45f;

        UTEST_FOREACH(count, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 32, 64, 65, 100, 999, 0xfff)
        {
            printf("Testing %s on input buffer of %d numbers...\n", label, int(count));

            FloatBuffer src(count);
            FloatBuffer dst(count);

            src.randomize_0to1();
            dst.randomize_sign();
            FloatBuffer dst1(dst);
            FloatBuffer dst2(dst);

            env.env         = randf(0.0f, 1.0f);
            env1            = env;
            env2            = env;

            // Call functions
            compressor_env_naive(dst1, src, &env1, count);
            func(dst2, src, &env2, count);

            UTEST_ASSERT_MSG(src.valid(), "Source buffer corrupted");
            UTEST_ASSERT_MSG(dst.valid(), "Destination buffer corrupted");
            UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
            UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");

            // Compare buffers
            if (!dst1.equals_relative(dst2, 1e-4))
            {
                src.dump("src ");
                dst.dump("dst ");
                dst1.dump("dst1");
                dst2.dump("dst2");
                printf("index=%d, %.6f vs %.6f\n", dst1.last_diff(), dst1.get_diff(), dst2.get_diff());
                UTEST_FAIL_MSG("Output of functions for test '%s' differs", label);
            }

            // Compare envelope value
            if (!float_equals_relative(env1.env, env2.env, 1e-4f))
            {
                src.dump("src ");
                dst.dump("dst ");
                dst1.dump("dst1");
                dst2.dump("dst2");
                UTEST_FAIL_MSG("Output envelope for test '%s' differs: %.6f vs %.6f", label, env1.env, env2.env);
            }
        }
    }

    UTEST_MAIN
    {
        #define CALL(func) \
            call(#func, func);

        CALL(generic::compressor_env);
        IF_ARCH_X86(CALL(sse2::compressor_env));
//        IF_ARCH_ARM(CALL(neon_d32::compressor_env));
//        IF_ARCH_AARCH64(CALL(asimd::compressor_env));
    }
UTEST_END



