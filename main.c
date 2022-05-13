#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include "watchtower.h"
#include "list.h"

void writeWatchTower(FILE *file, dcel_t *dcel, watchtower_t **watchTower, int watchTowerNum);

int main(int argc, char *argv[]) {
        
    int watchTowerNum = 0;
    char *filename = NULL;    
    watchtower_t **watchTower = NULL;
    dcel_t *dcel = NULL;
    
    /* Read watchtowers information */
    filename = argv[1];
    FILE *file1 = fopen(filename, "r");
    assert(file1);
    watchTower = readWatchtower(file1, watchTower, &watchTowerNum);

    /* Constructing initial dcel */
    filename = argv[2];
    FILE *file2 = fopen(filename, "r");
    assert(file2);
    dcel = constructInitialDcel(file2);

    /* Perform split */
    split(dcel);
    
    /* Write to output file and print content */ 
    filename = argv[3];
    FILE *file3 = fopen(filename, "w");
    assert(file3);
    writeWatchTower(file3, dcel, watchTower, watchTowerNum);
    
    freeWatchTower(watchTower, watchTowerNum);
    freeList(dcel);
    fclose(file1);
    fclose(file2);
    fclose(file3); 
         
    return 0;
}

/* Check if watchtower is in face and write the output to output file */
void writeWatchTower(FILE *file, dcel_t *dcel, watchtower_t **watchTower, int watchTowerNum) {

    size_t faces = dcel->facesNum;
    int isInFace, first;
    int facePopulation[faces];
    halfedge_t *tmp = NULL, *start = NULL;

    memset(facePopulation, 0, faces*sizeof(int));
       
    /* Traverse all half-edges in a face to check whether watchtower lies in that face */
    for (int i = 0; i < faces; i++) {
        fprintf(file, "%d\n", i);       
        for (int j = 0; j < watchTowerNum; j++) {            
            isInFace = 1;
            first = 1;
            tmp = dcel->faces[i].halfEdge;
            start = dcel->faces[i].halfEdge;
            while (tmp != start || first) {
                first = 0;
                /* Check watchtower coordinate against each half-edge */
                if (!isOfHalfPlane(tmp, dcel->vertices, watchTower[j]->x, watchTower[j]->y)) {
                    isInFace = 0;
                    break;
                }
                tmp = tmp->next;
            }
            if(isInFace) {
                fprintf(file, "Watchtower ID: %s, Postcode: %s, Population Served: %d, "
                              "Watchtower Point of Contact Name: %s, x: %lf, y: %lf\n", watchTower[j]->ID, 
                               watchTower[j]->postcode, watchTower[j]->populationServed, watchTower[j]->contact, 
                               watchTower[j]->x, watchTower[j]->y);
                facePopulation[i] += (watchTower[j]->populationServed);
            }
        }
    }
            
    /* Write the population served in each face to the file */
    for (int i = 0; i < faces; i++) {
        fprintf(file, "Face %d population served: %d\n", i, facePopulation[i]);
    }
}

