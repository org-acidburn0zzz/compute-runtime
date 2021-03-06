/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "opencl/test/unit_test/fixtures/cl_device_fixture.h"
#include "test.h"

using namespace NEO;

typedef Test<ClDeviceFixture> TigerlakeLpOnlyTest;

HWTEST2_F(TigerlakeLpOnlyTest, shouldPassOnTglLp, IsTGLLP) {
    EXPECT_EQ(IGFX_TIGERLAKE_LP, pDevice->getHardwareInfo().platform.eProductFamily);
}

typedef Test<ClDeviceFixture> Gen12LpOnlyTeset;

GEN12LPTEST_F(Gen12LpOnlyTeset, shouldPassOnGen12) {
    EXPECT_NE(IGFX_GEN9_CORE, pDevice->getRenderCoreFamily());
    EXPECT_NE(IGFX_GEN11_CORE, pDevice->getRenderCoreFamily());
    EXPECT_EQ(IGFX_GEN12LP_CORE, pDevice->getRenderCoreFamily());
}
