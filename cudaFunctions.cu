#include <cuda_runtime.h>
#include <stdio.h>
#include "cudaFunctions.h"

__global__  void calculateHist(int *arr, int numElements, int* output) {
    int index = blockIdx.x *256 + threadIdx.x;
    __shared__ int hist[256];
    hist[index] = 0;
    
    if (index < numElements )
    {
        atomicAdd(&hist[arr[index]],1);
    }
    __syncthreads();

    // The thread with index 0 will merge the shared memory histogram with the global histogram using atomicAdd()
    if(threadIdx.x==0)
    {
        for(int i=0; i< 256; i++)
        {
            output[i] = 0;
            atomicAdd(&output[i],hist[i]);   
        }
    }
}
int* computeOnGPU(int *data, int numElements) {
    // Error code to check return values for CUDA calls
    cudaError_t err = cudaSuccess;
    size_t size = numElements * sizeof(float);
    size_t outputSize = 256 * sizeof(float);

    // Allocate memory on GPU to copy the data from the host
    int *d_A;
    err = cudaMalloc((void **)&d_A, size);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Allocate memory on GPU to copy the output from the host
    int *d_out;
    err = cudaMalloc((void **)&d_out, outputSize);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to allocate device memory - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Copy data from host to the GPU memory
    err = cudaMemcpy(d_A, data, size, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    // Copy result array histogram from host to the GPU memory
    err = cudaMemcpy(d_out, data, outputSize, cudaMemcpyHostToDevice);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy data from host to device - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    // Launch the Kernel
    int threadsPerBlock = 256;
    int blocksPerGrid = ceil(float(numElements)/ float(threadsPerBlock));
    calculateHist<<<blocksPerGrid, threadsPerBlock>>>(d_A, numElements,d_out);
    err = cudaGetLastError();
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to launch vectorAdd kernel -  %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    // Copy the  result from GPU to the host memory.
    err = cudaMemcpy(data, d_A, size, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy local numbers array from device to host -%s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    int* output = (int*)malloc(sizeof(int)*256);
    for(int i=0 ; i<256; i++)
    {
        output[i]=0;
    }
    // Copy the  result from GPU to the host memory.
    err = cudaMemcpy(output, d_out, outputSize, cudaMemcpyDeviceToHost);
    if (err != cudaSuccess) {
        fprintf(stderr, "Failed to copy output array from device to host -%s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    // Free allocated memory on GPU
    if (cudaFree(d_A) != cudaSuccess) {
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }
    if (cudaFree(d_out) != cudaSuccess) {
        fprintf(stderr, "Failed to free device data - %s\n", cudaGetErrorString(err));
        exit(EXIT_FAILURE);
    }

    return output;
}
