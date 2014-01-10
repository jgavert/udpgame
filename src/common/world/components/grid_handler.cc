#include "common/world/components/grid_handler.h"

#include <iostream>
#include <cassert>

using namespace std;
using glm::vec3;

GridHandler::GridHandler()
{
}

void GridHandler::defaultGrid() {
  mArr.makeFloor();
}

void GridHandler::range_indices(const vec3& p, int ind[3][2]) const {
  for (unsigned a = 0; a < 3; ++a) {
    ind[a][0] = max(0, (int)(p[a] - 10));
    ind[a][1] = min(GRID_SIZE[a] - 1, (int)(p[a] + 10));
  }
}

void GridHandler::overlapping_indices(const PhysicsC& p, int ind[3][2]) const {
  for (unsigned a = 0; a < 3; ++a) {
    ind[a][0] = (int)(p.next_bb_min[a]);
    ind[a][1] = (int)(p.next_bb_max[a]);
  }
}

bool GridHandler::correct_one_hit(PhysicsC& p) const {
  int ind[3][2];
  overlapping_indices(p, ind);

  float max_overlap = FLT_MIN;
  unsigned max_axis = 4;
  bool hit = false;

  for (int y = ind[1][0]; y <= ind[1][1]; ++y) {
    for (int x = ind[0][0]; x <= ind[0][1]; ++x) {
      for (int z = ind[2][0]; z <= ind[2][1]; ++z) {
        if (mArr.get(x,y,z)) {
          float overlap;
          unsigned axis;
          correct_position(p, x, y, z, overlap, axis);
          if (fabsf(overlap) > fabsf(max_overlap)) {
            max_overlap = overlap;
            max_axis = axis;
            hit = true;
          }
        }
      }
    }
  }
  if (hit) {
    p.next_position[max_axis] += max_overlap * 1.1f;
    p.velocity[max_axis] = 0.0f;
    if (max_axis == 1) p.on_ground = true;

    p.position = p.next_position;
    p.update_bbs();
    p.update_next_bbs();
  }
  return hit;
}

bool GridHandler::handle_grid_collisions(PhysicsC& p, float dt) const {
  p.next_position = p.position + p.velocity * dt;
  p.position = p.next_position;
  p.update_bbs();
  p.update_next_bbs();

  for (int i = 0; correct_one_hit(p); ++i)
    if (i >= 8) return false;
  return true;
}

int GridHandler::TestAABBAABB(PhysicsC& p, int x, int y, int z) const {
  glm::vec3 block_min;
  glm::vec3 block_max;
  bb_min(x, y, z, block_min);
  bb_max(x, y, z, block_max);
  const glm::vec3& nbb_min = p.next_bb_min;
  const glm::vec3& nbb_max = p.next_bb_max;
  if (nbb_max[0] < block_min[0] || nbb_min[0] > block_max[0]) return 0;
  if (nbb_max[1] < block_min[1] || nbb_min[1] > block_max[1]) return 0;
  if (nbb_max[2] < block_min[2] || nbb_min[2] > block_max[2]) return 0;
  return 1;
}

void GridHandler::correct_position(PhysicsC& p, int x, int y, int z,
    float& min_overlap, unsigned& min_axis) const {
  glm::vec3 block_min;
  glm::vec3 block_max;
  bb_min(x, y, z, block_min);
  bb_max(x, y, z, block_max);

  unsigned axis = 4;
  float smallest_overlap = FLT_MAX;
  float smallest_overlap_abs = FLT_MAX;
  for (unsigned a = 0; a < 3; ++a) {
    float overlap = block_max[a] - p.next_bb_min[a];
    float overlap_abs = fabsf(overlap);
    if (overlap_abs > 0.0f && smallest_overlap_abs > overlap_abs) {
      smallest_overlap = overlap;
      smallest_overlap_abs = overlap_abs;
      axis = a;
    }

    overlap = block_min[a] - p.next_bb_max[a];
    overlap_abs = fabsf(overlap);
    if (overlap_abs > 0.0f && smallest_overlap_abs > overlap_abs) {
      smallest_overlap = overlap;
      smallest_overlap_abs = overlap_abs;
      axis = a;
    }
  }
  assert(axis != 4);

  min_overlap = smallest_overlap;
  min_axis = axis;
}

void GridHandler::bb_min(int x, int y, int z, glm::vec3& bb) const {
  bb = glm::vec3((float)x, (float)y, (float)z);
}

void GridHandler::bb_max(int x, int y, int z, glm::vec3& bb) const {
  bb = glm::vec3((float)x + BLOCK_SIZE, (float)y + BLOCK_SIZE,
    (float)z + BLOCK_SIZE);
}

bool GridHandler::check_collision(PhysicsC& p, float dt) const {
  bool ret = handle_grid_collisions(p, dt);
  p.position = p.next_position;
  if (belowBottom(p.position)) return false;
  p.update_bbs();
  p.update_next_bbs();
  return ret;
}

bool GridHandler::ray_block_collision(
    int x, int y, int z,
    const vec3& p, const vec3& d, float& tmin, int& axis, int& dir) const
{
  glm::vec3 block_min;
  glm::vec3 block_max;
  bb_min(x, y, z, block_min);
  bb_max(x, y, z, block_max);
  tmin = 0.0f;
  float tmax = FLT_MAX;
  axis = 4;
  for (int i = 0; i < 3; i++) {
    int swapped = 1;
    if (fabsf(d[i]) < FLT_MIN) {
      if (p[i] < block_min[i] || p[i] > block_max[i]) {
        return false;
      }
    } else {
      float div = 1.0f/d[i];
      float t1 = (block_min[i] - p[i]) * div;
      float t2 = (block_max[i] - p[i]) * div;

      if (t1 > t2) {
        swapped = -1;
        swap(t1, t2);
      }
      if (t1 > tmin) {
        tmin = t1;
        axis = i;
        dir = -1 * swapped;
      }
      if (tmax > t2) {
        tmax = t2;
      }
      if (tmin > tmax) {
        return false;
      }
    }
  }
  return axis != 4; // axis 4 up to no good
}

bool GridHandler::raycast(const vec3& s, const vec3& d, float& distance,
    char** hitBlock, char** faceBlock)
{
  glm::vec3 dn = glm::normalize(d);
  int b[3];
  distance = FLT_MAX;
  int baxis = 4;
  int dir = 0;
  bool hit = false;
  for (int x = 0; x < GRID_SIZE_X; ++x) {
    for (int y = 0; y < GRID_SIZE_Y; ++y) {
      for (int z = 0; z < GRID_SIZE_Z; ++z) {
        float t;
        int a;
        int d;
        char& block = mArr.getRef(x,y,z);
        if (block and ray_block_collision(x, y, z, s, dn, t, a, d)) {
          if (distance > t) {
            distance = t;
            hit = true;
            baxis = a;
            dir = d;
            *hitBlock = &block;
            b[0] = x;
            b[1] = y;
            b[2] = z;
            b[baxis] += dir;
            if (!mArr.outsideGrid(b[0], b[1], b[2])) {
              *faceBlock = &mArr.getRef(b[0], b[1], b[2]);
            } else {
              *faceBlock = nullptr;
            }
          }
        }
      }
    }
  }
  return hit;
}

void GridHandler::deserialize(const InitialState& i) {
  memcpy(mArr.mData, i.grid().c_str(), mArr.size());
}

void GridHandler::serialize(InitialState& i) const {
  i.set_grid(mArr.mData, mArr.size());
}

bool GridHandler::belowBottom(const glm::vec3& p) const {
  return p.y < BOTTOM;
}
