#include "vertex.h"
#include "vectors.h"
#include "edge.h"
class Pair {
public:
	Pair(Vertex* a, Vertex* b,Edge *ee) :p1(a), p2(b),e(ee),distance(0) {}
    Vertex* getP1(){return p1;}
    Vertex* getP2(){return p2;}
    Edge* getEdge(){return e;};
    void computeDistance(double MaxDis);
    double  getDistance(){return distance;};
    Vec3f getResult(){return result;};
    void setDistance(double dis){
        distance=dis;
    }
    void setResult(Vec3f v){
        result=v;
    }

private:
    Vertex* p1;
	Vertex* p2;
	double distance;
	Vec3f result;
	Edge* e;
};