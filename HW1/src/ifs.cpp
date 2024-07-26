#include <cassert>
#include <fstream>
#include "ifs.h"
#include "argparser.h"

// some helper stuff for VBOs
//#define NUM_CUBE_QUADS 6
//#define NUM_CUBE_VERTS NUM_CUBE_QUADS * 4
#define BUFFER_OFFSET(i) ((char *)NULL + (i))

// ====================================================================
// ====================================================================
int face_size = 6;
int cube_vert_size = face_size * 4;
std::vector<float> probabilities;
std::vector<Matrix> matrixs;
int num_transforms;

IFS::IFS(ArgParser *a) {

  args = a;

  // open the file
  std::ifstream input(args->input_file.c_str());
  if (!input) {
    std::cout << "ERROR: must specify valid input file"<< std::endl;
    exit(1);
  }

  // read the number of transforms

  input >> num_transforms;
  // read in the transforms
  for (int i = 0; i < num_transforms; i++) {
    float probability; 
    input >> probability;
    Matrix m; 
    input >> m;
    std::cout << probability << std::endl;
    
    // ASSIGNMENT: do something with each probability & matrix
    if(i==0)
        probabilities.push_back(probability);
    else
        probabilities.push_back(probability+probabilities[i-1]);
    matrixs.push_back(m);

  }  
  
  
}

// ====================================================================
// ====================================================================

void IFS::setupVBOs() {
  HandleGLError("enter setup vbos");
  if (args->cubes) {
    setupCube();
  } else {
    setupPoints();
  }
  HandleGLError("leaving setup vbos");
}

void IFS::drawVBOs() {
  HandleGLError("enter draw vbos");
  if (args->cubes) {


    // ASSIGNMENT: don't just draw one cube...
    drawCube();


  } else {
    drawPoints();
  }
  HandleGLError("leaving draw vbos");
}

// unfortunately, this is never called
// (but this is not a problem, since this data is used for the life of the program)
void IFS::cleanupVBOs() {
  HandleGLError("enter cleanup vbos");
  if (args->cubes) {
    cleanupCube();
  } else {
    cleanupPoints();
  }
  HandleGLError("leaving cleanup vbos");
}


// ====================================================================
// ====================================================================
//操纵点
void sierpinski_triangle_point(Vec3f &v, ArgParser* args) {
    for (int j = 0; j < args->iters; j++) {
        double p = args->mtrand.rand();
        //轮盘 找到做哪个变换
        Matrix m = matrixs[0];
        for (int i = 0; i < num_transforms - 1; i++)
        {
            if (p > probabilities[i] && p <= probabilities[i + 1]) {
                m = matrixs[i + 1];
                break;
            }
        }
        v = m * v;
    }
}

void IFS::setupPoints() {

  HandleGLError("in setup points");

  // allocate space for the data
  VBOPoint* points = new VBOPoint[(args->points)];

  // generate a block of random data
  for (int i = 0; i < args->points; i++) {
    double x = args->mtrand.rand();//生成0，1的点
    double y = args->mtrand.rand();
    double z = args->mtrand.rand();
    Vec3f v(x, y, z);
    // ASSIGNMENT: manipulate point
    sierpinski_triangle_point(v, args);//三角形

    points[i] = VBOPoint(v);

  }

  // create a pointer for the VBO
  glGenBuffers(1, &points_VBO);

  // copy the data to the VBO
  glBindBuffer(GL_ARRAY_BUFFER,points_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOPoint) * args->points,
	       points,
	       GL_STATIC_DRAW); 

  delete [] points;

  HandleGLError("leaving setup points");
}


void IFS::drawPoints() const {
  glColor3f(0,0,0); 
  glPointSize(1);

  HandleGLError("in draw points");

  // select the point buffer
  glBindBuffer(GL_ARRAY_BUFFER, points_VBO);
  glEnableClientState(GL_VERTEX_ARRAY);
  // describe the data
  glVertexPointer(3, GL_FLOAT, 0, 0);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
  // draw this data
  glDrawArrays(GL_POINTS, 0, args->points);
  
  glDisableClientState(GL_VERTEX_ARRAY);
  glDisableVertexAttribArray(0);

  HandleGLError("leaving draw points");
}


void IFS::cleanupPoints() {
  glDeleteBuffers(1, &points_VBO);
}

// ====================================================================
// ====================================================================
//转换一次cube
VBOVertex* translateCube(VBOVertex* cube_verts_origin,int size)
{
    VBOVertex* cube_verts_new = new VBOVertex[size *20];
    int cube_size = size / 24;
    for (int i = 0; i < cube_size; i++) {
        for (int it = 0; it < 20; it++) {
            Matrix m = matrixs[it];
            for (int j = 0; j < 24; j++) {
                //cube_verts_new[i*480+it*24+j] = m * cube_verts_origin[i * 24 + j];
                Vec3f v_origin(cube_verts_origin[i * 24 + j].x, 
                                cube_verts_origin[i * 24 + j].y, 
                                cube_verts_origin[i * 24 + j].z);
                Vec3f v_new = m * v_origin;
                cube_verts_new[i * 480 + it * 24 + j].x = v_new.x();
                cube_verts_new[i * 480 + it * 24 + j].y = v_new.y();
                cube_verts_new[i * 480 + it * 24 + j].z = v_new.z();
                cube_verts_new[i * 480 + it * 24 + j].nx = cube_verts_origin[i * 24 + j].nx;
                cube_verts_new[i * 480 + it * 24 + j].ny = cube_verts_origin[i * 24 + j].ny;
                cube_verts_new[i * 480 + it * 24 + j].nz = cube_verts_origin[i * 24 + j].nz;            
                cube_verts_new[i * 480 + it * 24 + j].cx = cube_verts_origin[i * 24 + j].cx;
                cube_verts_new[i * 480 + it * 24 + j].cy = cube_verts_origin[i * 24 + j].cy;
                cube_verts_new[i * 480 + it * 24 + j].cz = cube_verts_origin[i * 24 + j].cz;
            }
        }
    }
    return cube_verts_new;
}
void IFS::setupCube() {

  HandleGLError("in setup cube");

  VBOVertex* cube_verts = new VBOVertex[cube_vert_size];
  VBOQuad* cube_face_indices = new VBOQuad[face_size];
  Vec3f* colors = new Vec3f[6];
  if (args->color) {
      
      for (int i = 0; i < 6; i += 2) {
          double x = args->mtrand.rand();//生成0，1的点
          double y = args->mtrand.rand();
          double z = args->mtrand.rand();
          Vec3f v(x, y, z);
          Vec3f v1(1.0f - x, 1.0f - y, 1.0f - z);
          colors[i] = v;
          colors[i + 1] = v1;
      }
  }else {
      colors[0] = Vec3f(0, 1, 1);
      colors[1] = Vec3f(1, 0, 0);
      colors[2] = Vec3f(1, 0, 1);
      colors[3] = Vec3f(0, 1, 0);
      colors[4] = Vec3f(1, 1, 0);
      colors[5] = Vec3f(0, 0, 1);

  }
  
  // initialize vertex data
// back face, cyan
  cube_verts[0] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, 0, -1), colors[0]);
  cube_verts[1] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 0, -1), colors[0]);
  cube_verts[2] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 0, -1), colors[0]);
  cube_verts[3] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, 0, -1), colors[0]);
  //position normal color
  // front face, red
  cube_verts[4] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, 0, 1), colors[1]);
  cube_verts[5] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 0, 1), colors[1]);
  cube_verts[6] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 0, 1), colors[1]);
  cube_verts[7] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, 0, 1), colors[1]);
  // bottom face, purple
  cube_verts[8] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, -1, 0), colors[2]);
  cube_verts[9] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, -1, 0), colors[2]);
  cube_verts[10] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, -1, 0), colors[2]);
  cube_verts[11] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, -1, 0), colors[2]);
  // top face, green
  cube_verts[12] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 1, 0), colors[3]);
  cube_verts[13] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 1, 0), colors[3]);
  cube_verts[14] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 1, 0), colors[3]);
  cube_verts[15] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 1, 0), colors[3]);
  // left face, yellow
  cube_verts[16] = VBOVertex(Vec3f(0, 0, 0), Vec3f(-1, 0, 0), colors[4]);
  cube_verts[17] = VBOVertex(Vec3f(0, 0, 1), Vec3f(-1, 0, 0), colors[4]);
  cube_verts[18] = VBOVertex(Vec3f(0, 1, 1), Vec3f(-1, 0, 0), colors[4]);
  cube_verts[19] = VBOVertex(Vec3f(0, 1, 0), Vec3f(-1, 0, 0), colors[4]);
  // right face, blue
  cube_verts[20] = VBOVertex(Vec3f(1, 0, 0), Vec3f(1, 0, 0), colors[5]);
  cube_verts[21] = VBOVertex(Vec3f(1, 0, 1), Vec3f(1, 0, 0), colors[5]);
  cube_verts[22] = VBOVertex(Vec3f(1, 1, 1), Vec3f(1, 0, 0), colors[5]);
  cube_verts[23] = VBOVertex(Vec3f(1, 1, 0), Vec3f(1, 0, 0), colors[5]);
  //// initialize vertex data
  //// back face, cyan
  //cube_verts[0] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
  //cube_verts[1] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
  //cube_verts[2] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
  //cube_verts[3] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, 0, -1), Vec3f(0, 1, 1));
  ////position normal color
  //// front face, red
  //cube_verts[4] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
  //cube_verts[5] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
  //cube_verts[6] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
  //cube_verts[7] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, 0, 1), Vec3f(1, 0, 0));
  //// bottom face, purple
  //cube_verts[8] = VBOVertex(Vec3f(0, 0, 0), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
  //cube_verts[9] = VBOVertex(Vec3f(0, 0, 1), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
  //cube_verts[10] = VBOVertex(Vec3f(1, 0, 1), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
  //cube_verts[11] = VBOVertex(Vec3f(1, 0, 0), Vec3f(0, -1, 0), Vec3f(1, 0, 1));
  //// top face, green
  //cube_verts[12] = VBOVertex(Vec3f(0, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
  //cube_verts[13] = VBOVertex(Vec3f(0, 1, 1), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
  //cube_verts[14] = VBOVertex(Vec3f(1, 1, 1), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
  //cube_verts[15] = VBOVertex(Vec3f(1, 1, 0), Vec3f(0, 1, 0), Vec3f(0, 1, 0));
  //// left face, yellow
  //cube_verts[16] = VBOVertex(Vec3f(0, 0, 0), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
  //cube_verts[17] = VBOVertex(Vec3f(0, 0, 1), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
  //cube_verts[18] = VBOVertex(Vec3f(0, 1, 1), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
  //cube_verts[19] = VBOVertex(Vec3f(0, 1, 0), Vec3f(-1, 0, 0), Vec3f(1, 1, 0));
  //// right face, blue
  //cube_verts[20] = VBOVertex(Vec3f(1, 0, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
  //cube_verts[21] = VBOVertex(Vec3f(1, 0, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
  //cube_verts[22] = VBOVertex(Vec3f(1, 1, 1), Vec3f(1, 0, 0), Vec3f(0, 0, 1));
  //cube_verts[23] = VBOVertex(Vec3f(1, 1, 0), Vec3f(1, 0, 0), Vec3f(0, 0, 1));

  for (int i = 0; i < args->iters; i++)
  {
      //变换Cube
      cube_verts = translateCube(cube_verts, cube_vert_size);
      cube_vert_size *= 20;
      //面个数变化
      face_size *= 20;
  }
  //操纵面
  cube_face_indices = new VBOQuad[face_size];
  for (int j = 0; j < face_size / 6; j++)
  {
      cube_face_indices[0 + j * 6] = VBOQuad(0 + j * 24, 1 + j * 24, 2 + j * 24, 3 + j * 24);
      cube_face_indices[1 + j * 6] = VBOQuad(4 + j * 24, 7 + j * 24, 6 + j * 24, 5 + j * 24);
      cube_face_indices[2 + j * 6] = VBOQuad(8 + j * 24, 11 + j * 24, 10 + j * 24, 9 + j * 24);
      cube_face_indices[3 + j * 6] = VBOQuad(12 + j * 24, 13 + j * 24, 14 + j * 24, 15 + j * 24);
      cube_face_indices[4 + j * 6] = VBOQuad(16 + j * 24, 17 + j * 24, 18 + j * 24, 19 + j * 24);
      cube_face_indices[5 + j * 6] = VBOQuad(20 + j * 24, 23 + j * 24, 22 + j * 24, 21 + j * 24);
  }

  // create a pointer for the vertex & index VBOs
 
  glGenBuffers(1, &cube_verts_VBO);
  glGenBuffers(1, &cube_face_indices_VBO);

  // copy the data to each VBO
  glBindBuffer(GL_ARRAY_BUFFER,cube_verts_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       //sizeof(VBOVertex) * NUM_CUBE_VERTS,
	       sizeof(VBOVertex) * cube_vert_size,
	       cube_verts,
	       GL_STATIC_DRAW); 
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,cube_face_indices_VBO); 
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       //sizeof(unsigned int) * NUM_CUBE_QUADS * 4,
	       sizeof(unsigned int) * face_size * 4,
	       cube_face_indices, GL_STATIC_DRAW);

  HandleGLError("leaving setup cube");

  delete [] cube_verts;
  delete [] cube_face_indices;
}


void IFS::drawCube() const {

  HandleGLError("in draw cube");

  // select the vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, cube_verts_VBO);
  // describe the layout of data in the vertex buffer
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(0));
  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer(GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(12));
  glEnableClientState(GL_COLOR_ARRAY);
  glColorPointer(3, GL_FLOAT, sizeof(VBOVertex), BUFFER_OFFSET(24));

  // select the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cube_face_indices_VBO);
  // draw this data
  glDrawElements(GL_QUADS, 
		 //NUM_CUBE_QUADS*4, 
		 face_size*4, 
		 GL_UNSIGNED_INT,
		 BUFFER_OFFSET(0));

  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_COLOR_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  HandleGLError("leaving draw cube");
}


void IFS::cleanupCube() {
  glDeleteBuffers(1, &cube_verts_VBO);
  glDeleteBuffers(1, &cube_face_indices_VBO);
}

// ====================================================================
// ====================================================================

