#ifndef __ARG_PARSER_H__
#define __ARG_PARSER_H__

#include <cassert>
#include <string>

#include "vectors.h"
#include "MersenneTwister.h"

// ================================================================================
// ================================================================================
enum AnimateType { Animate, Runge_Kutta, AdaptiveTimestep};
class ArgParser {

public:

  ArgParser() { DefaultValues(); }

  ArgParser(int argc, char *argv[]) {
    DefaultValues();
    std::cout<<"The order is : " << std::endl;
    for (int i = 1; i < argc; i++) {
        std::cout<<argv[i] << " " ;
   }
    std::cout << std::endl;
    for (int i = 1; i < argc; i++) {
      if (argv[i] == std::string("-cloth")) {
        i++; assert (i < argc); 
        cloth_file = argv[i];
      } else if (argv[i] == std::string("-fluid")) {
        i++; assert (i < argc); 
        fluid_file = argv[i];
      } else if (argv[i] == std::string("-size")) {
        i++; assert (i < argc); 
	    width = height = atoi(argv[i]);
      } else if (argv[i] == std::string("-timestep")) {
	    i++; assert (i < argc); 
	    timestep = atof(argv[i]);
        assert (timestep > 0);
      } else if (argv[i] == std::string("-animatetype")) {
          i++; assert(i < argc);
          char str3[] = "adaptive_timestep";
          char str2[] = "runge_kutta";
          char str1[] = "animate";
          char* str0 = argv[i];

          if(0 == strcmp(str0, str1))
            animateType= AnimateType::Animate;
          else if (0 == strcmp(str0, str2))
             animateType = AnimateType::Runge_Kutta;
          else if (0 == strcmp(str0, str3))
              animateType = AnimateType::AdaptiveTimestep;
      }else if (argv[i] == std::string("-dynamicInverseConstraints")) {
          //i++; assert(i < argc);
          isDynamicInverseConstraints = true;
      }
      else if (argv[i] == std::string("-iterations")) {
          i++; assert(i < argc);
          num = atoi(argv[i]);
      }
      else {
	        printf ("whoops error with command line argument %d: '%s'\n",i,argv[i]);
	        assert(0);
      }
    }
    std::cout << "TimeStep: "<< timestep << std::endl;
    
    if (animateType == AnimateType::Animate)
        std::cout << "Method:  Basic  Method" << std::endl;
    else if (animateType == AnimateType::Runge_Kutta)
        std::cout << "Method:  Runge_Kutta Method" << std::endl;
    else if (animateType == AnimateType::AdaptiveTimestep)
        std::cout << "Method:  AdaptiveTimestep Method" << std::endl;
    if (isDynamicInverseConstraints)
        std::cout << "Dynamic Inverse Constraints On Deformation Rate: ON" << std::endl;
    else
        std::cout << "Dynamic Inverse Constraints On Deformation Rate: OFF" << std::endl;
    if(num<0)
        std::cout << "Iterations: INFINITE" << std::endl;
    else
        std::cout << "Iterations: " <<num<< std::endl;


  }

  // ===================================
  // ===================================

  void DefaultValues() {
    width = 500;
    height = 500;

    timestep = 0.01;
    animate = false;

    particles = true;
    velocity = true;
    force = true;

    face_velocity = 0;
    dense_velocity = 0;

    surface = false;
    isosurface = 0.7;

    wireframe = false;
    bounding_box = true;
    cubes = false;
    pressure = false;

    gravity = Vec3f(0,-9.8,0);

    // uncomment for deterministic randomness
    // mtrand = MTRand(37);

    //ZYF ADD
    animateType = AnimateType::Animate;
    isDynamicInverseConstraints = false;
    num = -1;
    
  }

  // ===================================
  // ===================================
  // REPRESENTATION
  // all public! (no accessors)

  std::string cloth_file;
  std::string fluid_file;
  int width;
  int height;

  // animation control
  double timestep;
  bool animate;
  Vec3f gravity;

  // display option toggles 
  // (used by both)
  bool particles;
  bool velocity;
  bool surface;
  bool bounding_box;

  // used by cloth
  bool force;
  bool wireframe;  

  // used by fluid
  int face_velocity;
  int dense_velocity;
  double isosurface;
  bool cubes;
  bool pressure;

  // default initialization
  MTRand mtrand;

  //ZYF ADD
  AnimateType animateType;
  bool isDynamicInverseConstraints;
  int num;
};

// ================================================================================

#endif
