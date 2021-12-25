#if defined DONOTDEFINE
	// Just a fix so BasicPawn can see my includes
	#include "..\parkourfortress.sp"
#endif

#if defined _OBJECTS_PIPES_INCLUDED
	#endinput
#endif
#define _OBJECTS_PIPES_INCLUDED

static ArrayList g_hPipeControl;
static StringMap g_hClimbableToPipe;

enum CPFPipe {};
enum CPFPipeController {};

enum ePFPipeProperties
{
	CLIMBABLE_IDX,
	PIPE_IDX,
	NEAREST_WALL_ANG,
	
	PIPEPROP_COUNT
};

methodmap CPFPipe < ArrayList
{
	public CPFPipe(int iClimbable)
	{
		const int PIPE_ANGLE_SCAN_INCREMENT = 1;
		
		if (!IsValidEntity(iClimbable))
			SetFailState("CPFPipe::CPFPipe --- Invalid entity passed to constructor! Entity: %d", iClimbable);
		
		float vecOrigin[3], flAngle, flLowestDist;
		GetEntPropVector(iClimbable, Prop_Send, "m_vecOrigin", vecOrigin);
		
		// For every x degrees, we run an infinite trace, and the lowest result will be the angle of the pipe climb animation
		for (int iTheta = 0; iTheta < 360; iTheta += PIPE_ANGLE_SCAN_INCREMENT)
		{
			float vecAngle[3];
			vecAngle[1] = float(iTheta);
			
			/**** START TRACE ****/
			TraceRayF hTrace = new TraceRayF(vecOrigin, vecAngle, MASK_PLAYERSOLID, RayType_Infinite, TraceNoSelf, iClimbable);
			bool bHit = hTrace.Hit;
			
			if (bHit)
			{
				float vecHit[3];
				hTrace.GetEndPosition(vecHit);
				float flDist = GetVectorDistance(vecHit, vecOrigin);
				if (flDist < flLowestDist || flDist == 0.0)
				{
					flLowestDist = flDist;
					flAngle = vecAngle[1];
				}
			}
			
			delete hTrace;
			/**** END TRACE ****/
		}
		
		int iPipeIndex;
		if (g_hPipeControl == null)
			SetFailState("CPFPipe::CPFPipe --- Controller not initiated! Handle: %X", g_hPipeControl);
		else
			iPipeIndex = g_hPipeControl.Length;
		
		// Create and return our pipe "object"
		ArrayList hPipe = new ArrayList(1, PIPEPROP_COUNT);
		hPipe.Set(CLIMBABLE_IDX, EntIndexToEntRef(iClimbable));
		hPipe.Set(PIPE_IDX, iPipeIndex);
		hPipe.Set(NEAREST_WALL_ANG, flAngle);
		return view_as<CPFPipe>(hPipe);
	}
	
	property int EntIndex
	{
		public get()
		{
			return EntRefToEntIndex(view_as<int>(this.Get(CLIMBABLE_IDX)));
		}
		
		public set(int idx)
		{
			if (!IsValidEntity(idx))
				SetFailState("CPFPipe.EntIndex.set --- Invalid Entity Index passed to pipe. Index: %d", idx);
			
			this.Set(CLIMBABLE_IDX, EntIndexToEntRef(idx));
		}
	}
	
	property int PipeIndex
	{
		public get()
		{
			return view_as<int>(this.Get(PIPE_IDX));
		}
	}
	
	property float Angle
	{
		public get()
		{
			return this.Get(NEAREST_WALL_ANG);
		}
		
		public set(float flAngle)
		{
			if (flAngle > 360.0 || flAngle < 0.0)
				SetFailState("CPFPipe.Angle.set --- Invalid Angle passed to pipe. Angle: %f", flAngle);
			
			view_as<float>(this.Set(NEAREST_WALL_ANG, flAngle));
		}
	}
};

#define CPFPIPE_INVALID view_as<CPFPipe>(0)
CPFPipe g_hClientPipe[MAXPLAYERS + 1] = { CPFPIPE_INVALID, CPFPIPE_INVALID, CPFPIPE_INVALID, ... };
CPFPipe g_hClientLastPipe[MAXPLAYERS + 1] = { CPFPIPE_INVALID, CPFPIPE_INVALID, CPFPIPE_INVALID, ... };

methodmap CPFPipeController
{
	public static void AddPipe(CPFPipe hPipe)
	{
		g_hPipeControl.Push(hPipe);
		
		char strKey[6];
		IntToString(hPipe.EntIndex, strKey, sizeof(strKey));
		g_hClimbableToPipe.SetValue(strKey, hPipe);
	}
	
	public static void RemovePipe(CPFPipe hPipe)
	{
		g_hPipeControl.Erase(hPipe.PipeIndex);
		
		char strKey[6];
		IntToString(hPipe.EntIndex, strKey, sizeof(strKey));
		g_hClimbableToPipe.Remove(strKey);
		delete hPipe;
	}
	
	public static CPFPipe GetPipe(int idx)
	{
		return g_hPipeControl.Get(idx);
	}
	
	public static CPFPipe GetPipeByEntIndex(int idx)
	{
		CPFPipe hPipe;
		char strKey[6];
		IntToString(idx, strKey, sizeof(strKey));
		if (g_hClimbableToPipe.GetValue(strKey, hPipe) && hPipe != null)
		{
			return hPipe;
		}
		else
		{
			SetFailState("CPFPipeController::GetPipeByEntIndex --- Pipe at index %d not registered in StringMap!", idx);
			return view_as<CPFPipe>(0);
		}
	}
	
	public static void ModifyPipe(CPFPipe hPipe, int idx)
	{
		CPFPipeController.RemovePipe(CPFPipeController.GetPipe(idx));
		g_hPipeControl.Set(idx, hPipe);
	}
	
	public static void SetClientPipe(int iClient, CPFPipe hPipe)
	{
		g_hClientPipe[iClient] = hPipe;
	}
	
	public static CPFPipe GetClientPipe(int iClient)
	{
		return g_hClientPipe[iClient];
	}
	
	public static void SetClientLastPipe(int iClient, CPFPipe hPipe)
	{
		g_hClientLastPipe[iClient] = hPipe;
	}
	
	public static CPFPipe GetClientLastPipe(int iClient)
	{
		return g_hClientLastPipe[iClient];
	}
	
	public static void Init(bool bLate = false)
	{
		g_hPipeControl = new ArrayList();
		g_hClimbableToPipe = new StringMap();
		
		for (int i = 1; i < MaxClients; i++)
			g_hClientPipe[i] = CPFPIPE_INVALID;
		
		SDKHookClassname("trigger_multiple", SDKHook_StartTouch, OnStartTouchTrigger);
		SDKHookClassname("trigger_multiple", SDKHook_EndTouch, OnEndTouchTrigger);
		
		if (bLate)
		{
			for (int i = 1; i < 2048; i++)
			{
				if (!IsValidEntity(i)) continue;
				
				char strClassname[128], strTargetname[256];
				if (!GetEntityClassname(i, strClassname, sizeof(strClassname))) continue;
				if (strncmp("trigger_", strClassname, 8)) continue;
			
				GetEntPropString(i, Prop_Data, "m_iName", strTargetname, sizeof(strTargetname));
				if (!StrEqual("climbable", strTargetname)) continue;
				
				CPFPipe hPipe = new CPFPipe(i);
				CPFPipeController.AddPipe(hPipe);
				
				PrintToServer("CPFPipeController::Init --- Late-loaded pipe %d", hPipe.PipeIndex);
				PrintToServer("CPFPipeController::Init --- Angle: %.3f", hPipe.Angle);
			}
		}
	}
	
	public static int Total()
	{
		return g_hPipeControl.Length;
	}
};

public bool TraceNoSelf(int iEntity, int iContentsMask, any aData)
{
	return (aData == iEntity);
}