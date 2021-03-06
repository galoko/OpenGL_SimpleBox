#include "InputManager.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include "Render.hpp"
#include "PoseManager.hpp"
#include "CharacterManager.hpp"
#include "Form.hpp"

void InputManager::Initialize(void) {

}

void InputManager::SetWindow(HWND WindowHandle)
{
	this->WindowHandle = WindowHandle;

	RAWINPUTDEVICE Rid[1] = {};

	Rid[0].usUsagePage = 0x01;
	Rid[0].usUsage = 0x02;
	Rid[0].dwFlags = RIDEV_INPUTSINK;
	Rid[0].hwndTarget = WindowHandle;

	if (!RegisterRawInputDevices(Rid, 1, sizeof(Rid[0])))
		throw new runtime_error("Couldn't register raw devices");
}

void InputManager::Tick(double dt) {

	ProcessKeyboardInput(dt);

	if (IsStateInverseKinematic(State) && Selection.HaveBone())
		PoseManager::GetInstance().InverseKinematic(Selection.Bone, Selection.LocalPoint, Selection.GetWorldPoint());
}

void InputManager::SetFocus(bool IsInFocusNow) {

	if (IsInFocusNow != IsInFocus) {

		IsInFocus = IsInFocusNow;

		ProcessMouseLockState();
	}
}

bool InputManager::IsStateInverseKinematic(InputState State)
{
	return State == InverseKinematic || State == InverseKinematicAutomate;
}

InputState InputManager::GetState(void)
{
	return State;
}

InputSelection InputManager::GetSelection(void)
{
	return Selection;
}

void InputManager::ProcessMouseLockState(void)
{
	if (IsCameraMode && IsInFocus) {

		POINT CursorPoint;
		if (!IsMouseLockEnforced && GetCursorPos(&CursorPoint)) {

			SkipMouseFormEventCount++;

			RECT Rect = { CursorPoint.x, CursorPoint.y, CursorPoint.x + 1, CursorPoint.y + 1 };
			ClipCursor(&Rect);

			ShowCursor(false);

			IsMouseLockEnforced = true;
		}
	}
	else if (IsMouseLockEnforced) {

		SkipMouseFormEventCount++;

		ShowCursor(true);
		ClipCursor(NULL);

		IsMouseLockEnforced = false;

		if (State == InverseKinematic && Selection.HaveBone()) 
			SetCursorToWorldPoint(Selection.GetWorldPoint());
	}
}

void InputManager::ProcessMouseInput(LONG dx, LONG dy) {

	if (IsCameraMode && IsInFocus) {

		const float Speed = 1.0f / 300;
		Render::GetInstance().RotateCamera((float)dx * Speed, (float)dy * Speed);
	}
}

void InputManager::SetState(InputState NewState)
{
	if (NewState == InverseKinematic && Selection.HaveBone())
		SetCursorToWorldPoint(Selection.GetWorldPoint());

	if (IsStateInverseKinematic(State) && !IsStateInverseKinematic(NewState))
		PoseManager::GetInstance().CancelInverseKinematic();

	State = NewState;
}

bool InputManager::IsInteractionMode(void)
{
	return !IsCameraMode;
}

void InputManager::SelectBoneAtScreenPoint(LONG x, LONG y) {

	Form::UpdateLock Lock;

	if (Selection.HaveBone()) {

		CancelSelection();

		if (State != None) 
			return;
	}

	Bone* SelectedBone;
	vec3 WorldPoint, WorldNormal;

	Render::GetInstance().GetBoneFromScreenPoint(MouseX, MouseY, SelectedBone, WorldPoint, WorldNormal);

	if (SelectedBone != nullptr) {

		Selection.Bone = SelectedBone;
		Selection.LocalPoint = inverse(SelectedBone->WorldTransform * SelectedBone->MiddleTranslation) * vec4(WorldPoint, 1);
		Selection.SetWorldPoint(WorldPoint);

		Form::GetInstance().UpdateBlocking();
		Form::GetInstance().UpdatePositionAndAngles();

		if (State == InverseKinematicAutomate)
			SetState(InverseKinematic);
	}
}

void InputManager::CancelSelection(void) {

	Selection.Bone = nullptr;

	if (IsStateInverseKinematic(State))
		PoseManager::GetInstance().CancelInverseKinematic();

	Form::GetInstance().UpdateBlocking();
	Form::GetInstance().UpdatePositionAndAngles();
}

void InputManager::ProcessCameraMovement(double dt)
{
	const float Speed = 1.0f;

	vec3 Offset = {};
	vec3 Direction = Render::GetInstance().GetLookingDirection();
	vec3 SideDirection = cross(Direction, Render::GetInstance().Up);
	vec3 ForwardDirection = cross(Render::GetInstance().Up, SideDirection);
	vec3 UpDirection = Render::GetInstance().Up;

	if (IsPressed('W'))
		Offset += ForwardDirection;
	if (IsPressed('S'))
		Offset -= ForwardDirection;
	if (IsPressed('D'))
		Offset += SideDirection;
	if (IsPressed('A'))
		Offset -= SideDirection;
	if (IsPressed(VK_LSHIFT))
		Offset -= UpDirection;
	if (IsPressed(VK_SPACE))
		Offset += UpDirection;

	if (length(Offset) > 0) {

		WasMoving = true;

		Render::GetInstance().MoveCamera(normalize(Offset) * Speed * (float)dt);

		if (State == InverseKinematic && Selection.HaveBone())
			SetCursorToWorldPoint(Selection.GetWorldPoint());
	}
	else 
		WasMoving = false;
}

vec3 InputManager::GetPlaneNormal(void)
{	
	vec3 CameraNormal = -Render::GetInstance().GetLookingDirection();

	vec3 Result = { 0, 0, 0 };

	switch (PlaneMode) {
	case PlaneCamera:
		Result = CameraNormal;
		break;
	case PlaneX:
		Result = { 1, 0, 0 };
		break;
	case PlaneY:
		Result = { 0, 1, 0 };
		break;
	case PlaneZ:
		Result = { 0, 0, 1 };
		break;
	}

	float Direction = dot(CameraNormal, Result) > 0 ? 1.0f : -1.0f;

	return Result * Direction;
}

void InputManager::ChangeBoneAngles(Bone* Bone, vec3 Angles)
{
	mat4 PreviousM = Bone->WorldTransform * Bone->MiddleTranslation;

	PhysicsManager::GetInstance().SetBoneAngles(Bone, Angles);

	mat4 CurrentM = Bone->WorldTransform * Bone->MiddleTranslation;

	if (Selection.HaveBone()) {

		vec3 WorldPoint = Selection.GetWorldPoint();

		WorldPoint = CurrentM * inverse(PreviousM) * vec4(WorldPoint, 1);

		Selection.SetWorldPoint(WorldPoint);
	}
}

void InputManager::SetupInverseKinematic(Bone* Bone, vec3 LocalPoint, vec3 DestWorldPoint, bool IsAutomatic)
{
	Selection.Bone = Bone;
	Selection.LocalPoint = LocalPoint;
	Selection.SetWorldPoint(DestWorldPoint);

	SetState(IsAutomatic ? InverseKinematicAutomate: InverseKinematic);
}

void InputManager::UpdateAnimationKinematicMode(void)
{
	bool ShouldBeKinematic = SerializationManager::GetInstance().IsInKinematicMode();
	bool IsKinematic = State == AnimationKinematic;
	if (ShouldBeKinematic != IsKinematic) 
		SetState(ShouldBeKinematic ? AnimationKinematic : None);
}

void InputManager::SetWorldPointToScreePoint(LONG x, LONG y) {

	vec3 Point, Direction;
	Render::GetInstance().GetPointAndDirectionFromScreenPoint(x, y, Point, Direction);

	vec3 PlaneNormal = GetPlaneNormal();

	float c = dot(Direction, PlaneNormal);
	if (fabs(c) > 10e-7) {

		float t = -(dot(Point, PlaneNormal) - dot(Selection.GetWorldPoint(), PlaneNormal)) / c;
		vec3 P = Point + t * Direction;

		Selection.SetWorldPoint(P);
	}
}

void InputManager::SetCursorToWorldPoint(vec3 WorldPoint)
{
	LONG x, y;
	Render::GetInstance().GetScreenPointFromPoint(Selection.GetWorldPoint(), x, y);

	POINT ScreenPoint = { x, y };
	ClientToScreen(WindowHandle, &ScreenPoint);

	SkipMouseFormEventCount++;

	if (IsMouseLockEnforced)
		ClipCursor(NULL);

	SetCursorPos(ScreenPoint.x, ScreenPoint.y);

	if (IsMouseLockEnforced) {
		RECT Rect = { ScreenPoint.x, ScreenPoint.y, ScreenPoint.x + 1, ScreenPoint.y + 1 };
		ClipCursor(&Rect);
	}
}

void InputManager::ProcessKeyboardInput(double dt) {

	if (!SerializationManager::GetInstance().IsInKinematicMode()) {

		if (WasPressed(VK_LBUTTON) && IsInteractionMode()) {

			SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput VK_LBUTTON");
			SelectBoneAtScreenPoint(MouseX, MouseY);
		}

		if (WasPressed(VK_ESCAPE)) {

			if (State == None)
				CancelSelection();
			else
				SetState(None);
		}

		if (WasPressed('Q')) {

			SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput Q");

			if (State == None)
				SetupInverseKinematic(Selection.Bone, Selection.LocalPoint, Selection.GetWorldPoint(), false);
			else {
				if (State == InverseKinematicAutomate)
					SetState(InverseKinematic);
				else
				if (State == InverseKinematic)
					SetState(None);
			}
		}

		if (WasPressed('U') && Selection.HaveBone()) {

			SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput U");

			if (!IsPressed(VK_LCONTROL)) {

				BlockingInfo Blocking = PoseManager::GetInstance().GetBoneBlocking(Selection.Bone);
				if (Blocking.IsFullyBlocked())
					Blocking = BlockingInfo::GetAllUnblocked();
				else
					Blocking = BlockingInfo::GetAllBlocked();

				PoseManager::GetInstance().SetBoneBlocking(Selection.Bone, Blocking);
			}
			else
				PoseManager::GetInstance().BlockEverythingExceptThisBranch(Selection.Bone->Parent, Selection.Bone);
		}

		if (IsPressed(VK_LCONTROL)) {

			MouseUnlockTime = GetTickCount64() + StateSwapMouseTimeout;

			if (WasPressed('Z'))
				SerializationManager::GetInstance().PopAndDeserializeStateFrame(false);
			else
			if (WasPressed('X'))
				SerializationManager::GetInstance().PopAndDeserializeStateFrame(true);
		}

		if (WasPressed('R')) {

			InputSelection Selection = InputManager::GetInstance().GetSelection();

			if (Selection.HaveBone()) {

				SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput R");

				if (!PoseManager::GetInstance().IsBonePositionConstrained(Selection.Bone))
					PoseManager::GetInstance().ConstrainBonePosition(Selection.Bone, Selection.GetWorldPoint());
				else
					PoseManager::GetInstance().RemoveBonePositionConstraint(Selection.Bone);
			}
		}

		if (WasPressed('I')) {

			InputSelection Selection = InputManager::GetInstance().GetSelection();

			if (Selection.HaveBone()) {

				SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput I");

				Character* Char = CharacterManager::GetInstance().GetCharacter();

				Selection.Bone->Rotation = mat4(1.0f);
				Char->UpdateWorldTranforms();

				PhysicsManager::GetInstance().SyncWorldWithCharacter();
			}
		}

		if (WasPressed('P')) {

			SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput P");

			PoseManager::GetInstance().UnblockAllBones();
		}

		if (WasPressed('K') && Selection.HaveBone()) {

			SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput K");

			BlockingInfo Blocking = PoseManager::GetInstance().GetBoneBlocking(Selection.Bone);

			Blocking.XAxis = !Blocking.XAxis;
			Blocking.YAxis = !Blocking.YAxis;
			Blocking.ZAxis = !Blocking.ZAxis;

			PoseManager::GetInstance().SetBoneBlocking(Selection.Bone, Blocking);
		}

		if (!IsPressed(VK_CONTROL) && (WasPressed('Z') || WasPressed('X') || WasPressed('C') || WasPressed('V'))) {

			enum PlaneMode NextPlaneMode;

			if (WasPressed('Z'))
				NextPlaneMode = PlaneZ;
			else
				if (WasPressed('X'))
					NextPlaneMode = PlaneX;
				else
					if (WasPressed('C'))
						NextPlaneMode = PlaneY;
					else
						if (WasPressed('V'))
							NextPlaneMode = PlaneCamera;

			if (PlaneMode != NextPlaneMode) {

				SerializationManager::GetInstance().PushStateFrame(L"ProcessKeyboardInput Z|X|C|V");

				PlaneMode = NextPlaneMode;
			}
		}
	}

	if (WasPressed(VK_RBUTTON)) {

		IsCameraMode = true;
		ProcessMouseLockState();
	}
	else
	if (WasUnpressed(VK_RBUTTON)) {

		IsCameraMode = false;
		ProcessMouseLockState();
	}

	ProcessCameraMovement(dt);

	// erase previous state
	memcpy(LastKeyboardState, CurrentKeyboardState, sizeof(LastKeyboardState));
}

void InputManager::ProcessMouseFormEvent(LONG x, LONG y) {

	if (MouseX == x && MouseY == y)
		return;

	MouseX = x;
	MouseY = y;

	if (SkipMouseFormEventCount > 0) {
		SkipMouseFormEventCount--;
		return;
	}

	ULONGLONG Now = GetTickCount64();
	if (MouseUnlockTime > Now)
		return;

	if (!SerializationManager::GetInstance().IsInKinematicMode()) {

		if (State == None && IsPressed(VK_LBUTTON))
			SelectBoneAtScreenPoint(MouseX, MouseY);

		if (State == InverseKinematic && !WasMoving && !IsCameraMode && Selection.HaveBone()) {

			SerializationManager::GetInstance().PushPendingStateFrame(PendingInverseKinematic, L"ProcessMouseFormEvent");

			SetWorldPointToScreePoint(MouseX, MouseY);
		}
	}
}

void InputManager::ProcessKeyboardEvent(void) {

	UpdateKeyboardState();
	ProcessKeyboardInput(0);
}

void InputManager::ProcessMouseWheelEvent(float Delta) {

	if (!SerializationManager::GetInstance().IsInKinematicMode()) {

		if (State == InverseKinematic && Selection.HaveBone()) {

			SerializationManager::GetInstance().PushPendingStateFrame(PendingInverseKinematic, L"ProcessMouseWheelEvent");

			vec3 PlaneNormal = GetPlaneNormal();

			Selection.SetWorldPoint(Selection.GetWorldPoint() - PlaneNormal * 0.05f * Delta);

			SetCursorToWorldPoint(Selection.GetWorldPoint());
		}
	}
}

void InputManager::Serialize(InputSerializedState& State)
{
	State.State = (uint32)this->State;
	State.PlaneMode = (uint32)this->PlaneMode;

	State.BoneName = Selection.HaveBone() ? Selection.Bone->GetName() : L"";
	State.LocalPoint = Selection.LocalPoint;
	State.WorldPoint = Selection.GetWorldPoint();
}

void InputManager::Deserialize(InputSerializedState& State)
{
	Character* Char = CharacterManager::GetInstance().GetCharacter();

	this->PlaneMode = (enum PlaneMode)State.PlaneMode;

	Selection.Bone = State.BoneName != L"" ? Char->FindBone(State.BoneName) : nullptr;
	Selection.LocalPoint = State.LocalPoint;
	Selection.SetWorldPoint(State.WorldPoint);

	if (Selection.HaveBone()) {

		SetState((InputState)State.State);
	}
	else
		SetState(None);
}

// General API

#define IsKeyPressed(x, y) ((x[y] & 0x80) != 0)

bool InputManager::WasPressed(int Key)
{
	return !IsKeyPressed(LastKeyboardState, Key) && IsKeyPressed(CurrentKeyboardState, Key);
}

bool InputManager::IsPressed(int Key)
{
	return IsKeyPressed(CurrentKeyboardState, Key);
}

bool InputManager::WasUnpressed(int Key)
{
	return IsKeyPressed(LastKeyboardState, Key) && !IsKeyPressed(CurrentKeyboardState, Key);
}

void InputManager::UpdateKeyboardState(void)
{
	GetKeyState(0);
	GetKeyboardState(CurrentKeyboardState);
}

// InputSelection

vec3 InputSelection::GetWorldPoint(void)
{
	return WorldPoint;
}

void InputSelection::SetWorldPoint(vec3 WorldPoint)
{
	const float BoxSize = 3.0f;

	WorldPoint.x = std::max(-BoxSize, std::min(WorldPoint.x, BoxSize));
	WorldPoint.y = std::max(-BoxSize, std::min(WorldPoint.y, BoxSize));
	WorldPoint.z = std::max(-BoxSize, std::min(WorldPoint.z, BoxSize));

	this->WorldPoint = WorldPoint;
}

bool InputSelection::HaveBone(void)
{
	return Bone != nullptr;
}
