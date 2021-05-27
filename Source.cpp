#include "DirectOverlay.h"
#include <sstream>
#include <string>
#include <algorithm>
#include <list>


//credits to big paster ritz lol
//
//

//

#define M_PI 3.14159265358979323846264338327950288419716939937510

#define OFFSET_UWORLD 0x9643c40



bool Menu = true;
bool Aimbot = true;
bool Softaim = false;
bool EnemyESP = true;
bool skeleton = true;
bool BoxESP = true;
bool LineESP = true;
bool DistanceESP = false;
bool DrawFov = true;

DWORD processID;
HWND hwnd = NULL;
 
int width;
int height;
int localplayerID;
int smoothing = 1;
float FovAngle;

HANDLE DriverHandle;
uint64_t base_address;

DWORD_PTR Uworld;
DWORD_PTR LocalPawn;
DWORD_PTR Localplayer;
DWORD_PTR Rootcomp;
DWORD_PTR PlayerController;
DWORD_PTR Ulevel;

Vector3 localactorpos;
Vector3 Localcam;

float AimFOV = 14; 

bool isaimbotting;
DWORD_PTR entityx;

FTransform GetBoneIndex(DWORD_PTR mesh, int index)
{
	DWORD_PTR bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x4A8);  

	if (bonearray == NULL) 
	{
		bonearray = read<DWORD_PTR>(DriverHandle, processID, mesh + 0x4A8 + 0x10); 
	}

	return read<FTransform>(DriverHandle, processID, bonearray + (index * 0x30));  
}

Vector3 GetBoneWithRotation(DWORD_PTR mesh, int id)
{
	FTransform bone = GetBoneIndex(mesh, id);
	FTransform ComponentToWorld = read<FTransform>(DriverHandle, processID, mesh + 0x1C0);  

	D3DMATRIX Matrix;
	Matrix = MatrixMultiplication(bone.ToMatrixWithScale(), ComponentToWorld.ToMatrixWithScale());

	return Vector3(Matrix._41, Matrix._42, Matrix._43);
}

D3DMATRIX Matrix(Vector3 rot, Vector3 origin = Vector3(0, 0, 0))
{
	float radPitch = (rot.x * float(M_PI) / 180.f);
	float radYaw = (rot.y * float(M_PI) / 180.f);
	float radRoll = (rot.z * float(M_PI) / 180.f);

	float SP = sinf(radPitch);
	float CP = cosf(radPitch);
	float SY = sinf(radYaw);
	float CY = cosf(radYaw);
	float SR = sinf(radRoll);
	float CR = cosf(radRoll);

	D3DMATRIX matrix;
	matrix.m[0][0] = CP * CY;
	matrix.m[0][1] = CP * SY;
	matrix.m[0][2] = SP;
	matrix.m[0][3] = 0.f;

	matrix.m[1][0] = SR * SP * CY - CR * SY;
	matrix.m[1][1] = SR * SP * SY + CR * CY;
	matrix.m[1][2] = -SR * CP;
	matrix.m[1][3] = 0.f;

	matrix.m[2][0] = -(CR * SP * CY + SR * SY);
	matrix.m[2][1] = CY * SR - CR * SP * SY;
	matrix.m[2][2] = CR * CP;
	matrix.m[2][3] = 0.f;

	matrix.m[3][0] = origin.x;
	matrix.m[3][1] = origin.y;
	matrix.m[3][2] = origin.z;
	matrix.m[3][3] = 1.f;

	return matrix;
}



extern Vector3 CameraEXT(0, 0, 0);

Vector3 ProjectWorldToScreen(Vector3 WorldLocation, Vector3 camrot)
{
	Vector3 Screenlocation = Vector3(0, 0, 0);
	Vector3 Camera;

	auto chain69 = read<uintptr_t>(DriverHandle, processID, Localplayer + 0xa8);
	uint64_t chain699 = read<uintptr_t>(DriverHandle, processID, chain69 + 8);

	Camera.x = read<float>(DriverHandle, processID, chain699 + 0x7F8);  
	Camera.y = read<float>(DriverHandle, processID, Rootcomp + 0x12C);  

	float test = asin(Camera.x);
	float degrees = test * (180.0 / M_PI);
	Camera.x = degrees;

	if (Camera.y < 0)
		Camera.y = 360 + Camera.y;

	D3DMATRIX tempMatrix = Matrix(Camera);
	Vector3 vAxisX, vAxisY, vAxisZ;

	vAxisX = Vector3(tempMatrix.m[0][0], tempMatrix.m[0][1], tempMatrix.m[0][2]);
	vAxisY = Vector3(tempMatrix.m[1][0], tempMatrix.m[1][1], tempMatrix.m[1][2]);
	vAxisZ = Vector3(tempMatrix.m[2][0], tempMatrix.m[2][1], tempMatrix.m[2][2]);

	uint64_t chain = read<uint64_t>(DriverHandle, processID, Localplayer + 0x70);
	uint64_t chain1 = read<uint64_t>(DriverHandle, processID, chain + 0x98);
	uint64_t chain2 = read<uint64_t>(DriverHandle, processID, chain1 + 0x130);

	Vector3 vDelta = WorldLocation - read<Vector3>(DriverHandle, processID, chain2 + 0x10); //camera location credits for Object9999
	Vector3 vTransformed = Vector3(vDelta.Dot(vAxisY), vDelta.Dot(vAxisZ), vDelta.Dot(vAxisX));

	if (vTransformed.z < 1.f)
		vTransformed.z = 1.f;

	float zoom = read<float>(DriverHandle, processID, chain699 + 0x590);

	FovAngle = 80.0f / (zoom / 1.19f);
	float ScreenCenterX = width / 2.0f;
	float ScreenCenterY = height / 2.0f;

	Screenlocation.x = ScreenCenterX + vTransformed.x * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	Screenlocation.y = ScreenCenterY - vTransformed.y * (ScreenCenterX / tanf(FovAngle * (float)M_PI / 360.f)) / vTransformed.z;
	CameraEXT = Camera;

	return Screenlocation;
}


void menu()
{
	//Ritz
	//
	int MenuX = 10;
	int MenuY = 500;

	if (Menu)
	{

		DrawBox(MenuX + 4, MenuY - 5, 2, 223, 0.0f, 0.f, 0.f, 0.f, 100, true);
		DrawBox(MenuX + 164, MenuY - 5, 2, 223, 0.0f, 0.f, 0.f, 0.f, 100, true);
		DrawBox(MenuX + 4, MenuY + 204 + 13, 162, 2, 0.0f, 0.f, 0.f, 0.f, 100, true);
		DrawBox(MenuX + 4, MenuY - 5, 162, 2, 0.0f, 0.f, 0.f, 0.f, 100, true);
		DrawString(_xor_("Title").c_str(), 17, MenuX + 10, MenuY + 3, 0.f, 0.f, 0.f, 255.f);
		DrawString(_xor_("[F7] [F8] Fov Slider").c_str(), 17, MenuX + 10, MenuY + 195, 0.f, 0.f, 0.f, 255.f);

		if (Aimbot)
			DrawString(_xor_("").c_str(), 13, MenuX + 10 + 110, MenuY + 10 + 20 + 10, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("").c_str(), 13, MenuX + 10 + 110, MenuY + 10 + 20 + 10, 255.f, 0.f, 0.f, 255.f);
		
		

		DrawString(_xor_("[F1] > Aimbot").c_str(), 13, MenuX + 10, MenuY + 10 + 20 + 10, 0.f, 0.f, 0.f, 255.f);

		if (Softaim)
			DrawString(_xor_("").c_str(), 13, MenuX + 10 + 93, MenuY + 10 + 40 + 10 + 5, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("").c_str(), 13, MenuX + 10 + 93, MenuY + 10 + 40 + 10 + 5, 255.f, 0.f, 0.f, 255.f);
		

		DrawString(_xor_("[F2] > Softaim").c_str(), 13, MenuX + 10, MenuY + 10 + 40 + 10 + 5, 0.f, 0.f, 0.f, 255.f);

		if (EnemyESP)
			DrawString(_xor_("x").c_str(), 13, MenuX + 10 + 105, MenuY + 10 + 60 + 10 + 8, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("o").c_str(), 13, MenuX + 10 + 105, MenuY + 10 + 60 + 10 + 8, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F3] > Enemy ESP").c_str(), 13, MenuX + 10, MenuY + 10 + 60 + 10 + 8, 0.f, 0.f, 0.f, 255.f);

		if (BoxESP)
			DrawString(_xor_("x").c_str(), 13, MenuX + 10 + 95, MenuY + 10 + 80 + 10 + 8, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("o").c_str(), 13, MenuX + 10 + 95, MenuY + 10 + 80 + 10 + 8, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F4] > Box ESP").c_str(), 13, MenuX + 10, MenuY + 10 + 80 + 10 + 8, 0.f, 0.f, 0.f, 255.f);


		if (LineESP)
			DrawString(_xor_("x").c_str(), 13, MenuX + 10 + 95, MenuY + 10 + 100 + 10 + 8, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("o").c_str(), 13, MenuX + 10 + 95, MenuY + 10 + 100 + 10 + 8, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F5] > Line ESP").c_str(), 13, MenuX + 10, MenuY + 10 + 100 + 10 + 8, 0.f, 0.f, 0.f, 255.f);

		if (DistanceESP)
			DrawString(_xor_("x").c_str(), 13, MenuX + 10 + 120, MenuY + 10 + 120 + 10 + 11, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("o").c_str(), 13, MenuX + 10 + 120, MenuY + 10 + 120 + 10 + 11, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F6] > Distance ESP").c_str(), 13, MenuX + 10, MenuY + 10 + 120 + 10 + 11, 0.f, 0.f, 0.f, 255.f);

		if (skeleton)
			DrawString(_xor_("x").c_str(), 13, MenuX + 10 + 120, MenuY + 10 + 140 + 10 + 14, 0.f, 255.f, 0.f, 255.f);
		else
			DrawString(_xor_("o").c_str(), 13, MenuX + 10 + 120, MenuY + 10 + 140 + 10 + 14, 255.f, 0.f, 0.f, 255.f);

		DrawString(_xor_("[F7] > Skeleton ESP").c_str(), 13, MenuX + 10, MenuY + 10 + 140 + 10 + 14, 0.f, 0.f, 0.f, 255.f);
	}
}

DWORD Menuthread(LPVOID in)
{
	while (1)
	{
		HWND test = FindWindowA(0, _xor_("Fortnite  ").c_str());

		if (test == NULL)
		{
			ExitProcess(0);
		}

		if (GetAsyncKeyState(VK_INSERT) & 1) {
			Menu = !Menu;
		}

		if (Menu)
		{
			if (GetAsyncKeyState(VK_F1) & 1) {
				
				Aimbot = !Aimbot;
				AimFOV = 100;
				smoothing = 1;
			}
			if (GetAsyncKeyState(VK_F2) & 1) {
				Softaim = !Softaim;

				AimFOV = 20;
				smoothing = 17;
			}
			if (GetAsyncKeyState(VK_F3) & 1) {
				EnemyESP = !EnemyESP;
			}

			if (GetAsyncKeyState(VK_F4) & 1) {
				BoxESP = !BoxESP;
			}

			if (GetAsyncKeyState(VK_F5) & 1) {
				LineESP = !LineESP;
			}

			if (GetAsyncKeyState(VK_F6) & 1) {
				DistanceESP = !DistanceESP;
			}

			if (GetAsyncKeyState(VK_F7) & 1) {
				skeleton = !skeleton;
			}
			
			
			

			if (AimFOV) {
				if (GetAsyncKeyState(VK_F8) & 1)
					AimFOV += 1;
				if (GetAsyncKeyState(VK_F9) & 1)
					AimFOV -= 1;
				if (AimFOV < 1)
					AimFOV = 1;
				if (AimFOV > 360)
					AimFOV = 360;
			}
			if (smoothing) {
				if (GetAsyncKeyState(VK_UP) & 1)
					smoothing += 1;
				if (GetAsyncKeyState(VK_DOWN) & 1)
					smoothing -= 1;
				if (smoothing < 1)
					smoothing = 1;
				if (smoothing > 20)
					smoothing = 20;
			}
		}
	}
}

bool GetAimKey()
{
	return (GetAsyncKeyState(VK_RBUTTON));
}

void aimbot(float x, float y)
{
	float ScreenCenterX = (width / 2);
	float ScreenCenterY = (height / 2);
	int AimSpeed = smoothing;
	float TargetX = 0;
	float TargetY = 0;

	if (x != 0)
	{
		if (x > ScreenCenterX)
		{
			TargetX = -(ScreenCenterX - x);
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX > ScreenCenterX * 2) TargetX = 0;
		}

		if (x < ScreenCenterX)
		{
			TargetX = x - ScreenCenterX;
			TargetX /= AimSpeed;
			if (TargetX + ScreenCenterX < 0) TargetX = 0;
		}
	}

	if (y != 0)
	{
		if (y > ScreenCenterY)
		{
			TargetY = -(ScreenCenterY - y);
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY > ScreenCenterY * 2) TargetY = 0;
		}

		if (y < ScreenCenterY)
		{
			TargetY = y - ScreenCenterY;
			TargetY /= AimSpeed;
			if (TargetY + ScreenCenterY < 0) TargetY = 0;
		}
	}

	mouse_event(MOUSEEVENTF_MOVE, static_cast<DWORD>(TargetX), static_cast<DWORD>(TargetY), NULL, NULL);

	return;
}

double GetCrossDistance(double x1, double y1, double x2, double y2)
{
	return sqrt(pow((x2 - x1), 2) + pow((y2 - y1), 2));
}

bool GetClosestPlayerToCrossHair(Vector3 Pos, float& max, float aimfov, DWORD_PTR entity)
{
	if (!GetAimKey() || !isaimbotting)
	{
		if (entity)
		{
			float Dist = GetCrossDistance(Pos.x, Pos.y, width / 2, height / 2);

			if (Dist < max)
			{
				max = Dist;
				entityx = entity;
				AimFOV = aimfov;
			}
		}
	}
	return false;
}

void AimAt(DWORD_PTR entity, Vector3 Localcam)
{
	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280);
	auto rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (rootHeadOut.x != 0 || rootHeadOut.y != 0)
	{
		if ((GetCrossDistance(rootHeadOut.x, rootHeadOut.y, width / 2, height / 2) <= AimFOV * 8) || isaimbotting)
		{
			aimbot(rootHeadOut.x, rootHeadOut.y);
		}
	}
}

void aimbot(Vector3 Localcam)
{
	if (entityx != 0)
	{
		if (GetAimKey())
		{
			AimAt(entityx, Localcam);
		}
		else
		{
			isaimbotting = false;
		}
	}
}

void AIms(DWORD_PTR entity, Vector3 Localcam)
{
	float max = 100.f;

	uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, entity + 0x280); 


	Vector3 rootHead = GetBoneWithRotation(currentactormesh, 66);
	Vector3 rootHeadOut = ProjectWorldToScreen(rootHead, Vector3(Localcam.y, Localcam.x, Localcam.z));

	if (GetClosestPlayerToCrossHair(rootHeadOut, max, AimFOV, entity))
		entityx = entity;
}

void DrawSkeleton(DWORD_PTR mesh)
{
	Vector3 vHeadBone = GetBoneWithRotation(mesh, 96);
	Vector3 vHip = GetBoneWithRotation(mesh, 2);
	Vector3 vNeck = GetBoneWithRotation(mesh, 65);
	Vector3 vUpperArmLeft = GetBoneWithRotation(mesh, 34);
	Vector3 vUpperArmRight = GetBoneWithRotation(mesh, 91);
	Vector3 vLeftHand = GetBoneWithRotation(mesh, 35);
	Vector3 vRightHand = GetBoneWithRotation(mesh, 63);
	Vector3 vLeftHand1 = GetBoneWithRotation(mesh, 33);
	Vector3 vRightHand1 = GetBoneWithRotation(mesh, 60);
	Vector3 vRightThigh = GetBoneWithRotation(mesh, 74);
	Vector3 vLeftThigh = GetBoneWithRotation(mesh, 67);
	Vector3 vRightCalf = GetBoneWithRotation(mesh, 75);
	Vector3 vLeftCalf = GetBoneWithRotation(mesh, 68);
	Vector3 vLeftFoot = GetBoneWithRotation(mesh, 69);
	Vector3 vRightFoot = GetBoneWithRotation(mesh, 76);

	Vector3 vHeadBoneOut = ProjectWorldToScreen(vHeadBone, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vHipOut = ProjectWorldToScreen(vHip, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vNeckOut = ProjectWorldToScreen(vNeck, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vUpperArmLeftOut = ProjectWorldToScreen(vUpperArmLeft, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vUpperArmRightOut = ProjectWorldToScreen(vUpperArmRight, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftHandOut = ProjectWorldToScreen(vLeftHand, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightHandOut = ProjectWorldToScreen(vRightHand, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftHandOut1 = ProjectWorldToScreen(vLeftHand1, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightHandOut1 = ProjectWorldToScreen(vRightHand1, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightThighOut = ProjectWorldToScreen(vRightThigh, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftThighOut = ProjectWorldToScreen(vLeftThigh, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightCalfOut = ProjectWorldToScreen(vRightCalf, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftCalfOut = ProjectWorldToScreen(vLeftCalf, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vLeftFootOut = ProjectWorldToScreen(vLeftFoot, Vector3(Localcam.y, Localcam.x, Localcam.z));
	Vector3 vRightFootOut = ProjectWorldToScreen(vRightFoot, Vector3(Localcam.y, Localcam.x, Localcam.z));

	DrawLine(vHipOut.x, vHipOut.y, vNeckOut.x, vNeckOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vUpperArmLeftOut.x, vUpperArmLeftOut.y, vNeckOut.x, vNeckOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vUpperArmRightOut.x, vUpperArmRightOut.y, vNeckOut.x, vNeckOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vUpperArmLeftOut.x, vUpperArmLeftOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vUpperArmRightOut.x, vUpperArmRightOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftHandOut.x, vLeftHandOut.y, vLeftHandOut1.x, vLeftHandOut1.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vRightHandOut.x, vRightHandOut.y, vRightHandOut1.x, vRightHandOut1.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftThighOut.x, vLeftThighOut.y, vHipOut.x, vHipOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vRightThighOut.x, vRightThighOut.y, vHipOut.x, vHipOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftCalfOut.x, vLeftCalfOut.y, vLeftThighOut.x, vLeftThighOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vRightCalfOut.x, vRightCalfOut.y, vRightThighOut.x, vRightThighOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);

	DrawLine(vLeftFootOut.x, vLeftFootOut.y, vLeftCalfOut.x, vLeftCalfOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
	DrawLine(vRightFootOut.x, vRightFootOut.y, vRightCalfOut.x, vRightCalfOut.y, 2.f, 225.f, 0.f, 0.f, 200.f);
}

void drawLoop(int width, int height) {
	menu();

	float radiusx = AimFOV * (width / 2 / 100);
	float radiusy = AimFOV * (height / 2 / 100);

	float calcradius = (radiusx + radiusy) / 2;

		DrawCircle(width / 2, height / 2, calcradius, 2.f, 255.f, 255.f, 255.f, 255.f, false);
		

	Uworld = read<DWORD_PTR>(DriverHandle, processID, base_address + OFFSET_UWORLD);
	//printf(_xor_("Uworld: %p.\n").c_str(), Uworld);

	DWORD_PTR Gameinstance = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x180); 

	if (Gameinstance == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Gameinstance: %p.\n").c_str(), Gameinstance);

	DWORD_PTR LocalPlayers = read<DWORD_PTR>(DriverHandle, processID, Gameinstance + 0x38);

	if (LocalPlayers == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayers: %p.\n").c_str(), LocalPlayers);

	Localplayer = read<DWORD_PTR>(DriverHandle, processID, LocalPlayers);

	if (Localplayer == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("LocalPlayer: %p.\n").c_str(), Localplayer);

	PlayerController = read<DWORD_PTR>(DriverHandle, processID, Localplayer + 0x30);

	if (PlayerController == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("playercontroller: %p.\n").c_str(), PlayerController);

	LocalPawn = read<uint64_t>(DriverHandle, processID, PlayerController + 0x2A0); 

	if (LocalPawn == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Pawn: %p.\n").c_str(), LocalPawn);

	Rootcomp = read<uint64_t>(DriverHandle, processID, LocalPawn + 0x130);  

	if (Rootcomp == (DWORD_PTR)nullptr)
		return;

	//printf(_xor_("Rootcomp: %p.\n").c_str(), Rootcomp);

	if (LocalPawn != 0) {
		localplayerID = read<int>(DriverHandle, processID, LocalPawn + 0x18);
	}

	Ulevel = read<DWORD_PTR>(DriverHandle, processID, Uworld + 0x30);
	//printf(_xor_("Ulevel: %p.\n").c_str(), Ulevel);

	if (Ulevel == (DWORD_PTR)nullptr)
		return;

	DWORD64 PlayerState = read<DWORD64>(DriverHandle, processID, LocalPawn + 0x240);


	if (PlayerState == (DWORD_PTR)nullptr)
		return;

	DWORD ActorCount = read<DWORD>(DriverHandle, processID, Ulevel + 0xA0);

	DWORD_PTR AActors = read<DWORD_PTR>(DriverHandle, processID, Ulevel + 0x98);
	//printf(_xor_("AActors: %p.\n").c_str(), AActors);

	if (AActors == (DWORD_PTR)nullptr)
		return;

	for (int i = 0; i < ActorCount; i++)
	{
		uint64_t CurrentActor = read<uint64_t>(DriverHandle, processID, AActors + i * 0x8);

		int curactorid = read<int>(DriverHandle, processID, CurrentActor + 0x18);

		if (curactorid == localplayerID || curactorid == 20328438 || curactorid == 20328753 || curactorid == 9343426)
		
		{
			if (CurrentActor == (uint64_t)nullptr || CurrentActor == -1 || CurrentActor == NULL)
				continue;

			uint64_t CurrentActorRootComponent = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x130);

			if (CurrentActorRootComponent == (uint64_t)nullptr || CurrentActorRootComponent == -1 || CurrentActorRootComponent == NULL)
				continue;

			uint64_t currentactormesh = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x280);

			if (currentactormesh == (uint64_t)nullptr || currentactormesh == -1 || currentactormesh == NULL)
				continue;

			int MyTeamId = read<int>(DriverHandle, processID, PlayerState + 0xED0);

			DWORD64 otherPlayerState = read<uint64_t>(DriverHandle, processID, CurrentActor + 0x240);

			if (otherPlayerState == (uint64_t)nullptr || otherPlayerState == -1 || otherPlayerState == NULL)
				continue;

			int ActorTeamId = read<int>(DriverHandle, processID, otherPlayerState + 0xED0);

			Vector3 Headpos = GetBoneWithRotation(currentactormesh, 66);
			Localcam = CameraEXT;
			localactorpos = read<Vector3>(DriverHandle, processID, Rootcomp + 0x11C);

			float distance = localactorpos.Distance(Headpos) / 100.f;

			if (distance < 1.5f)
				continue;

			
			Vector3 HeadposW2s = ProjectWorldToScreen(Headpos, Vector3(Localcam.y, Localcam.x, Localcam.z));
			Vector3 bone0 = GetBoneWithRotation(currentactormesh, 0);
			Vector3 bottom = ProjectWorldToScreen(bone0, Vector3(Localcam.y, Localcam.x, Localcam.z));
			Vector3 Headbox = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 15), Vector3(Localcam.y, Localcam.x, Localcam.z));
			Vector3 Aimpos = ProjectWorldToScreen(Vector3(Headpos.x, Headpos.y, Headpos.z + 10), Vector3(Localcam.y, Localcam.x, Localcam.z));

			float Height1 = abs(Headbox.y - bottom.y);
			float Width1 = Height1 * 0.65;

			if (MyTeamId != ActorTeamId) 
			{
				if (skeleton)
					DrawSkeleton(currentactormesh);

				if (BoxESP)
					DrawBox(Headbox.x - (Width1 / 2), Headbox.y, Width1, Height1, 2.f, 0.f, 0.f, 0.f, 1.f, false);

				if (EnemyESP)
					DrawString(_xor_("Player").c_str(), 13, HeadposW2s.x - 5, HeadposW2s.y - 25, 0, 1, 1);

				if (DistanceESP)
				{
					CHAR dist[50];
					sprintf_s(dist, _xor_("%.fm").c_str(), distance);

					DrawString(dist, 13, HeadposW2s.x + 40, HeadposW2s.y - 25, 0, 0, 0);
				}  

				if (LineESP)
					DrawLine(width / 2, height, bottom.x, bottom.y, 2.f, 0.f, 0.f, 0.f, 200.f);

				if (Aimbot)
				{
					AIms(CurrentActor, Localcam);
				}
			}
		}
	}

	if (Aimbot)
	{
		aimbot(Localcam);
	}
}

void main()
{
	DriverHandle = CreateFileW(_xor_(L"\\\\.\\d31usi0n445").c_str(), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

	if (DriverHandle == INVALID_HANDLE_VALUE)
	{
		printf(_xor_("Drivers Haven't Been Loaded. Closing...\n").c_str());
		Sleep(2000);
		exit(0);
	}

	info_t Input_Output_Data1;
	unsigned long int Readed_Bytes_Amount1;
	DeviceIoControl(DriverHandle, ctl_clear, &Input_Output_Data1, sizeof Input_Output_Data1, &Input_Output_Data1, sizeof Input_Output_Data1, &Readed_Bytes_Amount1, nullptr);

	while (hwnd == NULL)
	{
		hwnd = FindWindowA(0, _xor_("Fortnite  ").c_str());

		printf(_xor_(" Looking For FortniteClient-Win64-Shipping.exe\n").c_str());
		Sleep(300);
		system("cls");
	}
	GetWindowThreadProcessId(hwnd, &processID);

	RECT rect;
	if(GetWindowRect(hwnd, &rect))
	{
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
	}

	info_t Input_Output_Data;
	Input_Output_Data.pid = processID;
	unsigned long int Readed_Bytes_Amount;

	DeviceIoControl(DriverHandle, ctl_base, &Input_Output_Data, sizeof Input_Output_Data, &Input_Output_Data, sizeof Input_Output_Data, &Readed_Bytes_Amount, nullptr);
	base_address = (unsigned long long int)Input_Output_Data.data;
	std::printf(_xor_("Process base address: %p.\n").c_str(), (void*)base_address);

	CreateThread(NULL, NULL, Menuthread, NULL, NULL, NULL);
	
	DirectOverlaySetOption(D2DOV_DRAW_FPS | D2DOV_FONT_COURIER); 
	DirectOverlaySetup(drawLoop, hwnd);
	getchar();
}
