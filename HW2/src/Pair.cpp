#include "Pair.h"
#include "matrix.h"

void Pair::computeDistance(double MaxDis){
    Matrix Q = p1->getQ() + p2->getQ();
    Matrix Q1(Q);
    Q1.set(3, 0, 0); 
    Q1.set(3, 1, 0); 
    Q1.set(3, 2, 0); 
    Q1.set(3, 3, 1);
    Vec4f temp_result;
    Matrix Q1_Inverse;
    int inverse_able = Q1.Inverse(Q1_Inverse); 
    if (!inverse_able) {
        //Q1不可逆，从v1 v2上找一个cost最小的点
        double mink = 0, minDis = MaxDis; 
        double cost;
        for (double k = 0; k <= 1; k = k + 0.1) {
            temp_result = Vec4f((1 - k) * p1->x()+ k * p2->x(), (1 - k) * p1->y() + k * p2->y(), (1 - k) * p1->z() + k * p2->z(), 1);
            cost = temp_result.Dot4(Q * temp_result);
            if (cost < minDis) {
                mink = k; 
                minDis = cost;
            }
        }
        temp_result = Vec4f((1 - mink) * p1->x() + mink * p2->x(), (1 - mink) * p1->y() + mink * p2->y(), (1 - mink) * p1->z() + mink * p2->z(), 1);
        setDistance(minDis);
    }else {
        //Q1可逆
        temp_result=Vec4f(0, 0, 0, 1);
        Q1_Inverse.Transform(temp_result);
        setDistance(temp_result.Dot4(Q*temp_result));
    }
    setResult(Vec3f(temp_result.x(),temp_result.y(),temp_result.z()));
  }