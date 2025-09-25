//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
// Developed by Minigraph
//
// Author:  James Stanard
//

#pragma once

#include <vector>
#include <queue>
#include <mutex>
#include <stdint.h>
#include "d3d12-subsystem.hpp"

class gs_command_allocator_pool
{
public:
    gs_command_allocator_pool(gs_device_t* device, const D3D12_COMMAND_LIST_TYPE Type);
    ~gs_command_allocator_pool();

    ID3D12CommandAllocator* RequestAllocator(uint64_t CompletedFenceValue);
    void DiscardAllocator(uint64_t FenceValue, ID3D12CommandAllocator* Allocator);

    inline size_t Size() { return m_AllocatorPool.size(); }

private:
    const D3D12_COMMAND_LIST_TYPE m_CommandListType;

    gs_device_t* m_Device;
    std::vector<ID3D12CommandAllocator*> m_AllocatorPool;
    std::queue<std::pair<uint64_t, ID3D12CommandAllocator*>> m_ReadyAllocators;
    std::mutex m_AllocatorMutex;
};
