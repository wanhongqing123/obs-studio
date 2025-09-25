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
#include "d3d12-commandlist-manager.hpp"
#include "d3d12-pipeline-state.hpp"
#include "d3d12-root-signature.hpp"
#include "d3d12-gpu-buffer.hpp"
#include "d3d12-pixel-buffer.hpp"
#include "d3d12-dynamic-descriptor-heap.hpp"
#include "d3d12-linear-allocator.hpp"
#include "d3d12-command-signature.hpp"
#include <vector>

class gs_graphics_context;
class gs_compute_context;
class gs_upload_buffer;
class gs_readback_buffer;
class gs_command_context;

struct DWParam
{
    DWParam( FLOAT f ) : Float(f) {}
    DWParam( UINT u ) : Uint(u) {}
    DWParam( INT i ) : Int(i) {}

    void operator= ( FLOAT f ) { Float = f; }
    void operator= ( UINT u ) { Uint = u; }
    void operator= ( INT i ) { Int = i; }

    union
    {
        FLOAT Float;
        UINT Uint;
        INT Int;
    };
};

#define VALID_COMPUTE_QUEUE_RESOURCE_STATES \
    ( D3D12_RESOURCE_STATE_UNORDERED_ACCESS \
    | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE \
    | D3D12_RESOURCE_STATE_COPY_DEST \
    | D3D12_RESOURCE_STATE_COPY_SOURCE )

class gs_context_manager
{
public:
	gs_context_manager(void) {}

    gs_command_context* AllocateContext(D3D12_COMMAND_LIST_TYPE Type);
    void FreeContext(gs_command_context*);
    void DestroyAllContexts();

private:
    std::vector<std::unique_ptr<gs_command_context> > sm_ContextPool[4];
    std::queue<gs_command_context*> sm_AvailableContexts[4];
    std::mutex sm_ContextAllocationMutex;
};

struct NonCopyable
{
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable & operator=(const NonCopyable&) = delete;
};

class gs_command_context : NonCopyable
{
    friend gs_context_manager;
private:

	gs_command_context(D3D12_COMMAND_LIST_TYPE Type);

    void Reset( void );

public:

    ~gs_command_context(void);

    static void DestroyAllContexts(void);

    static gs_command_context& Begin(const std::wstring ID = L"");

    // Flush existing commands to the GPU but keep the context alive
    uint64_t Flush( bool WaitForCompletion = false );

    // Flush existing commands and release the current context
    uint64_t Finish( bool WaitForCompletion = false );

    // Prepare to render by reserving a command list and command allocator
    void Initialize(void);

    gs_graphics_context& GetGraphicsContext() {
        ASSERT(m_Type != D3D12_COMMAND_LIST_TYPE_COMPUTE, "Cannot convert async compute context to graphics");
        return reinterpret_cast<gs_graphics_context&>(*this);
    }

    ComputeContext& GetComputeContext() {
        return reinterpret_cast<ComputeContext&>(*this);
    }

    ID3D12GraphicsCommandList* GetCommandList() {
        return m_CommandList;
    }

    void CopyBuffer( gs_gpu_resource& Dest, gs_gpu_resource& Src );
    void CopyBufferRegion(gs_gpu_resource& Dest, size_t DestOffset, gs_gpu_resource& Src, size_t SrcOffset, size_t NumBytes );
    void CopySubresource(gs_gpu_resource& Dest, UINT DestSubIndex, gs_gpu_resource& Src, UINT SrcSubIndex);
    void CopyCounter(gs_gpu_resource& Dest, size_t DestOffset, gs_structured_buffer& Src);
    void CopyTextureRegion(gs_gpu_resource& Dest, UINT x, UINT y, UINT z, gs_gpu_resource& Source, RECT& rect);
    void ResetCounter(gs_structured_buffer& Buf, uint32_t Value = 0);

    // Creates a readback buffer of sufficient size, copies the texture into it,
    // and returns row pitch in bytes.
    uint32_t ReadbackTexture(gs_readback_buffer& DstBuffer, gs_pixel_buffer& SrcBuffer);

    gs_dyn_alloc ReserveUploadMemory(size_t SizeInBytes)
    {
        return m_CpuLinearAllocator.Allocate(SizeInBytes);
    }

    static void InitializeTexture( gs_gpu_resource& Dest, UINT NumSubresources, D3D12_SUBRESOURCE_DATA SubData[] );
    static void InitializeBuffer( gs_gpu_buffer& Dest, const void* Data, size_t NumBytes, size_t DestOffset = 0);
    static void InitializeBuffer( gs_gpu_buffer& Dest, const UploadBuffer& Src, size_t SrcOffset, size_t NumBytes = -1, size_t DestOffset = 0 );
    static void InitializeTextureArraySlice(gs_gpu_resource& Dest, UINT SliceIndex, gs_gpu_resource& Src);

    void WriteBuffer( gs_gpu_resource& Dest, size_t DestOffset, const void* Data, size_t NumBytes );
    void FillBuffer( gs_gpu_resource& Dest, size_t DestOffset, DWParam Value, size_t NumBytes );

    void TransitionResource(gs_gpu_resource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
    void BeginResourceTransition(gs_gpu_resource& Resource, D3D12_RESOURCE_STATES NewState, bool FlushImmediate = false);
    void InsertUAVBarrier(gs_gpu_resource& Resource, bool FlushImmediate = false);
    void InsertAliasBarrier(gs_gpu_resource& Before, gs_gpu_resource& After, bool FlushImmediate = false);
    inline void FlushResourceBarriers(void);

    void InsertTimeStamp( ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx );
    void ResolveTimeStamps( ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries );
    void PIXBeginEvent(const wchar_t* label);
    void PIXEndEvent(void);
    void PIXSetMarker(const wchar_t* label);

    void SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr );
    void SetDescriptorHeaps( UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[] );
    void SetPipelineState( const PSO& PSO );

    void SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op);

protected:

    void BindDescriptorHeaps( void );

    CommandListManager* m_OwningManager;
    ID3D12GraphicsCommandList* m_CommandList;
    ID3D12CommandAllocator* m_CurrentAllocator;

    ID3D12RootSignature* m_CurGraphicsRootSignature;
    ID3D12RootSignature* m_CurComputeRootSignature;
    ID3D12PipelineState* m_CurPipelineState;

    DynamicDescriptorHeap m_DynamicViewDescriptorHeap;		// HEAP_TYPE_CBV_SRV_UAV
    DynamicDescriptorHeap m_DynamicSamplerDescriptorHeap;	// HEAP_TYPE_SAMPLER

    D3D12_RESOURCE_BARRIER m_ResourceBarrierBuffer[16];
    UINT m_NumBarriersToFlush;

    ID3D12DescriptorHeap* m_CurrentDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

    gs_linear_allocator m_CpuLinearAllocator;
    gs_linear_allocator m_GpuLinearAllocator;

    std::wstring m_ID;
    void SetID(const std::wstring& ID) { m_ID = ID; }

    D3D12_COMMAND_LIST_TYPE m_Type;
};

class gs_graphics_context : public gs_command_context
{
public:

    static gs_graphics_context& Begin(const std::wstring& ID = L"")
    {
        return gs_command_context::Begin(ID).GetGraphicsContext();
    }

    void ClearUAV( gs_gpu_buffer& Target );
    void ClearUAV( gs_color_buffer& Target );
    void ClearColor( gs_color_buffer& Target, D3D12_RECT* Rect = nullptr);
    void ClearColor(gs_color_buffer& Target, float Colour[4], D3D12_RECT* Rect = nullptr);
    void ClearDepth( gs_depth_buffer& Target );
    void ClearStencil( gs_depth_buffer& Target );
    void ClearDepthAndStencil( gs_depth_buffer& Target );

    void BeginQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
    void EndQuery(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT HeapIndex);
    void ResolveQueryData(ID3D12QueryHeap* QueryHeap, D3D12_QUERY_TYPE Type, UINT StartIndex, UINT NumQueries, ID3D12Resource* DestinationBuffer, UINT64 DestinationBufferOffset);

    void SetRootSignature( const RootSignature& RootSig );

    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[]);
    void SetRenderTargets(UINT NumRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE RTVs[], D3D12_CPU_DESCRIPTOR_HANDLE DSV);
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV ) { SetRenderTargets(1, &RTV); }
    void SetRenderTarget(D3D12_CPU_DESCRIPTOR_HANDLE RTV, D3D12_CPU_DESCRIPTOR_HANDLE DSV ) { SetRenderTargets(1, &RTV, DSV); }
    void SetDepthStencilTarget(D3D12_CPU_DESCRIPTOR_HANDLE DSV ) { SetRenderTargets(0, nullptr, DSV); }

    void SetViewport( const D3D12_VIEWPORT& vp );
    void SetViewport( FLOAT x, FLOAT y, FLOAT w, FLOAT h, FLOAT minDepth = 0.0f, FLOAT maxDepth = 1.0f );
    void SetScissor( const D3D12_RECT& rect );
    void SetScissor( UINT left, UINT top, UINT right, UINT bottom );
    void SetViewportAndScissor( const D3D12_VIEWPORT& vp, const D3D12_RECT& rect );
    void SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h );
    void SetStencilRef( UINT StencilRef );
    void SetBlendFactor( Color BlendFactor );
    void SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY Topology );

    void SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants );
    void SetConstant( UINT RootIndex, UINT Offset, DWParam Val );
    void SetConstants( UINT RootIndex, DWParam X );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W );
    void SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV );
    void SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData );
    void SetBufferSRV( UINT RootIndex, const gs_gpu_buffer& SRV, UINT64 Offset = 0);
    void SetBufferUAV( UINT RootIndex, const gs_gpu_buffer& UAV, UINT64 Offset = 0);
    void SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle );

    void SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );
    void SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );

    void SetIndexBuffer( const D3D12_INDEX_BUFFER_VIEW& IBView );
    void SetVertexBuffer( UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView );
    void SetVertexBuffers( UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[] );
    void SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VBData );
    void SetDynamicIB( size_t IndexCount, const uint16_t* IBData );
    void SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData);

    void Draw( UINT VertexCount, UINT VertexStartOffset = 0 );
    void DrawIndexed(UINT IndexCount, UINT StartIndexLocation = 0, INT BaseVertexLocation = 0);
    void DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
        UINT StartVertexLocation = 0, UINT StartInstanceLocation = 0);
    void DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
        INT BaseVertexLocation, UINT StartInstanceLocation);
    void DrawIndirect( gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0 );
    void ExecuteIndirect(CommandSignature& CommandSig, gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
        uint32_t MaxCommands = 1, gs_gpu_buffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

private:
};

class gs_compute_context : public gs_command_context
{
public:

    static gs_compute_context& Begin(const std::wstring& ID = L"", bool Async = false);

    void ClearUAV( gs_gpu_buffer& Target );
    void ClearUAV( gs_color_buffer& Target );

    void SetRootSignature( const gs_root_signature& RootSig );

    void SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants );
    void SetConstant( UINT RootIndex, UINT Offset, DWParam Val );
    void SetConstants( UINT RootIndex, DWParam X );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z );
    void SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W );
    void SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV );
    void SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData );
    void SetDynamicSRV( UINT RootIndex, size_t BufferSize, const void* BufferData ); 
    void SetBufferSRV( UINT RootIndex, const gs_gpu_buffer& SRV, UINT64 Offset = 0);
    void SetBufferUAV( UINT RootIndex, const gs_gpu_buffer& UAV, UINT64 Offset = 0);
    void SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle );

    void SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );
    void SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle );
    void SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] );

    void Dispatch( size_t GroupCountX = 1, size_t GroupCountY = 1, size_t GroupCountZ = 1 );
    void Dispatch1D( size_t ThreadCountX, size_t GroupSizeX = 64);
    void Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX = 8, size_t GroupSizeY = 8);
    void Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ );
    void DispatchIndirect( gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentBufferOffset = 0 );
    void ExecuteIndirect(gs_command_signature& CommandSig, gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentStartOffset = 0,
        uint32_t MaxCommands = 1, gs_gpu_buffer* CommandCounterBuffer = nullptr, uint64_t CounterOffset = 0);

private:
};

inline void gs_command_context::FlushResourceBarriers( void )
{
    if (m_NumBarriersToFlush > 0)
    {
        m_CommandList->ResourceBarrier(m_NumBarriersToFlush, m_ResourceBarrierBuffer);
        m_NumBarriersToFlush = 0;
    }
}

inline void gs_graphics_context::SetRootSignature( const RootSignature& RootSig )
{
    if (RootSig.GetSignature() == m_CurGraphicsRootSignature)
        return;

    m_CommandList->SetGraphicsRootSignature(m_CurGraphicsRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseGraphicsRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseGraphicsRootSignature(RootSig);
}

inline void gs_compute_context::SetRootSignature( const RootSignature& RootSig )
{
    if (RootSig.GetSignature() == m_CurComputeRootSignature)
        return;

    m_CommandList->SetComputeRootSignature(m_CurComputeRootSignature = RootSig.GetSignature());

    m_DynamicViewDescriptorHeap.ParseComputeRootSignature(RootSig);
    m_DynamicSamplerDescriptorHeap.ParseComputeRootSignature(RootSig);
}

inline void gs_command_context::SetPipelineState( const PSO& PSO )
{
    ID3D12PipelineState* PipelineState = PSO.GetPipelineStateObject();
    if (PipelineState == m_CurPipelineState)
        return;

    m_CommandList->SetPipelineState(PipelineState);
    m_CurPipelineState = PipelineState;
}

inline void gs_graphics_context::SetViewportAndScissor( UINT x, UINT y, UINT w, UINT h )
{
    SetViewport((float)x, (float)y, (float)w, (float)h);
    SetScissor(x, y, x + w, y + h);
}

inline void gs_graphics_context::SetScissor( UINT left, UINT top, UINT right, UINT bottom )
{
    SetScissor(CD3DX12_RECT(left, top, right, bottom));
}

inline void gs_graphics_context::SetStencilRef( UINT ref )
{
    m_CommandList->OMSetStencilRef( ref );
}

inline void gs_graphics_context::SetBlendFactor( Color BlendFactor )
{
    m_CommandList->OMSetBlendFactor( BlendFactor.GetPtr() );
}

inline void gs_graphics_context::SetPrimitiveTopology( D3D12_PRIMITIVE_TOPOLOGY Topology )
{
    m_CommandList->IASetPrimitiveTopology(Topology);
}

inline void gs_compute_context::SetConstantArray( UINT RootEntry, UINT NumConstants, const void* pConstants )
{
    m_CommandList->SetComputeRoot32BitConstants( RootEntry, NumConstants, pConstants, 0 );
}

inline void gs_compute_context::SetConstant( UINT RootEntry, UINT Offset, DWParam Val )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Val.Uint, Offset );
}

inline void gs_compute_context::SetConstants( UINT RootEntry, DWParam X )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
}

inline void gs_compute_context::SetConstants( UINT RootEntry, DWParam X, DWParam Y )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
}

inline void gs_compute_context::SetConstants( UINT RootEntry, DWParam X, DWParam Y, DWParam Z )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Z.Uint, 2 );
}

inline void gs_compute_context::SetConstants( UINT RootEntry, DWParam X, DWParam Y, DWParam Z, DWParam W )
{
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, X.Uint, 0 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Y.Uint, 1 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, Z.Uint, 2 );
    m_CommandList->SetComputeRoot32BitConstant( RootEntry, W.Uint, 3 );
}

inline void gs_graphics_context::SetConstantArray( UINT RootIndex, UINT NumConstants, const void* pConstants )
{
    m_CommandList->SetGraphicsRoot32BitConstants( RootIndex, NumConstants, pConstants, 0 );
}

inline void gs_graphics_context::SetConstant( UINT RootEntry, UINT Offset, DWParam Val )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootEntry, Val.Uint, Offset );
}

inline void gs_graphics_context::SetConstants( UINT RootIndex, DWParam X )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
}

inline void gs_graphics_context::SetConstants( UINT RootIndex, DWParam X, DWParam Y )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
}

inline void gs_graphics_context::SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Z.Uint, 2 );
}

inline void gs_graphics_context::SetConstants( UINT RootIndex, DWParam X, DWParam Y, DWParam Z, DWParam W )
{
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, X.Uint, 0 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Y.Uint, 1 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, Z.Uint, 2 );
    m_CommandList->SetGraphicsRoot32BitConstant( RootIndex, W.Uint, 3 );
}

inline void gs_compute_context::SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV )
{
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, CBV);
}

inline void gs_graphics_context::SetConstantBuffer( UINT RootIndex, D3D12_GPU_VIRTUAL_ADDRESS CBV )
{
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, CBV);
}

inline void gs_graphics_context::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    //SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
    memcpy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetGraphicsRootConstantBufferView(RootIndex, cb.GpuAddress);
}

inline void gs_compute_context::SetDynamicConstantBufferView( UINT RootIndex, size_t BufferSize, const void* BufferData )
{
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    //SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
    memcpy(cb.DataPtr, BufferData, BufferSize);
    m_CommandList->SetComputeRootConstantBufferView(RootIndex, cb.GpuAddress);
}

inline void gs_graphics_context::SetDynamicVB( UINT Slot, size_t NumVertices, size_t VertexStride, const void* VertexData )
{
    ASSERT(VertexData != nullptr && Math::IsAligned(VertexData, 16));

    size_t BufferSize = Math::AlignUp(NumVertices * VertexStride, 16);
    DynAlloc vb = m_CpuLinearAllocator.Allocate(BufferSize);

    SIMDMemCopy(vb.DataPtr, VertexData, BufferSize >> 4);

    D3D12_VERTEX_BUFFER_VIEW VBView;
    VBView.BufferLocation = vb.GpuAddress;
    VBView.SizeInBytes = (UINT)BufferSize;
    VBView.StrideInBytes = (UINT)VertexStride;

    m_CommandList->IASetVertexBuffers(Slot, 1, &VBView);
}

inline void gs_graphics_context::SetDynamicIB( size_t IndexCount, const uint16_t* IndexData )
{
    ASSERT(IndexData != nullptr && Math::IsAligned(IndexData, 16));

    size_t BufferSize = Math::AlignUp(IndexCount * sizeof(uint16_t), 16);
    DynAlloc ib = m_CpuLinearAllocator.Allocate(BufferSize);

    SIMDMemCopy(ib.DataPtr, IndexData, BufferSize >> 4);

    D3D12_INDEX_BUFFER_VIEW IBView;
    IBView.BufferLocation = ib.GpuAddress;
    IBView.SizeInBytes = (UINT)(IndexCount * sizeof(uint16_t));
    IBView.Format = DXGI_FORMAT_R16_UINT;

    m_CommandList->IASetIndexBuffer(&IBView);
}

inline void gs_graphics_context::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, cb.GpuAddress);
}

inline void gs_compute_context::SetDynamicSRV(UINT RootIndex, size_t BufferSize, const void* BufferData)
{
    ASSERT(BufferData != nullptr && Math::IsAligned(BufferData, 16));
    DynAlloc cb = m_CpuLinearAllocator.Allocate(BufferSize);
    SIMDMemCopy(cb.DataPtr, BufferData, Math::AlignUp(BufferSize, 16) >> 4);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, cb.GpuAddress);
}

inline void gs_graphics_context::SetBufferSRV( UINT RootIndex, const gs_gpu_buffer& SRV, UINT64 Offset)
{
    ASSERT((SRV.m_UsageState & (D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE)) != 0);
    m_CommandList->SetGraphicsRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

inline void gs_compute_context::SetBufferSRV( UINT RootIndex, const gs_gpu_buffer& SRV, UINT64 Offset)
{
    ASSERT((SRV.m_UsageState & D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE) != 0);
    m_CommandList->SetComputeRootShaderResourceView(RootIndex, SRV.GetGpuVirtualAddress() + Offset);
}

inline void gs_graphics_context::SetBufferUAV( UINT RootIndex, const gs_gpu_buffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetGraphicsRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

inline void gs_compute_context::SetBufferUAV( UINT RootIndex, const gs_gpu_buffer& UAV, UINT64 Offset)
{
    ASSERT((UAV.m_UsageState & D3D12_RESOURCE_STATE_UNORDERED_ACCESS) != 0);
    m_CommandList->SetComputeRootUnorderedAccessView(RootIndex, UAV.GetGpuVirtualAddress() + Offset);
}

inline void gs_compute_context::Dispatch( size_t GroupCountX, size_t GroupCountY, size_t GroupCountZ )
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->Dispatch((UINT)GroupCountX, (UINT)GroupCountY, (UINT)GroupCountZ);
}

inline void gs_compute_context::Dispatch1D( size_t ThreadCountX, size_t GroupSizeX )
{
    Dispatch( Math::DivideByMultiple(ThreadCountX, GroupSizeX), 1, 1 );
}

inline void gs_compute_context::Dispatch2D( size_t ThreadCountX, size_t ThreadCountY, size_t GroupSizeX, size_t GroupSizeY )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY), 1);
}

inline void gs_compute_context::Dispatch3D( size_t ThreadCountX, size_t ThreadCountY, size_t ThreadCountZ, size_t GroupSizeX, size_t GroupSizeY, size_t GroupSizeZ )
{
    Dispatch(
        Math::DivideByMultiple(ThreadCountX, GroupSizeX),
        Math::DivideByMultiple(ThreadCountY, GroupSizeY),
        Math::DivideByMultiple(ThreadCountZ, GroupSizeZ));
}

inline void gs_command_context::SetDescriptorHeap( D3D12_DESCRIPTOR_HEAP_TYPE Type, ID3D12DescriptorHeap* HeapPtr )
{
    if (m_CurrentDescriptorHeaps[Type] != HeapPtr)
    {
        m_CurrentDescriptorHeaps[Type] = HeapPtr;
        BindDescriptorHeaps();
    }
}

inline void gs_command_context::SetDescriptorHeaps( UINT HeapCount, D3D12_DESCRIPTOR_HEAP_TYPE Type[], ID3D12DescriptorHeap* HeapPtrs[] )
{
    bool AnyChanged = false;

    for (UINT i = 0; i < HeapCount; ++i)
    {
        if (m_CurrentDescriptorHeaps[Type[i]] != HeapPtrs[i])
        {
            m_CurrentDescriptorHeaps[Type[i]] = HeapPtrs[i];
            AnyChanged = true;
        }
    }

    if (AnyChanged)
        BindDescriptorHeaps();
}

inline void gs_command_context::SetPredication(ID3D12Resource* Buffer, UINT64 BufferOffset, D3D12_PREDICATION_OP Op)
{
    m_CommandList->SetPredication(Buffer, BufferOffset, Op);
}

inline void gs_graphics_context::SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

inline void gs_compute_context::SetDynamicDescriptor( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicDescriptors(RootIndex, Offset, 1, &Handle);
}

inline void gs_graphics_context::SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicViewDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void gs_compute_context::SetDynamicDescriptors( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicViewDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void gs_graphics_context::SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

inline void gs_graphics_context::SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicSamplerDescriptorHeap.SetGraphicsDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void gs_compute_context::SetDynamicSampler( UINT RootIndex, UINT Offset, D3D12_CPU_DESCRIPTOR_HANDLE Handle )
{
    SetDynamicSamplers(RootIndex, Offset, 1, &Handle);
}

inline void gs_compute_context::SetDynamicSamplers( UINT RootIndex, UINT Offset, UINT Count, const D3D12_CPU_DESCRIPTOR_HANDLE Handles[] )
{
    m_DynamicSamplerDescriptorHeap.SetComputeDescriptorHandles(RootIndex, Offset, Count, Handles);
}

inline void gs_graphics_context::SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle )
{
    m_CommandList->SetGraphicsRootDescriptorTable( RootIndex, FirstHandle );
}

inline void gs_compute_context::SetDescriptorTable( UINT RootIndex, D3D12_GPU_DESCRIPTOR_HANDLE FirstHandle )
{
    m_CommandList->SetComputeRootDescriptorTable( RootIndex, FirstHandle );
}

inline void gs_graphics_context::SetIndexBuffer( const D3D12_INDEX_BUFFER_VIEW& IBView )
{
    m_CommandList->IASetIndexBuffer(&IBView);
}

inline void gs_graphics_context::SetVertexBuffer( UINT Slot, const D3D12_VERTEX_BUFFER_VIEW& VBView )
{
    SetVertexBuffers(Slot, 1, &VBView);
}

inline void gs_graphics_context::SetVertexBuffers( UINT StartSlot, UINT Count, const D3D12_VERTEX_BUFFER_VIEW VBViews[] )
{
    m_CommandList->IASetVertexBuffers(StartSlot, Count, VBViews);
}

inline void gs_graphics_context::Draw(UINT VertexCount, UINT VertexStartOffset)
{
    DrawInstanced(VertexCount, 1, VertexStartOffset, 0);
}

inline void gs_graphics_context::DrawIndexed(UINT IndexCount, UINT StartIndexLocation, INT BaseVertexLocation)
{
    DrawIndexedInstanced(IndexCount, 1, StartIndexLocation, BaseVertexLocation, 0);
}

inline void gs_graphics_context::DrawInstanced(UINT VertexCountPerInstance, UINT InstanceCount,
    UINT StartVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawInstanced(VertexCountPerInstance, InstanceCount, StartVertexLocation, StartInstanceLocation);
}

inline void gs_graphics_context::DrawIndexedInstanced(UINT IndexCountPerInstance, UINT InstanceCount, UINT StartIndexLocation,
    INT BaseVertexLocation, UINT StartInstanceLocation)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, StartIndexLocation, BaseVertexLocation, StartInstanceLocation);
}

inline void gs_graphics_context::ExecuteIndirect(CommandSignature& CommandSig,
    gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, gs_gpu_buffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitGraphicsRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

inline void gs_graphics_context::DrawIndirect(gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentBufferOffset)
{
    ExecuteIndirect(Graphics::DrawIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
}

inline void gs_compute_context::ExecuteIndirect(CommandSignature& CommandSig,
    gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentStartOffset,
    uint32_t MaxCommands, gs_gpu_buffer* CommandCounterBuffer, uint64_t CounterOffset)
{
    FlushResourceBarriers();
    m_DynamicViewDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_DynamicSamplerDescriptorHeap.CommitComputeRootDescriptorTables(m_CommandList);
    m_CommandList->ExecuteIndirect(CommandSig.GetSignature(), MaxCommands,
        ArgumentBuffer.GetResource(), ArgumentStartOffset,
        CommandCounterBuffer == nullptr ? nullptr : CommandCounterBuffer->GetResource(), CounterOffset);
}

inline void gs_compute_context::DispatchIndirect( gs_gpu_buffer& ArgumentBuffer, uint64_t ArgumentBufferOffset )
{
    ExecuteIndirect(Graphics::DispatchIndirectCommandSignature, ArgumentBuffer, ArgumentBufferOffset);
}

inline void gs_command_context::CopyBuffer( gs_gpu_resource& Dest, gs_gpu_resource& Src )
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyResource(Dest.GetResource(), Src.GetResource());
}

inline void gs_command_context::CopyBufferRegion( gs_gpu_resource& Dest, size_t DestOffset, gs_gpu_resource& Src, size_t SrcOffset, size_t NumBytes )
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    //TransitionResource(Src, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion( Dest.GetResource(), DestOffset, Src.GetResource(), SrcOffset, NumBytes);
}

inline void gs_command_context::CopyCounter(gs_gpu_resource& Dest, size_t DestOffset, StructuredBuffer& Src)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Src.GetCounterBuffer(), D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();
    m_CommandList->CopyBufferRegion(Dest.GetResource(), DestOffset, Src.GetCounterBuffer().GetResource(), 0, 4);
}

inline void gs_command_context::CopyTextureRegion(gs_gpu_resource& Dest, UINT x, UINT y, UINT z, gs_gpu_resource& Source, RECT& Rect)
{
    TransitionResource(Dest, D3D12_RESOURCE_STATE_COPY_DEST);
    TransitionResource(Source, D3D12_RESOURCE_STATE_COPY_SOURCE);
    FlushResourceBarriers();

    D3D12_TEXTURE_COPY_LOCATION destLoc = CD3DX12_TEXTURE_COPY_LOCATION(Dest.GetResource(), 0);
    D3D12_TEXTURE_COPY_LOCATION srcLoc = CD3DX12_TEXTURE_COPY_LOCATION(Source.GetResource(), 0);

    D3D12_BOX box = {};
    box.back = 1;
    box.left = Rect.left;
    box.right = Rect.right;
    box.top = Rect.top;
    box.bottom = Rect.bottom;

    m_CommandList->CopyTextureRegion(&destLoc, x, y, z, &srcLoc, &box);
}

inline void gs_command_context::ResetCounter(StructuredBuffer& Buf, uint32_t Value )
{
    FillBuffer(Buf.GetCounterBuffer(), 0, Value, sizeof(uint32_t));
    TransitionResource(Buf.GetCounterBuffer(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
}

inline void gs_command_context::InsertTimeStamp(ID3D12QueryHeap* pQueryHeap, uint32_t QueryIdx)
{
    m_CommandList->EndQuery(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, QueryIdx);
}

inline void gs_command_context::ResolveTimeStamps(ID3D12Resource* pReadbackHeap, ID3D12QueryHeap* pQueryHeap, uint32_t NumQueries)
{
    m_CommandList->ResolveQueryData(pQueryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, NumQueries, pReadbackHeap, 0);
}
