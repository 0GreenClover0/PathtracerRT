#pragma once

#include <d3d12.h>

class GPUProfiler {
public:
    GPUProfiler(ID3D12Device* device, ID3D12CommandQueue* commandQueue);
    ~GPUProfiler();

    void BeginQuery(ID3D12GraphicsCommandList* commandList);
    void EndQueryAndResolve(ID3D12GraphicsCommandList* commandList);
    double GetElapsedTime(bool accumulate = false);
    void Reset();

private:
    void Initialize();

    ID3D12Device* device = {};
    ID3D12CommandQueue* commandQueue = {};
    ID3D12QueryHeap* queryHeap = {};
    ID3D12Resource* readbackBuffer = {};
    UINT64 frequency = {};
    double accumulatedTime = 0.0;
};
