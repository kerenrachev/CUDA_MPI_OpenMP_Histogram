build:
	mpicxx -fopenmp -c histogram.c -o histogram.o
	nvcc -I./inc -c cudaFunctions.cu -o cudaFunctions.o
	mpicxx -o histogram  histogram.o cudaFunctions.o  /usr/local/cuda-11.0/lib64/libcudart_static.a -ldl -lrt -fopenmp

clean:
	rm -f *.o ./histogram

run:
	 mpiexec -n 2 ./histogram <input.txt





