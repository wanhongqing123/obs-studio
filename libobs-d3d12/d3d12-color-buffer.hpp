/******************************************************************************
    Copyright (C) 2023 by Lain Bailey <lain@obsproject.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

#pragma once

#include "d3d12-util.hpp"
#include "d3d12-pixel-buffer.hpp"
#include "d3d12-gpu-buffer.hpp"

class ColorBuffer : public PixelBuffer
{
public:
    ColorBuffer( Color ClearColor = Color(0.0f, 0.0f, 0.0f, 0.0f)  )
        : m_ClearColor(ClearColor), m_NumMipMaps(0), m_FragmentCount(1), m_SampleCount(1)
    {
        m_RTVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        m_SRVHandle.ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
        for (int i = 0; i < _countof(m_UAVHandle); ++i)
            m_UAVHandle[i].ptr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN;
    }

    // Create a color buffer from a swap chain buffer.  Unordered access is restricted.
    void CreateFromSwapChain( const std::wstring& Name, ID3D12Resource* BaseResource );

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.)
    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
    
    // Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
    // this functions the same as Create() without a video address.
    void Create(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t NumMips,
        DXGI_FORMAT Format, EsramAllocator& Allocator);

    // Create a color buffer.  If an address is supplied, memory will not be allocated.
    // The vmem address allows you to alias buffers (which can be especially useful for
    // reusing ESRAM across a frame.)
    void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
        DXGI_FORMAT Format, D3D12_GPU_VIRTUAL_ADDRESS VidMemPtr = D3D12_GPU_VIRTUAL_ADDRESS_UNKNOWN);
    
    // Create a color buffer.  Memory will be allocated in ESRAM (on Xbox One).  On Windows,
    // this functions the same as Create() without a video address.
    void CreateArray(const std::wstring& Name, uint32_t Width, uint32_t Height, uint32_t ArrayCount,
        DXGI_FORMAT Format, EsramAllocator& Allocator);

    // Get pre-created CPU-visible descriptor handles
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetSRV(void) const { return m_SRVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetRTV(void) const { return m_RTVHandle; }
    const D3D12_CPU_DESCRIPTOR_HANDLE& GetUAV(void) const { return m_UAVHandle[0]; }

    void SetClearColor( Color ClearColor ) { m_ClearColor = ClearColor; }

    void SetMsaaMode( uint32_t NumColorSamples, uint32_t NumCoverageSamples )
    {
        ASSERT(NumCoverageSamples >= NumColorSamples);
        m_FragmentCount = NumColorSamples;
        m_SampleCount = NumCoverageSamples;
    }

    Color GetClearColor(void) const { return m_ClearColor; }

    // This will work for all texture sizes, but it's recommended for speed and quality
    // that you use dimensions with powers of two (but not necessarily square.)  Pass
    // 0 for ArrayCount to reserve space for mips at creation time.
    void GenerateMipMaps(CommandContext& Context);

protected:

    D3D12_RESOURCE_FLAGS CombineResourceFlags( void ) const
    {
        D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

        if (Flags == D3D12_RESOURCE_FLAG_NONE && m_FragmentCount == 1)
            Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

        return D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | Flags;
    }

    // Compute the number of texture levels needed to reduce to 1x1.  This uses
    // _BitScanReverse to find the highest set bit.  Each dimension reduces by
    // half and truncates bits.  The dimension 256 (0x100) has 9 mip levels, same
    // as the dimension 511 (0x1FF).
    static inline uint32_t ComputeNumMips(uint32_t Width, uint32_t Height)
    {
        uint32_t HighBit;
        _BitScanReverse((unsigned long*)&HighBit, Width | Height);
        return HighBit + 1;
    }

    void CreateDerivedViews(ID3D12Device* Device, DXGI_FORMAT Format, uint32_t ArraySize, uint32_t NumMips = 1);

    Color m_ClearColor;
    D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_RTVHandle;
    D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle[12];
    uint32_t m_NumMipMaps; // number of texture sublevels
    uint32_t m_FragmentCount;
    uint32_t m_SampleCount;
};
