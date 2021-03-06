/*
 * Copyright (C) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "level_zero/core/test/unit_tests/mocks/mock_l0_debugger.h"

namespace NEO {

struct ICLFamily;
using GfxFamily = ICLFamily;
} // namespace NEO

namespace L0 {
namespace ult {
static MockDebuggerL0HwPopulateFactory<IGFX_GEN11_CORE, NEO::GfxFamily> mockDebuggerGen11;
}
} // namespace L0