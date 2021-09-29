build:
	mpicxx -fopenmp -c static_gcd.c -o static_gcd.o
	nvcc -I./inc -c cudaFunctions.cu -o cudaFunctions.o
	mpicxx -o histogram  static_gcd.o cudaFunctions.o  /usr/local/cuda-9.1/lib64/libcudart_static.a -ldl -lrt -fopenmp

clean:
	rm -f *.o ./histogram

run:
	 mpiexec -n 2 ./histogram <input.txt
run2:
	mpiexec -n 2 ./histogram <input.txt




