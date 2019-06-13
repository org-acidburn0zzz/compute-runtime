/*
 * Copyright (C) 2017-2019 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#pragma once

#include "core/helpers/ptr_math.h"
#include "runtime/helpers/debug_helpers.h"
#include "runtime/memory_manager/host_ptr_defines.h"
#include "runtime/memory_manager/memory_banks.h"
#include "runtime/memory_manager/memory_constants.h"
#include "runtime/memory_manager/memory_pool.h"
#include "runtime/memory_manager/residency_container.h"
#include "runtime/utilities/idlist.h"
#include "runtime/utilities/stackvec.h"

#include "engine_limits.h"
#include "storage_info.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace NEO {

using osHandle = unsigned int;
enum class HeapIndex : uint32_t;

namespace Sharing {
constexpr auto nonSharedResource = 0u;
}

class Gmm;
class MemoryManager;
struct AllocationProperties;

class GraphicsAllocation : public IDNode<GraphicsAllocation> {
  public:
    enum class AllocationType {
        UNKNOWN = 0,
        BUFFER,
        BUFFER_COMPRESSED,
        BUFFER_HOST_MEMORY,
        COMMAND_BUFFER,
        CONSTANT_SURFACE,
        DEVICE_QUEUE_BUFFER,
        EXTERNAL_HOST_PTR,
        FILL_PATTERN,
        GLOBAL_SURFACE,
        IMAGE,
        INDIRECT_OBJECT_HEAP,
        INSTRUCTION_HEAP,
        INTERNAL_HEAP,
        INTERNAL_HOST_MEMORY,
        KERNEL_ISA,
        LINEAR_STREAM,
        MCS,
        PIPE,
        PREEMPTION,
        PRINTF_SURFACE,
        PRIVATE_SURFACE,
        PROFILING_TAG_BUFFER,
        SCRATCH_SURFACE,
        SHARED_BUFFER,
        SHARED_CONTEXT_IMAGE,
        SHARED_IMAGE,
        SHARED_RESOURCE_COPY,
        SURFACE_STATE_HEAP,
        SVM_CPU,
        SVM_GPU,
        SVM_ZERO_COPY,
        TAG_BUFFER,
        TIMESTAMP_PACKET_TAG_BUFFER,
    };

    virtual ~GraphicsAllocation();
    GraphicsAllocation &operator=(const GraphicsAllocation &) = delete;
    GraphicsAllocation(const GraphicsAllocation &) = delete;

    GraphicsAllocation(AllocationType allocationType, void *cpuPtrIn, uint64_t gpuAddress, uint64_t baseAddress, size_t sizeIn, MemoryPool::Type pool, bool multiOsContextCapable);

    GraphicsAllocation(AllocationType allocationType, void *cpuPtrIn, size_t sizeIn, osHandle sharedHandleIn, MemoryPool::Type pool, bool multiOsContextCapable);

    void *getUnderlyingBuffer() const { return cpuPtr; }
    void *getDriverAllocatedCpuPtr() const { return driverAllocatedCpuPointer; }
    void setDriverAllocatedCpuPtr(void *allocatedCpuPtr) { driverAllocatedCpuPointer = allocatedCpuPtr; }

    void setCpuPtrAndGpuAddress(void *cpuPtr, uint64_t gpuAddress) {
        this->cpuPtr = cpuPtr;
        this->gpuAddress = gpuAddress;
    }
    size_t getUnderlyingBufferSize() const { return size; }
    void setSize(size_t size) { this->size = size; }

    uint64_t getAllocationOffset() const {
        return allocationOffset;
    }
    void setAllocationOffset(uint64_t offset) {
        allocationOffset = offset;
    }

    uint64_t getGpuBaseAddress() const {
        return gpuBaseAddress;
    }
    void setGpuBaseAddress(uint64_t baseAddress) {
        gpuBaseAddress = baseAddress;
    }
    uint64_t getGpuAddress() const {
        DEBUG_BREAK_IF(gpuAddress < gpuBaseAddress);
        return gpuAddress + allocationOffset;
    }
    uint64_t getGpuAddressToPatch() const {
        DEBUG_BREAK_IF(gpuAddress < gpuBaseAddress);
        return gpuAddress + allocationOffset - gpuBaseAddress;
    }

    void lock(void *ptr) { lockedPtr = ptr; }
    void unlock() { lockedPtr = nullptr; }
    bool isLocked() const { return lockedPtr != nullptr; }
    void *getLockedPtr() const { return lockedPtr; }

    bool isCoherent() const { return allocationInfo.flags.coherent; }
    void setCoherent(bool coherentIn) { allocationInfo.flags.coherent = coherentIn; }
    void setEvictable(bool evictable) { allocationInfo.flags.evictable = evictable; }
    bool peekEvictable() const { return allocationInfo.flags.evictable; }
    bool isFlushL3Required() const { return allocationInfo.flags.flushL3Required; }
    void setFlushL3Required(bool flushL3Required) { allocationInfo.flags.flushL3Required = flushL3Required; }
    bool isMultiOsContextCapable() const { return allocationInfo.flags.multiOsContextCapable; }
    bool is32BitAllocation() const { return allocationInfo.flags.is32BitAllocation; }
    void set32BitAllocation(bool is32BitAllocation) { allocationInfo.flags.is32BitAllocation = is32BitAllocation; }

    void setAubWritable(bool writable) { aubInfo.aubWritable = writable; }
    bool isAubWritable() const { return aubInfo.aubWritable; }
    void setTbxWritable(bool writable) { aubInfo.tbxWritable = writable; }
    bool isTbxWritable() const { return aubInfo.tbxWritable; }
    void setAllocDumpable(bool dumpable) { aubInfo.allocDumpable = dumpable; }
    bool isAllocDumpable() const { return aubInfo.allocDumpable; }
    bool isMemObjectsAllocationWithWritableFlags() const { return aubInfo.memObjectsAllocationWithWritableFlags; }
    void setMemObjectsAllocationWithWritableFlags(bool newValue) { aubInfo.memObjectsAllocationWithWritableFlags = newValue; }

    void incReuseCount() { sharingInfo.reuseCount++; }
    void decReuseCount() { sharingInfo.reuseCount--; }
    uint32_t peekReuseCount() const { return sharingInfo.reuseCount; }
    osHandle peekSharedHandle() const { return sharingInfo.sharedHandle; }

    void setAllocationType(AllocationType allocationType);
    AllocationType getAllocationType() const { return allocationType; }

    MemoryPool::Type getMemoryPool() const { return memoryPool; }

    bool isUsed() const { return registeredContextsNum > 0; }
    bool isUsedByManyOsContexts() const { return registeredContextsNum > 1u; }
    bool isUsedByOsContext(uint32_t contextId) const { return objectNotUsed != getTaskCount(contextId); }
    void updateTaskCount(uint32_t newTaskCount, uint32_t contextId);
    uint32_t getTaskCount(uint32_t contextId) const { return usageInfos[contextId].taskCount; }
    void releaseUsageInOsContext(uint32_t contextId) { updateTaskCount(objectNotUsed, contextId); }
    uint32_t getInspectionId(uint32_t contextId) const { return usageInfos[contextId].inspectionId; }
    void setInspectionId(uint32_t newInspectionId, uint32_t contextId) { usageInfos[contextId].inspectionId = newInspectionId; }

    bool isResident(uint32_t contextId) const { return GraphicsAllocation::objectNotResident != getResidencyTaskCount(contextId); }
    void updateResidencyTaskCount(uint32_t newTaskCount, uint32_t contextId) { usageInfos[contextId].residencyTaskCount = newTaskCount; }
    uint32_t getResidencyTaskCount(uint32_t contextId) const { return usageInfos[contextId].residencyTaskCount; }
    void releaseResidencyInOsContext(uint32_t contextId) { updateResidencyTaskCount(objectNotResident, contextId); }
    bool isResidencyTaskCountBelow(uint32_t taskCount, uint32_t contextId) const { return !isResident(contextId) || getResidencyTaskCount(contextId) < taskCount; }

    virtual std::string getAllocationInfoString() const;
    virtual uint64_t peekInternalHandle(MemoryManager *memoryManager) { return 0llu; }

    static bool isCpuAccessRequired(AllocationType allocationType) {
        return allocationType == AllocationType::COMMAND_BUFFER ||
               allocationType == AllocationType::CONSTANT_SURFACE ||
               allocationType == AllocationType::INTERNAL_HEAP ||
               allocationType == AllocationType::LINEAR_STREAM ||
               allocationType == AllocationType::TIMESTAMP_PACKET_TAG_BUFFER;
    }
    void *getReservedAddressPtr() const {
        return this->reservedAddressRangeInfo.addressPtr;
    }
    size_t getReservedAddressSize() const {
        return this->reservedAddressRangeInfo.rangeSize;
    }
    void setReservedAddressRange(void *reserveAddress, size_t size) {
        this->reservedAddressRangeInfo.addressPtr = reserveAddress;
        this->reservedAddressRangeInfo.rangeSize = size;
    }

    Gmm *getDefaultGmm() const {
        return getGmm(0u);
    }
    Gmm *getGmm(uint32_t handleId) const {
        return gmms[handleId];
    }
    void setDefaultGmm(Gmm *gmm) {
        setGmm(gmm, 0u);
    }
    void setGmm(Gmm *gmm, uint32_t handleId) {
        gmms[handleId] = gmm;
    }
    uint32_t getNumHandles() const { return storageInfo.getNumHandles(); }

    OsHandleStorage fragmentsStorage;
    StorageInfo storageInfo = {};

  protected:
    constexpr static uint32_t objectNotResident = std::numeric_limits<uint32_t>::max();
    constexpr static uint32_t objectNotUsed = std::numeric_limits<uint32_t>::max();

    struct UsageInfo {
        uint32_t taskCount = objectNotUsed;
        uint32_t residencyTaskCount = objectNotResident;
        uint32_t inspectionId = 0u;
    };
    struct AubInfo {
        bool aubWritable = true;
        bool tbxWritable = true;
        bool allocDumpable = false;
        bool memObjectsAllocationWithWritableFlags = false;
    };
    struct SharingInfo {
        uint32_t reuseCount = 0;
        osHandle sharedHandle = Sharing::nonSharedResource;
    };
    struct AllocationInfo {
        union {
            struct {
                uint32_t coherent : 1;
                uint32_t evictable : 1;
                uint32_t flushL3Required : 1;
                uint32_t is32BitAllocation : 1;
                uint32_t multiOsContextCapable : 1;
                uint32_t reserved : 27;
            } flags;
            uint32_t allFlags = 0u;
        };
        static_assert(sizeof(AllocationInfo::flags) == sizeof(AllocationInfo::allFlags), "");
        AllocationInfo() {
            flags.coherent = false;
            flags.evictable = true;
            flags.flushL3Required = false;
            flags.is32BitAllocation = false;
            flags.multiOsContextCapable = false;
        }
    };

    struct ReservedAddressRange {
        void *addressPtr = nullptr;
        size_t rangeSize = 0;
    };

    friend class SubmissionAggregator;

    AllocationInfo allocationInfo;
    AubInfo aubInfo;
    SharingInfo sharingInfo;
    ReservedAddressRange reservedAddressRangeInfo;

    uint64_t allocationOffset = 0u;
    uint64_t gpuBaseAddress = 0;
    uint64_t gpuAddress = 0;
    void *driverAllocatedCpuPointer = nullptr;
    size_t size = 0;
    void *cpuPtr = nullptr;
    void *lockedPtr = nullptr;

    MemoryPool::Type memoryPool = MemoryPool::MemoryNull;
    AllocationType allocationType = AllocationType::UNKNOWN;

    std::array<UsageInfo, maxOsContextCount> usageInfos;
    std::atomic<uint32_t> registeredContextsNum{0};
    std::array<Gmm *, maxHandleCount> gmms{};
};
} // namespace NEO
