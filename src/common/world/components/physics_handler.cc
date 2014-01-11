#include "common/world/components/physics_handler.h"
#include "common/world/components/grid_handler.h"
#include "common/world/components/collision.h"
#include "common/world/world.h"
#include "client/controller/input/input.h"

#include <iostream>
#include <cassert>
#include <algorithm>
#include <cstdio>

constexpr float HALF_PI = (float)M_PI/2.0f;
constexpr float PI = (float)M_PI;
constexpr float move_speed = 1.5f;
constexpr float jump_velocity = 5.0f;
constexpr float FRICTION = 0.85f;
constexpr float GRAVITY = 10.0f;

using namespace std;

void PhysicsHandler::tick(float dt, World& w) {
  for (PhysicsC& p : mComponents) {
    handleInput(p, w);

    p.velocity.y -= GRAVITY * dt;
    p.velocity.x *= FRICTION;
    p.velocity.z *= FRICTION;
    if (!w.grid().check_collision(p, dt))
      w.mDeleteList.insert(p.eid());
  }
  checkCollisions(w);
}

void PhysicsHandler::checkCollisions(World& w) {
  for (size_t i = 0; i < mComponents.size(); ++i) {
    for (size_t j = i+1; j < mComponents.size(); ++j) {
      PhysicsC& p1 = mComponents[i];
      PhysicsC& p2 = mComponents[j];
      if (AABBvsAABB(p1.bb, p2.bb)) {
        Inventory* i1 = w.inventory().get(p1.eid());
        Inventory* i2 = w.inventory().get(p2.eid());
        if ((!i1 and !i2) or (i1 and i2)) continue;
        if (i1)
          w.mDeleteList.insert(p2.eid());
        else
          w.mDeleteList.insert(p1.eid());
      }
    }
  }
}

void PhysicsHandler::handleInput(PhysicsC& p, World& w) {
  FrameInput* i = w.input().get(p.eid());
  if (!i) return;

  p.horizontal_angle -= i->horizontal_delta();
  p.vertical_angle -= i->vertical_delta();
  if (p.vertical_angle < -HALF_PI)
    p.vertical_angle = -HALF_PI;
  else if (p.vertical_angle > HALF_PI)
    p.vertical_angle = HALF_PI;

  glm::vec3 forward = glm::vec3(
    sin(p.horizontal_angle), 0.0f, cos(p.horizontal_angle));

  glm::vec3 right = glm::vec3(
    sin(p.horizontal_angle - HALF_PI),
    0.0f,
    cos(p.horizontal_angle - HALF_PI)
  );

  if (i->actions() & ContinousAction::MOVE_FORWARD)
    p.velocity += forward * move_speed;
  else if (i->actions() & ContinousAction::MOVE_BACK)
    p.velocity -= forward * move_speed;
  if (i->actions() & ContinousAction::MOVE_RIGHT)
    p.velocity += right * move_speed;
  else if (i->actions() & ContinousAction::MOVE_LEFT)
    p.velocity -= right * move_speed;

  if (i->actions() & ContinousAction::JUMP && p.on_ground) {
    p.velocity.y += jump_velocity;
    p.on_ground = false;
  }
}

unsigned PhysicsHandler::hash() {
  unsigned long hash = 5381;

  for (const PhysicsC& p : mComponents) {
    for (int a = 0; a < 3; ++a) {
      unsigned v;
      memcpy(&v, &p.position[a], sizeof(v));
      hash = ((hash << 5) + hash) + v;
    }
  }
  return hash;
}

void PhysicsHandler::serialize(
    google::protobuf::RepeatedPtrField<PhysicsData>* pd)
{
  for (const PhysicsC& p : mComponents) {
    PhysicsData* o = pd->Add();
    o->set_eid(p.entityid);
    o->set_x(p.position.x);
    o->set_y(p.position.y);
    o->set_z(p.position.z);
    o->set_vertical_angle(p.vertical_angle);
    o->set_horizontal_angle(p.horizontal_angle);
    o->set_dim_x(p.dimensions.x);
    o->set_dim_y(p.dimensions.y);
    o->set_dim_z(p.dimensions.z);
    o->set_type(p.type);
  }
}

void PhysicsHandler::deserialize(
    const google::protobuf::RepeatedPtrField<PhysicsData>& ps)
{
  mComponents.clear();
  for (int i = 0; i < ps.size(); ++i) {
    const PhysicsData& pd = ps.Get(i);
    PhysicsC p;
    p.entityid = pd.eid();
    p.position.x = pd.x();
    p.position.y = pd.y();
    p.position.z = pd.z();
    p.vertical_angle = pd.vertical_angle();
    p.horizontal_angle = pd.horizontal_angle();
    p.dimensions.x = pd.dim_x();
    p.dimensions.y = pd.dim_y();
    p.dimensions.z = pd.dim_z();
    p.type = pd.type();
    p.update_bbs();
    add(p);
  }
}
