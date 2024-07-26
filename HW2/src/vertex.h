#ifndef _VERTEX_H
#define _VERTEX_H

#include "vectors.h"
#include "matrix.h"

// ==========================================================
// Stores th vertex position, used by the Mesh class
enum VertexType{
  smooth,//s为0
  dart,//s=1
  reg_crease,//s=2valence 是6，crese两边都是smooth；边界的valence是4
  non_reg_crease,//s=2,非reg的
  corner//s>2
};
class Vertex {

public:

  // ========================
  // CONSTRUCTOR & DESTRUCTOR
  Vertex(int i, const Vec3f &pos) : position(pos) { index = i; 
  s = 0; v_type = smooth;
   }
  
  // =========
  // ACCESSORS
  int getIndex() const { return index; }
  double x() const { return position.x(); }
  double y() const { return position.y(); }
  double z() const { return position.z(); }
  const Vec3f& getPos() const { return position; }
  /// @brief 对Q累加
  /// @param Kp Kp=pp^T
  void addQ(Matrix Kp) { Q = Q + Kp; }
  Matrix getQ() { return Q; }
  void setQ(Matrix q) { Q = q; }
  void clearQ() { Q.clear(); }

  int getS(){return s;}
  void setS(int ss){s=ss;};
  VertexType getVertexType(){return v_type;}
  void setVertexType(VertexType vt) { v_type = vt; }
  void setNewPos(Vec3f v) { new_position = v; }
  void updatePos() { position = new_position; }
  Vec3f getNewPos() { return new_position; }

  // =========
  // MODIFIERS
  void setPos(Vec3f v) { position = v; }

private:

  // don't use these constructors
  Vertex() { assert(0); exit(0); }
  Vertex(const Vertex&) { assert(0); exit(0); }
  Vertex& operator=(const Vertex&) { assert(0); exit(0); }
  
  // ==============
  // REPRESENTATION
  Vec3f position;

  // this is the index from the original .obj file.
  // technically not part of the half-edge data structure, 
  // but we use it for hashing
  int index;  

  // NOTE: the vertices don't know anything about adjacency.  In some
  // versions of this data structure they have a pointer to one of
  // their incoming edges.  However, this data is very complicated to
  // maintain during mesh manipulation, so it has been omitted.

  Matrix Q;//Q矩阵

  ///该顶点连接的crease边的数量
  int s;

  ///顶点类型
  VertexType v_type;

  //用于LoopSubdivision()的第三步计算新位置，并统一更新
  Vec3f new_position;

};

// ==========================================================

#endif

