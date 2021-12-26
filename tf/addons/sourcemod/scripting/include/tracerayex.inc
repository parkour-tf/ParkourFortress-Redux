/*
** TraceRay/TraceHull - By Frosty Scales
** TODO: Fix coercion errors. Ignore until then.
*/

#if !defined _sdktools_trace_included
	#include <sdktools>
#endif

#if defined _tracerayex_included
	#endinput
#endif
#define _tracerayex_included

#define TR(%0)		view_as<TraceRay>(%0)
#define TRF(%0)		view_as<TraceRayF>(%0)
#define TRC(%0)		view_as<TraceRayC>(%0)
#define TH(%0)		view_as<TraceHull>(%0)
#define THF(%0)		view_as<TraceHullF>(%0)
#define THC(%0)		view_as<TraceHullC>(%0)
#define HNDL(%0)	view_as<Handle>(%0)

#define TRACE_GLOBAL	view_as<CBaseSDKTrace>(INVALID_HANDLE)

methodmap CBaseSDKTrace < Handle
{
	public CBaseSDKTrace() 
	{ 
		return; 
	}
	
	/**
	 * Get the contents mask and the entity index at the given position.
	 *
	 * @param pos			World position to test.
	 * @param entindex		Entity index found at the given position (by reference).
	 * @return				Contents mask.
	 */
	public static int GetPointContents( const float pos[3],
										int &entindex = -1	)
	{
		return TR_GetPointContents(pos, entindex);
	}
	
	/**
	 * Get the point contents testing only the given entity index.
	 *
	 * @param entindex		Entity index to test.
	 * @param pos			World position.
	 * @return				Contents mask.
	 */
	public static int GetPointContentsEnt(  int entindex,
											const float pos[3]	)
	{
		return TR_GetPointContentsEnt(entindex, pos);
	}
	
	/**
	 * Tests a point to see if it's outside any playable area
	 *
	 * @param pos		Vector buffer to store data in.
	 * @return		True if outside world, otherwise false.
	 */
	public static bool PointOutsideWorld( float pos[3] )
	{
		return TR_PointOutsideWorld(pos);
	}
	
	/**
	 * Clips the current global ray (or hull) to a particular entity.
	 *
	 * @param flags		Trace flags.
	 * @param entity	Entity to clip to.
	 */
	public static void ClipCurrentRayToEntity( int flags, 
												int entity	)
	{
		TR_ClipCurrentRayToEntity(flags, entity);
	}
	
	public static CBaseSDKTrace ClipCurrentRayToEntityEx( int flags, 
															int entity	)
	{
		return view_as<CBaseSDKTrace>(TR_ClipCurrentRayToEntity(flags, entity));
	}
	
	/**
	 * Get the name of the surface hit by the ray.
	 *
	 * @param buffer	Buffer to write the surface name into.
	 * @param maxlen	Length of the buffer.
	 */
	 public void GetSurfaceName( char[] buffer,
	 								int maxlen	)
	 {
	 	TR_GetSurfaceName(this, buffer, maxlen);
	 }
	
	/**
	 * Returns if there was any kind of collision along the trace ray.
	 *
	 * @return				True if any collision found, otherwise false.
	 * @error				Invalid Handle.
	 */
	property bool Hit
	{
		public get()
		{
			return TR_DidHit(this);
		}
	}
	
	/**
	 * Returns the entity index that collided with the trace.
	 *
	 * @return				Entity index or -1 for no collision.
	 * @error				Invalid Handle.
	 */
	property int EntityIndex
	{
		public get()
		{
			return TR_GetEntityIndex(this);
		}
	}
	
	/**
	 * Returns the time fraction from a trace result (1.0 means no collision).
	 *
	 * @return				Time fraction value of the trace.
	 * @error				Invalid Handle.
	 */
	property float Fraction
	{
		public get()
		{
			return TR_GetFraction(this);
		}
	}
	
	/**
	 * Returns the time fraction from a trace result when it left a solid.
	 * Only valid if trace started in solid
	 *
	 * @return				Time fraction left solid value of the trace.
	 * @error				Invalid Handle.
	 */
	property float FractionLeft
	{
		public get()
		{
			return TR_GetFractionLeftSolid(this);
		}
	}
	
	/**
	 * Returns in which body hit group the trace collided if any.
	 *
	 * @return				Body hit group.
	 * @error				Invalid Handle.
	 */
	property int HitGroup
	{
		public get()
		{
			return TR_GetHitGroup(this);
		}
	}
	
	/**
	 * Returns the displacement flags for the surface that was hit. See DISPSURF_FLAG_*.
	 *
	 * @return				Displacement flags.
	 * @error				Invalid Handle.
	 */
	property int DispFlags
	{
		public get()
		{
			return TR_GetDisplacementFlags(this);
		}
	}
	
	/**
	 * Returns the surface properties index of the surface that was hit.
	 *
	 * @return				Surface props.
	 * @error				Invalid Handle.
	 */
	property int SurfaceProps
	{
		public get()
		{
			return TR_GetSurfaceProps(this);
		}
	}
	
	/**
	 * Returns the surface flags. See SURF_*.
	 *
	 * @return				Surface flags.
	 * @error				Invalid Handle.
	 */
	property int SurfaceFlags
	{
		public get()
		{
			return TR_GetSurfaceFlags(this);
		}
	}
	
	/**
	 * Returns the index of the physics bone that was hit.
	 *
	 * @return				Physics bone index.
	 * @error				Invalid Handle.
	 */
	property int PhysBone
	{
		public get()
		{
			return TR_GetPhysicsBone(this);
		}
	}
	
	/**
	 * Returns whether the entire trace was in a solid area.
	 *
	 * @return				True if entire trace was in a solid area, otherwise false.
	 * @error				Invalid Handle.
	 */
	property bool AllSolid
	{
		public get()
		{
			return TR_AllSolid(this);
		}
	}
	
	/**
	 * Returns whether the initial point was in a solid area.
	 *
	 * @return				True if initial point was in a solid area, otherwise false.
	 * @error				Invalid Handle.
	 */
	property bool StartSolid
	{
		public get()
		{
			return TR_StartSolid(this);
		}
	}
	
	/**
	 * Returns the collision position of a trace result.
	 *
	 * @param pos			Vector buffer to store data in.
	 * @error				Invalid Handle.
	 */
	public void GetEndPosition( float pos[3] )
	{
		TR_GetEndPosition(pos, this);
	}
	
	/**
	 * Find the normal vector to the collision plane of a trace.
	 *
	 * @param normal	Vector buffer to store the vector normal to the collision plane
	 * @error			Invalid Handle
	 */
	public void GetPlaneNormal( float normal[3] )
	{
		TR_GetPlaneNormal(this, normal);
	}
};

methodmap TraceRay < CBaseSDKTrace
{
	/**
	 * Starts up a new trace ray using a new trace result.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Depending on RayType, it will be used as the ending 
	 *						point, or the direction angle.
	 * @param flags			Trace flags.
	 * @param rtype			Method to calculate the ray direction.
	 * @return				Ray trace handle, which must be deleted.
	 */
	public TraceRay( const float pos[3], 
					 const float vec[3], 
					 int flags, 
					 RayType rtype			)
	{
		return TR(TR_TraceRayEx(pos, vec, flags, rtype));
	}
	
	/**
	 * Starts up a new trace ray using a global trace result.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Depending on RayType, it will be used as the
	 *						ending point, or the direction angle.
	 * @param flags			Trace flags.
	 * @param rtype			Method to calculate the ray direction.
	 */
	public static void Start( const float pos[3],
										const float vec[3],
										int flags,
										RayType rtype		)
	{
		TR_TraceRay(pos, vec, flags, rtype);
	}
};

methodmap TraceRayF < CBaseSDKTrace
{
	/**
	 * Starts up a new trace ray using a new trace result and a customized 
	 * trace ray filter.
	 *
	 * Calling TR_Trace*Filter or TR_TraceRay*Ex from inside a filter 
	 * function is currently not allowed and may not work.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Depending on RayType, it will be used as the ending 
	 *						point, or the direction angle.
	 * @param flags			Trace flags.
	 * @param rtype			Method to calculate the ray direction.
	 * @param filter		Function to use as a filter.
	 * @param data			Arbitrary data value to pass through to the filter function.
	 * @return				Ray trace handle, which must be closed via CloseHandle().
	 */
	public TraceRayF( 	const float pos[3], 
					 	const float vec[3], 
					 	int flags, 
					 	RayType rtype,
					 	TraceEntityFilter filter,
					 	any data = 0							)
	{
		return TRF(TR_TraceRayFilterEx(pos, vec, flags, rtype, filter, data));
	}
	
	/**
	 * Starts up a new trace ray using a global trace result and a customized 
	 * trace ray filter.
	 *
	 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
	 * function is currently not allowed and may not work.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Depending on RayType, it will be used as the ending 
	 *						point, or the direction angle.
	 * @param flags			Trace flags.
	 * @param rtype			Method to calculate the ray direction.
	 * @param filter		Function to use as a filter.
	 * @param data			Arbitrary data value to pass through to the filter 
	 *						function.
	 */
	public static void Start( const float pos[3],
										const float vec[3],
										int flags,
										RayType rtype,
										TraceEntityFilter filter,
										any data = 0							)
	{
		TR_TraceRayFilter(pos, vec, flags, rtype, filter, data);
	}
};

methodmap TraceRayC < CBaseSDKTrace
{
	/**
	 * Clips a ray to a particular entity.
	 *
	 * @param pos		Starting position of the ray.
	 * @param vec		Depending on RayType, it will be used as the ending
	 *					point, or the direction angle.
	 * @param flags		Trace flags.
	 * @param rtype		Method to calculate the ray direction.
	 * @param entity	Entity to clip to.
	 * @return			Ray trace handle, which must be closed via CloseHandle().
	 */
	public TraceRayC( const float pos[3],
											const float vec[3],
											int flags,
											RayType rtype,
											int entity			)
	{
		return TRC(TR_ClipRayToEntityEx(pos, vec, flags, rtype, entity));
	}
	
	/**
	 * Clips a ray to a particular entity.
	 *
	 * @param pos		Starting position of the ray.
	 * @param vec		Depending on RayType, it will be used as the ending
	 *					point, or the direction angle.
	 * @param flags		Trace flags.
	 * @param rtype		Method to calculate the ray direction.
	 * @param entity	Entity to clip to.
	 */
	public static void Start( const float pos[3],
								const float vec[3],
								int flags,
								RayType rtype,
								int entity			)
	{
		TR_ClipRayToEntity(pos, vec, flags, rtype, entity);
	}
};

methodmap TraceHull < CBaseSDKTrace
{
	/**
	 * Starts up a new trace hull using a new trace result.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Ending position of the ray.
	 * @param mins			Hull minimum size.
	 * @param maxs			Hull maximum size.
	 * @param flags			Trace flags.
	 * @return				Ray trace handle, which must be deleted.
	 */
	public TraceHull(	const float pos[3], 
						const float vec[3], 
						const float mins[3], 
						const float maxs[3], 
						int flags				)
	{
		return TH(TR_TraceHullEx(pos, vec, mins, maxs, flags));
	}
	
	/**
	 * Starts up a new trace hull using a global trace result.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Ending position of the ray.
	 * @param mins			Hull minimum size.
	 * @param maxs			Hull maximum size.
	 * @param flags			Trace flags.
	 */
	public static void Start(  	const float pos[3], 
								const float vec[3], 
								const float mins[3], 
								const float maxs[3], 
								int flags				)
	{
		(filter == NULL_FUNC) ? TR_TraceHull(pos, vec, mins, maxs, flags) : TR_TraceHullFilter(pos, vec, mins, maxs, flags, filter, data)
	}
};

methodmap TraceHullF < CBaseSDKTrace
{
	/**
	 * Starts up a new trace hull using a new trace result and a customized 
	 * trace ray filter.
	 *
	 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
	 * function is currently not allowed and may not work.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Ending position of the ray.
	 * @param mins			Hull minimum size.
	 * @param maxs			Hull maximum size.
	 * @param flags			Trace flags.
	 * @param filter		Function to use as a filter.
	 * @param data			Arbitrary data value to pass through to the filter function.
	 * @return				Ray trace handle, which must be closed via CloseHandle().
	 */
	public TraceHullF(	const float pos[3], 
						const float vec[3], 
						const float mins[3], 
						const float maxs[3], 
						int flags,
						TraceEntityFilter filter,
						any data = 0							)
	{
		return THF(TR_TraceHullFilterEx(pos, vec, mins, maxs, flags, filter, data));
	}
	
	/**
	 * Starts up a new trace ray using a global trace result and a customized 
	 * trace ray filter.
	 *
	 * Calling TR_Trace*Filter or TR_Trace*FilterEx from inside a filter 
	 * function is currently not allowed and may not work.
	 *
	 * @param pos			Starting position of the ray.
	 * @param vec			Depending on RayType, it will be used as the ending 
	 *						point, or the direction angle.
	 * @param flags			Trace flags.
	 * @param rtype			Method to calculate the ray direction.
	 * @param filter		Function to use as a filter.
	 * @param data			Arbitrary data value to pass through to the filter 
	 *						function.
	 */
	public static void Start(  const float pos[3], 
										const float vec[3], 
										const float mins[3], 
										const float maxs[3], 
										int flags, 
										TraceEntityFilter filter, 
										any data = 0							)
	{
		TR_TraceHullFilter(pos, vec, mins, maxs, flags, filter, data);
	}
};

methodmap TraceHullC < CBaseSDKTrace
{
	public TraceHullC( const float pos[3],
					   const float vec[3],
					   const float mins[3],
					   const float maxs[3],
					   int flags,
					   int entity			)
	{
		return THC(TR_ClipRayHullToEntityEx(pos, vec, mins, maxs, flags, entity));
	}
	
	/**
	 * Clips a ray hull to a particular entity.
	 *
	 * @param pos		Starting position of the ray.
	 * @param vec		Ending position of the ray.
	 * @param mins		Hull minimum size.
	 * @param maxs		Hull maximum size.
	 * @param flags		Trace flags.
	 * @param entity	Entity to clip to.
	 */
	public static void Start( const float pos[3],
							   const float vec[3],
							   const float mins[3],
							   const float maxs[3],
							   int flags,
							   int entity			)
	{
		TR_ClipRayHullToEntity(pos, vec, mins, maxs, flags, rtype, entity);
	}
};