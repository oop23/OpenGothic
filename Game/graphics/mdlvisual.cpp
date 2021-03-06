#include "mdlvisual.h"

#include "graphics/skeleton.h"
#include "game/serialize.h"
#include "world/npc.h"
#include "world/item.h"
#include "world/world.h"

using namespace Tempest;

MdlVisual::MdlVisual()
  :skInst(std::make_unique<Pose>()) {
  }

void MdlVisual::save(Serialize &fout) {
  fout.write(fgtMode);
  if(skeleton!=nullptr)
    fout.write(skeleton->name()); else
    fout.write(std::string(""));
  solver.save(fout);
  skInst->save(fout);
  }

void MdlVisual::load(Serialize &fin,Npc& npc) {
  std::string s;

  fin.read(fgtMode);
  fin.read(s);
  npc.setVisual(s.c_str());
  solver.load(fin);
  skInst->load(fin,solver);
  }

void MdlVisual::setPos(float x, float y, float z) {
  pos.set(3,0,x);
  pos.set(3,1,y);
  pos.set(3,2,z);
  setPos(pos);
  }

void MdlVisual::setPos(const Tempest::Matrix4x4 &m) {
  // TODO: deferred setObjMatrix
  pos = m;
  head   .setObjMatrix(pos);
  sword  .setObjMatrix(pos);
  bow    .setObjMatrix(pos);
  for(auto& i:item)
    i.setObjMatrix(pos);
  pfx    .setObjMatrix(pos);
  view   .setObjMatrix(pos);
  }

// mdl_setvisual
void MdlVisual::setVisual(const Skeleton *v) {
  skeleton = v;
  solver.setSkeleton(skeleton);
  skInst->setSkeleton(v);
  head  .setAttachPoint(skeleton);
  view  .setAttachPoint(skeleton);
  setPos(pos); // update obj matrix
  }

// Mdl_SetVisualBody
void MdlVisual::setVisualBody(MeshObjects::Mesh &&h, MeshObjects::Mesh &&body) {
  head    = std::move(h);
  view    = std::move(body);

  head.setAttachPoint(skeleton,"BIP01 HEAD");
  view.setAttachPoint(skeleton);
  }

bool MdlVisual::hasOverlay(const Skeleton* sk) const {
  return solver.hasOverlay(sk);
  }

// Mdl_ApplyOverlayMdsTimed, Mdl_ApplyOverlayMds
void MdlVisual::addOverlay(const Skeleton *sk, uint64_t time) {
  solver.addOverlay(sk,time);
  }

// Mdl_RemoveOverlayMDS
void MdlVisual::delOverlay(const char *sk) {
  solver.delOverlay(sk);
  }

// Mdl_RemoveOverlayMDS
void MdlVisual::delOverlay(const Skeleton *sk) {
  solver.delOverlay(sk);
  }

void MdlVisual::setArmour(MeshObjects::Mesh &&a) {
  view = std::move(a);
  view.setAttachPoint(skeleton);
  setPos(pos);
  }

void MdlVisual::setSword(MeshObjects::Mesh &&s) {
  sword = std::move(s);
  setPos(pos);
  }

void MdlVisual::setRangeWeapon(MeshObjects::Mesh &&b) {
  bow = std::move(b);
  setPos(pos);
  }

void MdlVisual::setAmmoItem(MeshObjects::Mesh&& a, const char *bone) {
  ammunition = std::move(a);
  ammunition.setAttachPoint(skeleton,bone);
  setPos(pos);
  }

void MdlVisual::setMagicWeapon(PfxObjects::Emitter &&spell) {
  pfx = std::move(spell);
  setPos(pos);
  }

void MdlVisual::setSlotItem(MeshObjects::Mesh &&itm, const char *bone) {
  if(bone==nullptr)
    return;

  for(auto& i:item) {
    const char* b = i.attachPoint();
    if(b!=nullptr && std::strcmp(b,bone)==0) {
      i = std::move(itm);
      i.setAttachPoint(skeleton,bone);
      break;
      }
    }
  itm.setAttachPoint(skeleton,bone);
  item.emplace_back(std::move(itm));
  setPos(pos);
  }

void MdlVisual::setStateItem(MeshObjects::Mesh&& a, const char* bone) {
  stateItm = std::move(a);
  stateItm.setAttachPoint(skeleton,bone);
  setPos(pos);
  }

void MdlVisual::clearSlotItem(const char *bone) {
  for(size_t i=0;i<item.size();++i) {
    const char* b = item[i].attachPoint();
    if(bone==nullptr || (b!=nullptr && std::strcmp(b,bone)==0)) {
      item[i] = std::move(item.back());
      item.pop_back();
      }
    }
  setPos(pos);
  }

bool MdlVisual::setFightMode(const ZenLoad::EFightMode mode) {
  WeaponState f=WeaponState::NoWeapon;

  switch(mode) {
    case ZenLoad::FM_LAST:
      return false;
    case ZenLoad::FM_NONE:
      f=WeaponState::NoWeapon;
      break;
    case ZenLoad::FM_FIST:
      f=WeaponState::Fist;
      break;
    case ZenLoad::FM_1H:
      f=WeaponState::W1H;
      break;
    case ZenLoad::FM_2H:
      f=WeaponState::W2H;
      break;
    case ZenLoad::FM_BOW:
      f=WeaponState::Bow;
      break;
    case ZenLoad::FM_CBOW:
      f=WeaponState::CBow;
      break;
    case ZenLoad::FM_MAG:
      f=WeaponState::Mage;
      break;
    }

  return setToFightMode(f);
  }

bool MdlVisual::setToFightMode(const WeaponState f) {
  if(f==fgtMode)
    return false;
  fgtMode = f;
  return true;
  }

void MdlVisual::updateWeaponSkeleton(const Item* weapon,const Item* range) {
  auto st = fgtMode;
  if(st==WeaponState::W1H || st==WeaponState::W2H){
    sword.setAttachPoint(skeleton,"ZS_RIGHTHAND");
    } else {
    bool twoHands = weapon!=nullptr && weapon->is2H();
    sword.setAttachPoint(skeleton,twoHands ? "ZS_LONGSWORD" : "ZS_SWORD");
    }

  if(st==WeaponState::Bow || st==WeaponState::CBow){
    if(st==WeaponState::Bow)
      bow.setAttachPoint(skeleton,"ZS_LEFTHAND"); else
      bow.setAttachPoint(skeleton,"ZS_RIGHTHAND");
    } else {
    bool cbow  = range!=nullptr && range->isCrossbow();
    bow.setAttachPoint(skeleton,cbow ? "ZS_CROSSBOW" : "ZS_BOW");
    }
  if(st==WeaponState::Mage)
    pfx.setAttachPoint(skeleton,"ZS_RIGHTHAND");
  pfx.setActive(st==WeaponState::Mage);
  }

void MdlVisual::updateAnimation(Npc& npc,int comb) {
  Pose&    pose      = *skInst;
  uint64_t tickCount = npc.world().tickCount();

  if(npc.world().isInListenerRange(npc.position()))
    pose.processSfx(npc,tickCount);

  solver.update(tickCount);
  pose.update(solver,comb,tickCount);

  head      .setSkeleton(pose,pos);
  sword     .setSkeleton(pose,pos);
  bow       .setSkeleton(pose,pos);
  ammunition.setSkeleton(pose,pos);
  stateItm  .setSkeleton(pose,pos);
  for(auto& i:item)
    i.setSkeleton(pose,pos);
  pfx .setSkeleton(pose,pos);
  view.setSkeleton(pose,pos);
  }

Vec3 MdlVisual::mapBone(const char* b) const {
  Pose&  pose = *skInst;
  size_t id   = skeleton->findNode(b);
  if(id==size_t(-1))
    return {0,0,0};

  auto mat = pos;
  mat.mul(pose.bone(id));

  return {mat.at(3,0) - pos.at(3,0),
          mat.at(3,1) - pos.at(3,1),
          mat.at(3,2) - pos.at(3,2)};
  }

Vec3 MdlVisual::mapWeaponBone() const {
  if(fgtMode==WeaponState::Bow || fgtMode==WeaponState::CBow)
    return mapBone(ammunition.attachPoint());
  if(fgtMode==WeaponState::Mage)
    return mapBone("ZS_RIGHTHAND");
  return {0,0,0};
  }

void MdlVisual::stopAnim(Npc& npc,const char* ani) {
  skInst->stopAnim(ani);
  if(!skInst->hasAnim())
    startAnimAndGet(npc,AnimationSolver::Idle,fgtMode,npc.walkMode());
  }

void MdlVisual::stopItemStateAnim(Npc& npc) {
  skInst->stopItemStateAnim();
  if(!skInst->hasAnim())
    startAnimAndGet(npc,AnimationSolver::Idle,fgtMode,npc.walkMode());
  }

void MdlVisual::stopWalkAnim(Npc &npc) {
  auto state = pose().bodyState();
  if(state!=BS_STAND && state!=BS_MOBINTERACT) {
    skInst->stopAnim(nullptr);
    startAnimAndGet(npc,AnimationSolver::Idle,fgtMode,npc.walkMode());
    }
  }

bool MdlVisual::isStanding() const {
  return skInst->isStanding();
  }

bool MdlVisual::isItem() const {
  return skInst->isItem();
  }

bool MdlVisual::isAnimExist(const char* name) const {
  const Animation::Sequence *sq = solver.solveFrm(name);
  return sq!=nullptr;
  }

const Animation::Sequence* MdlVisual::startAnimAndGet(Npc &npc, const char *name,
                                                      bool forceAnim, BodyState bs) {
  const Animation::Sequence *sq = solver.solveFrm(name);
  if(skInst->startAnim(solver,sq,bs,forceAnim,npc.world().tickCount())) {
    return sq;
    }
  return nullptr;
  }

const Animation::Sequence* MdlVisual::startAnimAndGet(Npc& npc, AnimationSolver::Anim a,
                                                      WeaponState st, WalkBit wlk) {
  // for those use MdlVisual::setRotation
  assert(a!=AnimationSolver::Anim::RotL && a!=AnimationSolver::Anim::RotR);

  if(a==AnimationSolver::InteractIn ||
     a==AnimationSolver::InteractOut ||
     a==AnimationSolver::InteractToStand ||
     a==AnimationSolver::InteractFromStand) {
    auto inter = npc.interactive();
    const Animation::Sequence *sq = solver.solveAnim(inter,a,*skInst);
    if(sq!=nullptr){
      if(skInst->startAnim(solver,sq,BS_MOBINTERACT,false,npc.world().tickCount())) {
        return sq;
        }
      }
    return nullptr;
    }

  const Animation::Sequence *sq = solver.solveAnim(a,st,wlk,*skInst);

  bool forceAnim=false;
  if(a==AnimationSolver::Anim::DeadA || a==AnimationSolver::Anim::UnconsciousA ||
     a==AnimationSolver::Anim::DeadB || a==AnimationSolver::Anim::UnconsciousB) {
    if(sq!=nullptr)
      skInst->stopAllAnim();
    forceAnim = true;
    }
  if(a==AnimationSolver::Anim::StumbleA || a==AnimationSolver::Anim::StumbleB ||
     a==AnimationSolver::Anim::JumpHang)
    forceAnim = true;

  BodyState bs = BS_NONE;
  switch(a) {
    case AnimationSolver::Anim::NoAnim:
    case AnimationSolver::Anim::Fallen:
      bs = BS_NONE;
      break;
    case AnimationSolver::Anim::Idle:
    case AnimationSolver::Anim::MagNoMana:
      bs = BS_STAND;
      break;
    case AnimationSolver::Anim::DeadA:
    case AnimationSolver::Anim::DeadB:
      bs = BS_DEAD;
      break;
    case AnimationSolver::Anim::UnconsciousA:
    case AnimationSolver::Anim::UnconsciousB:
      bs = BS_UNCONSCIOUS;
      break;
    case AnimationSolver::Anim::Move:
    case AnimationSolver::Anim::MoveL:
    case AnimationSolver::Anim::MoveR:
    case AnimationSolver::Anim::MoveBack:
      if(bool(wlk & WalkBit::WM_Walk))
        bs = BS_WALK; else
        bs = BS_RUN;
      break;
    case AnimationSolver::Anim::RotL:
    case AnimationSolver::Anim::RotR:
      break;
    case AnimationSolver::Anim::Jump:
    case AnimationSolver::Anim::JumpUp:
      bs = BS_JUMP;
      break;
    case AnimationSolver::Anim::JumpUpLow:
    case AnimationSolver::Anim::JumpUpMid:
    case AnimationSolver::Anim::JumpHang:
      bs = BS_CLIMB;
      break;
    case AnimationSolver::Anim::Fall:
    case AnimationSolver::Anim::FallDeep:
      bs = BS_FALL;
      break;
    case AnimationSolver::Anim::SlideA:
    case AnimationSolver::Anim::SlideB:
      bs = BS_NONE;
      break;
    case AnimationSolver::Anim::InteractIn:
    case AnimationSolver::Anim::InteractOut:
    case AnimationSolver::Anim::InteractToStand:
    case AnimationSolver::Anim::InteractFromStand:
      bs = BS_MOBINTERACT;
      break;
    case AnimationSolver::Anim::Atack:
    case AnimationSolver::Anim::AtackL:
    case AnimationSolver::Anim::AtackR:
    case AnimationSolver::Anim::AtackFinish:
      bs = pose().bodyState()==BS_RUN ? BS_RUN : BS_NONE;
      break;
    case AnimationSolver::Anim::AtackBlock:
      bs = BS_PARADE;
      break;
    case AnimationSolver::Anim::StumbleA:
    case AnimationSolver::Anim::StumbleB:
      bs = BS_PARADE;
      break;
    case AnimationSolver::Anim::AimBow:
      bs = BS_AIMNEAR; //TODO: BS_AIMFAR
      break;
    }

  if(bool(wlk & WalkBit::WM_Swim))
    bs = BS_SWIM;

  if(skInst->startAnim(solver,sq,bs,forceAnim,npc.world().tickCount())) {
    return sq;
    }
  return nullptr;
  }

bool MdlVisual::startAnim(Npc &npc, WeaponState st) {
  const bool run = (skInst->bodyState()&BS_MAX)==BS_RUN;

  if(st==fgtMode)
    return true;
  const Animation::Sequence *sq = solver.solveAnim(st,fgtMode,run);
  if(sq==nullptr)
    return false;
  if(skInst->startAnim(solver,sq,run ? BS_RUN : BS_NONE,false,npc.world().tickCount()))
    return true;
  return false;
  }

void MdlVisual::setRotation(Npc &npc, int dir) {
  skInst->setRotation(solver,npc,fgtMode,dir);
  }

void MdlVisual::interrupt() {
  skInst->interrupt();
  }

Tempest::Vec3 MdlVisual::displayPosition() const {
  if(skeleton!=nullptr)
    return {0,skeleton->colisionHeight()*1.5f,0};
  return {0.f,0.f,0.f};
  }

const Animation::Sequence* MdlVisual::continueCombo(Npc& npc, AnimationSolver::Anim a,
                                                    WeaponState st, WalkBit wlk)  {
  if(st==WeaponState::Fist || st==WeaponState::W1H || st==WeaponState::W2H) {
    const Animation::Sequence *sq = solver.solveAnim(a,st,wlk,*skInst);
    if(auto ret = skInst->continueCombo(solver,sq,npc.world().tickCount()))
      return ret;
    }
  return startAnimAndGet(npc,a,st,wlk);
  }

uint32_t MdlVisual::comboLength() const {
  return skInst->comboLength();
  }

bool MdlVisual::startAnimItem(Npc &npc, const char *scheme) {
  return skInst->setAnimItem(solver,npc,scheme);
  }

bool MdlVisual::startAnimSpell(Npc &npc, const char *scheme) {
  char name[128]={};
  std::snprintf(name,sizeof(name),"S_%sSHOOT",scheme);

  const Animation::Sequence *sq = solver.solveFrm(name);
  if(skInst->startAnim(solver,sq,BS_CASTING,true,npc.world().tickCount())) {
    return true;
    }
  return false;
  }

bool MdlVisual::startAnimDialog(Npc &npc) {
  if((npc.bodyState()&BS_FLAG_FREEHANDS)==0 || fgtMode!=WeaponState::NoWeapon)
    return true;

  //const int countG1 = 21;
  const int countG2 = 11;
  const int id      = std::rand()%countG2 + 1;

  char name[32]={};
  std::snprintf(name,sizeof(name),"T_DIALOGGESTURE_%02d",id);

  const Animation::Sequence *sq = solver.solveFrm(name);
  if(skInst->startAnim(solver,sq,BS_STAND,false,npc.world().tickCount())) {
    return true;
    }
  return false;
  }

void MdlVisual::stopDlgAnim() {
  //const int countG1 = 21;
  const int countG2 = 11;
  for(uint16_t i=0; i<countG2; i++){
    char buf[32]={};
    std::snprintf(buf,sizeof(buf),"T_DIALOGGESTURE_%02d",i+1);
    skInst->stopAnim(buf);
    }
  }
