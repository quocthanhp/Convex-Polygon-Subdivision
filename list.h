#ifndef LIST_H
#define LIST_H

    typedef struct halfEdge halfedge_t;

    typedef struct {
        double x; 
        double y;
    } vertex_t;

    struct halfEdge {
        int startVertexIdx;
        int endVertexIdx;
        int faceIdx;
        int edgeIdx;
        halfedge_t *next;
        halfedge_t *prev;
        halfedge_t *twin;    
    };

    typedef struct {
        halfedge_t *halfEdge;
    } edge_t;

    typedef struct {
        halfedge_t *halfEdge;
    } face_t;

    typedef struct {
        int verticesNum;
        int edgesNum;
        int facesNum;
        vertex_t *vertices;
        edge_t *edges;
        face_t *faces;   
    } dcel_t;

    vertex_t *readVertices(FILE *file, int *currentSize);
    dcel_t *constructInitialDcel(FILE *file);
    void split(dcel_t *dcel);
    int isOfHalfPlane(halfedge_t *HalfEdge, vertex_t *vertices, double targetX, double targetY);
    void freeList(dcel_t *dcel);

#endif
