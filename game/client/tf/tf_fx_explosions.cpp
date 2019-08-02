//========= Copyright � 1996-2006, Valve Corporation, All rights reserved. ============//
//
// Purpose: Game-specific explosion effects
//
//=============================================================================//
#include "cbase.h"
#include "c_te_effect_dispatch.h"
#include "tempent.h"
#include "c_te_legacytempents.h"
#include "tf_shareddefs.h"
#include "engine/IEngineSound.h"
#include "tf_weapon_parse.h"
#include "tf_weapon_rocketlauncher.h"
#include "c_basetempentity.h"
#include "tier0/vprof.h"
#include "dlight.h"
#include "iefx.h"

extern ConVar of_muzzlelight;

//--------------------------------------------------------------------------------------------------------------
CTFWeaponInfo *GetTFWeaponInfo( int iWeapon )
{
	// Get the weapon information.
	const char *pszWeaponAlias = WeaponIdToAlias( iWeapon );
	if ( !pszWeaponAlias )
	{
		return NULL;
	}

	WEAPON_FILE_INFO_HANDLE	hWpnInfo = LookupWeaponInfoSlot( pszWeaponAlias );
	if ( hWpnInfo == GetInvalidWeaponInfoHandle() )
	{
		return NULL;
	}

	CTFWeaponInfo *pWeaponInfo = static_cast<CTFWeaponInfo*>( GetFileWeaponInfoFromHandle( hWpnInfo ) );
	return pWeaponInfo;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void TFExplosionCallback(const Vector &vecOrigin, const Vector &vecNormal, int iWeaponID, ClientEntityHandle_t hEntity, CBaseEntity *m_hLauncher)
{
	// Get the weapon information.
	CTFWeaponInfo *pWeaponInfo = NULL;
	pWeaponInfo = GetTFWeaponInfo(iWeaponID);
	if ( iWeaponID == TF_WEAPON_GRENADE_MIRVBOMB )
		pWeaponInfo = GetTFWeaponInfo( TF_WEAPON_GRENADELAUNCHER_MERCENARY );

	bool bIsPlayer = false;
	if (hEntity.Get())
	{
		C_BaseEntity *pEntity = C_BaseEntity::Instance(hEntity);
		if (pEntity && pEntity->IsPlayer())
		{
			bIsPlayer = true;
		}
	}

	// Calculate the angles, given the normal.
	bool bIsWater = (UTIL_PointContents(vecOrigin) & CONTENTS_WATER);
	bool bInAir = false;
	QAngle angExplosion(0.0f, 0.0f, 0.0f);

	// Cannot use zeros here because we are sending the normal at a smaller bit size.
	if (fabs(vecNormal.x) < 0.05f && fabs(vecNormal.y) < 0.05f && fabs(vecNormal.z) < 0.05f)
	{
		bInAir = true;
		angExplosion.Init();
	}
	else
	{
		VectorAngles(vecNormal, angExplosion);
		bInAir = false;
	}
	CTFOriginal *pOriginal = dynamic_cast<CTFOriginal *>(m_hLauncher);
	bool bClassic = false;
	if ( pOriginal )
		bClassic = pOriginal->m_bClassic;
	// Base explosion effect and sound.
	char *pszEffect = "ExplosionCore_wall";
	char *pszSound = "BaseExplosionEffect.Sound";

	if (pWeaponInfo)
	{
		// Explosions.
		if ( bIsWater )
		{
			if ( bClassic ){ if (Q_strlen(pWeaponInfo->m_szExplosionWaterEffectClassic) > 0)
								pszEffect = pWeaponInfo->m_szExplosionWaterEffectClassic;}
			else{ if (Q_strlen(pWeaponInfo->m_szExplosionWaterEffect) > 0)
					pszEffect = pWeaponInfo->m_szExplosionWaterEffect;}
		}
		else
		{
			if (bIsPlayer || bInAir)
			{
				if ( bClassic ){ if (Q_strlen(pWeaponInfo->m_szExplosionPlayerEffectClassic) > 0)
									pszEffect = pWeaponInfo->m_szExplosionPlayerEffectClassic;}
				else{ if (Q_strlen(pWeaponInfo->m_szExplosionPlayerEffect) > 0)
						pszEffect = pWeaponInfo->m_szExplosionPlayerEffect;}
			}
			else
			{
				if ( bClassic ){ if (Q_strlen(pWeaponInfo->m_szExplosionEffectClassic) > 0)
						pszEffect = pWeaponInfo->m_szExplosionEffectClassic;}
				else{ if (Q_strlen(pWeaponInfo->m_szExplosionEffect) > 0)
						pszEffect = pWeaponInfo->m_szExplosionEffect;}
			}
		}

		// Sound.
		if (Q_strlen(pWeaponInfo->m_szExplosionSound) > 0)
		{
			pszSound = pWeaponInfo->m_szExplosionSound;
		}
	}

	CLocalPlayerFilter filter;
	C_BaseEntity::EmitSound(filter, SOUND_FROM_WORLD, pszSound, &vecOrigin);

	if (of_muzzlelight.GetBool())
	{
		dlight_t *dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC);
		dl->origin = vecOrigin;
		dl->color.r = 255;
		dl->color.g = 250;
		dl->color.b = 140;
		dl->decay = 200;
		dl->radius = 512.f;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		dl->die = gpGlobals->curtime + 0.1f;
	}
	else
	{
		dlight_t *dl = effects->CL_AllocDlight(LIGHT_INDEX_TE_DYNAMIC);
		dl->origin = vecOrigin;
		dl->color.r = 255;
		dl->color.g = 220;
		dl->color.b = 128;
		dl->decay = 200;
		dl->radius = 340.f;
		dl->flags = DLIGHT_NO_MODEL_ILLUMINATION;
		dl->die = gpGlobals->curtime + 0.1f;
	}

	DispatchParticleEffect(pszEffect, vecOrigin, angExplosion);
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class C_TETFExplosion : public C_BaseTempEntity
{
public:

	DECLARE_CLASS( C_TETFExplosion, C_BaseTempEntity );
	DECLARE_CLIENTCLASS();

	C_TETFExplosion( void );

	virtual void	PostDataUpdate( DataUpdateType_t updateType );

public:

	Vector		m_vecOrigin;
	Vector		m_vecNormal;
	int			m_iWeaponID;
	ClientEntityHandle_t m_hEntity;
	CNetworkHandle( CBaseEntity, m_hLauncher );
};

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
C_TETFExplosion::C_TETFExplosion( void )
{
	m_vecOrigin.Init();
	m_vecNormal.Init();
	m_iWeaponID = TF_WEAPON_NONE;
	m_hEntity = INVALID_EHANDLE_INDEX;
	m_hLauncher = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TETFExplosion::PostDataUpdate( DataUpdateType_t updateType )
{
	VPROF( "C_TETFExplosion::PostDataUpdate" );

	TFExplosionCallback( m_vecOrigin, m_vecNormal, m_iWeaponID, m_hEntity, m_hLauncher );
}

static void RecvProxy_ExplosionEntIndex( const CRecvProxyData *pData, void *pStruct, void *pOut )
{
	int nEntIndex = pData->m_Value.m_Int;
	((C_TETFExplosion*)pStruct)->m_hEntity = (nEntIndex < 0) ? INVALID_EHANDLE_INDEX : ClientEntityList().EntIndexToHandle( nEntIndex );
}

IMPLEMENT_CLIENTCLASS_EVENT_DT( C_TETFExplosion, DT_TETFExplosion, CTETFExplosion )
	RecvPropFloat( RECVINFO( m_vecOrigin[0] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[1] ) ),
	RecvPropFloat( RECVINFO( m_vecOrigin[2] ) ),
	RecvPropVector( RECVINFO( m_vecNormal ) ),
	RecvPropInt( RECVINFO( m_iWeaponID ) ),
	RecvPropInt( "entindex", 0, SIZEOF_IGNORE, 0, RecvProxy_ExplosionEntIndex ),
	RecvPropEHandle( RECVINFO( m_hLauncher ) ),
END_RECV_TABLE()

