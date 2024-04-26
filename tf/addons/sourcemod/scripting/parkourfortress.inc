#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "parkourfortress.sp"
#endif

#if defined _PARKOURFORTRESS_INCLUDED
	#endinput
#endif
#define _PARKOURFORTRESS_INCLUDED

#define ROPE_COMMAND1 "r_drawropes"
#define ROPE_COMMAND2 "rope_rendersolid"

#define ROPE_TOTAL 2

#define DEFAULT_MAXSPEED 520.0

char g_strRopeCommands[ROPE_TOTAL][32] = {
	ROPE_COMMAND1, ROPE_COMMAND2
};

Address g_pPatchLocation;

bool g_bLate;
bool g_bMapLoaded;

bool g_bTutorialFetched[TF_MAXPLAYERS];

Handle g_hHookGiveNamedItem;
Handle g_hSDKEquipWearable;
Handle g_hSDKRemoveWearable;
Handle g_hSDKGetEquippedWearable;
Handle g_hSDKGetBaseEntity;
Handle g_hSDKAirAccelerate;
Handle g_hSDKAccelerate;

Handle g_hSDKGetMaxAmmo;
Handle g_hHookSetWinningTeam;

Handle g_hUnlockPlayerTimer[TF_MAXPLAYERS + 1] = {INVALID_HANDLE, ...};

int g_iHookIdGiveNamedItem[TF_MAXPLAYERS];
int g_iTEBeams;
int g_iRestoreData;

float g_flAirAccel[TF_MAXPLAYERS] = {10.0, ...};
float g_flStockAirAccel;

float g_flAccel[TF_MAXPLAYERS] = {10.0, ...};
float g_flStockAccel;

const float g_flMaxSpeedVal = 5200.0;

#if defined DEBUGROPES
ConVar g_cvarDebugRopes;
#endif

ConVar g_cvarDebugBeams;
ConVar g_cvarDebugSpeed;
ConVar g_cvarDebugState;
ConVar g_cvarDebugExtra;
ConVar g_cvarMusicEnabled;
ConVar g_cvarViewmodels;

ConVar g_cvarWallrunTraces;

ConVar g_cvarPvP;
ConVar g_cvarWeaponRespawn;
ConVar g_cvarWeaponRespawnMin;
ConVar g_cvarWeaponRespawnRandom;
ConVar g_cvarWeaponMaxRare;
ConVar g_cvarWeaponRareChance;
ConVar g_cvarWeaponGrabDistance;

ConVar g_cvarAirAcceleration;
ConVar g_cvarAcceleration;

Cookie g_cookieMusic;
Cookie g_cookieMusicVolume;
Cookie g_cookieSound;
Cookie g_cookieSelfAmbientSound;
Cookie g_cookieViewmodel;
Cookie g_cookieLerp;
Cookie g_cookieTutorialStage;

int g_iFoundPropBecauseICantReturnItFromTheEnumerator;

GlobalForward g_hForwardWeaponPickup;

ArrayList g_hRailControl;
ArrayList g_hRopeControl;
ArrayList g_hMoveRopeToIndex;

enum CPFRail {};
enum CPFRope {};

const CPFRope CPFROPE_INVALID = view_as<CPFRope>(INVALID_HANDLE);
const CPFRail CPFRAIL_INVALID = view_as<CPFRail>(INVALID_HANDLE);
const ArrayList ARRAYLIST_INVALID = view_as<ArrayList>(INVALID_HANDLE);

stock const float ZERO_VECTOR[3] = {0.0, 0.0, 0.0};

stock const float TICKRATE_STANDARD_FLOAT = 66.0;
stock const int TICKRATE_STANDARD = 66;

stock const float VECMINS_CLIENTLOOK[3] = {-32.000000, -32.000000, -32.000000};
stock const float VECMAXS_CLIENTLOOK[3] = {32.000000, 32.000000, 32.000000};

stock const float HEAL_ADD_PVP = 1.5;
stock const float HEAL_ADD_DEFAULT = 2.0;
stock const int HEAL_MAX_PVP = 100;
stock const int HEAL_MAX_DEFAULT = 125;

const MoveType MOVETYPE_DEFAULT = MOVETYPE_WALK;

enum ePFRotDirection
{
	Rot_Invalid = -1,
	Rot_CCW = 0,
	Rot_CW = 1
};

enum Collision_Group_t
{
	COLLISION_GROUP_NONE  = 0,
	COLLISION_GROUP_DEBRIS,				// Collides with nothing but world and static stuff
	COLLISION_GROUP_DEBRIS_TRIGGER,		// Same as debris, but hits triggers
	COLLISION_GROUP_INTERACTIVE_DEB,	// Collides with everything except other interactive debris or debris
	COLLISION_GROUP_INTERACTIVE,		// Collides with everything except interactive debris or debris
	COLLISION_GROUP_PLAYER,
	COLLISION_GROUP_BREAKABLE_GLASS,
	COLLISION_GROUP_VEHICLE,
	COLLISION_GROUP_PLAYER_MOVEMENT,	// For HL2, same as Collision_Group_Player, for
										// TF2, this filters out other players and CBaseObjects
	COLLISION_GROUP_NPC,				// Generic NPC group
	COLLISION_GROUP_IN_VEHICLE,			// for any entity inside a vehicle
	COLLISION_GROUP_WEAPON,				// for any weapons that need collision detection
	COLLISION_GROUP_VEHICLE_CLIP,		// vehicle clip brush to restrict vehicle movement
	COLLISION_GROUP_PROJECTILE,			// Projectiles!
	COLLISION_GROUP_DOOR_BLOCKER,		// Blocks entities not permitted to get near moving doors
	COLLISION_GROUP_PASSABLE_DOOR,		// Doors that the player shouldn't collide with
	COLLISION_GROUP_DISSOLVING,			// Things that are dissolving are in this group
	COLLISION_GROUP_PUSHAWAY,			// Nonsolid on client and server, pushaway in player code

	COLLISION_GROUP_NPC_ACTOR,			// Used so NPCs in scripts ignore the player.
	COLLISION_GROUP_NPC_SCRIPTED,		// USed for NPCs in scripts that should not collide with each other
	
	COLLISION_GROUP_T_COUNT
};

Collision_Group_t g_ePFCollisionGroup = COLLISION_GROUP_PLAYER_MOVEMENT;

static const char g_strCollisionGroupLUT[COLLISION_GROUP_T_COUNT][32] = {
	"NONE",
	"DEBRIS",
	"DEBRIS TRIGGER",
	"INTERACTIVE DEBRIS",
	"INTERACTIVE",
	"PLAYER",
	"BREAKABLE GLASS",
	"VEHICLE",
	"PLAYER MOVEMENT",
	"NPC",
	"IN VEHICLE",
	"WEAPON",
	"VEHICLE CLIP",
	"PROJECTILE",
	"DOOR BLOCKER",
	"PASSABLE DOOR",
	"DISSOLVING",
	"PUSHAWAY",
	"NPC ACTOR",
	"NPC SCRIPTED"
};

enum EffectFlags_t
{
    EF_BONEMERGE = (1 << 0),    		// Merges bones of names shared with a parent entity to the position and direction of the parent's.
    EF_BRIGHTLIGHT = (1 << 1),          // Emits a dynamic light of RGB(250,250,250) and a random radius of 400 to 431 from the origin.
    EF_DIMLIGHT = (1 << 2),             // Emits a dynamic light of RGB(100,100,100) and a random radius of 200 to 231 from the origin.
    EF_NOINTERP = (1 << 3),             // Don't interpolate on the next frame.
    EF_NOSHADOW = (1 << 4),             // Don't cast a shadow. To do: Does this also apply to shadow maps?
    EF_NODRAW = (1 << 5),               // Entity is completely ignored by the client. Can cause prediction errors if a player proceeds to collide with it on the server.
    EF_NORECEIVESHADOW = (1 << 6),      // Don't receive dynamic shadows.
    EF_BONEMERGE_FASTCULL = (1 << 7),   // For use with EF_BONEMERGE. If set, the entity will use its parent's origin to calculate whether it is visible; if not set, it will set up parent's bones every frame even if the parent is not in the PVS.
    EF_ITEM_BLINK = (1 << 8),           // Blink an item so that the user notices it. Added for Xbox 1, and really not very subtle.
    EF_PARENT_ANIMATES = (1 << 9)       // Assume that the parent entity is always animating. Causes it to realign every frame.
};

enum SpecMode_t
{
	SPECMODE_INVALID = -1,
	SPECMODE_FIRSTPERSON = 4,
	SPECMODE_THIRDPERSON,
	SPECMODE_FREELOOK,
};

enum TakeDamage_t
{
	DAMAGE_NO = 0,
	DAMAGE_EVENTS_ONLY = 1,
	DAMAGE_YES,
	DAMAGE_AIM,
};

enum struct CTFGameMovementOffsets
{
	int player;
}

CTFGameMovementOffsets offsets;

methodmap CGameMovement
{
	public CGameMovement(Address pGameMovement)
	{
		return view_as<CGameMovement>(pGameMovement);
	}
	
	property int player
	{
		public get() { return SDKCall(g_hSDKGetBaseEntity, LoadFromAddress(view_as<Address>(this) + view_as<Address>(offsets.player), NumberType_Int32)); }
	}
}

#if !defined _PF_INCLUDED
stock bool IsValidClient(int iClient)
{
	return !(iClient <= 0
			|| iClient > MaxClients
			|| !IsClientInGame(iClient)
			|| !IsClientConnected(iClient)
			|| GetEntProp(iClient, Prop_Send, "m_bIsCoaching")
			|| IsClientSourceTV(iClient)
			|| IsClientReplay(iClient)
			|| IsFakeClient(iClient));
			
}

stock bool IsDevServer()
{
	char strServer[8];
	ConVar server = FindConVar("pf_servername");
	if (server)
	{
		server.GetString(strServer, sizeof(strServer));
		return StrEqual(strServer, "DEV");
	}
	
	return false;
}
#endif

void DebugOutput(const char[] strMessage, any ...)
{
	if (g_cvarDebugExtra.IntValue)
	{
		char strBuf[512];
		VFormat(strBuf, sizeof(strBuf), strMessage, 2);
		PrintToChatAll(strBuf);
	}
}

stock int CreateSprite(float vecOrigin[3], int iColor[3] = {255, 0, 0})
{ 
	int iEntity = CreateEntityByName( "env_sprite" );
	
	SetEntityModel(iEntity, "materials/sprites/glow.vmt");
	SetEntityRenderColor(iEntity, iColor[0], iColor[1], iColor[2] );

	SetEdictFlags(iEntity, FL_EDICT_ALWAYS );

	static const char GLOW_SIZE[5] = "20.0";

	SetEntityRenderMode(iEntity, RENDER_WORLDGLOW);  
	DispatchKeyValue(iEntity, "GlowProxySize", GLOW_SIZE);
	DispatchKeyValue(iEntity, "renderamt", "255"); 
	DispatchKeyValue(iEntity, "framerate", "10.0"); 
	DispatchKeyValue(iEntity, "scale", GLOW_SIZE); 
	DispatchKeyValue(iEntity, "targetname", "debugsprite");

	SetEntProp(iEntity, Prop_Data, "m_bWorldSpaceScale", 1); 
	DispatchSpawn(iEntity); 

	#if defined DEBUGROPES
	if (g_cvarDebugRopes.IntValue) AcceptEntityInput(iEntity, "ShowSprite");
	#endif

	TeleportEntity(iEntity, vecOrigin, NULL_VECTOR, NULL_VECTOR);
	
	return iEntity;
}

stock void DrawBoundingBox(	const float vecMins[3], 
							const float vecMaxs[3], 
							const float vecOrigin[3], 
							float flDuration = 10.0, 
							int rgbColor[4] = {255, 0, 0, 255}, 
							ConVar cvarDebug = view_as<ConVar>(INVALID_HANDLE)	)
{
	float vecRelMinsSub1[3], vecRelMinsSub2[3], vecRelMinsSub3[3], vecRelMinsSub4[3];
	float vecRelMaxsSub1[3], vecRelMaxsSub2[3], vecRelMaxsSub3[3], vecRelMaxsSub4[3];
	
	AddVectors(vecOrigin, vecMins, vecRelMinsSub1);
	
	vecRelMinsSub2[0] = vecMins[0];
	vecRelMinsSub2[1] = -vecMins[1];
	vecRelMinsSub2[2] = vecMins[2];
	AddVectors(vecOrigin, vecRelMinsSub2, vecRelMinsSub2);
	
	vecRelMinsSub3[0] = -vecMins[0];
	vecRelMinsSub3[1] = -vecMins[1];
	vecRelMinsSub3[2] = vecMins[2];
	AddVectors(vecOrigin, vecRelMinsSub3, vecRelMinsSub3);
	
	vecRelMinsSub4[0] = -vecMins[0];
	vecRelMinsSub4[1] = vecMins[1];
	vecRelMinsSub4[2] = vecMins[2];
	AddVectors(vecOrigin, vecRelMinsSub4, vecRelMinsSub4);
	
	AddVectors(vecOrigin, vecMaxs, vecRelMaxsSub1);
	
	vecRelMaxsSub2[0] = vecMaxs[0];
	vecRelMaxsSub2[1] = -vecMaxs[1];
	vecRelMaxsSub2[2] = vecMaxs[2];
	AddVectors(vecOrigin, vecRelMaxsSub2, vecRelMaxsSub2);
	
	vecRelMaxsSub3[0] = -vecMaxs[0];
	vecRelMaxsSub3[1] = -vecMaxs[1];
	vecRelMaxsSub3[2] = vecMaxs[2];
	AddVectors(vecOrigin, vecRelMaxsSub3, vecRelMaxsSub3);
	
	vecRelMaxsSub4[0] = -vecMaxs[0];
	vecRelMaxsSub4[1] = vecMaxs[1];
	vecRelMaxsSub4[2] = vecMaxs[2];
	AddVectors(vecOrigin, vecRelMaxsSub4, vecRelMaxsSub4);
	
	DrawVectorPoints(vecRelMinsSub1, vecRelMaxsSub3, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub1, vecRelMaxsSub4, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub1, vecRelMaxsSub2, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub1, vecRelMinsSub2, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub1, vecRelMinsSub3, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub1, vecRelMinsSub4, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub2, vecRelMaxsSub1, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub2, vecRelMaxsSub4, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub2, vecRelMinsSub3, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub3, vecRelMaxsSub1, flDuration, rgbColor);
	DrawVectorPoints(vecRelMinsSub3, vecRelMaxsSub2, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub3, vecRelMinsSub4, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMinsSub4, vecRelMaxsSub2, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMaxsSub1, vecRelMaxsSub2, flDuration, rgbColor);
	DrawVectorPoints(vecRelMaxsSub1, vecRelMaxsSub3, flDuration, rgbColor);
	DrawVectorPoints(vecRelMaxsSub1, vecRelMaxsSub4, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMaxsSub2, vecRelMaxsSub3, flDuration, rgbColor);
	
	DrawVectorPoints(vecRelMaxsSub3, vecRelMaxsSub4, flDuration, rgbColor);
}

void GetForwardVector(int iClient, float vecBuffer[3], float flMagnitude = 1.0) 
{
	float vecPos[3], vecEyes[3], vecForward[3];
	GetClientAbsOrigin(iClient, vecPos);
	GetClientEyeAngles(iClient, vecEyes);
	GetAngleVectors(vecEyes, vecForward, NULL_VECTOR, NULL_VECTOR);
	vecForward[2] = 0.0;
	NormalizeVector(vecForward, vecBuffer);
	ScaleVector(vecBuffer, flMagnitude);
}

void ForwardVector(const float vecAngles[3], const float flMagnitude, float vecResult[3])
{
	float vecForward[3];
	GetAngleVectors(vecAngles, vecForward, NULL_VECTOR, NULL_VECTOR);
	vecForward[2] = 0.0;
	NormalizeVector(vecForward, vecForward);
	ScaleVector(vecForward, flMagnitude);
	vecResult = vecForward;
}

ePFRotDirection FindRotDirection(const float vecCenter[3], const float vecStart[3], const float vecEnd[3])
{
	float vecStartRelative[3], vecEndRelative[3];
	SubtractVectors(vecStart, vecCenter, vecStartRelative);
	SubtractVectors(vecEnd, vecCenter, vecEndRelative);
	
	
	if ((vecStartRelative[1] > 0.0 && vecEndRelative[0] > 0.0)	||
		(vecStartRelative[0] < 0.0 && vecEndRelative[1] > 0.0)	||
		(vecStartRelative[1] < 0.0 && vecEndRelative[0] < 0.0)	||
		(vecStartRelative[0] > 0.0 && vecEndRelative[1] < 0.0))
		return Rot_CW;
	
	if ((vecStartRelative[0] > 0.0 && vecEndRelative[1] > 0.0)	||
		(vecStartRelative[1] > 0.0 && vecEndRelative[0] < 0.0)	||
		(vecStartRelative[0] < 0.0 && vecEndRelative[1] < 0.0)	||
		(vecStartRelative[1] < 0.0 && vecEndRelative[0] > 0.0))
		return Rot_CCW;
	
	return Rot_Invalid;
}

/* TODO: Do we still need this? */
stock void GetPointOnVectorUnitsAway(float vecOrigin[3], float vecPos[3], float flUnits, float vecResult[3])
{ 
	float dir[3];
	dir[0] = vecOrigin[0] - vecPos[0];
	dir[1] = vecOrigin[1] - vecPos[1];
	dir[2] = 0.0;
	NormalizeVector(dir, dir);
	vecResult[0] = vecPos[0] + dir[0] * flUnits;
	vecResult[1] = vecPos[1] + dir[1] * flUnits;
	vecResult[2] = vecPos[2];
}

stock bool IsOnGround(int iClient) { return view_as<bool>(GetEntityFlags(iClient) & FL_ONGROUND); }
stock bool IsFullyDucked(int iClient) { return view_as<bool>(GetEntityFlags(iClient) & FL_DUCKING); }

stock bool IsOnGroundTrace(int iClient)
{
	float vecStart[3];
	float vecEnd[3];
	GetClientAbsOrigin(iClient, vecStart);
	vecEnd = vecStart;
	vecEnd[2] -= 4.0;
	TraceRayF hTrace = new TraceRayF(	vecStart, 
										vecEnd, 
										MASK_PLAYERSOLID, 
										RayType_EndPoint, 
										TraceRayNoPlayers, 
										iClient			);
	bool bReturn = hTrace.Hit;
	delete hTrace;
	return bReturn;
}

stock int abs(int x) { return (x<0) ? -x : x; }

public bool TraceRayNoPlayers(int entity, int mask, any data) 
{
	if (0 < data <= MaxClients)
	{
		char sClassname[32];
		GetEntityClassname(entity, sClassname, sizeof(sClassname));
		
		if (StrEqual(sClassname, "func_forcefield") && GetEntProp(entity, Prop_Data, "m_iTeamNum") == GetClientTeam(data))
			return false;
	}
	
	return !(entity == data || (entity >= 1 && entity <= MaxClients));
}

public bool TraceRayOnlyPlayers(int entity, int mask, any data) 
{
	return (entity == data || (entity >= 1 && entity <= MaxClients));
}

public bool TraceRayOnlyThisPlayer(int entity, int mask, any data) 
{
	DebugOutput("TraceRayOnlyThisPlayer --- Comparing %i with %i", entity, data);
	return (entity == data);
}

public bool TraceEntityFilterSolid(int entity, int mask) 
{
	return entity > MaxClients;
}

stock void ResetCollisions(int iClient)
{
	SetEntPropFloat(iClient, Prop_Send, "m_flModelScale", 1.0);
	SetEntPropVector(iClient, Prop_Send, "m_vecMins", view_as<float>({-24.000000, -24.000000, 0.000000}));
	SetEntPropVector(iClient, Prop_Send, "m_vecMaxs", view_as<float>({24.000000, 24.000000, 82.000000}));
}

stock bool CheckIfPlayerIsStuck(int iClient)
{
	float vecMins[3], vecMaxs[3], vecOrigin[3];
	
	GetClientMins(iClient, vecMins);
	GetClientMaxs(iClient, vecMaxs);
	GetClientAbsOrigin(iClient, vecOrigin);
	
	TraceHullF.Start(vecOrigin, vecOrigin, vecMins, vecMaxs, MASK_SOLID, TraceEntityFilterSolid);
	return TR_DidHit();
}

stock void SetCollisionGroup(int iEntity, Collision_Group_t eGroup)
{
	if (!IsValidEntity(iEntity))
		return;
	
	if (IsValidClient(iEntity))
		DebugOutput("SetCollisionGroup: %N %s", iEntity, g_strCollisionGroupLUT[view_as<int>(eGroup)]);
	else
		DebugOutput("SetCollisionGroup: %d %s", iEntity, g_strCollisionGroupLUT[view_as<int>(eGroup)]);
	
	SetEntProp(iEntity, Prop_Data, "m_CollisionGroup", view_as<int>(eGroup));
}

stock void AddEFlags(int iEntity, EffectFlags_t iEffects) 
{
	if (IsValidEntity(iEntity))
		SetEntProp(iEntity, Prop_Send, "m_fEffects", GetEntProp(iEntity, Prop_Send, "m_fEffects") | view_as<int>(iEffects));
}

stock void SetEFlags(int iEntity, EffectFlags_t iEffects)
{
	if (IsValidEntity(iEntity))
		SetEntProp(iEntity, Prop_Send, "m_fEffects", iEffects);
}

stock void RemoveEFlags(int iEntity, EffectFlags_t iEffects)
{
	if (IsValidEntity(iEntity))
		SetEntProp(iEntity, Prop_Send, "m_fEffects", GetEntProp(iEntity, Prop_Send, "m_fEffects") & ~view_as<int>(iEffects));
}

stock EffectFlags_t GetEFlags(int iEntity)
{ 
	return view_as<EffectFlags_t>(GetEntProp(iEntity, Prop_Send, "m_fEffects"));
}

stock bool IsEFlagSet(int iEntity, EffectFlags_t iEffect)
{
	return !!(view_as<EffectFlags_t>(GetEntProp(iEntity, Prop_Send, "m_fEffects")) & iEffect);
}

stock bool AreEFlagsSet(int iEntity, EffectFlags_t iEffects)
{
	return !!(view_as<EffectFlags_t>(GetEntProp(iEntity, Prop_Send, "m_fEffects")) & iEffects == iEffects);
}

stock void SuperPrecacheMaterial(const char[] strMaterial, bool bPremap = true)
{
	char strPath[PLATFORM_MAX_PATH];

	Format(strPath, sizeof(strPath), "materials/%s.vmt", strMaterial);
	AddFileToDownloadsTable(strPath);

	Format(strPath, sizeof(strPath), "materials/%s.vtf", strMaterial);
	AddFileToDownloadsTable(strPath);

	Format(strPath, sizeof(strPath), "%s.vmt", strMaterial);
	PrecacheModel(strMaterial, bPremap);
}

bool CheckPointAgainstPlayerHull(const int iClient, const float vecPos[3])
{
	float vecMaxs[3], vecMins[3]; 
	
	GetClientMins(iClient, vecMins);
	GetClientMaxs(iClient, vecMaxs);
	
	//DrawBoundingBox(vecMins, vecMaxs, vecPos);
	TraceHullF.Start(vecPos, vecPos, vecMins, vecMaxs, MASK_PLAYERSOLID, TraceEntityFilterSolid);
	return TRACE_GLOBAL.Hit;
}

bool CheckIfForceDucked(const int iClient)
{
	float vecMins[3] = {-24.000000, -24.000000, 0.000000};
	float vecMaxs[3] = {24.000000, 24.000000, 82.000000};
	float vecPos[3];
	
	GetClientAbsOrigin(iClient, vecPos);
	
	//DrawBoundingBox(vecMins, vecMaxs, vecPos);
	TraceHullF.Start(vecPos, vecPos, vecMins, vecMaxs, MASK_PLAYERSOLID, TraceEntityFilterSolid);
	return TRACE_GLOBAL.Hit;
}

int SuperPrecacheModel(const char[] strModel)
{
		char strBase[PLATFORM_MAX_PATH];
		char strPath[PLATFORM_MAX_PATH];
		Format(strBase, sizeof(strBase), strModel);
		SplitString(strBase, ".mdl", strBase, sizeof(strBase));

		Format(strPath, sizeof(strPath), "%s.phy", strBase);
		if (FileExists(strPath)) AddFileToDownloadsTable(strPath);

		Format(strPath, sizeof(strPath), "%s.sw.vtx", strBase);
		if (FileExists(strPath)) AddFileToDownloadsTable(strPath);

		Format(strPath, sizeof(strPath), "%s.vvd", strBase);
		if (FileExists(strPath)) AddFileToDownloadsTable(strPath);

		Format(strPath, sizeof(strPath), "%s.dx80.vtx", strBase);
		if (FileExists(strPath)) AddFileToDownloadsTable(strPath);

		Format(strPath, sizeof(strPath), "%s.dx90.vtx", strBase);
		if (FileExists(strPath)) AddFileToDownloadsTable(strPath);

		AddFileToDownloadsTable(strModel);
		return PrecacheModel(strModel);
}

stock float CalculateSlopeTheta(int iClient)
{
	/* Let's set up some values */
	float delta[3], x[3], y[3], theta, hyp, opp;
	GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", delta);
	GetClientAbsOrigin(iClient, x);
	AddVectors(delta, x, y);
	hyp = GetVectorDistance(x, y);
	opp = FloatAbs(delta[2]);
	theta = RadToDeg(ArcSine(opp / hyp));
	DebugOutput("CalculateSlopeTheta --- Hypotenuse: %.3f, Theta: %.3f", hyp, theta);
	
	return theta;
}

/* WARNING: Only use this function with stacks containing float values! */
stock float ArrayStackAverage(ArrayStack hStack)
{
	float flAverage;
	int iCount;
	
	while (!hStack.Empty)
	{
		flAverage += view_as<float>(hStack.Pop());
		iCount++;
	}
	
	return (flAverage / float(iCount));
}

stock float ClientForwardSpeed(int iClient)
{
	if (!IsValidClient(iClient))
		return 0.0;
	
	float vecVelocity[3], vecAngles[3], vecForwardAngles[3];
	GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
	GetClientEyeAngles(iClient, vecAngles);
	
	GetAngleVectors(vecAngles, vecForwardAngles, NULL_VECTOR, NULL_VECTOR);
	NormalizeVector(vecForwardAngles, vecForwardAngles);
	return GetVectorDotProduct(vecVelocity, vecForwardAngles);
}

stock float ClientHorizontalSpeed(int iClient)
{
	if (!IsValidClient(iClient))
		return 0.0;
	
	float vecVelocity[3];
	GetEntPropVector(iClient, Prop_Data, "m_vecVelocity", vecVelocity);
	return SquareRoot(vecVelocity[0]*vecVelocity[0] + vecVelocity[1]*vecVelocity[1]);
}

///////////////////////////////////////////////////////////////////////////////////////////

enum eWeaponsRarity
{
	eWeaponsRarity_Common,
	eWeaponsRarity_Uncommon,
	eWeaponsRarity_Rare,
	eWeaponsRarity_Pickup,
	WEAPONRARITY_COUNT,
};

enum
{
	WeaponSlot_Primary = 0,
	WeaponSlot_Secondary,
	WeaponSlot_Melee,
	WeaponSlot_PDABuild,
	WeaponSlot_PDADisguise = 3,
	WeaponSlot_PDADestroy,
	WeaponSlot_InvisWatch = 4,
	WeaponSlot_BuilderEngie,
	WEAPONSLOT_COUNT,
};

stock int FindEntityByClassname2(int startEnt, const char[] classname)
{
	/* If startEnt isn't valid shifting it back to the nearest valid one */
	while (startEnt > -1 && !IsValidEntity(startEnt)) startEnt--;
	return FindEntityByClassname(startEnt, classname);
}

stock int GetClientEntityVisible(int iClient, const char[] sClassname, float flDistance = 100.0)
{
	g_iFoundPropBecauseICantReturnItFromTheEnumerator = -1;
	DebugOutput("GetClientEntityVisible -- searching for %s %.1f units from %N", sClassname, flDistance, iClient);
	float vecOrigin[3], vecAngles[3], vecForward[3];
	GetClientEyePosition(iClient, vecOrigin);
	GetClientEyeAngles(iClient, vecAngles);
	GetAngleVectors(vecAngles, vecForward, NULL_VECTOR, NULL_VECTOR);
	NormalizeVector(vecForward, vecForward);
	ScaleVector(vecForward, flDistance);
	AddVectors(vecForward, vecOrigin, vecForward);
	DrawVectorPoints(vecOrigin, vecForward, 10.0, {255, 0, 0, 255});
	TraceRayF hSolidCheck = new TraceRayF(vecOrigin, vecForward, MASK_PLAYERSOLID, RayType_EndPoint, TraceFilter_NoTeammates, iClient);
	if (hSolidCheck.Hit)
		hSolidCheck.GetEndPosition(vecForward);
	delete hSolidCheck;
	DrawVectorPoints(vecOrigin, vecForward, 10.0, {255, 255, 0, 255});
	
	DataPack hData = new DataPack();
	hData.WriteCell(iClient);
	hData.WriteString(sClassname);
	
	TR_EnumerateEntities(vecOrigin, vecForward, PARTITION_NON_STATIC_EDICTS, RayType_EndPoint, TraceEnum_TestEntity, hData);
	if (IsValidEntity(g_iFoundPropBecauseICantReturnItFromTheEnumerator))
		DebugOutput("TraceEnum_TestEntity -- found %i", g_iFoundPropBecauseICantReturnItFromTheEnumerator);
	else
		DebugOutput("TraceEnum_TestEntity -- no valid ent found ");
	
	delete hData;
	return g_iFoundPropBecauseICantReturnItFromTheEnumerator;
}

public bool Trace_DontHitEntity(int iEntity, int iMask, any iData)
{
	if (iEntity == iData) return false;
	return true;
}

public bool TraceFilter_NoTeammates(int entity, int mask, int iClient) 
{
	if (entity == iClient) return false;
	if (IsValidClient(entity))
	{
		return (TF2_GetClientTeam(entity) != TF2_GetClientTeam(iClient));
	}
	return true;
}

public bool TraceEnum_TestEntity(int iHitEntity, DataPack hData)
{
	int iClient;
	char sClassname[64], sName[64];
	
	hData.Reset();
	iClient = hData.ReadCell();
	hData.ReadString(sClassname, sizeof(sClassname));
	GetEntPropString(iHitEntity, Prop_Data, "m_iName", sName, sizeof(sName));
	
	if (IsValidEntity(iHitEntity) && iHitEntity != iClient
	&& IsClassname(iHitEntity, sClassname) && (StrContains(sName, "pf_weapon", false) == 0))
	{
		DebugOutput("TraceEnum_TestEntity -- enumerating index %i", iHitEntity);
		
		/*Handle hClipEntity = TR_ClipCurrentRayToEntityEx(MASK_, iHitEntity);
		bool bTraceHit = TR_DidHit(hClipEntity);
		delete hClipEntity;
		
		if (bTraceHit)
		{*/
		g_iFoundPropBecauseICantReturnItFromTheEnumerator = iHitEntity;
		return false;
		//}
	}
	
	return true;
}

stock bool IsClassname(int iEntity, char[] sClassname)
{
	if (iEntity <= 0) return false;
	if (!IsValidEdict(iEntity)) return false;
	
	char sClassname2[32];
	GetEdictClassname(iEntity, sClassname2, sizeof(sClassname2));
	if (StrEqual(sClassname, sClassname2, false)) return true;
	
	return false;
}

void SDKHookClassname(const char[] strClassname, SDKHookType eType, SDKHookCB fCallback)
{
	for (int i = MaxClients; i < 2048; i++)
	{
		if (!IsValidEntity(i))
			continue;
		
		char strClassnameBuf[256];
		GetEntityClassname(i, strClassnameBuf, sizeof(strClassnameBuf));
		if (StrEqual(strClassname, strClassnameBuf))
			SDKHook(i, eType, fCallback);
	}
}

float GetTickRate()
{
	return 1.0 / GetTickInterval();
}

int TickModify(int iTicks)
{
	int iTickRate = RoundToFloor(GetTickRate());
	if (iTickRate == TICKRATE_STANDARD)
		return iTicks;
	
	//PrintToChatAll("%d %.3f %.3f %.3f", iTickRate, float(iTickRate), (float(iTicks) / TICKRATE_STANDARD), float(iTickRate) * (float(iTicks) / TICKRATE_STANDARD));
	return RoundToFloor(float(iTickRate) * (float(iTicks) / TICKRATE_STANDARD));
}

stock void DebugBeamRing(const float vecOrigin[3], const float flRadius, const float flLifetime = 999.9, const int iColor[4] = {255, 255, 255, 255})
{
	TE_SetupBeamRingPoint(vecOrigin, flRadius, flRadius - 1, PrecacheModel("materials/sprites/laserbeam.vmt"), PrecacheModel("materials/sprites/glow.vmt"), 0, 15, flLifetime, 5.0, 0.0, iColor, 10, 0);
	TE_SendToAll();
}

stock int GetCookieInt(Cookie hCookie, int iClient)
{
	if (!IsValidClient(iClient) || hCookie == null)
		return 0;
	
	char strBuf[256];
	hCookie.Get(iClient, strBuf, sizeof(strBuf));
	return StringToInt(strBuf);
}

bool HasAnyAmmo(int iClient, int iWeapon)
{
	if (!IsValidEntity(iWeapon) || !IsValidClient(iClient)) return false;
	char sSlot[16];
	int iItemDefinitionIndex = GetEntProp(iWeapon, Prop_Send, "m_iItemDefinitionIndex");
	if (TF2Econ_GetItemDefinitionString(iItemDefinitionIndex, "item_slot", sSlot, sizeof(sSlot)))
	{
		if (!StrEqual(sSlot, "melee", false))
		{
			char sClass[32];
			GetEntityClassname(iWeapon, sClass, sizeof(sClass));
			if (StrContains(sClass, "tf_weapon_sniperrifle") == 0)
				return !!(TF2_GetAmmo(iClient, iWeapon, false)); // sniper rifles don't have a clip
			else
				return ((GetEntProp(iWeapon, Prop_Send, "m_iClip1") > 0) || TF2_GetAmmo(iClient, iWeapon, false));
		}
	}
	return true;
}

bool HandleHookshot (int iClient, int iProjectile)
{
	if (FloatAbs(ClientForwardSpeed(iClient)) < 16) return false;
	
	float vecStart[3], vecEnd[3];
	float flDist;
	GetClientEyePosition(iClient, vecStart);
	GetEntPropVector(iProjectile, Prop_Send, "m_vecOrigin", vecEnd);
	flDist = GetVectorDistance(vecStart, vecEnd);
	return (flDist > 72.0);
}

bool PlayerHasRifleActive(int iClient)
{
	char sClassname[64];
	TF2_GetActiveWeaponClassname(iClient, sClassname, sizeof(sClassname));
	return (StrContains(sClassname, "tf_weapon_sniperrifle") == 0);
	//bRifleZoomed = (TF2_IsPlayerInCondition(iClient, TFCond_Zoomed));
}

bool VectorIsZero(const float vec[3], float tolerance = 0.01)
{
	return (vec[0] > -tolerance && vec[0] < tolerance &&
			vec[1] > -tolerance && vec[1] < tolerance &&
			vec[2] > -tolerance && vec[2] < tolerance);
}

/* Credit for these two functions to NotPaddy (Patric O.) */
void DrawVectorPoints(float vecOrigin[3], float vecEndpoint[3], float flLifespan, int iColor[4], float flWidth = 3.0, bool bDebug = true)
{
	if (!g_iTEBeams)
	{
		LogError("[parkourfortress] TE Beams not precached!");
		return;
	}
	
	if (bDebug && !g_cvarDebugBeams.IntValue)
		return;
	
	TE_SetupBeamPoints(vecOrigin, vecEndpoint, PrecacheModel("materials/sprites/laser.vmt"), 0, 0, 0, flLifespan, flWidth, 3.0, 1, 0.0, iColor, 0);
	TE_SendToAll();
}

void DrawVector(float vecOrigin[3], float vecDirection[3], float flLifespan, int iColor[4], float flWidth = 3.0, bool bDebug = true)
{
	if (!g_iTEBeams)
	{
		LogError("[parkourfortress] TE Beams not precached!");
		return;
	}
	
	if (bDebug && !g_cvarDebugBeams.IntValue)
		return;
	
	float vecEndpoint[3];
	AddVectors(vecOrigin, vecDirection, vecEndpoint);
	TE_SetupBeamPoints(vecOrigin, vecEndpoint, PrecacheModel("materials/sprites/laser.vmt"), 0, 0, 0, flLifespan, flWidth, 3.0, 1, 0.0, iColor, 0);
	TE_SendToAll();
}
/****/