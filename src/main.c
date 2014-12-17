

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>
#include "H.h"


/**
 * Typical errors:
 * -10 File not found
 * -11 out of memory
 **/

/**
 * methods
 **/


// William globals
// These two should match
const unsigned int MAX_THREADS = 6;
const int SIGNED_MAX_THREADS = 6;
const unsigned int NUM_COMPARISONS = 10;
sem_t threads;
sem_t mutex;
sem_t print;
sem_t matrixH;
sem_t save;
sem_t waiting;
sem_t increment;


RC findRefutations(int ** relation, H *matrixH, int Y, int numAttributes, int numTuples);
RC checkRefutationInH(H *matrixH, word_t refutation);

void printRelation(int **relation, unsigned int numAttributes, unsigned int numTuples);

// William functions
RC refMulti(int ** relation, H *matrixH, int Y, int numAttributes, int numTuples);
void * findRefMulti(void * args);
void checkRefMulti(void * argsThread);
void * checkRefHMulti(void * argsThread);

/*

Find a refutation
Check it in H by:
ANDing and XORing

if 0, set entry(X)->max = false
else XOR again with every entry and set entry(Xi's)->max = false
add to H

Save H for that attribute and repeat for next attribute

*/

// Helper function
int min(int x, int y)
{
	if (x < y)
		return x;
	return y;
}

// When the program is working Profile it with different datasets, different number of threads, make it run fastest.
// Then work on locking the cache. Profile tool probably unnecessary for now, just use the start and end times.

RC refMulti(int ** relation, H * matrixH, int Y, int numAttributes, int numTuples)
{
	// Make this an array of threads, 3, 4, etc. Make sure to kill all threads at very end, possibly join on each thread.
	pthread_t thread;
	// Make an array of semaphors to signal each thread when it needs to start again,
	// make sure to pass updated from and to lines for each thread
	// saving will be different.
	argumentsM *argsM = (argumentsM *) malloc(sizeof(argumentsM));
	argsM->relation = relation;
	argsM->Y = Y;
	argsM->numAttributes = numAttributes;
	argsM->numTuples = numTuples;
	argsM->refutations = createH(numAttributes, Y);

	argumentsH *argsH = (argumentsH *) malloc(sizeof(argumentsH));
	argsH->matrixH = matrixH;

	int numRows = (NUM_COMPARISONS/numTuples) + MAX_THREADS;
	int numRowsThread = numRows/MAX_THREADS + 1;

	int i, fromRow, toRow;
	i = 0;
	while (i <= numTuples) 
	{
		fromRow = i;
		i += numRows;
		toRow = min(i, numTuples);
		while (fromRow < toRow)
		{
			argsM->from = fromRow;
			fromRow += numRowsThread;
			argsM->to = min(fromRow, toRow);
			//sem_post(&signal); signal the thread
			// 
			sem_wait(&threads);
			// Save threads in my new array of threads. 
			int error = pthread_create(&thread, NULL, findRefMulti, argsM);
			if (error) {
				printf("ERROR BEING CREATED! number: %d \n", error);
				exit(1);
			}
			sem_wait(&save);			
		}
		
		// Waiting for threads to finish
		sem_wait(&waiting);

		// Find maximal refutations
		unsigned int n = 0;
		unsigned int j = 0;
		unsigned int m = argsM->refutations->numRefutations;
		// this needs to keep going if H has an empty slot
		while (n < m && j < argsM->refutations->size)
		{
			if (argsM->refutations->matrix[j]) {
				argsH->X = argsM->refutations->matrix[j];
				argsH->from = 0;
				argsH->to = argsH->matrixH->size;
				argsH->maximal = 1;
				argsH->numRefToStart = argsH->matrixH->numRefutations;
				argsH->numRefCompared = 0;
				
				checkRefMulti(argsH);
				
				if (argsH->maximal == 1) {
					addHi(argsH->matrixH, argsH->X);
				}
				sem_wait(&mutex);
				removeHi(argsM->refutations, j);
				sem_post(&mutex);
				n++;
			}
			j++;
		}
	}
	
	printH(matrixH);
	return 0;
}

void * findRefMulti(void * argsThread)
{
	argumentsM * args = (argumentsM *) argsThread;
	int i, X;
	unsigned int j = args->from;
	unsigned int k = args->to;
	sem_post(&save);

	unsigned int n;
	for (n = j; n < k; n++) 
	{
		for (i = n + 1; i < args->numTuples; i++) 
		{
			if (args->relation[n][args->Y] != args->relation[i][args->Y]) 
			{
				// new refutation found
				word_t refutation = 0x00000000;
				word_t mask = 1;
				for (X = args->numAttributes - 1; X >= 0; X--) 
				{
					if (args->Y != X && args->relation[n][X] == args->relation[i][X]) 
					{
						mask = 1;
						mask <<= X;
						refutation |= mask;
					}
				}
				if (!refutation)
				{
					continue;
				}
				sem_wait(&print);
				printf("refutation found: [%d][%d] ",n,i);
				printbits(refutation, args->numAttributes);
				printf("  %d",refutation);
				printf("\n");
				sem_post(&print);

				sem_wait(&mutex);
				addHi(args->refutations, refutation);
				sem_post(&mutex);
			}
		}
	}
	sem_post(&threads); 
	int value = 0;
	sem_getvalue(&threads, &value);
	if (value == SIGNED_MAX_THREADS){	
		sem_post(&waiting);
	}

	sem_wait
	// return 0;
	//pthread_exit(NULL);
}

void checkRefMulti(void * argsThread) 
{
	argumentsH * args = (argumentsH *) argsThread;
	pthread_t thread;
	if (args->matrixH->numRefutations == 0)
	{ 	// There is no refutations to compare with, add it
		args->maximal = 1;
	}
	else
	{
		unsigned int i, j, k;
		i = 0;
		k = args->matrixH->size;
		j = k/MAX_THREADS;
		while (j * i < k)
		{
			args->from = j * i;
			i++;
			args->to = min(j * i, k);

			sem_wait(&threads);
			// These threads will also need to be added to an array.
			int error = pthread_create(&thread, NULL, checkRefHMulti, args);
			if (error) {
				printf("ERROR BEING CREATED! number: %d \n", error);
				exit(1);
			}
			sem_wait(&save);
		}

		// Wait for threads to finish
		sem_wait(&waiting);
	}
}

void * checkRefHMulti(void * argsThread)
{
	argumentsH * args = (argumentsH *) argsThread;
	word_t temp, result;
	unsigned int i = args->from;
	unsigned int j = args->to;
	sem_post(&save);
	while (i < j && args->numRefCompared < args->numRefToStart) 
	{	
		if (!args->matrixH->matrix[i])
		{	
			i++;
			continue;
		}
		// temp = X and Xi
		temp = (args->X & args->matrixH->matrix[i]);
		//result = temp XOR X
		result = (temp ^ args->X);

		if (!result || args->maximal == 0) 
		{ 	// if t XOR X = 0
			// X is subset of Xi
			// throw away X
			args->maximal = 0;
			sem_post(&threads);
			int value = 0;
			sem_getvalue(&threads, &value);
			if (value == SIGNED_MAX_THREADS) {
				sem_post(&waiting);
			}
			return 0;
			//pthread_exit(NULL);
		} 
		else 
		{	// temp XOR Xi
			result = (temp ^ args->matrixH->matrix[i]);
			if (!result)
			{	// if temp XOR Xi == 0
				// Xi is subset of X
				// remove all Xi that are a subset of X
				sem_wait(&matrixH);
				removeHi(args->matrixH, i);
				sem_post(&matrixH);
			}
		}
		sem_wait(&increment);
		args->numRefCompared++;
		sem_post(&increment);
		i++;
	}

	sem_post(&threads);
	int value = 0;
	sem_getvalue(&threads, &value);
	if (value == SIGNED_MAX_THREADS) {
		sem_post(&waiting);
	}
	return 0;
	//pthread_exit(NULL);
}



/**
 * sequential method to find refutations for one determined attribute
 **/
RC findRefutations(int ** relation, H *matrixH, int Y, int numAttributes, int numTuples) {		
	int i, j, X;
	// quadratic search of refutations
	for (i = 0; i < numTuples; i++) {
		for (j = i + 1; j < numTuples; j++) {

			if (relation[i][Y] != relation[j][Y]) {
				// new refutation found
				word_t refutation = 0x00000000;
				word_t mask = 1;
				for (X = numAttributes - 1; X >= 0; X--) {
					if (Y!=X &&relation[i][X] == relation[j][X]) {
						mask = 1;
						mask<<=X;
						refutation|=mask;
					}
				}
				if (!refutation){
					continue;
        			}
				printf("refutation found: [%d][%d] ",i,j);
				printbits(refutation, numAttributes);
				printf("  %d",refutation);
				printf("\n");
				
				checkRefutationInH(matrixH, refutation);
			}
		}
	}
	//show H
	printH(matrixH);
	return 0;
}

/**
 * sequential method to check maximality of a refutation vs H
 **/
RC checkRefutationInH(H *matrixH, word_t X) {
	RC rc=0;
	unsigned int i, j=0;
	if (matrixH->numRefutations == 0){ // there is no refutations to compare with, add it
		if((rc=addHi(matrixH, X))!=0)
			return rc;
	}else {
		word_t t, result;
		for (i = 0; i < matrixH->size && j<matrixH->numRefutations; i++) {
			if (matrixH->matrix[i]==0)
				continue;
			// t= X and Xi
			t=(X&matrixH->matrix[i]);
			//result= t XOR X
			result= (t^X);
			if (!result) { // if t XOR X = 0
				// X is subset of Xi
				// throw away X
				return 0;
			} else {
				//t XOR Xi
				result= (t^matrixH->matrix[i]);
				if (!result) { // if t XOR Xi == 0
					// Xi is subset of X
					// remove all of Xi which are subset of X
					//printf("Xi is a subset of X, removing");
					//printf("\n");
					removeHi(matrixH, i);
				}
				else
				{
					j++;
				}
			}
		}
		addHi(matrixH, X);
	}
	//Idea: compact array of Hs
	return rc;
}

RC readDataSet(int ** relation, char nameRelation[], unsigned int numAttributes,
		unsigned int numTuples, char * delimiter) {
	char fileName[100];
	sprintf(fileName, "../datasets/%s", nameRelation);
	char line[numAttributes * 6];
	unsigned int col = 0, row = 0;

	printf("\nOpening %s", fileName);

	FILE * file = fopen(fileName, "r");

	if (file == NULL) {
		return -10;
	}

	col = 0, row = 0;
	while (row < numTuples && numAttributes > 0
			&& fgets(line, sizeof(line), file)) {
		char * ds = strdup(line);

		relation[row][col++] = atoi(strtok(ds, delimiter));

		while (col < numAttributes) {
			relation[row][col] = (int) atoi(strtok(NULL, delimiter));
			col++;
		}
		col = 0;
		row++;

	}
	fclose(file);
	return 0;

}

void printRelation(int **relation, unsigned int numAttributes,
		unsigned int numTuples) {
	unsigned int row, col;
	printf("\nDataset:\n");
	for (row = 0; row < numTuples; row++) {
		for (col = 0; col < numAttributes; col++)
			printf("%d ", relation[row][col]);
		printf("\n");
	}
}

/**
 * main method
 **/
int main(int argc, char* argv[]) {
	unsigned int numAttributes, numTuples;
	char delimiter[] = ",\n"; //IMPORTANT: set the delimiter from the dataset file
	RC rc;

	if (argc != 5) {
		printf("Error, you must use $./main numAttributes numTuples typeAlgorithm{seq=0,parall=1} datasetName");
		return -1;
	}

	numAttributes = atoi(argv[1]);
	numTuples = atoi(argv[2]);
	type_algorithm = atoi(argv[3]);
	char *nameRelation = argv[4];

	printf("Dataset: %s", nameRelation);
	printf(" size=%dx%d\n", numTuples, numAttributes);

	//request enough space for the dataset
	int ** relation = (int **) malloc(sizeof(int*) * numTuples);
	unsigned int i;
	for (i = 0; i < numTuples; i++) {
		relation[i] = (int *) malloc(sizeof(int) * numAttributes);
	}

	rc = readDataSet(relation, nameRelation, numAttributes, numTuples,
			delimiter);

	if (rc < 0) {
		printf("Error loading the dataset: %d", rc);
		return -1;
	}

	printRelation(relation, numAttributes, numTuples);

	//for each attribute, find its refutations and build H
	int k = 0;
	
	if (type_algorithm == SEQUENTIAL_ALGORITHM) {
		for (k = 0 /*numAttributes - 1*/; k >= 0; k--) {

			H * matrixH = createH(numAttributes, k);
			rc = findRefutations(relation, matrixH, k, numAttributes,
					numTuples);

			destroyH(matrixH);

		}
	} else {
		sem_init(&threads, 0, MAX_THREADS);
		sem_init(&print, 0, 1);
		sem_init(&matrixH, 0, 1);
		sem_init(&mutex, 0, 1);
		sem_init(&increment, 0, 1);
		sem_init(&waiting, 0, 0);
		sem_init(&save, 0, 0);
		for (k = 0 /*numAttributes - 1*/; k >= 0; k--) {
			H * matrixH = createH(numAttributes, k);
			rc = refMulti(relation, matrixH, k, numAttributes, numTuples);
			destroyH(matrixH);
		}
	}
	pthread_exit(NULL);
	return rc;
}


//void * checkRefutationInHMulti(void *argsThread);
//RC findRefutationsMulti(int ** relation, H *matrixH, int Y, int numAttributes, int numTuples);

/*

//multi-core method to find refutations for one determined attribute
RC findRefutationsMulti(int ** relation, H *matrixH, int Y, int numAttributes,
		int numTuples) {

	int i, j, X;
	//cuadratic search of refutations
	for (i = 0; i < numTuples; i++) {
		for (j = i + 1; j < numTuples; j++) {

			if (relation[i][Y] != relation[j][Y]) {
				//new refutation found
				word_t refutation = 0x00000000;
				word_t mask = 1;
				for (X = numAttributes - 1; X >= 0; X--) {
					if (Y!=X &&relation[i][X] == relation[j][X]) {
						mask = 1;
						mask<<=X;
						refutation|=mask;
					}
				}
				if (!refutation){
					continue;
				}

				printf("refutation found: [%d][%d] ",i,j);
				printbits(refutation, numAttributes);
				printf("  %d",refutation);
				printf("\n");


				//call multi-threading function
				//PENDIENT..
				// the idea is to split H and define 'from' and 'to' indexes for each thread
				argumentsH *args = (argumentsH *) malloc(sizeof(argumentsH));
				args->matrixH = matrixH;
				args->from = 0;
				args->to = matrixH->numRefutations;
				args->X = refutation;
				pthread_t thread;
				
				pthread_create(&thread, NULL, checkRefutationInHMulti, args);

				//show H
				printH(matrixH);

			}

		}

	}

	return 0;
}

//multi-core method to check maximality of a refutation vs H


void *checkRefutationInHMulti(void * argsThread) {
	argumentsH *args = (argumentsH *) argsThread;
	unsigned int i,j=0;
	int rc=0;
	if (args->matrixH->numRefutations == 0){ // there is no refutations to compare with, add it
		if((rc=addHi(args->matrixH, args->X))!=0)
			return NULL;
	}else {
		word_t t, result;
		for (i = 0; i < args->matrixH->size && j<args->matrixH->numRefutations; i++) {
			if (!args->matrixH->matrix[i])
				continue;

			// t= X and Xi
			t=(args->X&args->matrixH->matrix[i]);

			//result= t XOR X
			result= (t^args->X);

			if (!result) { // if t XOR X = 0
				// X is subset of Xi
				// throw away X
				return 0;
			} else {
				//t XOR Xi
				result= (t^args->matrixH->matrix[i]);
				if (!result) { // if t XOR Xi == 0
					// Xi is subset of X
					// remove all of Xi which are subset of X
					removeHi(args->matrixH, i);
				}

			}

		}
		addHi(args->matrixH, args->X);
	}


	pthread_exit(NULL);

}*/













