#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _OBJECTS_DOORS_INCLUDED
	#endinput
#endif
#define _OBJECTS_DOORS_INCLUDED

static ArrayList g_hDoorControl;
static StringMap g_hPropIndexToDoor;

enum CPFDoor {};
enum CPFDoorController {};

enum ePFDoorProperties
{
	PROP_IDX,
	DOOR_IDX,
	DOOR_STATE,
	
	DOORPROP_COUNT
};

enum ePFDoorState
{
	DOOR_CLOSED_IDLE = 0,
	DOOR_OPENING = 1,
	DOOR_OPEN,
	DOOR_CLOSING,
	
	DOORSTATE_COUNT
};

#define DOORMODEL_COUNT 6
static const char g_strDoorModels[DOORMODEL_COUNT][] = 
{
	"models/parkoursource/door_standard.mdl", 
	"models/reduxsource/door_standard.mdl", 
	"models/reduxsource/door_standard_static.mdl",
	"models/models/reduxsource/door_standard.mdl", 
	"models/models/reduxsource/door_standard_static.mdl",
	"models/props_parkour_tf/exit_door.mdl"
};

const float DOOR_OPEN_SPEED = 10000.0;
stock const char DOOR_CLOSE_DELAY[3] = "3.0";

methodmap CPFDoor < ArrayList
{
	public CPFDoor(int iDoor)
	{
		if (!IsValidEntity(iDoor))
			SetFailState("CPFDoor::CPFDoor --- Invalid entity passed to constructor! Entity: %d", iDoor);
		
		int iDoorIndex;
		if (g_hDoorControl == null)
			SetFailState("CPFDoor::CPFDoor --- Controller not initiated! Handle: %X", g_hDoorControl);
		else
			iDoorIndex = g_hDoorControl.Length;
		
		DispatchKeyValueFloat(iDoor, "speed", DOOR_OPEN_SPEED);
		DispatchKeyValue(iDoor, "returndelay", DOOR_CLOSE_DELAY);
		
		ArrayList hDoor = new ArrayList(1, DOORPROP_COUNT);
		hDoor.Set(PROP_IDX, EntIndexToEntRef(iDoor));
		hDoor.Set(DOOR_IDX, iDoorIndex);
		hDoor.Set(DOOR_STATE, DOOR_CLOSED_IDLE);
		
		char strKey[32];
		IntToString(EntIndexToEntRef(iDoor), strKey, sizeof(strKey));
		g_hPropIndexToDoor.SetValue(strKey, hDoor, true);
		return view_as<CPFDoor>(hDoor);
	}
	
	property int EntIndex
	{
		public get()
		{
			return view_as<int>((view_as<ArrayList>(this)).Get(PROP_IDX));
		}
		
		public set(int idx)
		{
			if (!IsValidEntity(idx))
				SetFailState("CPFPipe.EntIndex.set --- Invalid Entity Index passed to pipe. Index: %d", idx);
			
			(view_as<ArrayList>(this)).Set(PROP_IDX, idx);
		}
	}
	
	property int DoorIndex
	{
		public get()
		{
			return view_as<int>((view_as<ArrayList>(this)).Get(DOOR_IDX));
		}
	}
	
	property ePFDoorState State
	{
		public get()
		{
			return view_as<ePFDoorState>((view_as<ArrayList>(this)).Get(DOOR_STATE));
		}
		
		public set(ePFDoorState eState)
		{
			if (eState >= DOORSTATE_COUNT || eState < DOOR_CLOSED_IDLE)
				ThrowError("CPFDoor.State.set --- Invalid State %d passed", view_as<int>(eState));
			
			(view_as<ArrayList>(this)).Set(DOOR_STATE, eState);
		}
	}
};

methodmap CPFDoorController
{
	public static bool IsDoor(int idx)
	{
		if (!IsValidEntity(idx)) return false;

		// Check if it's a prop
		char strClassname[128], strModel[256];
		if (!GetEntityClassname(idx, strClassname, sizeof(strClassname))) return false;
		if (strncmp("prop_door", strClassname, 9)) return false;
		
		// Check if it's the right model
		GetEntPropString(idx, Prop_Data, "m_ModelName", strModel, sizeof(strModel));
		for (int i = 0; i < DOORMODEL_COUNT; i++)
		{
			if (StrEqual(g_strDoorModels[i], strModel))
				return true;
		}
		
		return false;
	}
	
	public static void AddDoor(CPFDoor hDoor)
	{
		g_hDoorControl.Push(hDoor);
	}
	
	public static void RemoveDoor(CPFDoor hDoor)
	{
		g_hDoorControl.Erase(hDoor.DoorIndex);
		delete hDoor;
	}
	
	public static void ModifyDoor(CPFDoor hDoor, int idx)
	{
		g_hDoorControl.Set(idx, hDoor);
	}
	
	public static CPFDoor GetDoor(int idx)
	{
		return g_hDoorControl.Get(idx);
	}
	
	public static CPFDoor GetDoorFromProp(int idx)
	{
		CPFDoor hDoor;
		char strKey[32];
		IntToString(EntIndexToEntRef(idx), strKey, sizeof(strKey));
		g_hPropIndexToDoor.GetValue(strKey, hDoor);
		return hDoor;
	}
	
	public static void Init(bool bLate = false)
	{
		if (bLate)
		{
			g_hDoorControl = new ArrayList();
			g_hPropIndexToDoor = new StringMap();
			
			HookEntityOutput("prop_door_rotating", "OnOpen", OnDoorStartOpen);
			HookEntityOutput("prop_door_rotating", "OnFullyOpen", OnDoorFullyOpened);
			HookEntityOutput("prop_door_rotating", "OnClose", OnDoorStartClose);
			HookEntityOutput("prop_door_rotating", "OnFullyClosed", OnDoorFullyClosed);
			
			for (int i = 1; i < 2048; i++)
			{
				if (!CPFDoorController.IsDoor(i))
					continue;
				
				CPFDoor hDoor = new CPFDoor(i);
				CPFDoorController.AddDoor(hDoor);
				
				PrintToServer("CPFDoorController::Init --- Late-loaded door %d", hDoor.DoorIndex);
			}
		}
	}

	public static int Total()
	{
		return g_hDoorControl.Length;
	}
};

public void OnDoorStartOpen(char[] strOutput, int iDoor, int iClient, float flDelay)
{
	CPFDoor hDoor = CPFDoorController.GetDoorFromProp(iDoor);
	if (hDoor == null || !IsValidEntity(EntRefToEntIndex(hDoor.EntIndex)))
		return;
	
	hDoor.State = DOOR_OPENING;
	
	CPFSoundController.PlayDoor(EntRefToEntIndex(hDoor.EntIndex), false);
}

public void OnDoorFullyOpened(char[] strOutput, int iDoor, int iClient, float flDelay)
{
	CPFDoor hDoor = CPFDoorController.GetDoorFromProp(iDoor);
	if (hDoor == null || !IsValidEntity(EntRefToEntIndex(hDoor.EntIndex)))
		return;
	
	hDoor.State = DOOR_OPEN;
}

public void OnDoorStartClose(char[] strOutput, int iDoor, int iClient, float flDelay)
{
	CPFDoor hDoor = CPFDoorController.GetDoorFromProp(iDoor);
	if (hDoor == null || !IsValidEntity(EntRefToEntIndex(hDoor.EntIndex)))
		return;
	
	hDoor.State = DOOR_CLOSING;
	
	CPFSoundController.PlayDoor(EntRefToEntIndex(hDoor.EntIndex), true);
}

public void OnDoorFullyClosed(char[] strOutput, int iDoor, int iClient, float flDelay)
{
	CPFDoor hDoor = CPFDoorController.GetDoorFromProp(iDoor);
	if (hDoor == null || !IsValidEntity(EntRefToEntIndex(hDoor.EntIndex)))
		return;
	
	hDoor.State = DOOR_CLOSED_IDLE;
}