/*
 * angle.cpp
 *
 *  Created on: 31 авг. 2018 г.
 *      Author: sadko
 */

#include <lsp-plug.in/test-fw/utest.h>
#include <lsp-plug.in/test-fw/helpers.h>
#include <lsp-plug.in/dsp/dsp.h>
#include <lsp-plug.in/stdlib/math.h>

namespace lsp
{
    namespace generic
    {
        float calc_angle3d_v2(const dsp::vector3d_t *v1, const dsp::vector3d_t *v2);
        float calc_angle3d_vv(const dsp::vector3d_t *v);
    }

    IF_ARCH_X86(
        namespace sse
        {
            float calc_angle3d_v2(const dsp::vector3d_t *v1, const dsp::vector3d_t *v2);
            float calc_angle3d_vv(const dsp::vector3d_t *v);
        }
    )
}

typedef float (* calc_angle3d_v2_t)(const dsp::vector3d_t *v1, const dsp::vector3d_t *v2);
typedef float (* calc_angle3d_vv_t)(const dsp::vector3d_t *v);

UTEST_BEGIN("dsp.3d", angle)

    void call(const char *label,
            calc_angle3d_v2_t calc_angle3d_v2,
            calc_angle3d_vv_t calc_angle3d_vv
            )
    {
        for (float phi=0.0f; phi<M_PI*2; phi += M_PI*0.125f)
        {
            for (float rho=0.0f; rho<M_PI; rho += M_PI*0.125f)
            {
                dsp::vector3d_t v[2];
                dsp::matrix3d_t m;

                dsp::init_vector_dxyz(&v[0], 1.0f, 0.0f, 0.0f);
                dsp::init_vector(&v[1], &v[0]);
                dsp::init_matrix3d_rotate_z(&m, rho);
                dsp::apply_matrix3d_mv1(&v[1], &m);

                dsp::init_matrix3d_rotate_y(&m, phi);
                dsp::apply_matrix3d_mv1(&v[0], &m);
                dsp::apply_matrix3d_mv1(&v[1], &m);

                float angle = calc_angle3d_v2(&v[0], &v[1]);
                UTEST_ASSERT_MSG(float_equals_relative(angle, cosf(rho)),
                            "calc_angle3d_v2 failed phi=%.3f, rho=%.3f, result=%.3f", phi, rho, angle);

                angle = calc_angle3d_vv(v);
                UTEST_ASSERT_MSG(float_equals_relative(angle, cosf(rho)),
                                        "calc_angle3d_vv failed phi=%.3f, rho=%.3f, result=%.3f", phi, rho, angle);
            }
        }
    }

    UTEST_MAIN
    {
        call("generic_calc_angle",
                generic::calc_angle3d_v2,
                generic::calc_angle3d_vv
            );
        IF_ARCH_X86(
            call("sse_calc_angle",
                    sse::calc_angle3d_v2,
                    sse::calc_angle3d_vv
                )
        );
    }

UTEST_END
