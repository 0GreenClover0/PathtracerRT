#include "GPUProfiler.h"

#include <cassert>
#include <string>
#include <wrl.h>

using Microsoft::WRL::ComPtr;

GPUProfiler::GPUProfiler(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
    : device(device), commandQueue(commandQueue), queryHeap(nullptr), readbackBuffer(nullptr), frequency(0)
{
    Initialize();
}

GPUProfiler::~GPUProfiler()
{
    if (queryHeap) queryHeap->Release();
    if (readbackBuffer) readbackBuffer->Release();
}

void GPUProfiler::Initialize()
{
    // Use ComPtr internally for easier resource management
    ComPtr<ID3D12QueryHeap> queryHeapComPtr;
    ComPtr<ID3D12Resource> readbackBufferComPtr;

    // Create query heap
    D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
    queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
    queryHeapDesc.Count = 2; // Two queries: start and end
    queryHeapDesc.NodeMask = 0;

    HRESULT hr = device->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&queryHeapComPtr));
    assert(SUCCEEDED(hr));

    queryHeap = queryHeapComPtr.Detach();

    // Create readback buffer
    D3D12_RESOURCE_DESC bufferDesc = {};
    bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    bufferDesc.Width = sizeof(UINT64) * 2; // Space for two timestamps
    bufferDesc.Height = 1;
    bufferDesc.DepthOrArraySize = 1;
    bufferDesc.MipLevels = 1;
    bufferDesc.SampleDesc.Count = 1;
    bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

    D3D12_HEAP_PROPERTIES heapProps = {};
    heapProps.Type = D3D12_HEAP_TYPE_READBACK;

    hr = device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &bufferDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(&readbackBufferComPtr));
    assert(SUCCEEDED(hr));

    readbackBuffer = readbackBufferComPtr.Detach();

    // Get GPU timestamp frequency
    hr = commandQueue->GetTimestampFrequency(&frequency);
    assert(SUCCEEDED(hr));
}

void GPUProfiler::BeginQuery(ID3D12GraphicsCommandList* commandList)
{
    commandList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0); // Start timestamp
}

void GPUProfiler::EndQueryAndResolve(ID3D12GraphicsCommandList* commandList)
{
    commandList->EndQuery(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 1); // End timestamp
    commandList->ResolveQueryData(queryHeap, D3D12_QUERY_TYPE_TIMESTAMP, 0, 2, readbackBuffer, 0);
}

double GPUProfiler::GetElapsedTime(bool accumulate)
{
    // Map and read back the timestamp data
    UINT64* mappedData = nullptr;
    D3D12_RANGE readRange = { 0, sizeof(UINT64) * 2 };
    readbackBuffer->Map(0, &readRange, reinterpret_cast<void**>(&mappedData));

    UINT64 startTimestamp = mappedData[0];
    UINT64 endTimestamp = mappedData[1];
    readbackBuffer->Unmap(0, nullptr);

    // Calculate elapsed time in milliseconds

    double time = static_cast<double>(endTimestamp - startTimestamp) / static_cast<double>(frequency) * 1000.0;

    if (accumulate)
    {
        accumulatedTime += time;
    }

    return time;
}

void GPUProfiler::Reset()
{
    double const averageTime = accumulatedTime / 10000.0;
    MessageBox(NULL, std::to_wstring(averageTime).c_str(), L"GPU Profiler", MB_OK);
    accumulatedTime = 0.0;
}
