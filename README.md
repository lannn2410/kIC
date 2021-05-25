This folder includes source code (C++) and dataset ("data" folder) of two kIC applications: Influence maximization and Max Cut. 

To build the C++ code, run the following command on terminal:
```
	g++ -std=c++11 *.cpp -o ksub -DIL_STD -I<source code path> -fopenmp -g
```
After building, to run our code, run:
```	
  ./ksub  -t <application type, 0: IM, 1: MaxCut> # default: 0
		      -a <algorithm type, 0: streaming, 1: greedy, 2: sc-greedy, 3: streamgreedy, 4: degree, 5: dstream, 6: rstream, 7:simplestream, 8: single> # default: 1
		      -b <individual budget> # default: 10
		      -k <in k-submodular> # default: 3
		      -alpha <streaming alpha> # default: 0.1
		      -delta <streaming delta> # default: 1.0
		      -p <number of threads> # default: 10
```
Or:
```
	./ksub 	-h
```
to print help.

The code will return the following result:
	+ Objective value
	+ Number of queries
