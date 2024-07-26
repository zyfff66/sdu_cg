#include "glCanvas.h"
#include <fstream>
#include <sstream>

#include "mesh.h"
#include "edge.h"
#include "vertex.h"
#include "triangle.h"


int Triangle::next_triangle_id = 0;

// helper for VBOs
#define BUFFER_OFFSET(i) ((char *)NULL + (i))
#define MaxDis 99999

// =======================================================================
// MESH DESTRUCTOR 
// =======================================================================

Mesh::~Mesh() {
  cleanupVBOs();

  // delete all the triangles
  std::vector<Triangle*> todo;
  for (triangleshashtype::iterator iter = triangles.begin();
    iter != triangles.end(); iter++) {
    Triangle *t = iter->second;
    todo.push_back(t);
  }
  int num_triangles = todo.size();
  for (int i = 0; i < num_triangles; i++) {
    removeTriangle(todo[i]);
  }
  // delete all the vertices
  int num_vertices = numVertices();
  for (int i = 0; i < num_vertices; i++) {
    delete vertices[i];
  }
}

// =======================================================================
// MODIFIERS:   ADD & REMOVE
// =======================================================================

Vertex* Mesh::addVertex(const Vec3f &position) {
  int index = numVertices();
  Vertex *v = new Vertex(index, position);
  vertices.push_back(v);
  v->clearQ();
  if (numVertices() == 1)
    bbox = BoundingBox(position,position);
  else 
    bbox.Extend(position);
  return v;
}


void Mesh::addTriangle(Vertex *a, Vertex *b, Vertex *c) {
  // create the triangle
    Triangle* t = new Triangle();
    // create the edges
    Edge* ea = new Edge(a, b, t);
    Edge* eb = new Edge(b, c, t);
    Edge* ec = new Edge(c, a, t);
    // point the triangle to one of its edges
    t->setEdge(ea);
    // connect the edges to each other
    ea->setNext(eb);
    eb->setNext(ec);
    ec->setNext(ea);
    assert(edges.find(std::make_pair(a, b)) == edges.end());
    assert(edges.find(std::make_pair(b, c)) == edges.end());
    assert(edges.find(std::make_pair(c, a)) == edges.end());
    // add the edges to the master list
    edges[std::make_pair(a, b)] = ea;
    edges[std::make_pair(b, c)] = eb;
    edges[std::make_pair(c, a)] = ec;
    // connect up with opposite edges (if they exist)
    edgeshashtype::iterator ea_op = edges.find(std::make_pair(b, a));
    edgeshashtype::iterator eb_op = edges.find(std::make_pair(c, b));
    edgeshashtype::iterator ec_op = edges.find(std::make_pair(a, c));
    if (ea_op != edges.end()) { ea_op->second->setOpposite(ea); }
    if (eb_op != edges.end()) { eb_op->second->setOpposite(eb); }
    if (ec_op != edges.end()) { ec_op->second->setOpposite(ec); }
    // add the triangle to the master list
    assert(triangles.find(t->getID()) == triangles.end());
    triangles[t->getID()] = t;
}


void Mesh::removeTriangle(Triangle *t) {
  Edge *ea = t->getEdge();
  Edge *eb = ea->getNext();
  Edge *ec = eb->getNext();
  Vertex *a = ea->getStartVertex();
  Vertex *b = eb->getStartVertex();
  Vertex *c = ec->getStartVertex();
  // remove these elements from master lists
  edges.erase(std::make_pair(a,b)); 
  edges.erase(std::make_pair(b,c)); 
  edges.erase(std::make_pair(c,a)); 
  triangles.erase(t->getID());
  // clean up memory
  delete ea;
  delete eb;
  delete ec;
  delete t;
}


// =======================================================================
// Helper functions for accessing data in the hash table
// =======================================================================

Edge* Mesh::getMeshEdge(Vertex *a, Vertex *b) const {
  edgeshashtype::const_iterator iter = edges.find(std::make_pair(a,b));
  if (iter == edges.end()) return NULL;
  return iter->second;
}

Vertex* Mesh::getChildVertex(Vertex *p1, Vertex *p2) const {
  vphashtype::const_iterator iter = vertex_parents.find(std::make_pair(p1,p2)); 
  if (iter == vertex_parents.end()) return NULL;
  return iter->second; 
}

void Mesh::setParentsChild(Vertex *p1, Vertex *p2, Vertex *child) {
  // assert (vertex_parents.find(std::make_pair(p1,p2)) == vertex_parents.end());
  vertex_parents[std::make_pair(p1,p2)] = child; 
}


// =======================================================================
// the load function parses very simple .obj files
// the basic format has been extended to allow the specification 
// of crease weights on the edges.
// =======================================================================

#define MAX_CHAR_PER_LINE 200

void Mesh::Load(const std::string &input_file) {

  std::ifstream istr(input_file.c_str());
  if (!istr) {
    std::cout << "ERROR! CANNOT OPEN: " << input_file << std::endl;
    return;
  }

  char line[MAX_CHAR_PER_LINE];
  std::string token, token2;
  float x,y,z;
  int a,b,c;
  int index = 0;
  int vert_count = 0;
  int vert_index = 1;

  // read in each line of the file
  while (istr.getline(line,MAX_CHAR_PER_LINE)) { 
    // put the line into a stringstream for parsing
    std::stringstream ss;
    ss << line;

    // check for blank line
    token = "";   
    ss >> token;
    if (token == "") continue;

    if (token == std::string("usemtl") ||token == std::string("g")) {
      vert_index = 1; 
      index++;
    } else if (token == std::string("v")) {
      vert_count++;
      ss >> x >> y >> z;
      addVertex(Vec3f(x,y,z));
    } else if (token == std::string("f")) {
      a = b = c = -1;
      ss >> a >> b >> c;
      a -= vert_index;
      b -= vert_index;
      c -= vert_index;
      assert (a >= 0 && a < numVertices());
      assert (b >= 0 && b < numVertices());
      assert (c >= 0 && c < numVertices());
      addTriangle(getVertex(a),getVertex(b),getVertex(c));
    } else if (token == std::string("e")) {
      a = b = -1;
      ss >> a >> b >> token2;
      // whoops: inconsistent file format, don't subtract 1
      assert (a >= 0 && a <= numVertices());
      assert (b >= 0 && b <= numVertices());
      if (token2 == std::string("inf")) x = 1000000; // this is close to infinity...
      x = atof(token2.c_str());
      Vertex *va = getVertex(a);
      Vertex *vb = getVertex(b);
      Edge *ab = getMeshEdge(va,vb);
      Edge *ba = getMeshEdge(vb,va);
      assert (ab != NULL);
      assert (ba != NULL);
      ab->setCrease(x);
      ba->setCrease(x);
      //计算入度
      va->setS(va->getS() + 1);
      vb->setS(vb->getS() + 1);
    } else if (token == std::string("vt")) {
    } else if (token == std::string("vn")) {
    } else if (token[0] == '#') {
    } else {
      printf ("LINE: '%s'",line);
    }
  }

  getAllQ();
}


// =======================================================================
// DRAWING
// =======================================================================

Vec3f ComputeNormal(const Vec3f &p1, const Vec3f &p2, const Vec3f &p3) {
  Vec3f v12 = p2;
  v12 -= p1;
  Vec3f v23 = p3;
  v23 -= p2;
  Vec3f normal;
  Vec3f::Cross3(normal,v12,v23);
  normal.Normalize();
  return normal;
}


void Mesh::initializeVBOs() {
  // create a pointer for the vertex & index VBOs
  glGenBuffers(1, &mesh_tri_verts_VBO);
  glGenBuffers(1, &mesh_tri_indices_VBO);
  glGenBuffers(1, &mesh_verts_VBO);
  glGenBuffers(1, &mesh_boundary_edge_indices_VBO);
  glGenBuffers(1, &mesh_crease_edge_indices_VBO);
  glGenBuffers(1, &mesh_other_edge_indices_VBO);
  setupVBOs();
}

void Mesh::setupVBOs() {
  HandleGLError("in setup mesh VBOs");
  setupTriVBOs();
  setupEdgeVBOs();
  HandleGLError("leaving setup mesh");
}


void Mesh::setupTriVBOs() {

  VBOTriVert* mesh_tri_verts;
  VBOTri* mesh_tri_indices;
  unsigned int num_tris = triangles.size();

  // allocate space for the data
  mesh_tri_verts = new VBOTriVert[num_tris*3];
  mesh_tri_indices = new VBOTri[num_tris];

  // write the vertex & triangle data
  unsigned int i = 0;
  triangleshashtype::iterator iter = triangles.begin();
  for (; iter != triangles.end(); iter++,i++) {
    Triangle *t = iter->second;
    Vec3f a = (*t)[0]->getPos();
    Vec3f b = (*t)[1]->getPos();
    Vec3f c = (*t)[2]->getPos();
    
    if (args->gouraud) {


      // =====================================
      // ASSIGNMENT: reimplement 
      Vec3f normal = ComputeNormal(a,b,c);
      mesh_tri_verts[i*3]   = VBOTriVert(a,normal);
      mesh_tri_verts[i*3+1] = VBOTriVert(b,normal);
      mesh_tri_verts[i*3+2] = VBOTriVert(c,normal);
      // =====================================


    } else {
      Vec3f normal = ComputeNormal(a,b,c);
      mesh_tri_verts[i*3]   = VBOTriVert(a,normal);
      mesh_tri_verts[i*3+1] = VBOTriVert(b,normal);
      mesh_tri_verts[i*3+2] = VBOTriVert(c,normal);
    }
    mesh_tri_indices[i] = VBOTri(i*3,i*3+1,i*3+2);
  }

  // cleanup old buffer data (if any)
  glDeleteBuffers(1, &mesh_tri_verts_VBO);
  glDeleteBuffers(1, &mesh_tri_indices_VBO);

  // copy the data to each VBO
  glBindBuffer(GL_ARRAY_BUFFER,mesh_tri_verts_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOTriVert) * num_tris * 3,
	       mesh_tri_verts,
	       GL_STATIC_DRAW); 
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_tri_indices_VBO); 
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
	       sizeof(VBOTri) * num_tris,
	       mesh_tri_indices, GL_STATIC_DRAW);

  delete [] mesh_tri_verts;
  delete [] mesh_tri_indices;

}


void Mesh::setupEdgeVBOs() {

  VBOVert* mesh_verts;
  VBOEdge* mesh_boundary_edge_indices;
  VBOEdge* mesh_crease_edge_indices;
  VBOEdge* mesh_other_edge_indices;

  mesh_boundary_edge_indices = NULL;
  mesh_crease_edge_indices = NULL;
  mesh_other_edge_indices = NULL;

  unsigned int num_verts = vertices.size();

  // first count the edges of each type
  num_boundary_edges = 0;
  num_crease_edges = 0;
  num_other_edges = 0;
  for (edgeshashtype::iterator iter = edges.begin();
       iter != edges.end(); iter++) {
    Edge *e = iter->second;
    int a = e->getStartVertex()->getIndex();
    int b = e->getEndVertex()->getIndex();
    if (e->getOpposite() == NULL) {
      num_boundary_edges++;
    } else {
      if (a < b) continue; // don't double count edges!
      if (e->getCrease() > 0) num_crease_edges++;
      else num_other_edges++;
    }
  }

  // allocate space for the data
  mesh_verts = new VBOVert[num_verts];
  if (num_boundary_edges > 0)
    mesh_boundary_edge_indices = new VBOEdge[num_boundary_edges];
  if (num_crease_edges > 0)
    mesh_crease_edge_indices = new VBOEdge[num_crease_edges];
  if (num_other_edges > 0)
    mesh_other_edge_indices = new VBOEdge[num_other_edges];

  // write the vertex data
  for (unsigned int i = 0; i < num_verts; i++) {
    mesh_verts[i] = VBOVert(vertices[i]->getPos());
  }

  // write the edge data
  int bi = 0;
  int ci = 0;
  int oi = 0; 
  for (edgeshashtype::iterator iter = edges.begin();
       iter != edges.end(); iter++) {
    Edge *e = iter->second;
    int a = e->getStartVertex()->getIndex();
    int b = e->getEndVertex()->getIndex();
    if (e->getOpposite() == NULL) {
      mesh_boundary_edge_indices[bi++] = VBOEdge(a,b);
    } else {
      if (a < b) continue; // don't double count edges!
      if (e->getCrease() > 0) 
	mesh_crease_edge_indices[ci++] = VBOEdge(a,b);
      else 
	mesh_other_edge_indices[oi++] = VBOEdge(a,b);
    }
  }
  assert (bi == num_boundary_edges);
  assert (ci == num_crease_edges);
  assert (oi == num_other_edges);

  // cleanup old buffer data (if any)
  glDeleteBuffers(1, &mesh_verts_VBO);
  glDeleteBuffers(1, &mesh_boundary_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_crease_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_other_edge_indices_VBO);

  // copy the data to each VBO
  glBindBuffer(GL_ARRAY_BUFFER,mesh_verts_VBO); 
  glBufferData(GL_ARRAY_BUFFER,
	       sizeof(VBOVert) * num_verts,
	       mesh_verts,
	       GL_STATIC_DRAW); 

  if (num_boundary_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_boundary_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_boundary_edges,
		 mesh_boundary_edge_indices, GL_STATIC_DRAW);
  }
  if (num_crease_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_crease_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_crease_edges,
		 mesh_crease_edge_indices, GL_STATIC_DRAW);
  }
  if (num_other_edges > 0) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,mesh_other_edge_indices_VBO); 
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		 sizeof(VBOEdge) * num_other_edges,
		 mesh_other_edge_indices, GL_STATIC_DRAW);
  }

  delete [] mesh_verts;
  delete [] mesh_boundary_edge_indices;
  delete [] mesh_crease_edge_indices;
  delete [] mesh_other_edge_indices;
}


void Mesh::cleanupVBOs() {
  glDeleteBuffers(1, &mesh_tri_verts_VBO);
  glDeleteBuffers(1, &mesh_tri_indices_VBO);
  glDeleteBuffers(1, &mesh_verts_VBO);
  glDeleteBuffers(1, &mesh_boundary_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_crease_edge_indices_VBO);
  glDeleteBuffers(1, &mesh_other_edge_indices_VBO);
}


void Mesh::drawVBOs() {

  HandleGLError("in draw mesh");

  // scale it so it fits in the window
  Vec3f center; bbox.getCenter(center);
  float s = 1/bbox.maxDim();
  glScalef(s,s,s);
  glTranslatef(-center.x(),-center.y(),-center.z());

  // this offset prevents "z-fighting" bewteen the edges and faces
  // so the edges will always win
  if (args->wireframe) {
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.1,4.0); 
  } 

  // ======================
  // draw all the triangles
  unsigned int num_tris = triangles.size();
  glColor3f(1,1,1);

  // select the vertex buffer
  glBindBuffer(GL_ARRAY_BUFFER, mesh_tri_verts_VBO);
  // describe the layout of data in the vertex buffer
  glEnableClientState(GL_VERTEX_ARRAY);
  glVertexPointer(3, GL_FLOAT, sizeof(VBOTriVert), BUFFER_OFFSET(0));
  glEnableClientState(GL_NORMAL_ARRAY);
  glNormalPointer(GL_FLOAT, sizeof(VBOTriVert), BUFFER_OFFSET(12));

  // select the index buffer
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_tri_indices_VBO);
  // draw this data
  glDrawElements(GL_TRIANGLES, 
		 num_tris*3,
		 GL_UNSIGNED_INT,
		 BUFFER_OFFSET(0));

  glDisableClientState(GL_NORMAL_ARRAY);
  glDisableClientState(GL_VERTEX_ARRAY);

  if (args->wireframe) {
    glDisable(GL_POLYGON_OFFSET_FILL); 
  }

  // =================================
  // draw the different types of edges
  if (args->wireframe) {
    glDisable(GL_LIGHTING);

    // select the vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, mesh_verts_VBO);
    // describe the layout of data in the vertex buffer
    glEnableClientState(GL_VERTEX_ARRAY);
    glVertexPointer(3, GL_FLOAT, sizeof(VBOVert), BUFFER_OFFSET(0));

    // draw all the boundary edges
    glLineWidth(3);
    glColor3f(1,0,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_boundary_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_boundary_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    // draw all the interior, crease edges
    glLineWidth(3);
    glColor3f(1,1,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_crease_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_crease_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    // draw all the interior, non-crease edges
    glLineWidth(1);
    glColor3f(0,0,0);
    // select the index buffer
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh_other_edge_indices_VBO);
    // draw this data
    glDrawElements(GL_LINES, num_other_edges*2, GL_UNSIGNED_INT, BUFFER_OFFSET(0));

    glDisableClientState(GL_VERTEX_ARRAY);
  }

  HandleGLError("leaving draw VBOs");
}

// =================================================================
// SUBDIVISION
// =================================================================

void Mesh::LoopSubdivision() {
  printf ("Subdivide the mesh!\n");
    //strp1:对边进行分类
    for (edgeshashtype::iterator iterVType = edges.begin(); iterVType != edges.end(); iterVType++) {
        Edge* eVType = iterVType->second;
        Vertex* v1 = eVType->getStartVertex();
        if (v1->getS() == 0) {
          v1->setVertexType(smooth);
        }
        else if(v1->getS() == 1){ 
          v1->setVertexType(dart); 
        }
        else if (v1->getS() == 2) {
            //是regular还是non-regular
            //regular有两种情况：
            //1.顶点不在边界上，要求顶点共与6条边相连，且两条crease每边有2条smooth边
            //2.顶点在边界上，周围相连边的数量为4
            //剩下的都是non-regular
          Edge* eTemp = eVType->getNext()->getNext();
          //计算与这条边相连的边
          std::vector<Edge*> edgeConnect;
          edgeConnect.push_back(eVType);
          bool boundary = false;
          while (eTemp->getOpposite() != NULL && eTemp->getOpposite() != eVType) {//crease边
              edgeConnect.push_back(eTemp->getOpposite());
              eTemp = eTemp->getOpposite()->getNext()->getNext();
          }
            if (eTemp->getOpposite() == NULL) {
                boundary = true;//顶点在边界上
                eTemp = eVType;
                while (eTemp->getOpposite() != NULL) {
                    eTemp = eTemp->getOpposite()->getNext();
                    edgeConnect.push_back(eTemp);
                }
            }
            if (boundary) {
              //顶点在边界上
              //如果顶点在边界上，那么只要周围相连边的数量为4即是regular
              if (edgeConnect.size() == 4) {
                v1->setVertexType(reg_crease);
              }
              else { 
                v1->setVertexType(non_reg_crease); 
              }
            }
            else {
              //不在边界上
              if(edgeConnect.size() != 6){       
                v1->setVertexType(non_reg_crease); 
              }
              else {
                //如果顶点不在边界上，要求顶点共与6条边相连，且两条crease一边有2条smooth边
                if (edgeConnect[0]->getCrease() > 0 && edgeConnect[3]->getCrease() > 0 &&edgeConnect[1]->getCrease() <= 0 && edgeConnect[2]->getCrease() <= 0 && edgeConnect[4]->getCrease() <= 0 && edgeConnect[5]->getCrease() <= 0) {
                  v1->setVertexType(reg_crease);//情况1
                }
                else{ 
                  v1->setVertexType(non_reg_crease); 
                }
              }
            }
        }
        else if (v1->getS() >= 3) {
            v1->setVertexType(corner);
        }
    }
    printf("第一步结束\n");
    //二、为每条边计算一个新的顶点
    //1.边界边1/2
    //2.crease边查表
    //3.普通边按smooth edge方式计算
    for (edgeshashtype::iterator iterEdges = edges.begin(); iterEdges != edges.end(); iterEdges++) {
        Edge* e = iterEdges->second;
        Vertex* v1 = e->getStartVertex();
        Vertex* v2 = e->getEndVertex();
        Vec3f new_vertex;
        if (e->getOpposite() == NULL) {//边界边，新点按1/2计算
            new_vertex = (v1->getPos() + v2->getPos()) * 0.5f;
            Vertex* New_Vertex = addVertex(new_vertex);
            if (e->getCrease() > 0) {
                New_Vertex->setS(2);
            }
            
            setParentsChild(v1, v2, New_Vertex);
        }
        else {
            //crease边 or 普通边 需要额外用到另外两个顶点
            Triangle* t1 = e->getTriangle();
            Triangle* t2 = e->getOpposite()->getTriangle();
            Vertex* v3, * v4;
            if ((*t1)[0]->getIndex() != v1->getIndex() && (*t1)[0]->getIndex() != v2->getIndex()) {
                v3 = (*t1)[0];
            }
            else if ((*t1)[1]->getIndex() !=v1->getIndex() && (*t1)[1]->getIndex() != v2->getIndex()) {
                v3 = (*t1)[1];
            }
            else if ((*t1)[2]->getIndex() != v1->getIndex() && (*t1)[2]->getIndex() != v2->getIndex()) {
                v3 = (*t1)[2];
            }
            else {  v3 = v1; }

            if ((*t2)[0]->getIndex() != e->getOpposite()->getStartVertex()->getIndex() && (*t2)[0]->getIndex() != e->getOpposite()->getEndVertex()->getIndex()) {
                v4 = (*t2)[0];
            }
            else if ((*t2)[1]->getIndex() != e->getOpposite()->getStartVertex()->getIndex() && (*t2)[1]->getIndex() != e->getOpposite()->getEndVertex()->getIndex()) {
                v4 = (*t2)[1];
            }
            else if ((*t2)[2]->getIndex() != e->getOpposite()->getStartVertex()->getIndex() && (*t2)[2]->getIndex() != e->getOpposite()->getEndVertex()->getIndex()) {
                v4 = (*t2)[2];
            }
            else {  v4 = v1; }

            if (e->getCrease() > 0) {
                //crease边,查表
                //操作3，查表有四种情况 2+2
                if ((v1->getVertexType() == reg_crease && v2->getVertexType() == non_reg_crease) ||
                    (v1->getVertexType() == reg_crease && v2->getVertexType() == corner)) {
                    new_vertex = 0.625f * v1->getPos() + 0.375f * v2->getPos();
                }
                else if ((v1->getVertexType() == non_reg_crease && v2->getVertexType() == reg_crease) ||
                    (v1->getVertexType() == corner && v2->getVertexType() == reg_crease)) 
                {new_vertex = 0.625f * v2->getPos() + 0.375f * v1->getPos();}
                //操作2 ，有五种情况
                else if ((v1->getVertexType() == reg_crease && v2->getVertexType() == reg_crease)
                    || (v1->getVertexType() == non_reg_crease && v2->getVertexType() == non_reg_crease)
                    || (v1->getVertexType() == corner && v2->getVertexType() == corner)
                    || (v1->getVertexType() == non_reg_crease && v2->getVertexType() == corner)
                    || (v1->getVertexType() == corner && v2->getVertexType() == non_reg_crease)) 
                {new_vertex = 0.5f * v1->getPos() + 0.5f * v2->getPos();}
                //其他均为操作1
                else { new_vertex = 0.375f * v1->getPos() + 0.375f * v2->getPos() + 0.125f * v3->getPos() + 0.125f * v4->getPos();}
            }
            //普通边 同操作1 
            else {
                new_vertex = 0.375f * v1->getPos() + 0.375f * v2->getPos() + 0.125f * v3->getPos() + 0.125f * v4->getPos();
            }

            Vertex* New_Vertex = addVertex(new_vertex);
            if (e->getCrease() > 0) {
                New_Vertex->setS(2);
            }
            setParentsChild(v1, v2, New_Vertex);
        }
    }
    std::cout << "第二步结束" << std::endl;

    //三、根据旧顶点类型，更新旧顶点的位置
    //情况1:顶点为边界点
    //情况2：顶点不为边界点，此时根据顶点的定义可以在分为三类
    int c = 0;
    std::vector<Vertex*> vertex_updated;
    for (edgeshashtype::iterator iterEdges = edges.begin(); iterEdges != edges.end(); ++iterEdges) {
        Edge* e= iterEdges->second;
        Vertex* v1 = e->getStartVertex();
        bool updated = false;
        for (int i = 0; i < vertex_updated.size(); i++) {
            if (v1->getIndex() == vertex_updated[i]->getIndex()) { 
              updated = true; break; 
            }
        }
        if (updated) { continue; }

        //寻找v1周围顶点
        std::vector<Vertex*> vertex_aroundv1;//存储在v1周围的点
        Edge* eTemp = e->getNext()->getNext();
        bool boundary = false;
        while (1) {
            if (eTemp->getOpposite() == NULL) {
                vertex_aroundv1.push_back(eTemp->getStartVertex());
                eTemp = e;
                boundary = true;
                while (eTemp->getOpposite() != NULL) {
                    eTemp=eTemp->getOpposite()->getNext();
                }
                vertex_aroundv1.push_back(eTemp->getEndVertex());
                break;
            }
            else if (eTemp->getOpposite() == e) { vertex_aroundv1.push_back(eTemp->getStartVertex()); break; }
            else { vertex_aroundv1.push_back(eTemp->getStartVertex()); eTemp = eTemp->getOpposite()->getNext()->getNext(); }
        }
        //计算v1的新位置
        if (boundary) {//顶点周围有边界的情况
            int n_v = vertex_aroundv1.size();
            Vec3f new_pos = 0.125 * (vertex_aroundv1[n_v-2]->getPos() + vertex_aroundv1[n_v - 1]->getPos()) + 0.75 * v1->getPos();
            v1->setNewPos(new_pos);
            vertex_updated.push_back(v1);
        }
        else {//没有边界，分三种情况
            if (v1->getVertexType() == corner) {//若为corner，不改变
                v1->setNewPos(v1->getPos());
            }
            else if (v1->getVertexType() == reg_crease || v1->getVertexType() == non_reg_crease) {//若为crease，先找两条crease边，再计算
                std::vector<Vertex*> vertex_crease;
                for (int i = 0; i < vertex_aroundv1.size(); i++) {
                    if (edges.find(std::make_pair(v1, vertex_aroundv1[i]))->second->getCrease() > 0)
                        vertex_crease.push_back(vertex_aroundv1[i]);
                }
                //std::cout << vertex_crease.size() << std::endl;
                Vec3f new_pos = 0.125 * (vertex_crease[0]->getPos() + vertex_crease[1]->getPos()) + 0.75 * v1->getPos();
                v1->setNewPos(new_pos);
            }
            else {//若为其他类型顶点
                int n = vertex_aroundv1.size();
                double beta = (0.625 - pow((0.375 + 0.25 * cos(2 * 3.1415926 / n)), 2)) / n;
                Vec3f new_pos = v1->getPos() * (1 - n * beta);
                for (int i = 0; i < n; i++) {
                    new_pos+= vertex_aroundv1[i]->getPos()* beta; 
                }
                v1->setNewPos(new_pos);
            }
            vertex_updated.push_back(v1);
        }
    }
    //更新
    for (int i = 0; i < vertex_updated.size(); i++) {                     
        vertex_updated[i]->updatePos();
    }
    std::cout << "第三步结束" << std::endl;

    //四、更新网格
    //add triangle mesh
    triangleshashtype tempTriangles = triangles;
    triangles.clear();//clear old triangle meshes
    edges.clear();//clear information of old edges
    triangleshashtype::iterator tIterator = tempTriangles.begin();
    for (; tIterator != tempTriangles.end(); tIterator++)
    {
        Triangle* t = tIterator->second;
        Vertex* v1, * v2, * v3, * c12, * c23, * c13;
        v1 = (*t)[0]; v2 = (*t)[1]; v3 = (*t)[2];
        c12 = getChildVertex(v1, v2);
        c23 = getChildVertex(v2, v3);
        c13 = getChildVertex(v1, v3);
        if (c12 == NULL || c23 == NULL || c13 == NULL) break;

        addTriangle(c12, c23, c13);
        addTriangle(v1, c12, c13);
        addTriangle(v2, c23, c12);
        addTriangle(v3, c13, c23);
    }

    //set crease
    for (edgeshashtype::iterator eIterator3 = edges.begin();
        eIterator3 != edges.end(); eIterator3++)
    {
        Edge* e = eIterator3->second;
        std::pair<Vertex*, Vertex*> ePair = eIterator3->first;

        if (vertex_parents.find(ePair) == vertex_parents.end()) continue;
        if (e->getCrease())
        {
            Vertex* childVertex = vertex_parents.find(ePair)->second;
            assert(edges.find(std::make_pair(e->getStartVertex(), childVertex)) != edges.end());
            edges.find(std::make_pair(e->getStartVertex(), childVertex))->second->setCrease(e->getCrease());
            assert(edges.find(std::make_pair(childVertex, e->getEndVertex())) != edges.end());
            edges.find(std::make_pair(childVertex, e->getEndVertex()))->second->setCrease(e->getCrease());
        }
    }
    vertex_parents.clear();
    std::cout << "第四步结束" << std::endl;

}
void Mesh::ButterflySubdivision(){
    printf("ButterflySubdivision the mesh!\n");

    for (edgeshashtype::iterator iterEdges = edges.begin(); iterEdges != edges.end(); iterEdges++) {
        Edge* e = iterEdges->second;
        Vertex* v1 = e->getStartVertex();
        Vertex* v2 = e->getEndVertex();

        if (getChildVertex(v1, v2) != NULL) {
            continue;
        }

        if (e->getOpposite() == NULL) {
            //情况d (1)边界
            Edge* boundary1;
            Edge* boundary2;
            Edge* e_temp = e->getNext()->getNext();
            while (e_temp->getOpposite() != NULL) {
                e_temp = e_temp->getOpposite()->getNext()->getNext();
            }
            boundary1 = e_temp;
            e_temp = e->getNext();
            while (e_temp->getOpposite() != NULL) {
                e_temp = e_temp->getOpposite()->getNext();
            }
            boundary2 = e_temp;

            Vec3f new_vertex = 0.5625 * (v1->getPos() + v2->getPos()) - 0.0625 * (boundary1->getStartVertex()->getPos() + boundary2->getEndVertex()->getPos());
            Vertex* New_Vertex = addVertex(new_vertex);
            setParentsChild(v1, v2, New_Vertex);
        }
        else if(e->getCrease()>0){
            //情况d (2)crease
            //找到v1 v2周围的所有crease 并平分-1/16这个权重
            std::vector<Edge*> v1_crease;
            std::vector<Edge*> v2_crease;
            //v1周围
            Edge* e_temp = e->getNext()->getNext();
            while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
                if (e_temp->getCrease() > 0) {//crease边
                    v1_crease.push_back(e_temp);
                }
                e_temp = e_temp->getOpposite()->getNext()->getNext();
            }
            if (e_temp->getOpposite() == NULL) {
                e_temp = e->getOpposite()->getNext();
                while (e_temp->getOpposite() != NULL) {
                    if (e_temp->getCrease() > 0) {//crease边
                        v1_crease.push_back(e_temp);
                    }
                    e_temp = e_temp->getOpposite()->getNext();
                }
                if (e_temp->getCrease() > 0) {//crease边
                    v1_crease.push_back(e_temp);
                }
            }
            //v2周围
            e = e->getOpposite();
            e_temp = e->getNext()->getNext();
            while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
                if (e_temp->getCrease() > 0) {//crease边
                    v2_crease.push_back(e_temp);
                }
                e_temp = e_temp->getOpposite()->getNext()->getNext();
            }
            if (e_temp->getOpposite() == NULL) {
                e_temp = e->getOpposite()->getNext();
                while (e_temp->getOpposite() != NULL) {
                    if (e_temp->getCrease() > 0) {//crease边
                        v2_crease.push_back(e_temp);
                    }
                    e_temp = e_temp->getOpposite()->getNext();
                }
                if (e_temp->getCrease() > 0) {//crease边
                    v2_crease.push_back(e_temp);
                }
            }
            Vec3f v1_new, v2_new;
            //计算v1相关的边权重
            if (v1_crease.size() == 0) {
                v1_new = v1->getPos() * 0.5;
            }
            else {
                float v1_weight = -0.0625 / v1_crease.size();
                for (int i = 0; i < v1_crease.size(); i++) {
                    v1_new += v1_weight * v1_crease[i]->getStartVertex()->getPos();
                }
                v1_new+= v1->getPos() * 0.5625;
            }
            //计算v2相关边权重
            if (v2_crease.size() == 0) {
                v2_new = v2->getPos() * 0.5;
            }
            else {
                float v2_weight = -0.0625 / v2_crease.size();
                for (int i = 0; i < v2_crease.size(); i++) {
                    v2_new += v2_weight * v2_crease[i]->getStartVertex()->getPos();
                }
                v2_new += v2->getPos() * 0.5625;
            }
            Vec3f new_pos = v1_new + v2_new;
            Vertex* New_Vertex = addVertex(new_pos);
            setParentsChild(v1, v2, New_Vertex);
        }
        else {
            //正常边 情况(a)(b)(c)
            //找v1周围边
            std::vector<Edge*> v1_around;
            std::vector<Edge*> v2_around;
            Edge* e_temp = e->getNext()->getNext();
            while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
                v1_around.push_back(e_temp);
                e_temp = e_temp->getOpposite()->getNext()->getNext();
            }
            if (e_temp->getOpposite() == NULL) {
                v1_around.push_back(e_temp);
                e_temp = e->getOpposite()->getNext();
                std::vector<Edge*> v1_back;
                while (e_temp->getOpposite() != NULL) {
                    v1_back.push_back(e_temp);
                    e_temp = e_temp->getOpposite()->getNext();
                }
                v1_back.push_back(e_temp);
                //倒序存储
                for (int i = v1_back.size() - 1; i >= 0; i--) {
                    v1_around.push_back(v1_back[i]);
                }
            }
            //同理 寻找v2周围的边
            e = e->getOpposite();
            e_temp = e->getNext()->getNext();
            while (e_temp->getOpposite() != NULL && e_temp->getOpposite() != e) {
                v2_around.push_back(e_temp);
                e_temp = e_temp->getOpposite()->getNext()->getNext();
            }
            if (e_temp->getOpposite() == NULL) {
                v2_around.push_back(e_temp);
                e_temp = e->getOpposite()->getNext();
                std::vector<Edge*> v2_back;
                while (e_temp->getOpposite() != NULL) {
                    v2_back.push_back(e_temp);
                    e_temp = e_temp->getOpposite()->getNext();
                }
                v2_back.push_back(e_temp);
                //倒序存储
                for (int i = v2_back.size() - 1; i >= 0; i--) {
                    v2_around.push_back(v2_back[i]);
                }
            }
            //边找完了  分情况计算新顶点
            Vec3f new_pos;
            if (v1_around.size() == 5 && v2_around.size() == 5) {
                //情况a
                new_pos += (v1->getPos() + v2->getPos()) * 0.5;
                new_pos += (v1_around[0]->getStartVertex()->getPos() + v1_around[4]->getStartVertex()->getPos()) * 0.125;
                new_pos += (v1_around[1]->getStartVertex()->getPos() + v1_around[3]->getStartVertex()->getPos()) * (-0.0625);
                new_pos += (v2_around[1]->getStartVertex()->getPos() + v2_around[3]->getStartVertex()->getPos()) * (-0.0625);
            }
            else if (v1_around.size() != 5 && v2_around.size() == 5) {
                //情况b(1)
                new_pos += v1->getPos() * 0.75;
                if (v1_around.size() == 2) {//k==3    
                    new_pos += v2->getPos() * (5 /12)+ (v1_around[0]->getStartVertex()->getPos() + v1_around[1]->getStartVertex()->getPos()) * (-1 / 12);
                }
                else if (v1_around.size() == 3) {//k==4            
                    new_pos += v2->getPos() * 0.375+ v1_around[1]->getStartVertex()->getPos() * (-0.125);
                }
                else {
                    double k = (v1_around.size() + 1);
                    new_pos += v2->getPos() * (1.75 / k);
                    for (int i = 0; i < v1_around.size(); i++) {
                        int kk = i + 1;
                        double weight = (0.25 + cos(2 * 3.1415926 * kk / k) + 0.5 * cos(4 * kk * 3.1415926 / k)) / k;
                        new_pos += v1_around[i]->getStartVertex()->getPos() * weight;
                    }
                }
            }
            else if (v1_around.size() == 5 && v2_around.size() != 5) {
                //情况b(2) 正好相反
                new_pos += v2->getPos() * 0.75;
                if (v2_around.size() == 2) {//k==3    
                    new_pos += v1->getPos() * (5 / 12) + (v2_around[0]->getStartVertex()->getPos() + v2_around[1]->getStartVertex()->getPos()) * (-1 / 12);
                }
                else if (v2_around.size() == 3) {//k==4            
                    new_pos += v1->getPos() * 0.375 + v2_around[1]->getStartVertex()->getPos() * (-0.125);
                }
                else {
                    double k = (v2_around.size() + 1);
                    new_pos += v1->getPos() * (1.75 / k);
                    for (int i = 0; i < v2_around.size(); i++) {
                        int kk = i + 1;
                        double weight = (0.25 + cos(2 * 3.1415926 * kk / k) + 0.5 * cos(4 * kk * 3.1415926 / k)) / k;
                        new_pos += v2_around[i]->getStartVertex()->getPos() * weight;
                    }
                }
            }
            else {
                //情况c,对两个点都执行一次b,之后取平均值
                Vec3f new_pos_v1, new_pos_v2;
                //v1
                new_pos_v1 += v1->getPos() * 0.75;
                if (v1_around.size() == 2) {//k==3    
                    new_pos_v1 += v2->getPos() * (5 / 12) + (v1_around[0]->getStartVertex()->getPos() + v1_around[1]->getStartVertex()->getPos()) * (-1 / 12);
                }
                else if (v1_around.size() == 3) {//k==4            
                    new_pos_v1 += v2->getPos() * 0.375 + v1_around[1]->getStartVertex()->getPos() * (-0.125);
                }
                else {
                    double k = (v1_around.size() + 1);
                    new_pos_v1 += v2->getPos() * (1.75 / k);
                    for (int i = 0; i < v1_around.size(); i++) {
                        int kk = i + 1;
                        double weight = (0.25 + cos(2 * 3.1415926 * kk / k) + 0.5 * cos(4 * kk * 3.1415926 / k)) / k;
                        new_pos_v1 += v1_around[i]->getStartVertex()->getPos() * weight;
                    }
                }

                //v2
                new_pos_v2 += v2->getPos() * 0.75;
                if (v2_around.size() == 2) {//k==3    
                    new_pos_v2 += v1->getPos() * (5 / 12) + (v2_around[0]->getStartVertex()->getPos() + v2_around[1]->getStartVertex()->getPos()) * (-1 / 12);
                }
                else if (v2_around.size() == 3) {//k==4            
                    new_pos_v2 += v1->getPos() * 0.375 + v2_around[1]->getStartVertex()->getPos() * (-0.125);
                }
                else {
                    double k = (v2_around.size() + 1);
                    new_pos_v2 += v1->getPos() * (1.75 / k);
                    for (int i = 0; i < v2_around.size(); i++) {
                        int kk = i + 1;
                        double weight = (0.25 + cos(2 * 3.1415926 * kk / k) + 0.5 * cos(4 * kk * 3.1415926 / k)) / k;
                        new_pos_v2 += v2_around[i]->getStartVertex()->getPos() * weight;
                    }
                }
                new_pos = (new_pos_v1 + new_pos_v2)*0.5;
            }
            Vertex* New_Vertex = addVertex(new_pos);
            setParentsChild(v1, v2, New_Vertex);

        }
    }

    //更新网格
    //add triangle mesh
    triangleshashtype tempTriangles = triangles;
    triangles.clear();//clear old triangle meshes
    edges.clear();//clear information of old edges
    triangleshashtype::iterator tIterator = tempTriangles.begin();
    for (; tIterator != tempTriangles.end(); tIterator++)
    {
        Triangle* t = tIterator->second;
        Vertex* v1, * v2, * v3, * c12, * c23, * c13;
        v1 = (*t)[0]; v2 = (*t)[1]; v3 = (*t)[2];
        c12 = getChildVertex(v1, v2);
        c23 = getChildVertex(v2, v3);
        c13 = getChildVertex(v1, v3);
        if (c12 == NULL || c23 == NULL || c13 == NULL) break;

        addTriangle(c12, c23, c13);
        addTriangle(v1, c12, c13);
        addTriangle(v2, c23, c12);
        addTriangle(v3, c13, c23);
    }

    //set crease
    for (edgeshashtype::iterator eIterator3 = edges.begin();
        eIterator3 != edges.end(); eIterator3++)
    {
        Edge* e = eIterator3->second;
        std::pair<Vertex*, Vertex*> ePair = eIterator3->first;

        if (vertex_parents.find(ePair) == vertex_parents.end()) continue;
        if (e->getCrease())
        {
            Vertex* childVertex = vertex_parents.find(ePair)->second;
            assert(edges.find(std::make_pair(e->getStartVertex(), childVertex)) != edges.end());
            edges.find(std::make_pair(e->getStartVertex(), childVertex))->second->setCrease(e->getCrease());
            assert(edges.find(std::make_pair(childVertex, e->getEndVertex())) != edges.end());
            edges.find(std::make_pair(childVertex, e->getEndVertex()))->second->setCrease(e->getCrease());
        }
    }
    vertex_parents.clear();
}

void Mesh::simply(Edge* e1,Vec3f new_vec) {
    Vertex* v1=e1->getStartVertex();
    Vertex* v2=e1->getEndVertex();
    //需要修改的边所在的三角形（需删除的三角形）
    Triangle* triangle1 = e1->getTriangle();
    //另外半边的三角形
    Triangle* triangle2 = e1->getOpposite()->getTriangle();
    //寻找需要修改的三角形
    std::vector<Triangle*> changedTriangles;
    Edge* tempEdge = e1->getNext()->getOpposite();
    while (tempEdge != NULL && tempEdge != e1) {
        changedTriangles.push_back(tempEdge->getTriangle());
        tempEdge = tempEdge->getNext()->getOpposite();
    }
    //去掉最后一个重复的
    if (tempEdge != NULL) {
        if (tempEdge->getTriangle()->getID() == e1->getTriangle()->getID() || tempEdge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
            if (changedTriangles.size() != 0)
                changedTriangles.pop_back();
        }
    }
    //选择从对边开始的
    tempEdge = e1->getOpposite()->getNext()->getOpposite();
    while (tempEdge != NULL && tempEdge != e1->getOpposite()) {
        changedTriangles.push_back(tempEdge->getTriangle());
        tempEdge = tempEdge->getNext()->getOpposite();
    }

    if (tempEdge != NULL) {
        if (tempEdge->getTriangle()->getID() == e1->getTriangle()->getID() || tempEdge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
            if (changedTriangles.size() != 0)
                changedTriangles.pop_back();
        }
    }

    Vertex* v_new = addVertex(new_vec);
    //涉及到的点
    std::vector<Vertex*> changedVertex;
    tempEdge = e1->getNext();
    Vertex* tempVertex = tempEdge->getEndVertex();
    while (1) {
        if (tempEdge->getOpposite() == NULL) {
            changedVertex.push_back(tempVertex);
            break;
        }
        if (tempVertex == e1->getOpposite()->getNext()->getEndVertex()) {
            break;
        }
        changedVertex.push_back(tempVertex);
        tempEdge = tempEdge->getOpposite()->getNext();
        tempVertex = tempEdge->getEndVertex();
    }

    tempEdge = e1->getOpposite()->getNext();
    tempVertex = tempEdge->getEndVertex();
    while (1) {
        if (tempEdge->getOpposite() == NULL) {
            changedVertex.push_back(tempVertex);
            break;
        }
        //终止条件
        if (tempVertex == e1->getNext()->getEndVertex()) {
            break;
        }
        changedVertex.push_back(tempVertex);
        tempEdge = tempEdge->getOpposite()->getNext();
        tempVertex = tempEdge->getEndVertex();
    }
    //判断是否重复即是否有两个相同元素
    std::vector<Vertex*>::iterator it1 = changedVertex.begin();
    std::vector<Vertex*>::iterator it2;
    for (; it1 < changedVertex.end(); it1++) {
        int count = 0;
        for (it2 = changedVertex.begin(); it2 < changedVertex.end(); it2++) {
            if ((*it1)->getIndex() == (*it2)->getIndex()) {
                count++;
            }
        }
        if (count > 1){
          printf( "存在重复，重新选择边\n") ;
          e1->setIsOK(false);
          return;
        }
    }
    //去除三角形
    removeTriangle(triangle1);
    removeTriangle(triangle2);
    //更新三角形
    std::vector<Triangle*>::iterator iter_tri = changedTriangles.begin();
    for (; iter_tri != changedTriangles.end(); iter_tri++) {
        Triangle* temp_tri = (*iter_tri);
        Vertex* a = (*temp_tri)[0];
        Vertex* b = (*temp_tri)[1];
        Vertex* c = (*temp_tri)[2];
        //先删除
        removeTriangle(temp_tri);

        if (a->getIndex() == v1->getIndex() || a->getIndex() == v2->getIndex()) {
            a = v_new;
        }
        else if (b->getIndex() == v1->getIndex() || b->getIndex() == v2->getIndex()) {
            b = v_new;
        }
        else if (c->getIndex() == v1->getIndex() || c->getIndex() == v2->getIndex()) {
            c = v_new;
        }

        //后添加
        addTriangle(a, b, c);
    }
}

int Mesh::getGoodEdge(){
  int size=0;
  for (edgeshashtype::iterator iter = edges.begin(); iter != edges.end(); iter++) {
        Edge* e = iter->second;
        if (!(e->getIsOK())) {
            continue;
        }
        if (e->getOpposite() == NULL) {
            e->setIsOK(false);
            continue;
        }
        else {
            Triangle* t1 = e->getTriangle();
            Triangle* t2 = e->getOpposite()->getTriangle();
            //此边所在的三角形位于边界
            if (t1->getEdge()->getOpposite() == NULL || t1->getEdge()->getNext()->getOpposite() == NULL || t1->getEdge()->getNext()->getNext()->getOpposite() == NULL||
                t2->getEdge()->getOpposite() == NULL || t2->getEdge()->getNext()->getOpposite() == NULL || t2->getEdge()->getNext()->getNext()->getOpposite() == NULL) {
                e->setIsOK(false);
                continue;
            }
        }
        size++;        
    }
    return size;
}

void Mesh::getAllQ() {
    //计算每个顶点的Q
    //对面进行迭代，并计算Kp,给Vertex设置Q变量
    for (triangleshashtype::iterator iter = triangles.begin(); iter != triangles.end(); iter++) {
        Triangle* t = iter->second;
        Vec3f p1 = (*t)[0]->getPos();
        Vec3f p2 = (*t)[1]->getPos();
        Vec3f p3 = (*t)[2]->getPos();
        Vec3f normal = ComputeNormal(p1, p2, p3);//计算法向量
        float d = -normal.Dot3(p1);
        //计算每个平面的Kp
        Matrix Kp;
        //p[a,b,c,d]ax+by+cz+d=0
        float plane[4] = { normal.x(),normal.y(),normal.z(),d };
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                Kp.set(i, j, (double)(plane[i] * plane[j]));
            }
        }
        //对该平面上的每个顶点操作，对Kp进行累积
        (*t)[0]->addQ(Kp);
        (*t)[1]->addQ(Kp);
        (*t)[2]->addQ(Kp);
    }
}
void Mesh::getAllDistance(){
  for (int i = 0; i < allPairs.size(); i++) {
        allPairs[i].computeDistance(MaxDis);
        if (allPairs[i].getDistance() >= MaxDis) {
            allPairs[i].getEdge()->setIsOK(false);
        }
    }
}
bool cmp(Pair a, Pair b) { return a.getDistance() < b.getDistance(); }

void Mesh::getAllPairs(){
  for (edgeshashtype::iterator iter = edges.begin(); iter != edges.end(); iter++) {
        Edge* e_ab = iter->second;
        Vertex* a, * b;
       
        a = e_ab->getStartVertex();
        b = e_ab->getEndVertex();
        if (!(e_ab->getIsOK())) {
            continue;
        }
        if (e_ab->getOpposite() == NULL) {
            e_ab->setIsOK(false);
            continue;
        }
        else {
            Triangle* t1 = e_ab->getTriangle();
            Triangle* t2 = e_ab->getOpposite()->getTriangle();
            if (t1->getEdge()->getOpposite() == NULL || t1->getEdge()->getNext()->getOpposite() == NULL || t1->getEdge()->getNext()->getNext()->getOpposite() == NULL||
                t2->getEdge()->getOpposite() == NULL || t2->getEdge()->getNext()->getOpposite() == NULL || t2->getEdge()->getNext()->getNext()->getOpposite() == NULL) {//此边所在的三角形位于边界
                e_ab->setIsOK(false);
                continue;
            }
        }
        Pair pair(a, b, e_ab);
        allPairs.push_back(pair);      
        
    }
}
void Mesh::Simplification(int target_tri_count) {
  vertex_parents.clear();

  printf ("Simplify the mesh! %d -> %d\n", numTriangles(), target_tri_count);

  // 随机挑选
  MTRand rand;
  int reChoose=0;
  while(numTriangles()>target_tri_count){
    //没有可以选择的边
    if(getGoodEdge()==0)
    {
      printf("不存在可以选择的边了，无法优化！\n");
      break;
    }
    //重选的次数过多
    if(reChoose>15)break;
    int edge_id = rand.randInt(numEdges() - 1);//随机挑选一个边
    //找到这个三角形
    edgeshashtype::iterator iter = edges.begin();
    for (int i = 0; i < edge_id; i++) {
        if (iter != edges.end()) {
            iter++;
        }
    }
    //要删除的边
    Edge* e1=iter->second;
    if (e1->getIsOK()) {//选择这个边，不会存在相同的
        Vertex* v1 = e1->getStartVertex();
        Vertex* v2 = e1->getEndVertex();
        //计算新点位置
        Vec3f new_vec = (v1->getPos() + v2->getPos()) * 0.5;
        simply(e1,new_vec);
        reChoose = 0;
    }
    else {
      reChoose++;
    }
  }
  
}
void Mesh::simply(Pair p){
    Edge* e1 = p.getEdge();//需要修改的边
    Edge* e2 = e1->getOpposite();
    Edge* e_temp = e1->getNext()->getNext();
    while (e_temp->getOpposite() != e1) {
        if (e_temp->getOpposite() == NULL)//此边的顶点在边界上
        {
            e1->setIsOK(false);
            e2->setIsOK(false);
            return;
        }
        e_temp = e_temp->getOpposite()->getNext()->getNext();
    }
    //另一个顶点
    e_temp = e2->getNext()->getNext();
    while (e_temp->getOpposite() != e2) {
        if (e_temp->getOpposite() == NULL)//此边的顶点在边界上
        {
            e1->setIsOK(false);
            e2->setIsOK(false);
            return;
        }
        e_temp = e_temp->getOpposite()->getNext()->getNext();
    }

    //需要修改的边所在的三角形（需删除的三角形）
    Triangle* triangle1 = e1->getTriangle();
    //另外半边的三角形
    Triangle* triangle2 = e1->getOpposite()->getTriangle();
    //寻找需要修改的三角形
    std::vector<Triangle*> changedTriangles;
    Edge* tempEdge = e1->getNext()->getOpposite();
    while (tempEdge != NULL && tempEdge != e1) {
        changedTriangles.push_back(tempEdge->getTriangle());
        tempEdge = tempEdge->getNext()->getOpposite();
    }
    //去掉最后一个重复的
    if (tempEdge != NULL) {
        if (tempEdge->getTriangle()->getID() == e1->getTriangle()->getID() || tempEdge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
            if (changedTriangles.size() != 0)
                changedTriangles.pop_back();
        }
    }
    //选择从对边开始的
    tempEdge = e1->getOpposite()->getNext()->getOpposite();
    while (tempEdge != NULL && tempEdge != e1->getOpposite()) {
        changedTriangles.push_back(tempEdge->getTriangle());
        tempEdge = tempEdge->getNext()->getOpposite();
    }

    if (tempEdge != NULL) {
        if (tempEdge->getTriangle()->getID() == e1->getTriangle()->getID() || tempEdge->getTriangle()->getID() == e1->getOpposite()->getTriangle()->getID()) {
            if (changedTriangles.size() != 0)
                changedTriangles.pop_back();
        }
    }
    //添加顶点
    Vertex* v_new = addVertex(p.getResult());
    v_new->setQ(p.getP1()->getQ() + p.getP2()->getQ());

    //涉及到的点
    std::vector<Vertex*> changedVertex;
    tempEdge = e1->getNext();
    Vertex* tempVertex = tempEdge->getEndVertex();
    while (1) {
        if (tempEdge->getOpposite() == NULL) {
            changedVertex.push_back(tempVertex);
            break;
        }
        if (tempVertex == e1->getOpposite()->getNext()->getEndVertex()) {
            break;
        }
        changedVertex.push_back(tempVertex);
        tempEdge = tempEdge->getOpposite()->getNext();
        tempVertex = tempEdge->getEndVertex();
    }

    tempEdge = e1->getOpposite()->getNext();
    tempVertex = tempEdge->getEndVertex();
    while (1) {
        if (tempEdge->getOpposite() == NULL) {
            changedVertex.push_back(tempVertex);
            break;
        }
        //终止条件
        if (tempVertex == e1->getNext()->getEndVertex()) {
            break;
        }
        changedVertex.push_back(tempVertex);
        tempEdge = tempEdge->getOpposite()->getNext();
        tempVertex = tempEdge->getEndVertex();
    }
    // 判断是否重复即是否有两个相同元素
    std::vector<Vertex*>::iterator it1 = changedVertex.begin();
    std::vector<Vertex*>::iterator it2;
    for (; it1 < changedVertex.end(); it1++) {
        int count = 0;
        for (it2 = changedVertex.begin(); it2 < changedVertex.end(); it2++) {
            if ((*it1)->getIndex() == (*it2)->getIndex()) {
                count++;
            }
        }
        if (count > 1){
          printf( "存在重复，重新选择边\n") ;
          e1->setIsOK(false);
          return;
        }
    }


    //去除三角形
    removeTriangle(triangle1);
    removeTriangle(triangle2);
    //更新三角形
    std::vector<Triangle*>::iterator iter_tri = changedTriangles.begin();
    for (; iter_tri != changedTriangles.end(); iter_tri++) {
        Triangle* temp_tri = (*iter_tri);
        Vertex* a = (*temp_tri)[0];
        Vertex* b = (*temp_tri)[1];
        Vertex* c = (*temp_tri)[2];
        //先删除
        removeTriangle(temp_tri);
        if (a->getIndex() == p.getP1()->getIndex() || a->getIndex() == p.getP2()->getIndex()) {
            a = v_new;
        }
        else if (b->getIndex() == p.getP1()->getIndex() || b->getIndex() == p.getP2()->getIndex()) {
            b = v_new;
        }
        else if (c->getIndex() == p.getP1()->getIndex() || c->getIndex() == p.getP2()->getIndex()) {
            c = v_new;
        }
        //后添加
        addTriangle(a, b, c);
    }
}
void Mesh::Simplification_QEM(int target_tri_count) {
    vertex_parents.clear();

    printf("Simplification_QEM the mesh! %d -> %d\n", numTriangles(), target_tri_count);
    
    getAllPairs();
    getAllDistance();
    while (numTriangles() > target_tri_count) {
        if (allPairs.size() == 0) break;
        if (allPairs[0].getDistance() >= MaxDis) break;
        sort(allPairs.begin(), allPairs.end(), cmp);
        Pair p = allPairs[0];
        simply(p);

        allPairs.clear();
        //重新选择
        getAllPairs();
        getAllDistance();
    }

}
// =================================================================

