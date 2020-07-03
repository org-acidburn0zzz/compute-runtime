/*
 * Copyright (C) 2018-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "shared/source/gmm_helper/gmm.h"

#include "opencl/test/unit_test/os_interface/windows/wddm_fixture.h"
#include "test.h"

namespace NEO {

using WddmTests = WddmTestWithMockGdiDll;

TEST_F(WddmTests, whenCreatingAllocation64kThenDoNotCreateResource) {
    init();

    D3DKMT_HANDLE handle;
    Gmm gmm(executionEnvironment->rootDeviceEnvironments[0]->getGmmClientContext(), nullptr, 20, false, true, true, {});

    EXPECT_TRUE(wddm->createAllocation64k(&gmm, handle));
    auto gdiParam = getMockAllocationFcn();
    EXPECT_EQ(FALSE, gdiParam->Flags.CreateResource);
}

TEST_F(WddmTests, whenInitializingWddmThenSetMinAddressToCorrectValue) {
    constexpr static uintptr_t mockedInternalGpuVaRange = 0x9876u;
    auto gmmMemory = new MockGmmMemoryBase(wddm->rootDeviceEnvironment.getGmmClientContext());
    gmmMemory->overrideInternalGpuVaRangeLimit(mockedInternalGpuVaRange);
    wddm->gmmMemory.reset(gmmMemory);

    ASSERT_EQ(0u, wddm->getWddmMinAddress());
    wddm->init();

    const bool obtainFromGmm = defaultHwInfo->platform.eRenderCoreFamily == IGFX_GEN12LP_CORE;
    const auto expectedMinAddress = obtainFromGmm ? mockedInternalGpuVaRange : windowsMinAddress;
    ASSERT_EQ(expectedMinAddress, wddm->getWddmMinAddress());
}

} // namespace NEO