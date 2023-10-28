/*
 * Copyright (C) 2023 Linux Studio Plugins Project <https://lsp-plug.in/>
 *           (C) 2023 Vladimir Sadovnikov <sadko4u@gmail.com>
 *
 * This file is part of lsp-dsp-lib
 * Created on: 26 окт. 2023 г.
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

#ifndef PRIVATE_DSP_ARCH_X86_AVX512_PMATH_LRAMP_H_
#define PRIVATE_DSP_ARCH_X86_AVX512_PMATH_LRAMP_H_

#ifndef PRIVATE_DSP_ARCH_X86_AVX512_IMPL
    #error "This header should not be included directly"
#endif /* PRIVATE_DSP_ARCH_X86_AVX512_IMPL */

namespace lsp
{
    namespace avx512
    {
        IF_ARCH_X86(
            static const float lramp_const[] __lsp_aligned64 =
            {
                0.0f,   1.0f, 2.0f,   3.0f,  4.0f,  5.0f,  6.0f,  7.0f,         /* Initial values 0..7 */
                8.0f,   9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f,         /* Initial values 8..15 */
                16.0f, 17.0f, 18.0f, 19.0f, 20.0f, 21.0f, 22.0f, 23.0f,         /* Initial values 16..23 */
                24.0f, 25.0f, 26.0f, 27.0f, 28.0f, 29.0f, 30.0f, 31.0f,         /* Initial values 8..15 */
                LSP_DSP_VEC16(32.0f)                                            /* Step */
            };
        )

        void lramp_set1(float *dst, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fill(dst, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                __ASM_EMIT("xor                 %[off], %[off]")
                __ASM_EMIT("vbroadcastss        %[v1], %%zmm6")                 /* zmm6     = v1 */
                __ASM_EMIT("vbroadcastss        %[delta], %%zmm7")              /* zmm7     = delta */
                __ASM_EMIT("vmovaps             0x00(%[CC]), %%zmm3")           /* zmm3     = i[0] */
                __ASM_EMIT("vmovaps             0x40(%[CC]), %%zmm4")           /* zmm4     = i[1] */
                __ASM_EMIT("vmovaps             0x80(%[CC]), %%zmm5")           /* zmm5     = step */
                /* 32x blocks */
                __ASM_EMIT("sub                 $32, %[count]")
                __ASM_EMIT("jb                  2f")
                __ASM_EMIT("1:")
                __ASM_EMIT("vmulps              %%zmm7, %%zmm3, %%zmm0")        /* zmm0     = i[0]*delta */
                __ASM_EMIT("vmulps              %%zmm7, %%zmm4, %%zmm1")        /* zmm1     = i[1]*delta */
                __ASM_EMIT("vaddps              %%zmm5, %%zmm3, %%zmm3")        /* zmm3     = x[0]' = x[0] + step */
                __ASM_EMIT("vaddps              %%zmm5, %%zmm4, %%zmm4")        /* zmm4     = x[1]' = x[1] + step */
                __ASM_EMIT("vaddps              %%zmm6, %%zmm0, %%zmm0")        /* zmm0     = v1 + i[0]*delta */
                __ASM_EMIT("vaddps              %%zmm6, %%zmm1, %%zmm1")        /* zmm1     = v1 + i[1]*delta */
                __ASM_EMIT("vmovups             %%zmm0, 0x00(%[dst], %[off])")
                __ASM_EMIT("vmovups             %%zmm1, 0x40(%[dst], %[off])")
                __ASM_EMIT("add                 $0x80, %[off]")
                __ASM_EMIT("sub                 $32, %[count]")
                __ASM_EMIT("jae                 1b")
                __ASM_EMIT("2:")
                /* 16x block */
                __ASM_EMIT("add                 $16, %[count]")
                __ASM_EMIT("jl                  4f")
                __ASM_EMIT("vmulps              %%zmm7, %%zmm3, %%zmm0")        /* zmm0     = i[0]*delta */
                __ASM_EMIT("vmovaps             %%zmm4, %%zmm3")                /* zmm3     = x[0]' = x[0] + step */
                __ASM_EMIT("vaddps              %%zmm6, %%zmm0, %%zmm0")        /* zmm0     = v1 + i[0]*delta */
                __ASM_EMIT("vmovups             %%zmm0, 0x00(%[dst], %[off])")
                __ASM_EMIT("sub                 $16, %[count]")
                __ASM_EMIT("add                 $0x40, %[off]")
                __ASM_EMIT("4:")
                /* 8x blocks */
                __ASM_EMIT("add                 $8, %[count]")
                __ASM_EMIT("jl                  4f")
                __ASM_EMIT("vmulps              %%ymm7, %%ymm3, %%ymm0")        /* ymm0     = i[0]*delta */
                __ASM_EMIT("vextractf32x8       $1, %%zmm3, %%ymm3")            /* ymm3     = x[0]' = x[0] + step */
                __ASM_EMIT("vaddps              %%ymm6, %%ymm0, %%ymm0")        /* ymm0     = v1 + i[0]*delta */
                __ASM_EMIT("vmovups             %%ymm0, 0x00(%[dst], %[off])")
                __ASM_EMIT("sub                 $8, %[count]")
                __ASM_EMIT("add                 $0x20, %[off]")
                __ASM_EMIT("4:")
                /* 4x block */
                __ASM_EMIT("add                 $4, %[count]")
                __ASM_EMIT("jl                  6f")
                __ASM_EMIT("vmulps              %%xmm7, %%xmm3, %%xmm0")        /* xmm0     = i[0]*delta */
                __ASM_EMIT("vextractf128        $1, %%ymm3, %%xmm3")            /* xmm3     = x[0]' = x[0] + step */
                __ASM_EMIT("vaddps              %%xmm6, %%xmm0, %%xmm0")        /* xmm0     = v1 + i[0]*delta */
                __ASM_EMIT("vmovups             %%xmm0, 0x00(%[dst], %[off])")
                __ASM_EMIT("sub                 $4, %[count]")
                __ASM_EMIT("add                 $0x10, %[off]")
                __ASM_EMIT("6:")
                /* 1x blocks */
                __ASM_EMIT("add                 $3, %[count]")
                __ASM_EMIT("jl                  8f")
                __ASM_EMIT("vmulps              %%xmm7, %%xmm3, %%xmm0")        /* xmm0     = x[0]*delta */
                __ASM_EMIT("vaddps              %%xmm6, %%xmm0, %%xmm0")        /* xmm0     = v1 + i[0]*delta */
                __ASM_EMIT("7:")
                __ASM_EMIT("vmovss              %%xmm0, 0x00(%[dst], %[off])")
                __ASM_EMIT("vshufps             $0x39, %%xmm0, %%xmm0, %%xmm0") /* shift xmm0 */
                __ASM_EMIT("add                 $0x04, %[off]")
                __ASM_EMIT("dec                 %[count]")
                __ASM_EMIT("jge                 7b")
                __ASM_EMIT("8:")

                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        #define LRAMP_CORE(DST, SRC) \
            __ASM_EMIT("xor                 %[off], %[off]") \
            __ASM_EMIT("vbroadcastss        %[v1], %%zmm6")                 /* zmm6     = v1 */ \
            __ASM_EMIT("vbroadcastss        %[delta], %%zmm7")              /* zmm7     = delta */ \
            __ASM_EMIT("vmovaps             0x00(%[CC]), %%zmm3")           /* zmm3     = i[0] */ \
            __ASM_EMIT("vmovaps             0x40(%[CC]), %%zmm4")           /* zmm4     = i[1] */ \
            __ASM_EMIT("vmovaps             0x80(%[CC]), %%zmm5")           /* zmm5     = step */ \
            /* 32x blocks */ \
            __ASM_EMIT("sub                 $32, %[count]") \
            __ASM_EMIT("jb                  2f") \
            __ASM_EMIT("1:") \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm3, %%zmm0")        /* zmm0     = i[0]*delta */ \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm4, %%zmm1")        /* zmm1     = i[1]*delta */ \
            __ASM_EMIT("vaddps              %%zmm5, %%zmm3, %%zmm3")        /* zmm3     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%zmm5, %%zmm4, %%zmm4")        /* zmm4     = x[1]' = x[1] + step */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm0, %%zmm0")        /* zmm0     = v1 + i[0]*delta */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm1, %%zmm1")        /* zmm1     = v1 + i[1]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" SRC "], %[off]), %%zmm0, %%zmm0")        /* zmm0     = src[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmulps              0x40(%[" SRC "], %[off]), %%zmm1, %%zmm1")        /* zmm1     = src[1] * (v1 + i[1]*delta) */ \
            __ASM_EMIT("vmovups             %%zmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("vmovups             %%zmm1, 0x40(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x80, %[off]") \
            __ASM_EMIT("sub                 $32, %[count]") \
            __ASM_EMIT("jae                 1b") \
            __ASM_EMIT("2:") \
            /* 16x blocks */ \
            __ASM_EMIT("add                 $16, %[count]") \
            __ASM_EMIT("jl                  4f") \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm3, %%zmm0")        /* zmm0     = i[0]*delta */ \
            __ASM_EMIT("vmovaps             %%zmm4, %%zmm3")                /* zmm3     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm0, %%zmm0")        /* zmm0     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" SRC "], %[off]), %%zmm0, %%zmm0")        /* zmm0     = src[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%zmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("sub                 $16, %[count]") \
            __ASM_EMIT("add                 $0x40, %[off]") \
            __ASM_EMIT("4:") \
            /* 8x blocks */ \
            __ASM_EMIT("add                 $8, %[count]") \
            __ASM_EMIT("jl                  6f") \
            __ASM_EMIT("vmulps              %%ymm7, %%ymm3, %%ymm0")        /* ymm0     = i[0]*delta */ \
            __ASM_EMIT("vextractf32x8       $1, %%zmm3, %%ymm3")            /* ymm3     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%ymm6, %%ymm0, %%ymm0")        /* ymm0     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" SRC "], %[off]), %%ymm0, %%ymm0")        /* ymm0     = src[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%ymm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("sub                 $8, %[count]") \
            __ASM_EMIT("add                 $0x20, %[off]") \
            __ASM_EMIT("6:") \
            /* 4x block */ \
            __ASM_EMIT("add                 $4, %[count]") \
            __ASM_EMIT("jl                  8f") \
            __ASM_EMIT("vmulps              %%xmm7, %%xmm3, %%xmm0")        /* xmm0     = i[0]*delta */ \
            __ASM_EMIT("vextractf128        $1, %%ymm3, %%xmm3")            /* xmm3     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%xmm6, %%xmm0, %%xmm0")        /* xmm0     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" SRC "], %[off]), %%xmm0, %%xmm0")        /* ymm0     = src[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%xmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("sub                 $4, %[count]") \
            __ASM_EMIT("add                 $0x10, %[off]") \
            __ASM_EMIT("8:") \
            /* 1x blocks */ \
            __ASM_EMIT("add                 $3, %[count]") \
            __ASM_EMIT("jl                  10f") \
            __ASM_EMIT("9:") \
            __ASM_EMIT("vmulss              %%xmm7, %%xmm3, %%xmm0")        /* xmm0     = x[0]*delta */ \
            __ASM_EMIT("vshufps             $0x39, %%xmm3, %%xmm3, %%xmm3") /* shift xmm3 */ \
            __ASM_EMIT("vaddss              %%xmm6, %%xmm0, %%xmm0")        /* xmm0     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulss              0x00(%[" SRC "], %[off]), %%xmm0, %%xmm0")        /* ymm0     = src[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovss              %%xmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x04, %[off]") \
            __ASM_EMIT("dec                 %[count]") \
            __ASM_EMIT("jge                 9b") \
            __ASM_EMIT("10:")


        void lramp1(float *dst, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::mul_k2(dst, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_CORE("dst", "dst")
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::mul_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_CORE("dst", "src")
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        #undef LRAMP_CORE

        #define OP_DSEL(a, b)   a
        #define OP_RSEL(a, b)   b

        #define LRAMP_OP3_CORE(DST, A, B, OP, SEL) \
            __ASM_EMIT("xor                 %[off], %[off]") \
            __ASM_EMIT("vbroadcastss        %[v1], %%zmm6")                             /* zmm6     = v1 */ \
            __ASM_EMIT("vbroadcastss        %[delta], %%zmm7")                          /* zmm7     = delta */ \
            __ASM_EMIT("vmovaps             0x00(%[CC]), %%zmm4")                       /* zmm4     = i[0] */ \
            __ASM_EMIT("vmovaps             0x40(%[CC]), %%zmm5")                       /* zmm5     = i[1] */ \
            /* 32x blocks */ \
            __ASM_EMIT("sub                 $32, %[count]") \
            __ASM_EMIT("jb                  2f") \
            __ASM_EMIT("1:") \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm4, %%zmm2")                    /* zmm2     = i[0]*delta */ \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm5, %%zmm3")                    /* zmm3     = i[1]*delta */ \
            __ASM_EMIT("vmovups             0x00(%[" A "], %[off]), %%zmm0")            /* zmm0     = a[0] */ \
            __ASM_EMIT("vmovups             0x40(%[" A "], %[off]), %%zmm1")            /* zmm1     = a[1] */ \
            __ASM_EMIT("vaddps              0x80(%[CC]), %%zmm4, %%zmm4")               /* zmm4     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              0x80(%[CC]), %%zmm5, %%zmm5")               /* zmm5     = x[1]' = x[1] + step */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm2, %%zmm2")                    /* zmm2     = v1 + i[0]*delta */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm3, %%zmm3")                    /* zmm3     = v1 + i[1]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" B "], %[off]), %%zmm2, %%zmm2")    /* zmm0     = b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmulps              0x40(%[" B "], %[off]), %%zmm3, %%zmm3")    /* zmm1     = b[1] * (v1 + i[1]*delta) */ \
            __ASM_EMIT(OP "ps "             SEL("%%zmm2, %%zmm0, %%zmm0", "%%zmm0, %%zmm2, %%zmm0")) /* zmm0     = a[0] OP b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT(OP "ps "             SEL("%%zmm3, %%zmm1, %%zmm1", "%%zmm1, %%zmm3, %%zmm1")) /* zmm1     = a[1] OP b[1] * (v1 + i[1]*delta) */ \
            __ASM_EMIT("vmovups             %%zmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("vmovups             %%zmm1, 0x40(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x80, %[off]") \
            __ASM_EMIT("sub                 $32, %[count]") \
            __ASM_EMIT("jae                 1b") \
            __ASM_EMIT("2:") \
            /* 16x blocks */ \
            __ASM_EMIT("add                 $16, %[count]") \
            __ASM_EMIT("jl                  4f") \
            __ASM_EMIT("1:") \
            __ASM_EMIT("vmulps              %%zmm7, %%zmm4, %%zmm2")                    /* zmm2     = i[0]*delta */ \
            __ASM_EMIT("vmovups             0x00(%[" A "], %[off]), %%zmm0")            /* zmm0     = a[0] */ \
            __ASM_EMIT("vmovaps             %%zmm5, %%zmm4")                            /* zmm4     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%zmm6, %%zmm2, %%zmm2")                    /* zmm2     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" B "], %[off]), %%zmm2, %%zmm2")    /* zmm0     = b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT(OP "ps "             SEL("%%zmm2, %%zmm0, %%zmm0", "%%zmm0, %%zmm2, %%zmm0")) /* zmm0     = a[0] OP b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%zmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x40, %[off]") \
            __ASM_EMIT("sub                 $16, %[count]") \
            __ASM_EMIT("4:") \
            /* 8x blocks */ \
            __ASM_EMIT("add                 $8, %[count]") \
            __ASM_EMIT("jl                  6f") \
            __ASM_EMIT("vmulps              %%ymm7, %%ymm4, %%ymm2")                    /* ymm2     = i[0]*delta */ \
            __ASM_EMIT("vmovups             0x00(%[" A "], %[off]), %%ymm0")            /* ymm0     = a[0] */ \
            __ASM_EMIT("vextractf32x8       $1, %%zmm4, %%ymm4")                        /* ymm4     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%ymm6, %%ymm2, %%ymm2")                    /* zmm2     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" B "], %[off]), %%ymm2, %%ymm2")    /* ymm0     = b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT(OP "ps "             SEL("%%ymm2, %%ymm0, %%ymm0", "%%ymm0, %%ymm2, %%ymm0")) /* ymm0     = a[0] OP b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%ymm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x20, %[off]") \
            __ASM_EMIT("sub                 $8, %[count]") \
            __ASM_EMIT("6:") \
            /* 4x block */ \
            __ASM_EMIT("add                 $4, %[count]") \
            __ASM_EMIT("jl                  8f") \
            __ASM_EMIT("vmulps              %%xmm7, %%xmm4, %%xmm2")                    /* xmm2     = i[0]*delta */ \
            __ASM_EMIT("vmovups             0x00(%[" A "], %[off]), %%xmm0")            /* xmm0     = a[0] */ \
            __ASM_EMIT("vextractf128        $1, %%ymm4, %%xmm4")                        /* xmm4     = x[0]' = x[0] + step */ \
            __ASM_EMIT("vaddps              %%xmm6, %%xmm2, %%xmm2")                    /* xmm2     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulps              0x00(%[" B "], %[off]), %%xmm2, %%xmm2")    /* xmm0     = b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT(OP "ps "             SEL("%%xmm2, %%xmm0, %%xmm0", "%%xmm0, %%xmm2, %%xmm0")) /* xmm0     = a[0] OP b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovups             %%xmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x10, %[off]") \
            __ASM_EMIT("sub                 $4, %[count]") \
            __ASM_EMIT("8:") \
            /* 1x blocks */ \
            __ASM_EMIT("add                 $3, %[count]") \
            __ASM_EMIT("jl                  10f") \
            __ASM_EMIT("9:") \
            __ASM_EMIT("vmulss              %%xmm7, %%xmm4, %%xmm2")                    /* xmm2     = i[0]*delta */ \
            __ASM_EMIT("vmovss              0x00(%[" A "], %[off]), %%xmm0")            /* xmm0     = a[0] */ \
            __ASM_EMIT("vshufps             $0x39, %%xmm4, %%xmm4, %%xmm4")             /* shift xmm4 */ \
            __ASM_EMIT("vaddss              %%xmm6, %%xmm2, %%xmm2")                    /* xmm2     = v1 + i[0]*delta */ \
            __ASM_EMIT("vmulss              0x00(%[" B "], %[off]), %%xmm2, %%xmm2")    /* xmm0     = b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT(OP "ss "             SEL("%%xmm2, %%xmm0, %%xmm0", "%%xmm0, %%xmm2, %%xmm0")) /* xmm0     = a[0] OP b[0] * (v1 + i[0]*delta) */ \
            __ASM_EMIT("vmovss              %%xmm0, 0x00(%[" DST "], %[off])") \
            __ASM_EMIT("add                 $0x04, %[off]") \
            __ASM_EMIT("dec                 %[count]") \
            __ASM_EMIT("jge                 9b") \
            __ASM_EMIT("10:")

        void lramp_add2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmadd_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vadd", OP_DSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_sub2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmsub_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vsub", OP_DSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_rsub2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmrsub_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vsub", OP_RSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_mul2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmmul_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vmul", OP_DSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_div2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmdiv_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vdiv", OP_DSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_rdiv2(float *dst, const float *src, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmrdiv_k3(dst, src, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "dst", "src", "vdiv", OP_RSEL)
                : [count] "+r" (count), [off] "=&r" (off)
                : [dst] "r" (dst), [src] "r" (src),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_add3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmadd_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vadd", OP_DSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_sub3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmsub_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vsub", OP_DSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                  : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_rsub3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmrsub_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vsub", OP_RSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                  : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_mul3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmmul_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vmul", OP_DSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                  : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_div3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmdiv_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vdiv", OP_DSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                  : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        void lramp_rdiv3(float *dst, const float *a, const float *b, float v1, float v2, size_t count)
        {
            float delta = v2 - v1;
            if (delta == 0.0f)
            {
                dsp::fmrdiv_k4(dst, a, b, v1, count);
                return;
            }
            if (count == 0)
                return;

            delta /= count;
            IF_ARCH_X86( size_t off );
            ARCH_X86_ASM(
                LRAMP_OP3_CORE("dst", "a", "b", "vdiv", OP_RSEL)
                : [off] "=&r" (off),
                  __IF_32([count] "+g" (count))
                  __IF_64([count] "+r" (count))
                  : [dst] "r" (dst), [a] "r" (a), [b] "r" (b),
                  [CC] "r" (lramp_const),
                  [delta] "m" (delta),
                  [v1] "m" (v1)
                : "cc", "memory",
                  "xmm0", "xmm1", "xmm2", "xmm3",
                  "xmm4", "xmm5", "xmm6", "xmm7"
            );
        }

        #undef LRAMP_OP3_CORE
        #undef OP_DSEL
        #undef OP_RSEL

    } /* namespace avx512 */
} /* namespace lsp */




#endif /* PRIVATE_DSP_ARCH_X86_AVX512_IMPL */
