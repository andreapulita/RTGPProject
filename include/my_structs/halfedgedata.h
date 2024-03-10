#pragma once
#include <glad/glad.h>
#include <utils/mesh.h>

#include <glm/glm.hpp>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cmath>

namespace my_structs {
// hash function for glm::vec3
struct Vec3Hash {
    std::size_t operator()(const glm::vec3& vertex) const {
        std::size_t seed = 0;

        // Combine the hash values of the position
        seed ^= std::hash<float>{}(vertex.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(vertex.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= std::hash<float>{}(vertex.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

        return seed;
    }
};
bool operator==(const glm::vec3& lhs, const glm::vec3& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}
// hash function for std::pair<glm::vec3, glm::vec3>
struct PairVec3Hash {
  std::size_t operator()(const std::pair<glm::vec3, glm::vec3>& key) const {
    // Compute the hash value based on the components of the glm::vec3 values
    std::size_t seed = 0;
    const glm::vec3& first = key.first;
    const glm::vec3& second = key.second;

    // Combine the hash values of the components using a simple hash function
    // like the one below
    seed ^=
        std::hash<float>{}(first.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^=
        std::hash<float>{}(first.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^=
        std::hash<float>{}(first.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^=
        std::hash<float>{}(second.x) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^=
        std::hash<float>{}(second.y) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^=
        std::hash<float>{}(second.z) + 0x9e3779b9 + (seed << 6) + (seed >> 2);

    return seed;
  }
};
bool operator==(const std::pair<glm::vec3, glm::vec3>& lhs, const std::pair<glm::vec3, glm::vec3>& rhs) {
  // Compare the components of the glm::vec3 values for equality
  return lhs.first == rhs.first && lhs.second == rhs.second;
}

class HalfEdge;
class HalfEdgeFace;
class HalfEdgeMesh;

class HalfEdgeVertex {
 public:
  glm::vec3 position;
  glm::vec3 normal;
  HalfEdge* edge{nullptr};
  HalfEdgeVertex(glm::vec3 position) : position{position} {};
  HalfEdgeVertex(glm::vec3 position, glm::vec3 normal)
      : position{position}, normal{normal} {};
  ~HalfEdgeVertex() = default;
  std::vector<HalfEdge*> GetEdgesPointingToVertex(const HalfEdgeMesh* mesh);
};
class HalfEdgeFace {
 public:
  HalfEdge* edge{nullptr};
  HalfEdgeFace(HalfEdge* edge) : edge{edge} {};
  ~HalfEdgeFace() = default;
  std::vector<HalfEdge*> GetEdges();
};
class HalfEdge {
 public:
  HalfEdgeVertex* v{nullptr};
  HalfEdgeFace* f{nullptr};
  HalfEdge* next_edge{nullptr};
  HalfEdge* opposite_edge{nullptr};
  HalfEdge(HalfEdgeVertex* v) : v{v} {};
  ~HalfEdge() = default;
};
std::vector<HalfEdge*> HalfEdgeFace::GetEdges() {
  std::vector<HalfEdge*> edges;
  edges.push_back(edge);
  edges.push_back(edge->next_edge);
  edges.push_back(edge->next_edge->next_edge);
  return edges;
}

class HalfEdgeMesh {
 public:
  std::vector<HalfEdgeVertex*> vertices;
  std::vector<HalfEdgeFace*> faces;
  std::vector<HalfEdge*> edges;
  HalfEdgeMesh() {
    vertices = std::vector<HalfEdgeVertex*>();
    faces = std::vector<HalfEdgeFace*>();
    edges = std::vector<HalfEdge*>();
  }
  HalfEdgeMesh(const Mesh& mesh) {
    std::vector<Vertex> all_vertices = mesh.vertices;
    std::vector<GLuint> all_indices = mesh.indices;
    for (int i = 0; i < all_indices.size(); i += 3) {
      int index1 = all_indices[i];
      int index2 = all_indices[i + 1];
      int index3 = all_indices[i + 2];
      glm::vec3 v1 = all_vertices[index1].Position;
      glm::vec3 v2 = all_vertices[index2].Position;
      glm::vec3 v3 = all_vertices[index3].Position;
      glm::vec3 n1 = all_vertices[index1].Normal;
      glm::vec3 n2 = all_vertices[index2].Normal;
      glm::vec3 n3 = all_vertices[index3].Normal;
      AddFace(v1, v2, v3, n1, n2, n3);
    }
    ConnectAllEdges();
  }
  ~HalfEdgeMesh() {
    for (auto vertex : vertices) {
      delete vertex;
    }
    for (auto face : faces) {
      delete face;
    }
    for (auto edge : edges) {
      delete edge;
    }
  }
  void RemoveVertex(HalfEdgeVertex* v) {
    vertices.erase(std::remove(vertices.begin(), vertices.end(), v), vertices.end());
    delete v;
  }
  void RemoveEdge(HalfEdge* e) {
    if (e->opposite_edge != nullptr) {
      e->opposite_edge->opposite_edge = nullptr;
    }
    edges.erase(std::remove(edges.begin(), edges.end(), e), edges.end());
    e->f = nullptr;
    //delete e;
  }
  void RemoveFace(HalfEdgeFace* f) {
    faces.erase(std::remove(faces.begin(), faces.end(), f), faces.end());
    f->edge = nullptr;
    delete f;
  }
  Mesh* ConvertToMesh(bool smooth_normals = false) {
    std::vector<Vertex> vertices_out;
    std::vector<GLuint> indices_out;
    std::unordered_map<glm::vec3, int, Vec3Hash> vertex_map;
    // Recalculate normals
    for (auto f : faces) {
      if(f->edge == nullptr) continue;
      auto e1 = f->edge;
      auto e2 = f->edge->next_edge;
      auto e3 = f->edge->next_edge->next_edge;
      glm::vec3 v1 = e3->v->position;
      glm::vec3 v2 = e1->v->position;
      glm::vec3 v3 = e2->v->position;
      glm::vec3 normal = glm::normalize(glm::cross(v2 - v1, v3 - v1));
      e1->v->normal = normal;
      e2->v->normal = normal;
      e3->v->normal = normal;
      if(!smooth_normals) {
        vertices_out.push_back(Vertex{v1, normal});
        vertices_out.push_back(Vertex{v2, normal});
        vertices_out.push_back(Vertex{v3, normal});
        indices_out.push_back(vertices_out.size() - 3);
        indices_out.push_back(vertices_out.size() - 2);
        indices_out.push_back(vertices_out.size() - 1);
      }
    }
    // Smooth normals with all the adjacent faces
    if(smooth_normals) {
      std::unordered_set<glm::vec3, Vec3Hash> vertex_visited;
      for (auto v : vertices) {
        auto it = vertex_visited.find(v->position);
        if(it == vertex_visited.end()) {
          std::vector<HalfEdge*> edges_to_vertex = v->GetEdgesPointingToVertex(this);
          glm::vec3 normal = glm::vec3(0.0f);
          for(auto edge : edges_to_vertex) {
            normal += edge->v->normal;
          }
          normal /= edges_to_vertex.size();
          normal = glm::normalize(normal);
          if(std::isnan(normal.x) || std::isnan(normal.y) || std::isnan(normal.z)) {
            normal = glm::vec3(0.0f, 0.0f, 0.0f);
          }
          vertex_visited.insert(v->position);
          vertex_map[v->position] = vertices_out.size();
          vertices_out.push_back(Vertex{v->position, normal});
        }
      }
      for (auto f : faces) {
        if(f->edge == nullptr) continue;
        auto e1 = f->edge;
        auto e2 = f->edge->next_edge;
        auto e3 = f->edge->next_edge->next_edge;
        indices_out.push_back(vertex_map[e1->v->position]);
        indices_out.push_back(vertex_map[e2->v->position]);
        indices_out.push_back(vertex_map[e3->v->position]);
      }
    }
    return new Mesh(vertices_out, indices_out);
  }
  std::vector<HalfEdge*> ContractHalfEdge(HalfEdge* e, glm::vec3 mergePos) {
    HalfEdgeVertex* v1 = e->next_edge->next_edge->v;
    HalfEdgeVertex* v2 = e->v;

    std::vector<HalfEdge*> edges_to_v1 = v1->GetEdgesPointingToVertex(this);
    std::vector<HalfEdge*> edges_to_v2 = v2->GetEdgesPointingToVertex(this);

    RemoveTriangleAndConnect(e);
    if (e->opposite_edge != nullptr && e->opposite_edge->f != nullptr) {
      RemoveTriangleAndConnect(e->opposite_edge);
    }
    std::vector<HalfEdge*> edges_to_new_v = std::vector<HalfEdge*>();
    for (auto edge : edges_to_v1) {
      if (edge->f != nullptr) {
        edge->v->position = mergePos;
        edges_to_new_v.push_back(edge);
      }
    }
    for (auto edge : edges_to_v2) {
      if (edge->f != nullptr) {
        edge->v->position = mergePos;
        edges_to_new_v.push_back(edge);
      }
    }
    return edges_to_new_v;
  }
  void RemoveTriangleAndConnect(HalfEdge* e1) {
    HalfEdge* e2 = e1->next_edge;
    HalfEdge* e3 = e2->next_edge;
    HalfEdgeFace* f = e1->f;
    RemoveTriangle(f);
    if (e2->opposite_edge != nullptr) {
      e2->opposite_edge->opposite_edge = e3->opposite_edge;
    }
    if (e3->opposite_edge != nullptr) {
      e3->opposite_edge->opposite_edge = e2->opposite_edge;
    }
  }
  void RemoveTriangle(HalfEdgeFace* f) {
    std::vector<HalfEdge*> edges_face = f->GetEdges();
    for (auto edge_to_remove : edges_face) {
      // Remove vertex from vertices
      RemoveVertex(edge_to_remove->v);
      // Remove edge from edges
      RemoveEdge(edge_to_remove);
    }
    // Remove face from faces
    RemoveFace(f);
  }

 private:
  void AddFace(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3, glm::vec3 n1,
               glm::vec3 n2, glm::vec3 n3) {
    HalfEdgeVertex* vertex1 = new HalfEdgeVertex(v1, n1);
    HalfEdgeVertex* vertex2 = new HalfEdgeVertex(v2, n2);
    HalfEdgeVertex* vertex3 = new HalfEdgeVertex(v3, n3);
    HalfEdge* edge1 = new HalfEdge(vertex1);
    HalfEdge* edge2 = new HalfEdge(vertex2);
    HalfEdge* edge3 = new HalfEdge(vertex3);
    HalfEdgeFace* face = new HalfEdgeFace(edge1);
    edge1->next_edge = edge2;
    edge2->next_edge = edge3;
    edge3->next_edge = edge1;
    edge1->f = face;
    edge2->f = face;
    edge3->f = face;
    vertex1->edge = edge2;
    vertex2->edge = edge3;
    vertex3->edge = edge1;
    vertices.push_back(vertex1);
    vertices.push_back(vertex2);
    vertices.push_back(vertex3);
    edges.push_back(edge1);
    edges.push_back(edge2);
    edges.push_back(edge3);
    faces.push_back(face);
  }
  void ConnectAllEdges() {
    std::unordered_map<std::pair<glm::vec3, glm::vec3>, HalfEdge*, PairVec3Hash>
        edges_map;
    /*for (auto edge : edges) {
      if (edge->opposite_edge != nullptr) continue;
      edges_map[std::make_pair(edge->next_edge->next_edge->v->position,
                               edge->v->position)] = edge;
    }
    for (auto edge : edges) {
      if (edge->opposite_edge != nullptr) continue;
      HalfEdge* eOther = edges_map[std::make_pair(
          edge->v->position, edge->next_edge->next_edge->v->position)];
      if (eOther != nullptr) {
        edge->opposite_edge = eOther;
        eOther->opposite_edge = edge;
      } else {
        std::cout << "ConnectAllEdges: eOther == nullptr" << std::endl;
      }
    }*/
    for (auto edge : edges) {
      auto it = edges_map.find(std::make_pair(edge->v->position, edge->next_edge->next_edge->v->position));
      if (it == edges_map.end()) {
        edges_map[std::make_pair(edge->next_edge->next_edge->v->position,
                                 edge->v->position)] = edge;
      } else {
        it->second->opposite_edge = edge;
        edge->opposite_edge = it->second;
      }
    }
  }
};

std::vector<HalfEdge*> HalfEdgeVertex::GetEdgesPointingToVertex(const HalfEdgeMesh* mesh) {
  std::vector<HalfEdge*> edges;
  HalfEdge* current = edge->next_edge->next_edge;
  do {
    edges.push_back(current);
    if(current->opposite_edge == nullptr) {
      std::cout << "Opposite edge is null" << std::endl;
      edges.clear();
      break;
    }
    current = current->opposite_edge->next_edge->next_edge;
  } while (current->next_edge != edge);
  if(edges.size() == 0 && mesh != nullptr) {
    for(auto edge : mesh->edges) {
      if(edge->v->position == this->position) {
        edges.push_back(edge);
      }
    }
  }
  return edges;
}
}  // namespace my_structs