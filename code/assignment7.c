#include "assignment7.h"
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#define SORT_THRESHOLD      40

typedef struct _sortParams {
    char** array;
    int left;
    int right;
} SortParams;

static int maximumThreads;              /* maximum # of threads to be used */

int threadCounter = 0;
pthread_mutex_t mutex;

/* This is an implementation of insert sort, which although it is */
/* n-squared, is faster at sorting short lists than quick sort,   */
/* due to its lack of recursive procedure call overhead.          */

static void insertSort(char** array, int left, int right) {
    int i, j;
    for (i = left + 1; i <= right; i++) {
        char* pivot = array[i];
        j = i - 1;
        while (j >= left && (strcmp(array[j],pivot) > 0)) {
            array[j + 1] = array[j];
            j--;
        }
        array[j + 1] = pivot;
    }
}

/* Recursive quick sort, but with a provision to use */
/* insert sort when the range gets small.            */

static void quickSort(void* p) {
    SortParams* params = (SortParams*) p;
    char** array = params->array;
    int left = params->left;
    int right = params->right;
    int i = left, j = right;

    if (j - i > SORT_THRESHOLD) {           /* if the sort range is substantial, use quick sort */

        int m = (i + j) >> 1;               /* pick pivot as median of         */
        char* temp, *pivot;                 /* first, last and middle elements */
        if (strcmp(array[i],array[m]) > 0) {
            temp = array[i]; array[i] = array[m]; array[m] = temp;
        }
        if (strcmp(array[m],array[j]) > 0) {
            temp = array[m]; array[m] = array[j]; array[j] = temp;
            if (strcmp(array[i],array[m]) > 0) {
                temp = array[i]; array[i] = array[m]; array[m] = temp;
            }
        }
        pivot = array[m];

        for (;;) {
            while (strcmp(array[i],pivot) < 0) i++; /* move i down to first element greater than or equal to pivot */
            while (strcmp(array[j],pivot) > 0) j--; /* move j up to first element less than or equal to pivot      */
            if (i < j) {
                char* temp = array[i];      /* if i and j have not passed each other */
                array[i++] = array[j];      /* swap their respective elements and    */
                array[j--] = temp;          /* advance both i and j                  */
            } else if (i == j) {
                i++; j--;
            } else break;                   /* if i > j, this partitioning is done  */
        }

        bool createLeft = false, createRight = false;

        SortParams first;  first.array = array; first.left = left; first.right = j;
        SortParams second; second.array = array; second.left = i; second.right = right;

        pthread_t threads[2];
        int lock = 1;

        // left thread
        if (first . left < first . right) {
            if (threadCounter < maximumThreads) {
                while (lock != 0) {
                    lock = pthread_mutex_trylock(&mutex);
                }
                if (pthread_create(&threads[0], NULL, (void *) quickSort,
                                   &first) != 0) {
                    write(2, strerror(errno), strlen(strerror(errno)));
                }
                createLeft = true;
                threadCounter += 1;
                pthread_mutex_unlock(&mutex);
            } else
                quickSort(&first);  /* sort the left partition */
        }

        // right thread
        if (second . left < second . right) {
            if (threadCounter < maximumThreads) {
                while (lock != 0) {
                    lock = pthread_mutex_trylock(&mutex);
                }
                if (pthread_create(&threads[1], NULL, (void *) quickSort,
                                   &second) != 0) {
                    write(2, strerror(errno), strlen(strerror(errno)));
                }
                createRight = true;
                threadCounter += 1;
                pthread_mutex_unlock(&mutex);
            } else
                quickSort(&second);/* sort the right partition */
        }

        if (createLeft) {
            pthread_join(threads[0], NULL);
            threadCounter -= 1;
        }
        if (createRight) {
            pthread_join(threads[1], NULL);
            threadCounter -= 1;
        }
    } else insertSort(array,i,j);           /* for a small range use insert sort */
}

/* user interface routine to set the number of threads sortT is permitted to use */

void setSortThreads(int count) {
    maximumThreads = count;
}

/* user callable sort procedure, sorts array of count strings, beginning at address array */

void sortThreaded(char** array, unsigned int count) {
    if (count <= 1) {  // if empty or contain only one
        return;
    }

    SortParams parameters;
    parameters.array = array;
    parameters.left = 0;
    parameters.right = count - 1;

    // when array is shorter than SORT_THRESHOLD from the start
    if (parameters.right < SORT_THRESHOLD) {
        insertSort(array, parameters.left, parameters.right);
        return;
    }

    if (pthread_mutex_init(&mutex, NULL) != 0) {
        write(2, strerror(errno), strlen(strerror(errno)));
    }

    quickSort(&parameters);

    if (pthread_mutex_destroy(&mutex) != 0) {
        write(2, strerror(errno), strlen(strerror(errno)));
    }
}
