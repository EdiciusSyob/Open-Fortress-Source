//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: Base VoteController.  Handles holding and voting on issues.
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "shareddefs.h"
#include "eiface.h"
#include "team.h"
#include "gameinterface.h"
#include "fmtstr.h"

#include "tf_shareddefs.h"
#include "tf_voteissues.h"
#include "tf_gamerules.h"
#include "tf_player.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar sv_vote_issue_restart_game_allowed( "sv_vote_issue_restart_game_allowed", "1", FCVAR_NONE, "Can players call votes to restart the game?" );
ConVar sv_vote_issue_restart_game_cooldown( "sv_vote_issue_restart_game_cooldown", "300", FCVAR_NONE, "Minimum time before another restart vote can occur (in seconds)." );
ConVar sv_vote_kick_ban_duration( "sv_vote_kick_ban_duration", "20", FCVAR_NONE, "The number of minutes a vote ban should last. (0 = Disabled)" );
ConVar sv_vote_issue_changelevel_allowed( "sv_vote_issue_changelevel_allowed", "1", FCVAR_NONE, "Can players call votes to change levels?" );
ConVar sv_vote_issue_changemutator_allowed( "sv_vote_issue_changemutator_allowed", "1", FCVAR_NONE, "Can players call votes to change the mutators?" );
ConVar sv_vote_issue_nextlevel_allowed( "sv_vote_issue_nextlevel_allowed", "1", FCVAR_NONE, "Can players call votes to set the next level?" );
// TODO:
// sv_vote_issue_nextlevel_allowextend( "sv_vote_issue_nextlevel_allowextend", "1", FCVAR_NONE, "Allow players to extend the current map?" );
//ConVar sv_vote_issue_extendlevel_quorum( "sv_vote_issue_extendlevel_quorum", "60", FCVAR_NONE, "What is the ratio of voters needed to reach quorum?" );
ConVar sv_vote_issue_scramble_teams_allowed( "sv_vote_issue_scramble_teams_allowed", "1", FCVAR_NONE, "Can players call votes to scramble the teams?" );
ConVar sv_vote_issue_scramble_teams_cooldown( "sv_vote_issue_scramble_teams_cooldown", "1200", FCVAR_NONE, "Minimum time before another scramble vote can occur (in seconds)." );
// TODO:
//ConVar sv_vote_issue_autobalance_allowed( "sv_vote_issue_autobalance_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable auto team balance?" );
//ConVar sv_vote_issue_autobalance_cooldown( "sv_vote_issue_autobalance_cooldown", "300", FCVAR_NONE, "Minimum time before another auto team balance vote can occur (in seconds)." );
//ConVar sv_vote_issue_classlimits_allowed( "sv_vote_issue_classlimits_allowed", "0", FCVAR_NONE, "Can players call votes to enable or disable per-class limits?" );
// this convar is clamped to min 1
//ConVar sv_vote_issue_classlimits_max( "sv_vote_issue_classlimits_max", "0", FCVAR_NONE, "Maximum number of players (per-team) that can be any one class." );
//ConVar sv_vote_issue_classlimits_cooldown( "sv_vote_issue_classlimits_cooldown", "0", FCVAR_NONE, "Minimum time before another classlimits vote can occur (in seconds)." );

//-----------------------------------------------------------------------------
// VTABLE GLOBAL: https://raw.githubusercontent.com/sigsegv-mvm/mvm-reversed/7ce8ac98fe187a07d71df87b02b4c038548837e9/Useful/symbols/ServerLinux-server_srv.txt
// VTABLE LOCAL: https://github.com/sigsegv-mvm/mvm-reversed/blob/7ce8ac98fe187a07d71df87b02b4c038548837e9/Useful/vtable/server_srv/CBaseTFIssue.txt
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CBaseTFIssue::ExecuteCommand()
{
	// nothing
}

void CBaseTFIssue::ListIssueDetails( CBasePlayer *pForWhom )
{
	// stub from vtable
	return;
}

//-----------------------------------------------------------------------------
// Purpose: Ban player
//-----------------------------------------------------------------------------
const char *CKickIssue::GetDisplayString()
{
	// shouldn't there be something else for bots?
	return "#TF_vote_kick_player";
}

const char *CKickIssue::GetDetailsString( void )
{
	const char* pszDetails = m_szDetailsString;

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( atoi( pszDetails ) );

	if ( !pPlayer )
		return m_szDetailsString;

	// bots need their own treatment
	if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
		pszDetails = UTIL_VarArgs( "%s (BOT)", pPlayer->GetPlayerName() );
	else
		pszDetails = UTIL_VarArgs( "%s", pPlayer->GetPlayerName() );

	return pszDetails;
}

const char *CKickIssue::GetVotePassedString()
{
	// shouldn't there be something else for bots?
	return "#TF_vote_passed_kick_player";
}

bool CKickIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{
	// The default vote issue is a Yes/No vote
	vecNames.AddToHead( "Yes" );
	vecNames.AddToTail( "No" );

	return true;
}

bool CKickIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	// string -> int
	int m_iPlayerID = atoi( pszDetails );

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( m_iPlayerID );

	if ( !pPlayer )
		return false;

	return CBaseTFIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
}

void CKickIssue::ExecuteCommand( void )
{
	const char* pszDetails = m_szDetailsString;

	CBasePlayer *pPlayer = UTIL_PlayerByUserId( atoi( pszDetails ) );

	if ( !pPlayer )
		return;

	if ( ( pPlayer->GetFlags() & FL_FAKECLIENT ) )
	{
		engine->ServerCommand( UTIL_VarArgs( "kickid %d;", pPlayer->GetUserID() ) );
	}
	else
	{
		engine->ServerCommand( UTIL_VarArgs( "banid %i %s kick\n", sv_vote_kick_ban_duration.GetInt(), m_szNetworkIDString.String() ) );

		engine->ServerCommand( "writeip\n" );
		engine->ServerCommand( "writeid\n" );
	}
}

void CKickIssue::SetIssueDetails( const char *pszDetails )
{
	CBaseTFIssue::SetIssueDetails( pszDetails );

	int iID = atoi( pszDetails );

	CBasePlayer* pPlayer = UTIL_PlayerByUserId( iID );

	if ( pPlayer )
		m_szNetworkIDString = pPlayer->GetNetworkIDString();
}

bool CKickIssue::IsEnabled()
{
	if ( sv_vote_kick_ban_duration.GetInt() <= 0 )
		return false;
	else
		return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CRestartGameIssue::Init()
{
	SetIssueCooldownDuration( sv_vote_issue_restart_game_cooldown.GetFloat() );
}

const char *CRestartGameIssue::GetDisplayString()
{
	return "#TF_vote_restart_game";
}

const char *CRestartGameIssue::GetVotePassedString()
{
	return "#TF_vote_passed_restart_game";
}

void CRestartGameIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( "mp_restartgame 3\n" );
}

bool CRestartGameIssue::IsEnabled()
{
	if ( sv_vote_issue_restart_game_allowed.GetBool() )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CScrambleTeams::Init()
{
	SetIssueCooldownDuration( sv_vote_issue_scramble_teams_cooldown.GetFloat() );
}

const char *CScrambleTeams::GetDisplayString()
{
	return "#TF_vote_scramble_teams";
}

const char *CScrambleTeams::GetVotePassedString()
{
	return "#TF_vote_passed_scramble_teams";
}

void CScrambleTeams::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( "mp_scrambleteams 1\n" );
}

bool CScrambleTeams::IsEnabled()
{
	if ( TFGameRules() && ( TFGameRules()->IsDMGamemode() && !TFGameRules()->IsTDMGamemode() ) )
		return false;

	if ( sv_vote_issue_scramble_teams_allowed.GetBool() )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CChangeLevelIssue::GetDisplayString()
{
	return "#TF_vote_changelevel";
}

const char *CChangeLevelIssue::GetVotePassedString()
{
	return "#TF_vote_passed_changelevel";
}

void CChangeLevelIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( UTIL_VarArgs( "changelevel %s\n", m_szDetailsString ) );
}

bool CChangeLevelIssue::IsEnabled()
{
	if ( sv_vote_issue_changelevel_allowed.GetBool() )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CChangeMutatorIssue::GetDisplayString()
{
	return "#TF_vote_changemutator";
}

const char *CChangeMutatorIssue::GetDetailsString( void )
{
	int iMutator = ( atoi( m_szDetailsString ) );

	if ( iMutator == NO_MUTATOR )
		return "#TF_vote_mutator_disabled";
	else if ( iMutator == INSTAGIB )
		return "#TF_vote_mutator_instagibrailguncrowbar";
	else if ( iMutator == INSTAGIB_NO_MELEE )
		return "#TF_vote_mutator_instagibrailgun";
	else if ( iMutator == CLAN_ARENA )
		return "#TF_vote_mutator_clanarena";
	else if ( iMutator == UNHOLY_TRINITY )
		return "#TF_vote_mutator_unholytrinity";
	else if ( iMutator == ROCKET_ARENA )
		return "#TF_vote_mutator_rocketarena";
	else if ( iMutator == GUN_GAME )
		return "#TF_vote_mutator_gungame";
	else
		return m_szDetailsString;
}

const char *CChangeMutatorIssue::GetVotePassedString()
{
	return "#TF_vote_passed_changemutator";
}

void CChangeMutatorIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	engine->ServerCommand( UTIL_VarArgs( "of_mutator %s\n", m_szDetailsString ) );
}

bool CChangeMutatorIssue::IsEnabled()
{
	if ( sv_vote_issue_changemutator_allowed.GetBool() )
		return true;
	else
		return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
const char *CNextLevelIssue::GetDisplayString()
{
	// dedicated server vote
	if ( m_szDetailsString[ 0 ] == '\0' )
		return "#TF_vote_nextlevel_choices";
	else
		return "#TF_vote_nextlevel";
}

const char *CNextLevelIssue::GetVotePassedString()
{
	return "#TF_vote_passed_changelevel";
}

bool CNextLevelIssue::IsYesNoVote( void )
{
	if ( m_szDetailsString[0] != '\0' )
		return true;
	else
		return false;
}

bool CNextLevelIssue::CanCallVote( int nEntIndex, const char *pszDetails, vote_create_failed_t &nFailCode, int &nTime )
{
	return CBaseTFIssue::CanCallVote( nEntIndex, pszDetails, nFailCode, nTime );
}

bool CNextLevelIssue::GetVoteOptions( CUtlVector <const char*> &vecNames )
{	
	if ( m_szDetailsString[0] != '\0' )
	{
		// The default vote issue is a Yes/No vote
		vecNames.AddToHead( "Yes" );
		vecNames.AddToTail( "No" );

		return true;
	}
	else
	{	
		CUtlVector< char* > m_MapList;

		m_MapList.AddVectorToTail( MultiplayRules()->GetMapList() );

		while ( vecNames.Count() < 6 )
		{
			if ( !m_MapList.Count() )
				break;

			int i;

			i = RandomInt( 0, m_MapList.Count() - 1 );

			vecNames.AddToTail( m_MapList[ i ] );
			m_MapList.Remove( i );
		}

		return true;
	}
}

void CNextLevelIssue::ExecuteCommand()
{
	CBaseTFIssue::ExecuteCommand();

	// extern ConVar nextlevel;
	ConVarRef nextlevel( "nextlevel" );

	nextlevel.SetValue( m_szDetailsString );
}

bool CNextLevelIssue::IsEnabled()
{
	if ( sv_vote_issue_nextlevel_allowed.GetBool() )
		return true;
	else
		return false;
}