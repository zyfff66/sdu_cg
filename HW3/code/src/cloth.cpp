#include "glCanvas.h"

#include <fstream>
#include "cloth.h"
#include "argparser.h"
#include "vectors.h"
#include "utils.h"


// ================================================================================
// ================================================================================

Cloth::Cloth(ArgParser *_args) {
  args =_args;

  // open the file
  std::ifstream istr(args->cloth_file.c_str());
  //assert (istr != NULL);
  assert(istr);


  std::string token;

  // read in the simulation parameters
  istr >> token >> k_structural; assert (token == "k_structural");  // (units == N/m)  (N = kg*m/s^2)
  istr >> token >> k_shear; assert (token == "k_shear");
  istr >> token >> k_bend; assert (token == "k_bend");
  istr >> token >> damping; assert (token == "damping");
  // NOTE: correction factor == .1, means springs shouldn't stretch more than 10%
  //       correction factor == 100, means don't do any correction
  istr >> token >> provot_structural_correction; assert (token == "provot_structural_correction");
  istr >> token >> provot_shear_correction; assert (token == "provot_shear_correction");

  // the cloth dimensions
  istr >> token >> nx >> ny; // (units == meters)
  assert (token == "m");
  assert (nx >= 2 && ny >= 2);

  // the corners of the cloth
  Vec3f a,b,c,d;
  istr >> token >> a; assert (token == "p");
  istr >> token >> b; assert (token == "p");
  istr >> token >> c; assert (token == "p");
  istr >> token >> d; assert (token == "p");

  // fabric weight  (units == kg/m^2)
  // denim ~300 g/m^2
  // silk ~70 g/m^2
  double fabric_weight;
  istr >> token >> fabric_weight; assert (token == "fabric_weight");
  double area = AreaOfTriangle(a,b,c) + AreaOfTriangle(a,c,d);

  // create the particles
  particles = new ClothParticle[nx*ny];
  double mass = area*fabric_weight / double(nx*ny);
  for (int i = 0; i < nx; i++) {
    double x = i/double(nx-1);
    Vec3f ab = (1-x)*a + x*b;
    Vec3f dc = (1-x)*d + x*c;
    for (int j = 0; j < ny; j++) {
      double y = j/double(ny-1);
      ClothParticle &p = getParticle(i,j);
      Vec3f abdc = (1-y)*ab + y*dc;
      p.setOriginalPosition(abdc);
      p.setPosition(abdc);
      p.setVelocity(Vec3f(0,0,0));
      p.setMass(mass);
      p.setFixed(false);
    }
  }

  // the fixed particles
  while (istr >> token) {
    assert (token == "f");
    int i,j;
    double x,y,z;
    istr >> i >> j >> x >> y >> z;
    ClothParticle &p = getParticle(i,j);
    p.setPosition(Vec3f(x,y,z));
    p.setFixed(true);
  }

  computeBoundingBox();
  initializeVBOs();
  setupVBOs();
}

// ================================================================================

void Cloth::computeBoundingBox() {
  box = BoundingBox(getParticle(0,0).getPosition());
  for (int i = 0; i < nx; i++) {
    for (int j = 0; j < ny; j++) {
      box.Extend(getParticle(i,j).getPosition());
      box.Extend(getParticle(i,j).getOriginalPosition());
    }
  }
}
/// <summary>
/// 获取最大速度
/// </summary>
/// <param name="type"></param>
/// <returns></returns>
double Cloth::GetMaxVelocity(int type) {
    double MaxVelocity = 0;

    if (type == 1) {
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                ClothParticle& p = getParticle(i, j);
                double LastV = p.getLastVelocity().Length();
                if (MaxVelocity < LastV)MaxVelocity = LastV;
            }
        }
    }
    else if (type == 2) {
        for (int i = 0; i < nx; i++) {
            for (int j = 0; j < ny; j++) {
                ClothParticle& p = getParticle(i, j);
                double V = p.getVelocity().Length();
                if (MaxVelocity < V)MaxVelocity = V;
            }
        }
    }
    return MaxVelocity;
}
/// <summary>
/// 重置粒子状态
/// </summary>
void Cloth::ResetParticle() {
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            p.setPosition(p.getLastPosition());
            p.setVelocity(p.getLastVelocity());
            p.setAcceleration(p.getLastAcceleration());
        }
    }
}
/// <summary>
/// 自适应步长
/// </summary>
void Cloth::AdaptiveTimestep() {

    
    //添加自适应步长
    Animate();
    double max_Velocity = GetMaxVelocity(2);
    double min_timestep = 0.0005;
    double max_timestep = 0.015;
    double max_V = 3;
    double min_V = 0.2;
    
    bool isChange = false;
    if (max_Velocity > max_V) {//denim_curtain:3    provot_correct_structural_and_shear:2
        if (args->timestep * 0.5 >= min_timestep && args->timestep * 0.5 <= max_timestep) {
            args->timestep = args->timestep * 0.5;
            std::cout << "max_Velocity > "<< max_V <<" time_step 修改为"<< args->timestep << std::endl;
            isChange = true;
        }
    }
    else if (max_Velocity < min_V) {//denim_curtain:0.2  provot_correct_structural_and_shear:0.5
        if (args->timestep * 2 >= min_timestep && args->timestep * 2 <= max_timestep) {
            args->timestep = args->timestep * 2;
            std::cout << "max_Velocity < "<< min_V << "time_step 修改为" << args->timestep << std::endl;
            isChange = true;
        }
    }

    if (!isChange)
        last_maxV++;
    /*if (last_maxV > 20) {
        last_maxV=0;
        if (args->timestep +0.001 <= max_timestep) {
            args->timestep = args->timestep + 0.001;
            std::cout << "上次更新为20次前，增大步长为" << args->timestep << std::endl;
        }
    }*/
    ResetParticle();
    Animate();
    //std::cout <<"timestep: "<<args->timestep << std::endl;
    // redo VBOs for rendering
    setupVBOs();
}
void Cloth::Animate() {


  // *********************************************************************  
  // ASSIGNMENT:
  //
  // Compute the forces on each particle, and update the state
  // (position & velocity) of each particle.
  //
  // *********************************************************************    
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            p.setLastPosition(p.getPosition());
            p.setLastVelocity(p.getVelocity());
            p.setLastAcceleration(p.getAcceleration());

            Vec3f F = computeF(p, i, j);

            //更新参数
            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f velocity = p.getVelocity() + acceleration * args->timestep;

                velocity = (velocity + p.getVelocity()) * 0.5;
                acceleration = (acceleration + p.getAcceleration()) * 0.5;

                Vec3f position = p.getPosition() + velocity * args->timestep;
                p.setPosition(position);
                p.setVelocity(velocity);
                p.setAcceleration(acceleration);
            }


        }
    }
    if (args->isDynamicInverseConstraints)
        Dynamic_Inverse_Constraints_On_Deformation_Rate();



  // redo VBOs for rendering
  setupVBOs();
}
/// <summary>
/// 根据论文计算力F
/// </summary>
/// <param name="p">粒子</param>
/// <param name="i">粒子对应坐标</param>
/// <param name="j">粒子对应坐标</param>
/// <returns></returns>
Vec3f Cloth::computeF(ClothParticle& p, int i, int j) {

    std::vector<ClothParticle> p_structural_springs;
    std::vector<ClothParticle> p_shear_springs;
    std::vector<ClothParticle> p_flexion_springs;

    //structural springs
    if (i + 1 >= 0 && i + 1 < nx) {
        p_structural_springs.push_back(getParticle(i + 1, j));
    }
    if (i - 1 >= 0 && i - 1 < nx) {
        p_structural_springs.push_back(getParticle(i - 1, j));
    }
    if (j + 1 >= 0 && j + 1 < ny) {
        p_structural_springs.push_back(getParticle(i, j + 1));
    }
    if (j - 1 >= 0 && j - 1 < ny) {
        p_structural_springs.push_back(getParticle(i, j - 1));
    }
    //shear springs
    if (i + 1 >= 0 && i + 1 < nx && j + 1 >= 0 && j + 1 < ny) {
        p_shear_springs.push_back(getParticle(i + 1, j + 1));
    }
    if (i + 1 >= 0 && i + 1 < nx && j - 1 >= 0 && j - 1 < ny) {
        p_shear_springs.push_back(getParticle(i + 1, j - 1));
    }
    if (i - 1 >= 0 && i - 1 < nx && j + 1 >= 0 && j + 1 < ny) {
        p_shear_springs.push_back(getParticle(i - 1, j + 1));
    }
    if (i - 1 >= 0 && i - 1 < nx && j - 1 >= 0 && j - 1 < ny) {
        p_shear_springs.push_back(getParticle(i - 1, j - 1));
    }
    //flexion springs
    if (i + 2 >= 0 && i + 2 < nx) {
        p_flexion_springs.push_back(getParticle(i + 2, j));
    }
    if (i - 2 >= 0 && i - 2 < nx) {
        p_flexion_springs.push_back(getParticle(i - 2, j));
    }
    if (j + 2 >= 0 && j + 2 < ny) {
        p_flexion_springs.push_back(getParticle(i, j + 2));
    }
    if (j - 2 >= 0 && j - 2 < ny) {
        p_flexion_springs.push_back(getParticle(i, j - 2));
    }

    Vec3f p_position = p.getPosition();
    Vec3f p_original_position = p.getOriginalPosition();
    Vec3f F_structural_springs(0, 0, 0);
    Vec3f F_shear_springs(0, 0, 0);
    Vec3f F_flexion_springs(0, 0, 0);
    // F_structural_springs
    for (int e = 0; e < p_structural_springs.size(); e++) {
        ClothParticle& p_temp = p_structural_springs[e];
        Vec3f p_temp_position = p_temp.getPosition();
        Vec3f p_temp_original_position = p_temp.getOriginalPosition();
        Vec3f F_direction = p_position - p_temp_position;
        F_direction.Normalize();
        float original_length = (p_original_position - p_temp_original_position).Length();
        float new_length = (p_position - p_temp_position).Length();
        Vec3f F_p_p_temp = (-1) * k_structural * F_direction * (new_length - original_length);
        F_structural_springs = F_structural_springs + F_p_p_temp;
    }
    //F_shear_springs
    for (int e = 0; e < p_shear_springs.size(); e++) {
        ClothParticle& p_temp = p_shear_springs[e];
        Vec3f p_temp_position = p_temp.getPosition();
        Vec3f p_temp_original_position = p_temp.getOriginalPosition();
        Vec3f F_direction = p_position - p_temp_position;
        F_direction.Normalize();
        float original_length = (p_original_position - p_temp_original_position).Length();
        float new_length = (p_position - p_temp_position).Length();
        Vec3f F_p_p_temp = (-1) * k_shear * F_direction * (new_length - original_length);
        F_shear_springs = F_shear_springs + F_p_p_temp;
    }
    //F_flexion_springs
    for (int e = 0; e < p_flexion_springs.size(); e++) {
        ClothParticle& p_temp = p_flexion_springs[e];
        Vec3f p_temp_position = p_temp.getPosition();
        Vec3f p_temp_original_position = p_temp.getOriginalPosition();
        Vec3f F_direction = p_position - p_temp_position;
        F_direction.Normalize();
        float original_length = (p_original_position - p_temp_original_position).Length();
        float new_length = (p_position - p_temp_position).Length();
        Vec3f F_p_p_temp = (-1) * k_bend * F_direction * (new_length - original_length);
        F_flexion_springs = F_flexion_springs + F_p_p_temp;
    }
    Vec3f F_gr = args->gravity * p.getMass();
    Vec3f F_dis = (-1) * damping * p.getVelocity();

    Vec3f F = F_structural_springs + F_shear_springs + F_flexion_springs + F_gr + F_dis;
    return F;
}

/// <summary>
/// 当变形率超过阈值时，对弹簧进行修改
/// </summary>
void Cloth::Dynamic_Inverse_Constraints_On_Deformation_Rate() {

    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            for (int n = -1; n < 2; n++) {
                for (int m = -1; m < 2; m++) {
                    if (i + n < 0 || i + n >= nx || j + m < 0 || j + m >= ny || (n == 0 && m == 0)) {
                        continue;
                    }
                    double correction = 0;
                    if (abs(n) + abs(m) == 1) {//structural
                        correction = provot_structural_correction;
                    }
                    else if (abs(n) + abs(m) == 2) {//shear
                        correction = provot_shear_correction;
                    }
                    ClothParticle& p2 = getParticle(i + n, j + m);
                    float original_length = (p.getOriginalPosition() - p2.getOriginalPosition()).Length();
                    float new_length = (p.getPosition() - p2.getPosition()).Length();
                    float defor_rate = (new_length - original_length) / original_length;

                    if (defor_rate > correction || defor_rate < -correction) {
                        if (defor_rate < -correction) correction = -correction;


                        if (p.isFixed() && !p2.isFixed()) {//p固定
                            Vec3f p2_new_position = p.getPosition() + (p2.getPosition() - p.getPosition()) * (1 + correction) * (original_length / new_length);
                            Vec3f p2_new_velocity = (p2_new_position - p2.getLastPosition()) * (1 / (p2_new_position - p2.getLastPosition()).Length()) * ((1 + correction) * original_length / new_length) * p2.getVelocity().Length();
                            // 更新位置和速度
                            p2.setPosition(p2_new_position);
                            p2.setVelocity(p2_new_velocity);
                        }
                        else if (p2.isFixed() && !p.isFixed()) {//p2固定
                            Vec3f p_new_position = p2.getPosition() + (p.getPosition() - p2.getPosition()) * (1 + correction) * (original_length / new_length);
                            Vec3f p_new_velocity = (p_new_position - p.getLastPosition()) * (1 / (p_new_position - p.getLastPosition()).Length()) * ((1 + correction) * original_length / new_length) * p.getVelocity().Length();
                            // 更新位置和速度
                            p.setPosition(p_new_position);
                            p.setVelocity(p_new_velocity);
                        }
                        else if (!p.isFixed() && !p2.isFixed()) {//两个都不固定
                            Vec3f p2_new_position = p.getPosition() + (p2.getPosition() - p.getPosition()) * ((1 + correction) / 2 + (new_length / original_length) / 2) * (original_length / new_length);
                            Vec3f p_new_position = p2.getPosition() + (p.getPosition() - p2.getPosition()) * ((1 + correction) / 2 + (new_length / original_length) / 2) * (original_length / new_length);
                            // 更新位置和速度
                            p2.setPosition(p2_new_position);
                            p.setPosition(p_new_position);

                            //计算速度
                            Vec3f p2_new_velocity = (p2_new_position - p2.getLastPosition()) * (1 / (p2_new_position - p2.getLastPosition()).Length()) * ((1 + correction) * original_length / new_length) * p2.getVelocity().Length();
                            Vec3f p_new_velocity = (p_new_position - p.getLastPosition()) * (1 / (p_new_position - p.getLastPosition()).Length()) * ((1 + correction) * original_length / new_length) * p.getVelocity().Length();
                            p2.setVelocity(p2_new_velocity);
                            p.setVelocity(p_new_velocity);
                        }
                    }
                }
            }

        }
    }

}
/// <summary>
/// 四阶Runge-Kutta
/// </summary>
void Cloth::Runge_Kutta() {
    //第一部分计算+1/6
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            p.setLastPosition(p.getPosition());
            p.setLastVelocity(p.getVelocity());
            p.setLastAcceleration(p.getAcceleration());
            Vec3f F = computeF(p, i, j);

            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f velocity = p.getVelocity() + acceleration * args->timestep * 0.5;
                Vec3f position = p.getPosition() + velocity * args->timestep * 0.5;
                Vec3f RKv = p.getVelocity() * 0.167 ;
                p.setRKVelocity(RKv);
                p.setPosition(position);
                p.setVelocity(velocity);
                p.setAcceleration(acceleration);

            }
        }
    }
    //第二部分计算  +1/3
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            p.setLastPosition(p.getPosition());
            p.setLastVelocity(p.getVelocity());
            p.setLastAcceleration(p.getAcceleration());
            Vec3f F = computeF(p, i, j);
            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f velocity = p.getVelocity() + acceleration * args->timestep * 0.5;
                Vec3f position = p.getPosition() + velocity * args->timestep * 0.5;
                Vec3f RKv = p.getRKVelocity()  + velocity * 0.333;
                p.setRKVelocity(RKv);
                p.setPosition(position);
                p.setVelocity(velocity);
                p.setAcceleration(acceleration);

            }
        }
    }
    //第三部分计算  +1/3
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            Vec3f F = computeF(p, i, j);

            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f velocity = p.getVelocity() + acceleration * args->timestep * 0.5;
                Vec3f position = p.getPosition() + velocity * args->timestep * 0.5;
                Vec3f RKv = p.getRKVelocity() + velocity * 0.333;
                p.setRKVelocity(RKv);
                p.setPosition(position);
                p.setVelocity(velocity);
                p.setAcceleration(acceleration);

            }
        }
    }
    //第四部分计算 +1/6
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            Vec3f F = computeF(p, i, j);
            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f velocity = p.getVelocity() + acceleration * args->timestep;
                Vec3f position = p.getPosition() + velocity * args->timestep;
                Vec3f RKv = p.getRKVelocity() + velocity * 0.167;
                p.setRKVelocity(RKv);
                p.setPosition(position);
                p.setVelocity(velocity);
                p.setAcceleration(acceleration);

            }
        }
    }
    //整合
    for (int i = 0; i < nx; i++) {
        for (int j = 0; j < ny; j++) {
            ClothParticle& p = getParticle(i, j);
            Vec3f F = computeF(p, i, j);
            if (!p.isFixed()) {
                double mass = p.getMass();
                Vec3f acceleration = F * (1 / mass);
                Vec3f position = p.getLastPosition() + p.getRKVelocity() * args->timestep;
                p.setVelocity(p.getRKVelocity());
                p.setPosition(position);
                p.setAcceleration(acceleration);

            }
        }
    }
    if(args->isDynamicInverseConstraints)
        Dynamic_Inverse_Constraints_On_Deformation_Rate();
    setupVBOs();
}


