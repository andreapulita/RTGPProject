#pragma once
#include <my_structs/halfedgedata.h>

namespace my_structs {
class QEM_Edge {
 public:
  HalfEdge* edge;
  glm::vec3 mergePosition;
  float qem;
  QEM_Edge(HalfEdge* edge, glm::mat4 Q1, glm::mat4 Q2) {
    UpdateEdge(edge, Q1, Q2);
  }
  void UpdateEdge(HalfEdge* edge, glm::mat4 Q1, glm::mat4 Q2) {
    this->edge = edge;
    CalculateMergePosition(edge, Q1, Q2);
  }
  struct Comparator {
        bool operator()(const QEM_Edge* q1, const QEM_Edge* q2) const {
            // Sort in ascending order based on the error value
            if(q1->qem == q2->qem) {
              return q1 < q2;
            }
            return q1->qem < q2->qem;
        }
    };
 private:
  void CalculateMergePosition(HalfEdge* edge, glm::mat4 Q1, glm::mat4 Q2) {
    glm::vec3 p1 = edge->next_edge->next_edge->v->position;
    glm::vec3 p2 = edge->v->position;
    glm::vec3 p3 = (p1 + p2) * 0.5f;

    float qem1 = CalculateQEM(p1, Q1, Q2);
    float qem2 = CalculateQEM(p2, Q1, Q2);
    float qem3 = CalculateQEM(p3, Q1, Q2);
    if (qem1 < qem2 && qem1 < qem3) {
      mergePosition = p1;
      qem = qem1;
    } else if (qem2 < qem1 && qem2 < qem3) {
      mergePosition = p2;
      qem = qem2;
    } else {
      mergePosition = p3;
      qem = qem3;
    }
  }
  float CalculateQEM(glm::vec3 v, glm::mat4 Q1, glm::mat4 Q2) {
    glm::mat4 Q = Q1 + Q2;

    float x = v.x;
    float y = v.y;
    float z = v.z;

    // v^T * Q * v
    // Verify that this is true (was found at bottom in research paper)
    float qemCalculations = 0.0f;
    qemCalculations += (1.0f * Q[0][0] * x * x);
    qemCalculations += (2.0f * Q[0][1] * x * y);
    qemCalculations += (2.0f * Q[0][2] * x * z);
    qemCalculations += (2.0f * Q[0][3] * x);
    qemCalculations += (1.0f * Q[1][1] * y * y);
    qemCalculations += (2.0f * Q[1][2] * y * z);
    qemCalculations += (2.0f * Q[1][3] * y);
    qemCalculations += (1.0f * Q[2][2] * z * z);
    qemCalculations += (2.0f * Q[2][3] * z);
    qemCalculations += (1.0f * Q[3][3]);

    float qem = qemCalculations;

    return qem;
  }
};
}  // namespace my_structs