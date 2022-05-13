#ifndef WATCHTOWER_H
#define WATCHTOWER_H

    #define STRING_SIZE 129

    typedef struct {
        char ID[STRING_SIZE];
        char postcode[STRING_SIZE];
        char contact[STRING_SIZE];
        int populationServed;
        double x, y;
    } watchtower_t;

    watchtower_t **readWatchtower(FILE *file, watchtower_t **watchTower, int *currentSize);
    void freeWatchTower(watchtower_t **watchTower, int watchTowerNum);

#endif
