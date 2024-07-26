#ifndef _IFS_H_
#define _IFS_H_

#include <cassert>
#include <vector>
#include <string>

#include "glCanvas.h"
#include "matrix.h"
#include "vectors.h"

class ArgParser;

// ====================================================================
// ====================================================================
// class to manage the data and drawing for an "iterated function system"

class IFS {

public:

  // CONSTRUCTOR
  IFS(ArgParser *a);

  // HELPER FUNCTIONS FOR RENDERING
  void setupVBOs();
  void drawVBOs();
  void cleanupVBOs();

private:

  // private helper functions for VBOs
  void setupPoints();
  void drawPoints() const;
  void cleanupPoints();
  void setupCube();
  void drawCube() const;
  void cleanupCube();

  // REPRESENTATION
  ArgParser *args;
  GLuint points_VBO;
  GLuint cube_verts_VBO;
  GLuint cube_face_indices_VBO;
};

// ====================================================================
// ====================================================================

// helper structures for VBOs
// (note, the data stored in each of these is application specific, 
// adjust as needed!)

struct VBOPoint {
  VBOPoint() {}
  VBOPoint(const Vec3f &p) {
    x = p.x(); y = p.y(); z = p.z();
  }
  float x, y, z;    // position
};

struct VBOVertex {
  VBOVertex() {}
  VBOVertex(const Vec3f &p, const Vec3f &n, const Vec3f &c) {
    x = p.x(); y = p.y(); z = p.z();
    nx = n.x(); ny = n.y(); nz = n.z();
    cx = c.x(); cy = c.y(); cz = c.z();
  }
  float x, y, z;    // position
  float nx, ny, nz; // normal
  float cx, cy, cz; // color
  float padding[7]; // padding to make struct 32 bytes in size
};

struct VBOQuad {
  VBOQuad() {}
  VBOQuad(unsigned int a, unsigned int b, unsigned int c, unsigned int d) {
    verts[0] = a;
    verts[1] = b;
    verts[2] = c;
    verts[3] = d;
  }
  unsigned int verts[4];
};

// ====================================================================
// ====================================================================

#endif
