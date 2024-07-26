#ifndef _CLOTH_H_
#define _CLOTH_H_

#include "argparser.h"
#include "boundingbox.h"
#include "vbo_structs.h"
#include <vector>

// =====================================================================================
// Cloth Particles
// =====================================================================================

class ClothParticle {
public:
  // ACCESSORS
  const Vec3f& getOriginalPosition() const{ return original_position; }
  const Vec3f& getPosition() const{ return position; }
  const Vec3f& getVelocity() const{ return velocity; }
  const Vec3f& getAcceleration() const { return acceleration; }
  Vec3f getForce() const { return mass*acceleration; }
  double getMass() const { return mass; }
  bool isFixed() const { return fixed; }
  // MODIFIERS
  void setOriginalPosition(const Vec3f &p) { original_position = p; }
  void setPosition(const Vec3f &p) { position = p; }
  void setVelocity(const Vec3f &v) { velocity = v; }
  void setAcceleration(const Vec3f &a) { acceleration = a; }
  void setMass(double m) { mass = m; }
  void setFixed(bool b) { fixed = b; }

  //ZYF ADD
  void setLastPosition(const Vec3f& p) { last_position = p; }
  void setLastVelocity(const Vec3f& v) { last_velocity = v; }
  void setLastAcceleration(const Vec3f& a) { last_acceleration = a; }
  void setRKVelocity(const Vec3f& v) { Runge_Kutta_velocity = v; }

  const Vec3f& getLastPosition() const { return last_position; }
  const Vec3f& getLastVelocity() const { return last_velocity; }
  const Vec3f& getLastAcceleration() const { return last_acceleration; }
  const Vec3f& getRKVelocity() const { return Runge_Kutta_velocity; }


private:
  // REPRESENTATION
  Vec3f original_position;
  Vec3f position;
  Vec3f velocity;
  Vec3f acceleration;
  double mass;
  bool fixed;

  //ZYF ADD
  Vec3f last_position;
  Vec3f last_velocity;
  Vec3f last_acceleration;
  Vec3f Runge_Kutta_velocity;
};

// =====================================================================================
// Cloth System
// =====================================================================================

class Cloth {

public:
  Cloth(ArgParser *args);
  ~Cloth() { delete [] particles; cleanupVBOs(); }

  // ACCESSORS
  const BoundingBox& getBoundingBox() const { return box; }

  // PAINTING & ANIMATING
  void Paint() const;
  void Animate();

  void initializeVBOs();
  void setupVBOs();
  void drawVBOs();
  void cleanupVBOs();

  //ZYF ADD
  Vec3f computeF(ClothParticle& p, int i, int j);
  void Dynamic_Inverse_Constraints_On_Deformation_Rate();
  void AdaptiveTimestep();
  double GetMaxVelocity(int type);
  void ResetParticle();
  void Runge_Kutta();

private:

  // PRIVATE ACCESSORS
  const ClothParticle& getParticle(int i, int j) const {
    assert (i >= 0 && i < nx && j >= 0 && j < ny);
    return particles[i + j*nx]; }
  ClothParticle& getParticle(int i, int j) {
    assert (i >= 0 && i < nx && j >= 0 && j < ny);
    return particles[i + j*nx]; }

  Vec3f computeGouraudNormal(int i, int j) const;

  // HELPER FUNCTION
  void computeBoundingBox();
  void AddVBOEdge(int i1, int j1, int i2, int j2, double correction);

  // REPRESENTATION
  ArgParser *args;
  // grid data structure
  int nx, ny;
  ClothParticle *particles;
  BoundingBox box;
  // simulation parameters
  double damping;
  // spring constants
  double k_structural;
  double k_shear;
  double k_bend;
  // correction thresholds
  double provot_structural_correction;
  double provot_shear_correction;

  // VBOs
  GLuint cloth_verts_VBO;
  GLuint cloth_quad_indices_VBO;
  GLuint cloth_happy_edge_indices_VBO;
  GLuint cloth_unhappy_edge_indices_VBO;
  GLuint cloth_velocity_visualization_VBO;
  GLuint cloth_force_visualization_VBO;
  std::vector<VBOPosNormalColor> cloth_verts; 
  std::vector<VBOIndexedQuad> cloth_quad_indices;
  std::vector<VBOIndexedEdge> cloth_happy_edge_indices;
  std::vector<VBOIndexedEdge> cloth_unhappy_edge_indices;
  std::vector<VBOPosColor> cloth_velocity_visualization;
  std::vector<VBOPosColor> cloth_force_visualization;

  //ZYF ADD
  double last_maxV=0;
  

};

// ========================================================================

#endif
