#ifndef _EDGE_H_
#define _EDGE_H_

#include "vertex.h"
#include "edge.h"


// ===========
// CONSTRUCTOR
Edge::Edge(Vertex *vs, Vertex *ve, Triangle *t) {
  start_vertex = vs;
  end_vertex = ve;
  triangle = t;
  next = NULL;
  opposite = NULL;
  crease = 0;
  isOK=true;
}


// ==========
// DESTRUCTOR
Edge::~Edge() { 
  // disconnect from the opposite edge
  if (opposite != NULL)
    opposite->opposite = NULL;
  // NOTE: the "prev" edge (which has a "next" pointer pointing to
  // this edge) will also be deleted as part of the triangle removal,
  // so we don't need to disconnect that
}


// ========
// ACCESSOR
float Edge::Length() const {
  Vec3f diff = start_vertex->getPos() - end_vertex->getPos();
  return diff.Length();
}

#endif
