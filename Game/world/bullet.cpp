#include "bullet.h"

#include "world.h"

Bullet::Bullet(World& owner, size_t itemInstance)
  :owner(owner){
  // FIXME: proper item creation
  Daedalus::GEngineClasses::C_Item  hitem={};
  owner.script().initializeInstance(hitem,itemInstance);
  owner.script().clearReferences(hitem);

  setView(owner.getStaticView(hitem.visual,hitem.material));
  }

Bullet::~Bullet() {
  }

void Bullet::setPosition(const std::array<float,3> &p) {
  pos = p;
  updateMatrix();
  }

void Bullet::setPosition(float x, float y, float z) {
  pos = {x,y,z};
  updateMatrix();
  }

void Bullet::setDirection(float x, float y, float z) {
  dir  = {x,y,z};
  dirL = std::sqrt(x*x + y*y + z*z);
  updateMatrix();
  }

void Bullet::setView(StaticObjects::Mesh &&m) {
  view = std::move(m);
  view.setObjMatrix(mat);
  updateMatrix();
  }

void Bullet::tick(uint64_t dt) {
  float k  = dt/1000.f;
  float dx = dir[0]*k;
  float dy = dir[1]*k;
  float dz = dir[2]*k;

  dir[1] -= 9.8f*100.f*k; // FIXME: gravity

  owner.physic()->moveBullet(*this,dx,dy,dz);
  }

void Bullet::updateMatrix() {
  const float dx = dir[0]/dirL;
  const float dy = dir[1]/dirL;
  const float dz = dir[2]/dirL;

  float a2  = std::asin(dy)*float(180/M_PI);
  float ang = std::atan2(dz,dx)*float(180/M_PI)+180.f;

  mat.identity();
  mat.translate(pos[0],pos[1],pos[2]);
  mat.rotateOY(-ang);
  mat.rotateOX(a2);
  view.setObjMatrix(mat);
  }