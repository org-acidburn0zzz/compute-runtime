/*
 * Copyright (C) 2019-2020 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once
#include "shared/source/memory_manager/memory_operations_handler.h"

namespace NEO {

class GraphicsAllocation;

class MockMemoryOperationsHandler : public MemoryOperationsHandler {
  public:
    MockMemoryOperationsHandler() {}
    MemoryOperationsStatus makeResident(Device *device, ArrayRef<GraphicsAllocation *> gfxAllocations) override { return MemoryOperationsStatus::UNSUPPORTED; }
    MemoryOperationsStatus evict(Device *device, GraphicsAllocation &gfxAllocation) override { return MemoryOperationsStatus::UNSUPPORTED; }
    MemoryOperationsStatus isResident(Device *device, GraphicsAllocation &gfxAllocation) override { return MemoryOperationsStatus::UNSUPPORTED; }
};
} // namespace NEO
