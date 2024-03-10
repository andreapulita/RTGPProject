#pragma once
#include <my_structs/halfedgedata.h>
#include <my_structs/qem_edge.h>
#include <unordered_map>
#include <iostream>
#include <set>
namespace my_structs { 
class MeshSimplification_QEM {
  public:
    HalfEdgeMesh& mesh_data;
    std::unordered_map<glm::vec3, glm::mat4, Vec3Hash> q_matrices = std::unordered_map<glm::vec3, glm::mat4, Vec3Hash>();
    //std::vector<QEM_Edge*> qem_edges = std::vector<QEM_Edge*>();
    std::set<QEM_Edge*,QEM_Edge::Comparator> qem_edges = std::set<QEM_Edge*,QEM_Edge::Comparator>();
    //MinHeap min_heap_QEM = MinHeap();
    std::unordered_map<HalfEdge*, QEM_Edge*> edge_QEM_lookup = std::unordered_map<HalfEdge*, QEM_Edge*>();
    std::pair<glm::vec3, glm::vec3> next_edge_to_collapse = std::make_pair(glm::vec3(0.0f), glm::vec3(0.0f));
    //QEM_Edge* smalles_error_edge;
    QEM_Edge* smallest_error_edge;
    MeshSimplification_QEM(HalfEdgeMesh& mesh_data) : mesh_data(mesh_data){
      for(auto v : mesh_data.vertices) {
        if(q_matrices.find(v->position) != q_matrices.end()) {
          continue;
        }
        std::vector<HalfEdge*> edges_to_vertex = v->GetEdgesPointingToVertex(&mesh_data);
        glm::mat4 Q = CalculateQMatrix(edges_to_vertex);
        q_matrices[v->position] = Q;
      }
      for(auto e : mesh_data.edges) {
        glm::vec3 p1 = e->next_edge->next_edge->v->position;
        glm::vec3 p2 = e->v->position;
        glm::mat4 Q1 = q_matrices[p1];
        glm::mat4 Q2 = q_matrices[p2];
        QEM_Edge* qem_edge = new QEM_Edge(e, Q1, Q2);
        //qem_edges.push_back(qem_edge);
        //min_heap_QEM.insert(qem_edge);
        edge_QEM_lookup[e] = qem_edge;
        qem_edges.insert(qem_edge);
      }
      //smalles_error_edge = *std::min_element(qem_edges.begin(), qem_edges.end(), [](QEM_Edge* a, QEM_Edge* b) {
      //    return a->qem < b->qem;
      //  });
      //smalles_error_edge = min_heap_QEM.extractMin();
      smallest_error_edge = *qem_edges.begin();
      qem_edges.erase(qem_edges.begin());
    };
    ~MeshSimplification_QEM() = default;
    bool SimplifyMesh(int max_edges, float max_error) {
      for(int i = 0; i < max_edges; ++i) {
        if(mesh_data.faces.size() <= 5) {
          std::cout << "MeshSimplification_QEM: Mesh has less than 4 faces" << std::endl;
          return false;
        }
        //if(smalles_error_edge->edge->f == nullptr) {
          //std::cout << "MeshSimplification_QEM: Edge has no face" << std::endl;
          //qem_edges.erase(std::remove(qem_edges.begin(), qem_edges.end(), smalles_error_edge), qem_edges.end());
          //--i;
          //continue;
        //}
        if(smallest_error_edge->qem > max_error) {
          std::cout << "MeshSimplification_QEM: Edge has too much error" << std::endl;
          return false;
        }
        //HalfEdge* edge_to_contract = smalles_error_edge->edge;
        HalfEdge* edge_to_contract = smallest_error_edge->edge;
        
        q_matrices.erase(edge_to_contract->v->position);
        q_matrices.erase(edge_to_contract->next_edge->next_edge->v->position);
        std::vector<HalfEdge*> edges_to_new_vertex = mesh_data.ContractHalfEdge(edge_to_contract, smallest_error_edge->mergePosition);
        glm::mat4 Q_new = CalculateQMatrix(edges_to_new_vertex);
        q_matrices[smallest_error_edge->mergePosition] = Q_new;
        for(auto edge_to_v : edges_to_new_vertex) {
          HalfEdge* edge_from_v = edge_to_v->next_edge;
          // TO
          QEM_Edge* qem_edge_to_v = edge_QEM_lookup[edge_to_v];
          glm::vec3 p1 = edge_to_v->next_edge->next_edge->v->position;
          glm::vec3 p2 = edge_to_v->v->position;
          glm::mat4 Q1_edge_to_v = q_matrices[p1];
          glm::mat4 Q2_edge_to_v = Q_new;
          //qem_edge_to_v->UpdateEdge(edge_to_v, Q1_edge_to_v, Q2_edge_to_v);
          //min_heap_QEM.updateError(qem_edge_to_v, qem_edge_to_v->qem);
          auto it = qem_edges.find(qem_edge_to_v);
          if(it != qem_edges.end()) {
            qem_edges.erase(it);
            delete qem_edge_to_v;
            QEM_Edge* new_qem_edge_to_v = new QEM_Edge(edge_to_v, Q1_edge_to_v, Q2_edge_to_v);
            qem_edges.insert(new_qem_edge_to_v);
            edge_QEM_lookup[edge_to_v] = new_qem_edge_to_v;
          }
          // FROM
          QEM_Edge* qem_edge_from_v = edge_QEM_lookup[edge_from_v];
          glm::vec3 p3 = edge_from_v->next_edge->next_edge->v->position;
          glm::vec3 p4 = edge_from_v->v->position;
          glm::mat4 Q3_edge_from_v = Q_new;
          glm::mat4 Q4_edge_from_v = q_matrices[p4];
          //qem_edge_from_v->UpdateEdge(edge_from_v, Q3_edge_from_v, Q4_edge_from_v);
          //min_heap_QEM.updateError(qem_edge_from_v, qem_edge_from_v->qem);
          it = qem_edges.find(qem_edge_from_v);
          if(it != qem_edges.end()) {
            qem_edges.erase(it);
            delete qem_edge_from_v;
            QEM_Edge* new_qem_edge_from_v = new QEM_Edge(edge_from_v, Q3_edge_from_v, Q4_edge_from_v);
            qem_edges.insert(new_qem_edge_from_v);
            edge_QEM_lookup[edge_from_v] = new_qem_edge_from_v;
          }
        }
        //qem_edges.erase(std::remove(qem_edges.begin(), qem_edges.end(), smalles_error_edge), qem_edges.end());
        //smalles_error_edge = *std::min_element(qem_edges.begin(), qem_edges.end(), [](QEM_Edge* a, QEM_Edge* b) {
        //  return a->qem < b->qem;
        //});
        //smalles_error_edge = min_heap_QEM.extractMin();
        smallest_error_edge = *qem_edges.begin();
        //while (smalles_error_edge->edge->f == nullptr) {
          //smalles_error_edge = min_heap_QEM.extractMin();
        //}
        while (smallest_error_edge->edge->f == nullptr) {
          smallest_error_edge = *qem_edges.begin();
          qem_edges.erase(qem_edges.begin());
        }
        //next_edge_to_collapse = std::make_pair(smalles_error_edge->edge->v->position, smalles_error_edge->edge->next_edge->next_edge->v->position);
        next_edge_to_collapse = std::make_pair(smallest_error_edge->edge->v->position, smallest_error_edge->edge->next_edge->next_edge->v->position);
      }
      return true;
    }
  private:
    glm::mat4 CalculateQMatrix(std::vector<HalfEdge*> edges) {
      glm::mat4 Q = glm::mat4(0.0f);
      for(auto e : edges) {
        glm::vec3 p1 = e->v->position;
        glm::vec3 p2 = e->next_edge->v->position;
        glm::vec3 p3 = e->next_edge->next_edge->v->position;
        glm::vec3 normal = glm::normalize(glm::cross(p2 - p1, p3 - p1));
        if(std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z)) {
          normal = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        float a = normal.x;
        float b = normal.y;
        float c = normal.z;
        float d = -(a*p1.x + b*p1.y + c*p1.z);
        glm::mat4 Kp = glm::mat4(a*a, a*b, a*c, a*d,
                                  a*b, b*b, b*c, b*d,
                                  a*c, b*c, c*c, c*d,
                                  a*d, b*d, c*d, d*d);
        Q += Kp;
      }
      return Q;
    }
};
} // namespace my_structs