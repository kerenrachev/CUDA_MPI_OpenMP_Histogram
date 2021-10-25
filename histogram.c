#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>
#include <unistd.h>
#include <cstring>
//#include "/usr/lib/openmpi/include/mpi.h"
#include <mpi.h>
#include "cudaFunctions.h"

#define ROOT 0
#define NUMBERS_FILE "./input.txt"
typedef struct {
    int* hist;
}histStruct;
int calculateHistogram(int *argc, char **argv[]);
int auclideanAlgorithm(int a, int b);

int main(int argc, char *argv[])
{

    if (calculateHistogram(&argc, &argv) < 0)
        return -1;

    return 0;
}

int calculateHistogram(int *argc, char **argv[])
{
    int my_rank, num_procs, numOfNumbers, local_numOfNumbers;
    double t0, t1;
    FILE *calcResult;
    FILE *data;
    int *arr;
    int *histogram;
    int *local_arr;
    int *local_histogram;

    MPI_Init(argc, argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // ROOT proccess is reading the number of numbers, and all the numbers from text file input.txt
    if (my_rank == ROOT)
    {
        data = fopen(NUMBERS_FILE, "r");
        // Reading the first integer in the text file which is the number of numbers
        fscanf(data,"%d", &numOfNumbers);
        printf("NUM OF NUMBERS: %d\n",numOfNumbers);
        arr = (int *)malloc(sizeof(int) * numOfNumbers);
        for (int i = 0; i < numOfNumbers ; i ++)
        {
            int number;
            fscanf(data,"%d", &number);

            arr[i] = number;
        }

        local_numOfNumbers = (numOfNumbers) / num_procs;
    }
    // Sending local number of numbers to all other proccesses
    MPI_Bcast(&local_numOfNumbers, 1, MPI_INTEGER, ROOT, MPI_COMM_WORLD);

    local_arr = (int *)malloc(sizeof(int) * local_numOfNumbers);
    memset(local_arr, 0, sizeof(int) * local_numOfNumbers);
    local_histogram = (int *)malloc(sizeof(int) * 256);
    memset(local_histogram, 0, sizeof(int) * 256);

    if (my_rank == ROOT)
    {
        histogram = (int *)malloc(sizeof(int) * 256);
	memset(histogram, 0, sizeof(int) * 256);
    }
    // Sending pieces of the array
    MPI_Scatter(arr, local_numOfNumbers, MPI_INT, local_arr, local_numOfNumbers, MPI_INT, ROOT, MPI_COMM_WORLD);

    t0 = MPI_Wtime();


    // ROOT proccess calculating histogram with OPENMP with reduction
    if(my_rank == ROOT)
    {
        // Using reduction to merge and calculate the private histogram arrays
        int privateHist[256] = {0};
        #pragma omp parallel for shared(local_arr) reduction(+: privateHist)
        for(int i = 0; i< local_numOfNumbers; i++)
        {
           privateHist[local_arr[i]]++;
        }
        // Using parallel for to copy the result into the dynamic local hist array.
        #pragma omp parallel for
        for (int i = 0; i < 256; i ++)
        {
            local_histogram[i] +=privateHist[i];
        }
    }

    // Second proccess calculating histogram with OPENMP and CUDA
    if(my_rank != ROOT )
    {
        int* local_histogramForOMP = (int*)malloc(sizeof(int)*256);
	memset(local_histogramForOMP, 0, sizeof(int)*256);
        int* local_histogramForCUDA = (int*)malloc(sizeof(int)*256);
	memset(local_histogramForCUDA, 0, sizeof(int)*256);
        
        // Half is calculated using OpenMp
        histStruct* shared_hists_array= (histStruct*)malloc(omp_get_max_threads() * sizeof(histStruct));
        for( int i=0; i< omp_get_max_threads(); i++)
        {
             shared_hists_array[i].hist = (int*)malloc(256*sizeof(int));
	     memset(shared_hists_array[i].hist, 0, sizeof(int)*256);
        }
        
        #pragma omp parallel
        {
            #pragma omp for  // local_numOfNumbers/2 beacuse we want to calculate only half using OpenMP and the other with CUDA
            for (int i=0 ;i<local_numOfNumbers/2; i++){
                shared_hists_array[omp_get_thread_num()].hist[local_arr[i]]++; 
            }            
        }
        // Merging the calculated hists
        #pragma omp parallel for 
        for( int i=0 ; i<256; i++)
            for (int j=0; j< omp_get_max_threads(); j++)
                local_histogramForOMP[i] += shared_hists_array[j].hist[i];
        
        // Second half is calculated using CUDA

        //Allocating an array for the other half 
        int sizeOfArray = local_numOfNumbers - local_numOfNumbers/2;
        int* new_arr = (int*)malloc(sizeof(int)*sizeOfArray);
        for(int i=local_numOfNumbers/2,j=0; i<local_numOfNumbers;i++,j++)
        {
            new_arr[j] = local_arr[i];  
		}

        local_histogramForCUDA= computeOnGPU(new_arr,sizeOfArray);
        // Merging both iteratively
        for( int i=0; i<256; i++)
        {
           local_histogram[i] = local_histogramForCUDA[i]+local_histogramForOMP[i];  
		}

        
    }

    // Sending the local histogram of the second process to the ROOT proccess
    if( my_rank != ROOT )
    {
        MPI_Send(local_histogram,256,MPI_INT,ROOT,0,MPI_COMM_WORLD);
    }
    if( my_rank == ROOT)
    {
        int *rec_buff = (int*)malloc(256 * sizeof(int));
        MPI_Recv(rec_buff,256,MPI_INT,MPI_ANY_SOURCE,MPI_ANY_TAG,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        // calculating the final Histogram
        for( int i=0 ;i < 256; i++)
        {
            histogram[i] = local_histogram[i]+rec_buff[i];
        }
    }
    // Validating if all elements in the array has been calculated, if not, ROOT proccess will deal with the last element.
    if (my_rank == ROOT)
    {
        if (numOfNumbers%2!=0)
        {
            histogram[arr[sizeof(arr)]]++;
        }
    }

    // Displaying time
    t1 = MPI_Wtime();
    double time = t1 - t0;
    if (my_rank == ROOT)
        printf("The time it took to calculate is %f seconds\n", time);


    // Printing histogram with the ROOT proccess
    if (my_rank == ROOT)
    {
        printf("The histogram result is:\n");
        for (int i = 0; i < 256; i ++)
        {
            if(histogram[i]!=0)
                printf("%d:     %d \n",i, histogram[i]);
        }
        fclose(data);
        free(arr);
        free(histogram);
    }
    free(local_arr);
    free(local_histogram);
    MPI_Finalize();
}

