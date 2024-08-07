#ifndef EDGE_H
#define EDGE_H

#include <cassert>

class Vertex;
class Triangle;

// ===================================================================
// half-edge data structure used by the Mesh class

class Edge { 

public:

  // ========================
  // CONSTRUCTORS & DESTRUCTOR
  Edge(Vertex *vs, Vertex *ve, Triangle *t);
  ~Edge();

  // =========
  // ACCESSORS
  Vertex* getStartVertex() const { assert (start_vertex != NULL); return start_vertex; }
  Vertex* getEndVertex() const { assert (end_vertex != NULL); return end_vertex; }
  Edge* getNext() const { 
    // assert (next != NULL); 
    return next; }
  Triangle* getTriangle() const { assert (triangle != NULL); return triangle; }
  Edge* getOpposite() const {
    // warning!  the opposite edge might be NULL!
    return opposite; }
  float getCrease() const { return crease; }
  float Length() const;
  
  // =========
  // MODIFIERS
  void setOpposite(Edge *e) {
    assert (opposite == NULL); 
    assert (e != NULL);
    assert (e->opposite == NULL);
    opposite = e; 
    e->opposite = this; 
  }
  void clearOpposite() { 
    if (opposite == NULL) return; 
    assert (opposite->opposite == this); 
    opposite->opposite = NULL;
    opposite = NULL; 
  }
  void setNext(Edge *e) {
    assert (next == NULL);
    assert (e != NULL);
    assert (triangle == e->triangle);
    next = e;
  }
  void setCrease(float c) { crease = c; }
  // bool getHaveCommon(){return haveCommon;};
  // void setHaveCommon(bool x){haveCommon=x;};
  bool getIsOK(){return isOK;};
  void setIsOK(bool x){isOK=x;};


private:

  Edge(const Edge&) { assert(0); }
  Edge& operator=(const Edge&) { assert(0); exit(0); }

  // ==============
  // REPRESENTATION
  // in the half edge data adjacency data structure, the edge stores everything!
  // note: it's technically not necessary to store both vertices, but it makes
  //   dealing with non-closed meshes easier
  Vertex *start_vertex;
  Vertex *end_vertex;
  Triangle *triangle;
  Edge *opposite;
  Edge *next;
  // the crease value is an extra field used during subdivision
  float crease;
  // bool haveCommon=false;//表示一条边如果在此次更新中是否有重复的。
  bool isOK=true;//表示删除这条边是不是可以的
};

// ===================================================================

#endif
