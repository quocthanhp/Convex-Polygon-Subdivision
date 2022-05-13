#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "list.h"

#define VERTICES 3
#define FACE 1
#define EXTRA_VERTICES 2
#define EXTRA_EDGES 3
#define EXTRA_FACE 1
#define EPSILON 0.000001d

/* Read vertices from input file */
vertex_t *readVertices(FILE *file, int *currentSize) {

    vertex_t *vertices;
    double xCoord, yCoord;
    int maxVertices = VERTICES;

    vertices = (vertex_t *) malloc(VERTICES * sizeof(vertex_t));
    assert(vertices);
    
    while (fscanf(file, "%lf %lf", &xCoord, &yCoord) > 0) {
        if (*currentSize == maxVertices) {
            maxVertices *= 2;
            vertices = realloc(vertices, maxVertices * sizeof(vertex_t));
            assert(vertices);
        }
        vertices[*currentSize].x = xCoord;
        vertices[*currentSize].y = yCoord;
        (*currentSize)++;
    }

    return vertices;
}

/* Construct initial doubly connected edge list with vertices, (half)edges and face read from input files */
dcel_t *constructInitialDcel(FILE *file) {

    dcel_t *dcel = (dcel_t *) malloc(sizeof(dcel_t));
    assert(dcel);

    /* Read vertices */
    dcel->verticesNum = 0;
    dcel->vertices = readVertices(file, &(dcel->verticesNum));

    /* Create edge and its corresponding half-edge */
    dcel->edgesNum = dcel->verticesNum;
    dcel->edges = (edge_t *) malloc((dcel->edgesNum) * sizeof(edge_t));
    for (int i = 0; i < dcel->edgesNum; i++) {
        dcel->edges[i].halfEdge = (halfedge_t *)malloc(sizeof(halfedge_t)); 
        assert(dcel->edges[i].halfEdge);
        dcel->edges[i].halfEdge->startVertexIdx = i;
        dcel->edges[i].halfEdge->endVertexIdx = (i + 1) % (dcel->verticesNum);
        dcel->edges[i].halfEdge->faceIdx = 0;
        dcel->edges[i].halfEdge->edgeIdx = i;
        dcel->edges[i].halfEdge->twin = NULL;
        dcel->edges[i].halfEdge->next = NULL;
        dcel->edges[i].halfEdge->prev = NULL;         
    }

    /* Link each halfEdge to its next and previous halfEdges */
    for (int i = 0; i < dcel->verticesNum; i++) {
        dcel->edges[i].halfEdge->next = dcel->edges[(i + 1) % (dcel->verticesNum)].halfEdge;
        dcel->edges[i].halfEdge->prev = dcel->edges[(i + ((dcel->verticesNum) - 1)) % (dcel->verticesNum)].halfEdge;
    }

    /* Intitially, there is only face 0 and it will point to the first halfedge */
    dcel->faces = (face_t *) malloc (FACE * sizeof(face_t));
    assert(dcel->faces);
    dcel->faces[0].halfEdge = dcel->edges[0].halfEdge;
    dcel->facesNum = FACE;

    return dcel;
}

/* Calculate midpoint of an edge with given end vertex and start vertex */
vertex_t midPoint(vertex_t start, vertex_t end) {
    vertex_t mid;
    mid.x = (start.x + end.x) / 2;
    mid.y = (start.y + end.y) / 2;
    return mid;
}

/* Check if a given point is of half-plane of an edge */ 
int isOfHalfPlane(halfedge_t *halfEdge, vertex_t *vertices, double targetX, double targetY) {

    int isOfHalfPlane = 0;
    double xStart = vertices[halfEdge->startVertexIdx].x, yStart = vertices[halfEdge->startVertexIdx].y;
    double xEnd = vertices[halfEdge->endVertexIdx].x, yEnd = vertices[halfEdge->endVertexIdx].y;

    if ((fabs(xStart - xEnd) < EPSILON) && yStart < yEnd && targetX > xStart) {
        isOfHalfPlane = 1;
    } 
    else if (xStart < xEnd) {
        double gradient = (yEnd - yStart)/(xEnd - xStart);
        double intercept = yEnd - gradient * xEnd;
        double yPredicted = gradient * targetX + intercept;
        double yR = targetY - yPredicted;
        if (yR <= 0) {
            isOfHalfPlane = 1;
        }
    }
    else if ((fabs(xStart - xEnd) < EPSILON) && (yStart > yEnd || (fabs(yStart - yEnd) < EPSILON)) && targetX <= xStart) {
        isOfHalfPlane = 1;
    } 
    else if (xStart > xEnd) {
        double gradient = (yEnd - yStart)/(xEnd - xStart);
        double intercept = yEnd - gradient * xEnd;
        double yPredicted = gradient * targetX + intercept;
        double yR = targetY - yPredicted;
        if (yR >= 0) {
            isOfHalfPlane = 1;
        }
    }

    return isOfHalfPlane;
}

/* Perform split on polygon with split read from file */
void split(dcel_t *dcel) {

    int startSplit, endSplit, splitFace, newStartVertexIdx, newEndVertexIdx, newEdgeIdx, newFaceIdx, 
        oldEndOfStart, oldStartOfEnd, isAdjacent, oldStartOfStartTwin, oldEndOfEndTwin;
    vertex_t midStartHalfEdge, midEndHalfEdge; 
    halfedge_t *startHalfEdge = NULL, *endHalfEdge = NULL, *oldStartHalfEdgeNext = NULL, *oldEndHalfEdgePrev = NULL, 
               *joiningHalfEdge = NULL, *otherStartHalfEdge = NULL, *otherEndHalfEdge = NULL, *joiningHalfEdgeTwin = NULL, 
               *oldStartHalfEdgeTwinPrev = NULL, *oldEndHalfEdgeTwinNext = NULL, *tmp = NULL;

    /* Process the split */
    while (scanf("%d %d", &startSplit, &endSplit) == 2) {

        isAdjacent = 0;

        /* Set new index for vertices, edges and faces */
        newStartVertexIdx = dcel->verticesNum;
        newEndVertexIdx = dcel->verticesNum + 1;
        newEdgeIdx = dcel->edgesNum;
        newFaceIdx = dcel->facesNum;  

        /* Allocate extra space to store new vertices, new edges and new face */
        dcel->vertices = realloc(dcel->vertices, (dcel->verticesNum + EXTRA_VERTICES) * sizeof(vertex_t));
        assert(dcel->vertices);
        dcel->edges = realloc(dcel->edges, (dcel->edgesNum + EXTRA_EDGES) * sizeof(edge_t));
        assert(dcel->edges);
        dcel->faces = realloc(dcel->faces, (dcel->facesNum + EXTRA_FACE) * sizeof(face_t));
        assert(dcel->faces);

        /* Create new vertices */
        midStartHalfEdge = midPoint(dcel->vertices[dcel->edges[startSplit].halfEdge->startVertexIdx],
                                    dcel->vertices[dcel->edges[startSplit].halfEdge->endVertexIdx]);
        midEndHalfEdge = midPoint(dcel->vertices[dcel->edges[endSplit].halfEdge->startVertexIdx],
                                  dcel->vertices[dcel->edges[endSplit].halfEdge->endVertexIdx]);

        /* Choose the correct half-edges for the split */
        if (dcel->edges[startSplit].halfEdge->faceIdx == dcel->edges[endSplit].halfEdge->faceIdx) {
            startHalfEdge = dcel->edges[startSplit].halfEdge;
            endHalfEdge = dcel->edges[endSplit].halfEdge;    
        } else if (dcel->edges[startSplit].halfEdge->twin != NULL && 
                   dcel->edges[startSplit].halfEdge->twin->faceIdx == dcel->edges[endSplit].halfEdge->faceIdx) {
            startHalfEdge = dcel->edges[startSplit].halfEdge->twin; 
            endHalfEdge = dcel->edges[endSplit].halfEdge;
        } else if (dcel->edges[endSplit].halfEdge->twin != NULL && 
                   dcel->edges[endSplit].halfEdge->twin->faceIdx == dcel->edges[startSplit].halfEdge->faceIdx) {
            startHalfEdge = dcel->edges[startSplit].halfEdge;
            endHalfEdge = dcel->edges[endSplit].halfEdge->twin;
        } else {
            startHalfEdge = dcel->edges[startSplit].halfEdge->twin;
            endHalfEdge = dcel->edges[endSplit].halfEdge->twin;
        }

        splitFace = startHalfEdge->faceIdx;

        if (startHalfEdge->next == endHalfEdge) {
            isAdjacent = 1;
        }

        /* Store anything that will be updated */
        oldEndOfStart = startHalfEdge->endVertexIdx; 
        oldStartOfEnd = endHalfEdge->startVertexIdx; 
        oldStartHalfEdgeNext = startHalfEdge->next; 
        oldEndHalfEdgePrev = endHalfEdge->prev; 
        if (startHalfEdge->twin != NULL) {
            oldStartOfStartTwin = startHalfEdge->twin->startVertexIdx;
            oldStartHalfEdgeTwinPrev = startHalfEdge->twin->prev;
        }
        if (endHalfEdge->twin != NULL) {
            oldEndOfEndTwin = endHalfEdge->twin->endVertexIdx;
            oldEndHalfEdgeTwinNext = endHalfEdge->twin->next;
        }
        
        /* Update end point of start half-edge and start point of end half-edge */
        startHalfEdge->endVertexIdx = newStartVertexIdx;
        endHalfEdge->startVertexIdx = newEndVertexIdx;

        /* Create new joining half-edge */
        joiningHalfEdge = (halfedge_t *) malloc(sizeof(halfedge_t));
        assert(joiningHalfEdge);

        joiningHalfEdge->startVertexIdx = newStartVertexIdx;
        joiningHalfEdge->endVertexIdx = newEndVertexIdx;
        joiningHalfEdge->faceIdx = startHalfEdge->faceIdx;
        joiningHalfEdge->edgeIdx = newEdgeIdx;
        joiningHalfEdge->next = endHalfEdge;
        joiningHalfEdge->prev = startHalfEdge;
      
        /* Update pointers of start half-edge and end half-edge in dcel */
        startHalfEdge->next = joiningHalfEdge;
        endHalfEdge->prev = joiningHalfEdge;
        
        /* Create a twin for the joining half-edge */
        joiningHalfEdgeTwin = (halfedge_t *) malloc(sizeof(halfedge_t));
        assert(joiningHalfEdgeTwin);

        joiningHalfEdge->twin = joiningHalfEdgeTwin;
        joiningHalfEdgeTwin->twin = joiningHalfEdge;
        joiningHalfEdgeTwin->startVertexIdx = joiningHalfEdge->endVertexIdx;
        joiningHalfEdgeTwin->endVertexIdx = joiningHalfEdge->startVertexIdx;      
        joiningHalfEdgeTwin->edgeIdx = joiningHalfEdge->edgeIdx;
        joiningHalfEdgeTwin->faceIdx = newFaceIdx;
        
        /* Create other halfs of the start half-edge and the old half-edge */ 
        otherStartHalfEdge = (halfedge_t *) malloc(sizeof(halfedge_t));
        assert(otherStartHalfEdge);
        otherEndHalfEdge = (halfedge_t *) malloc(sizeof(halfedge_t));
        assert(otherEndHalfEdge);       

        otherStartHalfEdge->startVertexIdx = startHalfEdge->endVertexIdx;
        otherStartHalfEdge->endVertexIdx = oldEndOfStart;
        otherStartHalfEdge->edgeIdx = newEdgeIdx + 1;
        otherStartHalfEdge->twin = NULL;       
        if (isAdjacent) {
            otherStartHalfEdge->next = otherEndHalfEdge;
            otherStartHalfEdge->prev = joiningHalfEdgeTwin;
        } else {
            otherStartHalfEdge->next = oldStartHalfEdgeNext;
            otherStartHalfEdge->prev = joiningHalfEdgeTwin;
            oldStartHalfEdgeNext->prev = otherStartHalfEdge;
        }
         
        otherEndHalfEdge->startVertexIdx = oldStartOfEnd;
        otherEndHalfEdge->endVertexIdx = endHalfEdge->startVertexIdx;
        otherEndHalfEdge->edgeIdx = newEdgeIdx + 2;
        otherEndHalfEdge->twin = NULL;
        if (isAdjacent) {
            otherEndHalfEdge->next = joiningHalfEdgeTwin;
            otherEndHalfEdge->prev = otherStartHalfEdge;
        } else {
            otherEndHalfEdge->next = joiningHalfEdgeTwin;
            otherEndHalfEdge->prev = oldEndHalfEdgePrev;
            oldEndHalfEdgePrev->next = otherEndHalfEdge;
        }      

        /* Connect the twin of the joining half-edge with them */
        joiningHalfEdgeTwin->next = otherStartHalfEdge;
        joiningHalfEdgeTwin->prev = otherEndHalfEdge; 

        /* Work with twin of start half-edge if exists */
        if (startHalfEdge->twin != NULL) {
            halfedge_t *startHalfEdgeTwin = startHalfEdge->twin;
            halfedge_t *startHalfEdgeTwinOther = NULL;      

            /* Update the twin */
            startHalfEdgeTwin->startVertexIdx = newStartVertexIdx;
                      
            /* Create other half of twin */
            startHalfEdgeTwinOther = (halfedge_t *) malloc(sizeof(halfedge_t));
            assert(startHalfEdgeTwinOther);

            startHalfEdgeTwin->prev = startHalfEdgeTwinOther;
            startHalfEdgeTwinOther->startVertexIdx = oldStartOfStartTwin;
            startHalfEdgeTwinOther->endVertexIdx = newStartVertexIdx;
            startHalfEdgeTwinOther->faceIdx = startHalfEdgeTwin->faceIdx;
            startHalfEdgeTwinOther->edgeIdx = otherStartHalfEdge->edgeIdx;
            startHalfEdgeTwinOther->next = startHalfEdgeTwin;
            startHalfEdgeTwinOther->prev = oldStartHalfEdgeTwinPrev;
            startHalfEdgeTwinOther->twin = otherStartHalfEdge;

            /* Update other pointers to other half of start half-edge twin */
            oldStartHalfEdgeTwinPrev->next = startHalfEdgeTwinOther;
            otherStartHalfEdge->twin = startHalfEdgeTwinOther;

            /* Update face pointer */
            dcel->faces[startHalfEdgeTwin->faceIdx].halfEdge = startHalfEdgeTwin;
        } 

         /* Work with twin of end half-edge if exist */
        if (endHalfEdge->twin != NULL) {
            halfedge_t *endHalfEdgeTwin = endHalfEdge->twin;
            halfedge_t *endHalfEdgeTwinOther = NULL;
    
            /* Update the twin */
            endHalfEdgeTwin->endVertexIdx = newEndVertexIdx;
                      
            /* Create other half of twin */
            endHalfEdgeTwinOther = (halfedge_t *) malloc(sizeof(halfedge_t));
            assert(endHalfEdgeTwinOther);

            endHalfEdgeTwin->next = endHalfEdgeTwinOther;       
            endHalfEdgeTwinOther->startVertexIdx = newEndVertexIdx;
            endHalfEdgeTwinOther->endVertexIdx = oldEndOfEndTwin;
            endHalfEdgeTwinOther->faceIdx = endHalfEdgeTwin->faceIdx;
            endHalfEdgeTwinOther->edgeIdx = otherEndHalfEdge->edgeIdx;
            endHalfEdgeTwinOther->next = oldEndHalfEdgeTwinNext;
            endHalfEdgeTwinOther->prev = endHalfEdgeTwin;
            endHalfEdgeTwinOther->twin = otherEndHalfEdge;
            
            /* Update other pointers to other half of start half-edge twin */
            oldEndHalfEdgeTwinNext->prev = endHalfEdgeTwinOther;
            otherEndHalfEdge->twin = endHalfEdgeTwinOther;

            /* Update face pointer */
            dcel->faces[endHalfEdgeTwin->faceIdx].halfEdge = endHalfEdgeTwin;
        } 

        /* Update original dcel with new vertices */
        dcel->vertices[newStartVertexIdx] = midStartHalfEdge;
        dcel->vertices[newEndVertexIdx] = midEndHalfEdge;
        dcel->verticesNum = (dcel->verticesNum) + 2;   

        /* Update original dcel with new edges and halfedges */
        dcel->edges[newEdgeIdx].halfEdge = joiningHalfEdge;
        dcel->edges[newEdgeIdx + 1].halfEdge = otherStartHalfEdge;
        dcel->edges[newEdgeIdx + 2].halfEdge = otherEndHalfEdge;
        dcel->edgesNum = (dcel->edgesNum) + EXTRA_EDGES;

        /* Update old face and all half edges in old face */
        dcel->faces[splitFace].halfEdge = joiningHalfEdge;
        
        tmp = joiningHalfEdge->next;
        while (tmp->edgeIdx != joiningHalfEdge->edgeIdx) {
            tmp->faceIdx = splitFace;
            tmp = tmp->next;
        } 
           
        /* Update new face and all half edges in new face */
        dcel->faces[newFaceIdx].halfEdge = joiningHalfEdgeTwin;
        dcel->facesNum = (dcel->facesNum) + EXTRA_FACE;
        
        tmp = joiningHalfEdgeTwin->next;
        while (tmp->edgeIdx != joiningHalfEdgeTwin->edgeIdx) {
            tmp->faceIdx = newFaceIdx;
            tmp = tmp->next;
        } 
    }
}   

/* Free doubly connected edge list */   
void freeList(dcel_t *dcel) {
    
    for (int i = 0; i < dcel->edgesNum; i++) {
        if (dcel->edges[i].halfEdge->twin != NULL) {
            free(dcel->edges[i].halfEdge->twin);
        }
        free(dcel->edges[i].halfEdge);
    }

    free(dcel->edges);

    free(dcel->faces);

    free(dcel->vertices);

    free(dcel);
}
