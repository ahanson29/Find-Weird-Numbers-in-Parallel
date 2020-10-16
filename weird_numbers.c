#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>
#include <stdbool.h>
#include <mpi.h> 


bool isWeirdNumber(unsigned long long int num, int proc_rank, int proc_count);
bool isSubsetSum(unsigned long long int* set, unsigned long long int n, unsigned long long int sum);
unsigned long long int* getDivisors(unsigned long long int num, unsigned long long int* count, int proc_rank, int proc_count);
unsigned long long int getSum(unsigned long long int *divisors, unsigned long long int count);
void printArray(unsigned long long int* array, int size, int rank);
void printInfo(int rank, unsigned long long int info);

int main(int argc, char *argv[]){

    int rank, p;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    MPI_Comm_size(MPI_COMM_WORLD,&p);

    unsigned long long int weird_number;

    if(rank == 0){
        if(argc >= 2){

            weird_number = (unsigned long long int)atoi(argv[1]);
        }
        else{
            for(unsigned long long int i = 1; i < 10000; i++){
                bool isWeird = isWeirdNumber(i, rank, p);

                if(isWeird){
                    printf("%llu is a weird number!\n", i);
                }
            }
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(&weird_number, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

    bool isWeird = isWeirdNumber(weird_number, rank, p);

    if(rank == 0){
        if(isWeird){
        printf("%llu is a weird number!\n", weird_number);
        }
        else{
            printf("%llu is not a weird number.\n", weird_number);
        }
    }
    
    
    MPI_Finalize();
}

bool isWeirdNumber(unsigned long long int num, int proc_rank, int proc_count){

    unsigned long long int divisorsCount = 0;
    unsigned long long int totalDivisorsCount = 1;

    unsigned long long int* divisorList = getDivisors(num, &divisorsCount, proc_rank, proc_count);
    unsigned long long int* recvDivisors;

    printArray(divisorList, divisorsCount, proc_rank);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Reduce(&divisorsCount, &totalDivisorsCount, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    
    if(proc_rank == 0){
        printf("Rank %d total Divisors: %llu\n", proc_rank, totalDivisorsCount);
    }

    recvDivisors = (unsigned long long int*)malloc(sizeof(unsigned long long int) * totalDivisorsCount);

    MPI_Barrier(MPI_COMM_WORLD);
    if(divisorsCount > 0 && proc_rank != 0){
        for(int i = 0; i < divisorsCount; i++){
            MPI_Send(&divisorList[i], 1, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD);
        }
    }
    else if(proc_rank == 0){
        for(int i = 0; i < divisorsCount; i++){
            recvDivisors[i] = divisorList[i];
        }
        MPI_Status status;
        for(int i = divisorsCount; i < totalDivisorsCount; i++){
            MPI_Recv(&recvDivisors[i], 1, MPI_UNSIGNED_LONG_LONG , MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD , &status);
        }
    }

    unsigned long long int divisorsSum = 0;
    
    if(divisorsCount > 0)
        divisorsSum = getSum(divisorList, divisorsCount);
    
    unsigned long long int globalSum;

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Allreduce(&divisorsSum, &globalSum, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, MPI_COMM_WORLD);

    if(proc_rank == 0)
        printf("Global sum: %llu\n", globalSum);

    if(globalSum <= num){
        return false;
    }

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Bcast(recvDivisors, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

    //18,446,744,073,709,551,615
    //18446744073709551615
    //8000000000
    //9999999999999
    //999999999999

    printArray(recvDivisors, totalDivisorsCount, proc_rank);

    bool isThereSubset = isSubsetSum(recvDivisors, totalDivisorsCount, num);

    free(divisorList);
    free(recvDivisors);

    if(!isThereSubset)
        return true;
    else
        return false;

}

unsigned long long int getSum(unsigned long long int *divisors, unsigned long long int count){
    
    unsigned long long int sum = 0;

    for(unsigned long long int i = 0; i < count; i++){
        sum += divisors[i];
    }

    return sum;
}

bool isSubsetSum(unsigned long long int* set, unsigned long long int n, unsigned long long int sum){ 
    // Base Cases 
    if (sum == 0) 
        return true; 
    if (n == 0) 
        return false; 
  
    // If last element is greater than sum, then ignore it 
    if (set[n - 1] > sum) 
        return isSubsetSum(set, n - 1, sum); 
  
    /* else, check if sum can be obtained by any of the following: 
      (a) including the last element 
      (b) excluding the last element   */
    return isSubsetSum(set, n - 1, sum) || isSubsetSum(set, n - 1, sum - set[n - 1]); 
} 

unsigned long long int* getDivisors(unsigned long long int num, unsigned long long int* count, int rank, int proc_count){

    unsigned long long int* getDivisorsList = (unsigned long long int*)malloc(sizeof(unsigned long long int));

    if(rank == 0){
        getDivisorsList[0] = 1;
        *count = 1;
        for(unsigned long long int i = rank + 2, index = 1; i * i <= num ; i += proc_count){
            if(num % i == 0){
                *count += 1;
                unsigned long long int j = num / i;
                getDivisorsList = (unsigned long long int*)realloc(getDivisorsList, sizeof(unsigned long long int)*(*count));
                getDivisorsList[index++] = i;
                if (i != j){
                    *count += 1;
                    getDivisorsList = (unsigned long long int*)realloc(getDivisorsList, sizeof(unsigned long long int)*(*count));
                    getDivisorsList[index++] = j;
                }
            }
        }
    }
    else{
        for(unsigned long long int i = rank + 2, index = 0; i * i <= num ; i += proc_count){
            if(num % i == 0){
                *count += 1;
                unsigned long long int j = num / i;
                getDivisorsList = (unsigned long long int*)realloc(getDivisorsList, sizeof(unsigned long long int)*(*count));
                getDivisorsList[index++] = i;
                if (i != j){
                    *count += 1;
                    getDivisorsList = (unsigned long long int*)realloc(getDivisorsList, sizeof(unsigned long long int)*(*count));
                    getDivisorsList[index++] = j;
                }
            }
        }
    }

    return getDivisorsList;
}

void printArray(unsigned long long int* array, int size, int rank){
    printf("%d has divisors: ", rank);
    for(int i = 0; i < size; i++){
        printf("%llu ", array[i]);
    }
    putchar('\n');
}

void printInfo(int rank, unsigned long long int info){
    printf("%d has info: %llu\n", rank, info);
}

