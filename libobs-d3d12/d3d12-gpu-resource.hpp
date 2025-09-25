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

#include "d3d12-util.hpp"

#define D3D12_GPU_VIRTUAL_ADDRESS_NULL      ((D3D12_GPU_VIRTUAL_ADDRESS)0)
#define D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN   ((D3D12_GPU_VIRTUAL_ADDRESS)-1)

class gs_gpu_resource
{
    friend class gs_command_context;
    friend class gs_graphics_context;
    friend class gs_compute_context;

public:
	gs_gpu_resource() :
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_UsageState(D3D12_RESOURCE_STATE_COMMON),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {
    }

	gs_gpu_resource(ID3D12Resource* pResource, D3D12_RESOURCE_STATES CurrentState) :
        m_GpuVirtualAddress(D3D12_GPU_VIRTUAL_ADDRESS_NULL),
        m_pResource(pResource),
        m_UsageState(CurrentState),
        m_TransitioningState((D3D12_RESOURCE_STATES)-1)
    {
    }

    ~gs_gpu_resource() { Destroy(); }

    virtual void Destroy()
    {
        m_pResource = nullptr;
        m_GpuVirtualAddress = D3D12_GPU_VIRTUAL_ADDRESS_NULL;
        ++m_VersionID;
    }

    ID3D12Resource* operator->() { return m_pResource.Get(); } 
    const ID3D12Resource* operator->() const { return m_pResource.Get(); }

    ID3D12Resource* GetResource() { return m_pResource.Get(); } 
    const ID3D12Resource* GetResource() const { return m_pResource.Get(); }

    ID3D12Resource** GetAddressOf() { return &m_pResource; }

    D3D12_GPU_VIRTUAL_ADDRESS GetGpuVirtualAddress() const { return m_GpuVirtualAddress; }

    uint32_t GetVersionID() const { return m_VersionID; }

protected:

    ComPtr<ID3D12Resource> m_pResource;
    D3D12_RESOURCE_STATES m_UsageState;
    D3D12_RESOURCE_STATES m_TransitioningState;
    D3D12_GPU_VIRTUAL_ADDRESS m_GpuVirtualAddress;

    // Used to identify when a resource changes so descriptors can be copied etc.
    uint32_t m_VersionID = 0;
};
