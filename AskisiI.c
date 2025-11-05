#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

int main(int argc, char **argv) {
	int i, rc, n, rank, p, begin, finish, rem, cnt, pos, receive_cnt;
	int choice, terminate;
	int process_n; //Number of elements every process will deal with
	int process_sum = 0; //The sum that every process calculates by its own
	int process_max = 0;
	int total_sum = 0, m = 0;
	int receive_sum, receive_max, receive_m;
	int *X, *process_X, *D, *process_D;
	float average = 0, process_average;
	float process_var = 0, total_var = 0, receive_var = 0;
	MPI_Status status;


	//Initialization of MPI
	rc = MPI_Init(&argc, &argv);
	if (rc != 0) {
		printf("MPI initialization error\n");
		MPI_Abort(MPI_COMM_WORLD, rc);
	}

	//Calculate number of processes (p) user declares
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	//Gives ranks to every process
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	//Process 0 is the master process of the simultaneous environment
	//Process 0 asks from user the number of elements of vector X and then initializes these elements into X
	while (1) {
		if (rank == 0) {
			//Ask user if he wants to continue or to exit the program
			printf("\n\nMENU\n\n1. Continue\n2. Exit\n\nChoice: ");
			scanf("%d", &choice);

			if (choice == 2) {
				terminate = 0;
				for (i = 1; i < p; i++) {
					MPI_Send(&terminate, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
				}

				printf("\n\n...Programm terminated\n\n");
				MPI_Finalize();
				exit(0);
			}

			printf("Type size of vector X: ");
			scanf("%d", &n);

			X = (int*)malloc(sizeof(int) * n);
			if (!X) {
				printf("Memory allocation error...");
				exit(1);
			}

			for (i = 0; i < n; i++) {
				printf("Type %d element of X: ", i+1);
				scanf("%d", &X[i]);
			}

			//Master process 0 calculates the number of elements every process will deal with
			process_n = n/p;
			rem = n%p;

			//Master process 0 sends process_n elements to other processes
			for (i = 1; i < p; i++) {
				//Master process 0 sends the number of total elements to every other process
				MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

				begin = i * process_n;

				if (i == p-1) {
					cnt = process_n + rem;
				}
				else {
					cnt = process_n;
				}
				MPI_Send(&X[begin], cnt, MPI_INT, i, 0, MPI_COMM_WORLD);


			}


			//a
			//Process 0 calculates its own sum
			process_sum = 0;
			for (i = 0; i < process_n; i++) {
				process_sum += X[i];
			}

			//Master process 0 receives the local sum of every other process and adds it to the total_sum
			total_sum = process_sum;
			for (i = 1; i < p; i++) {
				MPI_Recv(&receive_sum, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
				total_sum += receive_sum;
			}

			average = (float)total_sum/n;


			//b
			//Master calculates its own max
			process_max = X[0];
			for (i = 1; i < process_n; i++) {
				if (X[i] > process_max) {
					process_max = X[i];
				}
			}

			//Master process 0 receives the local maxes of every other process and calculates total m
			m = process_max;
			for (i = 1; i < p; i++) {
				MPI_Recv(&receive_max, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
				if (receive_max > m) {
					m = receive_max;
				}
			}


			//c
			//Master sends average to every other process for the correct calculation of var (question c)
			for (i = 1; i < p; i++) {
				MPI_Send(&average, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD);
			}

			//Master process 0 calculates its own var
			process_var = 0;
			for (i = 0; i < process_n; i++) {
				process_var += pow((X[i] - average), 2);
			}

			//Master process 0 receives the local var of every other process and calculates total_var
			total_var = process_var;
			for (i = 1; i < p; i++) {
				MPI_Recv(&receive_var, 1, MPI_FLOAT, i, 0, MPI_COMM_WORLD, &status);
				total_var += receive_var;
			}
			total_var /= n;


			//d
			D = (int*)malloc(sizeof(int) * n);
			if (!D) {
				printf("Memory allocation error...");
				exit(1);
			}

			process_D = (int*)malloc(sizeof(int) * process_n);
			if (!process_D) {
				printf("Memory allocation error...");
				exit(1);
			}

			//Master sends m to other processes
			for (i = 1; i < p; i++) {
				MPI_Send(&m, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
			}

			//Then master process 0 calculates its elements of di
			for (i = 0; i < process_n; i++) {
				process_D[i] = (X[i]-m) * (X[i]-m);
			}

			pos = process_n;
			for (i = 1; i < p; i++) {
				if (i == p-1) {
					receive_cnt = process_n + rem;
				}
				else {
					receive_cnt = process_n;
				}
				MPI_Recv(&D[pos], receive_cnt, MPI_INT, i, 0, MPI_COMM_WORLD, &status);

				pos += receive_cnt;
			}

			for (i = 0; i < process_n; i++) {
				D[i] = process_D[i];
			}


			//Process 0 prints the elements of vector X
			printf("\nI am process %d and I print vector X as following:\n", rank);
			printf("[");
			for (i = 0; i < n-1; i++) {
				printf("%d, ", X[i]);
			}
			printf("%d]\n", X[n-1]);

			printf("\nVector X average is: %.2f\n\n", average);
			printf("\nVector X max element is: %d\n", m);
			printf("\nVector X var is: %.2f\n", total_var);
			printf("\nI am process %d and I print vector D, where di = (xi-m)^2, as following:\n", rank);
			printf("[");
			for (i = 0; i < n-1; i++) {
				printf("%d, ", D[i]);
			}
			printf("%d]\n", D[n-1]);

			free(process_D);
		}



		//OTHER PROCESSES
		//Other processes will take on to calculate average of vector X elements
		else {
			MPI_Recv(&n, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			//Check if user wants to terminate the program. Other processes will get n = 0 from master and they will also terminate.
			if (n == 0) {
				printf("\nProcess %d terminated...", rank);
				break;
			}

			//Calculates how many elements this process will receive
			process_n = n/p;

			if (rank == p-1) {
				process_n += n%p;
			}
			//Allocates memory depending on the process (if it is the last process then its going to allocate memory for the rest of the elements of vector X too)
			process_X = (int*)malloc(sizeof(int) * (process_n));
			if (!process_X) {
				printf("Memory allocation error...");
				exit(1);
			}


			//a
			//Process receives the sub vector of X, calculates its local sum and sends it to the master
			process_sum = 0;
			MPI_Recv(process_X, process_n, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
			for (i = 0; i < process_n; i++) {
				process_sum += process_X[i];
			}
			MPI_Send(&process_sum, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);


			//b
			//Other processes calculate their local max element and send it to the master
			process_max = process_X[0];
			for (i = 1; i < process_n; i++) {
				if (process_X[i] > process_max) {
					process_max = process_X[i];
				}
			}
			MPI_Send(&process_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);


			//c
			//Every process receives value average from master
			MPI_Recv(&process_average, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD, &status);

			process_var = 0;
			for (i = 0; i < process_n; i++) {
				process_var += pow(process_X[i]-process_average, 2);
			}
			MPI_Send(&process_var, 1, MPI_FLOAT, 0, 0, MPI_COMM_WORLD);


			//d
			//Every process calculates its di
			process_D = (int*)malloc(sizeof(int) * process_n);
			if (!process_D) {
				printf("Memory allocation error...");
				exit(1);
			}

			//Every process receives m
			MPI_Recv(&receive_m, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);

			for (i = 0; i < process_n; i++) {
				process_D[i] = (process_X[i]-receive_m) * (process_X[i]-receive_m);
			}
			MPI_Send(process_D, process_n, MPI_INT, 0, 0, MPI_COMM_WORLD);


			free(process_X);
			free(process_D);
		}

		//Process 0 as the master takes the responsibility to free vector X and vector D
		if (rank == 0) {
			free(X);
			free(D);
		}
	}

	//Finalize MPI
	MPI_Finalize();

	return 0;
}
