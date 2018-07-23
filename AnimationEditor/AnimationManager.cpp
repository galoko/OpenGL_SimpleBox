#include "AnimationManager.hpp"

#include "PhysicsManager.hpp"
#include "CharacterManager.hpp"

using namespace psm;

typedef struct AnimationManagerInfo {

	bool IsBlocked;

	btPoint2PointConstraint* PositionConstraint;
	btGeneric6DofConstraint* RotationConstraint;

} AnimationManagerInfo;

void AnimationManager::Initialize(void) {

	PhysicsManager::GetInstance().PreSolveCallback = bind(&AnimationManager::PhysicsPreSolve, this);
}

void AnimationManager::Tick(double dt) {

	PhysicsManager::GetInstance().Tick(dt);
}

void AnimationManager::SetupInverseKinematicMass(Bone* PickedBone) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {
		
		// float NewMass = Bone->Mass * (Bone == PickedBone ? 1 : (Bone->IsLocked ? 0 : 0.01f));
		// float NewMass = Bone->Mass * (Bone == PickedBone ? 1 : (Bone->IsLocked ? 0 : 1));
		float NewMass = Bone->Mass;

		PhysicsManager::GetInstance().ChangeObjectMass(Bone->PhysicBody, NewMass);
	}
}

void AnimationManager::InverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 WorldDestPoint) {

	SetupInverseKinematicMass(Bone);

	InverseKinematicTask.Bone = Bone;
	InverseKinematicTask.LocalPoint = LocalPoint;

	InverseKinematic(WorldDestPoint);
}

void AnimationManager::InverseKinematic(vec3 WorldDestPoint) {

	InverseKinematicTask.WorldDestPoint = WorldDestPoint;
	InverseKinematicTask.IsActive = true;
}

void AnimationManager::CancelInverseKinematic(void)
{
	InverseKinematicTask.IsActive = false;
}

bool AnimationManager::IsBoneBlocked(Bone* Bone)
{
	return false;
}

void AnimationManager::PhysicsPreSolve(void) {

	Character* Char = CharacterManager::GetInstance().GetCharacter();

	for (Bone* Bone : Char->Bones) {
		
		btRigidBody* Body = Bone->PhysicBody;

		Body->setLinearVelocity(GLMToBullet({ 0, 0, 0 }));
		Body->setAngularVelocity(GLMToBullet({ 0, 0, 0 }));

		mat4 World = PhysicsManager::GetBoneWorldTransform(Bone);

		vec3 LocalBonePoint = InverseKinematicTask.LocalPoint;
		vec3 WorldBonePoint = World * vec4(LocalBonePoint, 1);
		vec3 WorldBoneCenter = World * Bone->MiddleTranslation * vec4(0, 0, 0, 1);

		vec3 Delta = InverseKinematicTask.WorldDestPoint - WorldBonePoint;

		const float dist_tolerance = 0.01f;
		float dist = length(Delta);
		if (dist <= dist_tolerance)
			InverseKinematicTask.IsActive = false;

		if (InverseKinematicTask.IsActive && Bone == InverseKinematicTask.Bone) {

			float new_dist = min(max(dist_tolerance * 2.0f, dist), 1.0f);

			vec3 Force = Delta * (new_dist / dist) * 10000.0f / (float)Body->getInvMass();
			
			vec3 RelativePos = WorldBonePoint - WorldBoneCenter;

			Body->applyForce(GLMToBullet(Force), GLMToBullet(RelativePos));
		}
	}
}
