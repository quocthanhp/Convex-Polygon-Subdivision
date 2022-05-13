#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "watchtower.h"
#include "list.h"

#define BUFFERSIZE 513
#define WATCHTOWER 1

/* Read information of wacthtowers from input file */
watchtower_t **readWatchtower(FILE *file, watchtower_t **watchTower, int *currentSize) {

    char *line = NULL;
    size_t lineBufferLength = BUFFERSIZE;
    int maxSize = WATCHTOWER;

    watchTower = (watchtower_t **) malloc(WATCHTOWER * sizeof(*watchTower));
    assert(watchTower);
    
    /* Skip the header */
    getline(&line, &lineBufferLength, file);
    free(line);
    line = NULL;

    /* Read the rest */
    while (getline(&line, &lineBufferLength, file) > 0) {
        if (*currentSize == maxSize) {
            maxSize *= 2;
            watchTower = realloc(watchTower, maxSize * sizeof(*watchTower));
            assert(watchTower);
        } 
        watchTower[*currentSize] = (watchtower_t *) malloc(sizeof(watchtower_t));
        assert(watchTower[*currentSize]);
        sscanf(line, "%[^,],%[^,],%d,%[^,],%lf,%lf\n", 
        watchTower[*currentSize]->ID, watchTower[*currentSize]->postcode, 
        &(watchTower[*currentSize]->populationServed), watchTower[*currentSize]->contact, 
        &(watchTower[*currentSize]->x), &(watchTower[*currentSize]->y));
        (*currentSize)++;   
    }

    free(line);
    
    return watchTower;   
}

/* Free watchtower */
void freeWatchTower(watchtower_t **watchTower, int watchTowerNum) {
    
    for (int i = 0; i < watchTowerNum; i++) {
        free(watchTower[i]);
    }
    free(watchTower);
}
