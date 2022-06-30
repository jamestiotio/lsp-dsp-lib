/*
 * Copyright (C) 2020 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2020 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-lib
 * Created on: 31 мар. 2020 г.
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
#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/FloatBuffer.h>

#define MIN_RANK 8
#define MAX_RANK 16

namespace lsp
{
    namespace generic
    {
        void    add2(float *dst, const float *src, size_t count);
        void    sub2(float *dst, const float *src, size_t count);
        void    rsub2(float *dst, const float *src, size_t count);
        void    mul2(float *dst, const float *src, size_t count);
        void    div2(float *dst, const float *src, size_t count);
        void    rdiv2(float *dst, const float *src, size_t count);
        void    mod2(float *dst, const float *src, size_t count);
        void    rmod2(float *dst, const float *src, size_t count);
    }

    IF_ARCH_X86(
        namespace sse
        {
            void    add2(float *dst, const float *src, size_t count);
            void    sub2(float *dst, const float *src, size_t count);
            void    rsub2(float *dst, const float *src, size_t count);
            void    mul2(float *dst, const float *src, size_t count);
            void    div2(float *dst, const float *src, size_t count);
            void    rdiv2(float *dst, const float *src, size_t count);
        }

        namespace sse2
        {
            void    mod2(float *dst, const float *src, size_t count);
            void    rmod2(float *dst, const float *src, size_t count);
        }

        namespace avx
        {
            void    add2(float *dst, const float *src, size_t count);
            void    sub2(float *dst, const float *src, size_t count);
            void    rsub2(float *dst, const float *src, size_t count);
            void    mul2(float *dst, const float *src, size_t count);
            void    div2(float *dst, const float *src, size_t count);
            void    rdiv2(float *dst, const float *src, size_t count);
            void    mod2(float *dst, const float *src, size_t count);
            void    rmod2(float *dst, const float *src, size_t count);
            void    mod2_fma3(float *dst, const float *src, size_t count);
            void    rmod2_fma3(float *dst, const float *src, size_t count);
        }
    )

    IF_ARCH_ARM(
        namespace neon_d32
        {
            void    add2(float *dst, const float *src, size_t count);
            void    sub2(float *dst, const float *src, size_t count);
            void    rsub2(float *dst, const float *src, size_t count);
            void    mul2(float *dst, const float *src, size_t count);
            void    div2(float *dst, const float *src, size_t count);
            void    rdiv2(float *dst, const float *src, size_t count);
            void    mod2(float *dst, const float *src, size_t count);
            void    rmod2(float *dst, const float *src, size_t count);
        }
    )

    IF_ARCH_AARCH64(
        namespace asimd
        {
            void    add2(float *dst, const float *src, size_t count);
            void    sub2(float *dst, const float *src, size_t count);
            void    rsub2(float *dst, const float *src, size_t count);
            void    mul2(float *dst, const float *src, size_t count);
            void    div2(float *dst, const float *src, size_t count);
            void    rdiv2(float *dst, const float *src, size_t count);
            void    mod2(float *dst, const float *src, size_t count);
            void    rmod2(float *dst, const float *src, size_t count);
        }
    )
}

typedef void (* func2)(float *dst, const float *src, size_t count);

//-----------------------------------------------------------------------------
// Unit test for simple operations
UTEST_BEGIN("dsp.pmath", op2)

    void call(const char *label, size_t align, func2 func1, func2 func2)
    {
        if (!UTEST_SUPPORTED(func1))
            return;
        if (!UTEST_SUPPORTED(func2))
            return;

        UTEST_FOREACH(count, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                32, 64, 65, 100, 999, 0xfff)
        {
            for (size_t mask=0; mask <= 0x03; ++mask)
            {
                printf("Testing %s on input buffer of %d numbers, mask=0x%x...\n", label, int(count), int(mask));

                FloatBuffer src(count, align, mask & 0x01);
                FloatBuffer dst1(count, align, mask & 0x02);
                FloatBuffer dst2(dst1);

                // Call functions
                src.randomize_sign();
                func1(dst1, src, count);
                func2(dst2, src, count);

                UTEST_ASSERT_MSG(src.valid(), "Source buffer corrupted");
                UTEST_ASSERT_MSG(dst1.valid(), "Destination buffer 1 corrupted");
                UTEST_ASSERT_MSG(dst2.valid(), "Destination buffer 2 corrupted");

                // Compare buffers
                if (!dst1.equals_absolute(dst2, 1e-4))
                {
                    src.dump("src");
                    dst1.dump("dst1");
                    dst2.dump("dst2");
                    printf("index=%d, %.6f vs %.6f\n", dst1.last_diff(), dst1.get_diff(), dst2.get_diff());
                    UTEST_FAIL_MSG("Output of functions for test '%s' differs", label);
                }
            }
        }
    }

    UTEST_MAIN
    {
        #define CALL(generic, func, align) \
            call(#func, align, generic, func)

        IF_ARCH_X86(CALL(generic::add2, sse::add2, 16));
        IF_ARCH_X86(CALL(generic::sub2, sse::sub2, 16));
        IF_ARCH_X86(CALL(generic::rsub2, sse::rsub2, 16));
        IF_ARCH_X86(CALL(generic::mul2, sse::mul2, 16));
        IF_ARCH_X86(CALL(generic::div2, sse::div2, 16));
        IF_ARCH_X86(CALL(generic::rdiv2, sse::rdiv2, 16));
        IF_ARCH_X86(CALL(generic::mod2, sse2::mod2, 16));
        IF_ARCH_X86(CALL(generic::rmod2, sse2::rmod2, 16));

        IF_ARCH_X86(CALL(generic::add2, avx::add2, 32));
        IF_ARCH_X86(CALL(generic::sub2, avx::sub2, 32));
        IF_ARCH_X86(CALL(generic::rsub2, avx::rsub2, 32));
        IF_ARCH_X86(CALL(generic::mul2, avx::mul2, 32));
        IF_ARCH_X86(CALL(generic::div2, avx::div2, 32));
        IF_ARCH_X86(CALL(generic::rdiv2, avx::rdiv2, 32));
        IF_ARCH_X86(CALL(generic::mod2, avx::mod2, 32));
        IF_ARCH_X86(CALL(generic::rmod2, avx::rmod2, 32));
        IF_ARCH_X86(CALL(generic::mod2, avx::mod2_fma3, 32));
        IF_ARCH_X86(CALL(generic::rmod2, avx::rmod2_fma3, 32));

        IF_ARCH_ARM(CALL(generic::add2, neon_d32::add2, 16));
        IF_ARCH_ARM(CALL(generic::sub2, neon_d32::sub2, 16));
        IF_ARCH_ARM(CALL(generic::rsub2, neon_d32::rsub2, 16));
        IF_ARCH_ARM(CALL(generic::mul2, neon_d32::mul2, 16));
        IF_ARCH_ARM(CALL(generic::div2, neon_d32::div2, 16));
        IF_ARCH_ARM(CALL(generic::rdiv2, neon_d32::rdiv2, 16));
        IF_ARCH_ARM(CALL(generic::mod2, neon_d32::mod2, 16));
        IF_ARCH_ARM(CALL(generic::rmod2, neon_d32::rmod2, 16));

        IF_ARCH_AARCH64(CALL(generic::add2, asimd::add2, 16));
        IF_ARCH_AARCH64(CALL(generic::sub2, asimd::sub2, 16));
        IF_ARCH_AARCH64(CALL(generic::rsub2, asimd::rsub2, 16));
        IF_ARCH_AARCH64(CALL(generic::mul2, asimd::mul2, 16));
        IF_ARCH_AARCH64(CALL(generic::div2, asimd::div2, 16));
        IF_ARCH_AARCH64(CALL(generic::rdiv2, asimd::rdiv2, 16));
        IF_ARCH_AARCH64(CALL(generic::mod2, asimd::mod2, 16));
        IF_ARCH_AARCH64(CALL(generic::rmod2, asimd::rmod2, 16));
    }
UTEST_END


