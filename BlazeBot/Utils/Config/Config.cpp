#include <windows.h>
#include <iostream>
#include <shlobj.h>
#include "Config.h"

#include <fstream>

#pragma comment(lib, "shell32.lib")
#include "..\..\json.hpp"
using json = nlohmann::json;


#include <direct.h>


std::string BETTING_CLASS = { "betting" };
std::string PREDICTING_CLASS = { "predicting" };
std::string GAME_CLASS = { "game" };


class VARIABLE {


	void * ptr;
	int type;
	std::string name;
	std::vector<std::string > classe;

public:

	VARIABLE( ) {
		ptr = nullptr;
		name = "";
	}

	VARIABLE( void * ptr , int type , std::string name , std::vector<std::string> classe )
	{
		this->type = type;
		this->ptr = ptr;
		this->name = name;
		this->classe = classe;
	}

	void * GetPtr( ) {
		return this->ptr;
	}

	int * GetAdressAsInt( ) {
		return ( reinterpret_cast< int * >( ptr ) );
	}

	float * GetAdressAsFloat( ) {
		return ( reinterpret_cast< float * >( ptr ) );
	}

	bool * GetAdressAsBool( ) {
		return ( reinterpret_cast< bool * >( ptr ) );
	}

	Vector2D * GetAdressAsPoint( ) {
		return ( reinterpret_cast< Vector2D* >( ptr ) );
	}

	std::vector<float> * GetAdressAsVector( ) {
		return ( reinterpret_cast< std::vector<float> * >( ptr ) );
	}	

	std::string GetName( ) {
		return this->name;
	}

	std::vector<std::string> GetClass( ) {
		return this->classe;
	}

	int GetType( ) {
		return this->type;
	}

};

std::vector<VARIABLE> variables;

enum {
	B ,
	I ,
	F ,
	C,
	P,
	V
};

json getValueFromJson( json jsonObject , std::vector<std::string> keys ) {
	json value = jsonObject;
	for ( const auto & key : keys ) {
		value = value[ key ];
	}
	return value;
}

void setValueInJson( json * jsonObject , std::vector<std::string> keys , json newValue ) {
	json * value = jsonObject;
	for ( auto it = keys.begin( ); it != keys.end( ) - 1; ++it ) {
		value = &( ( *value )[ *it ] );
	}
	( *value )[ *( keys.end( ) - 1 ) ] = newValue;
}

void Load( VARIABLE & var , json * cfgjson )
{
	bool * BoolPtr = nullptr;
	int * IntPtr = nullptr;
	float * FloatPtr = nullptr;
	std::vector<float> * VecPtr = nullptr;
	Vector2D * PointPtr  = nullptr;
	int type = var.GetType( );

	if ( type != C ) {

		switch ( type ) {
		case B:
			BoolPtr = var.GetAdressAsBool( );
			break;
		case I:
			IntPtr = var.GetAdressAsInt( );
			break;
		case F:
			FloatPtr = var.GetAdressAsFloat( );
			break;
		case V:
			VecPtr = var.GetAdressAsVector( );
			break;
		case P:
			PointPtr = var.GetAdressAsPoint( );
			break;
		}

		auto Class = getValueFromJson( *cfgjson , var.GetClass( ) );
		auto Value = Class[ var.GetName( ) ];
		if ( !Value.empty( ) )
		{
			switch ( type ) {
			case B:
				*BoolPtr = Value;
				break;
			case I:
				*IntPtr = Value;
				break;
			case F:
				*FloatPtr = Value;
				break;
			case V:	
				for ( auto & value : Value ) {
					VecPtr->emplace_back( value );
				}
				break;
			case P:
				PointPtr->x = Value[ "x" ];
				PointPtr->y = Value[ "y" ];
				break;
			}
		}
	}
	else { //Color
	

		auto ColorClass = getValueFromJson( *cfgjson , var.GetClass( ) );

		int r , g , b , a;

		if ( !ColorClass[ "R" ].empty( ) )
			r = ColorClass[ "R" ];

		if ( !ColorClass[ "G" ].empty( ) )
			g = ColorClass[ "G" ];

		if ( !ColorClass[ "B" ].empty( ) )
			b = ColorClass[ "B" ];

		if ( !ColorClass[ "A" ].empty( ) )
			a = ColorClass[ "A" ];
	}
}

void Save( VARIABLE * var , json * cfgjson )
{
	bool * BoolPtr = nullptr;
	int * IntPtr = nullptr;
	float * FloatPtr = nullptr;
	std::vector<float> * VecPtr = nullptr;
	Vector2D * PointPtr = nullptr;

	json js2;
	js2 = *cfgjson;

	int type = var->GetType( );

	if ( type != C ) {

		std::vector<std::string > key = var->GetClass( );
		key.emplace_back( var->GetName( ) );

		auto Value = getValueFromJson( js2 , key );

		switch ( var->GetType( ) ) {
		case B:
			BoolPtr = var->GetAdressAsBool( );
			Value[ var->GetName( ) ] = *BoolPtr;
			break;
		case I:
			IntPtr = var->GetAdressAsInt( );
			Value[ var->GetName( ) ] = *IntPtr;
			break;
		case F:
			FloatPtr = var->GetAdressAsFloat( );
			Value[ var->GetName( ) ] = *FloatPtr;
			break;
		case V:
			VecPtr = var->GetAdressAsVector( );
			Value[ var->GetName( ) ] = *VecPtr;
			break;
		case P:
			PointPtr = var->GetAdressAsPoint( );
			Value[ var->GetName() ][ "x" ] = PointPtr->x;
			Value[ var->GetName( ) ][ "y" ] = PointPtr->y;
			break;
		}

		auto SndValue = Value[ var->GetName( ) ];
		setValueInJson( cfgjson , key , SndValue );
	}
}

void Add( void * ptr , int type , std::string name , std::vector<std::string> classe ) {
	variables.emplace_back( VARIABLE( ptr , type , name , classe ) );
}

void AddPoint( ) {
	
	//MAKE THIS
}


void AddVars( ) {

	std::vector<std::string> bBettingClass = { BETTING_CLASS };

	Add( &cfg::Get( ).Betting.MaxBetPercentage , F , "MaxBetPercentage" , bBettingClass );
	Add( &cfg::Get( ).Betting.MinBetPercentage , F , "MinBetPercentage" , bBettingClass );
	Add( &cfg::Get( ).Betting.StopPercentage , F , "StopPercentage" , bBettingClass );
	Add( &cfg::Get( ).Betting.TargetParcentage , F , "TargetParcentage" , bBettingClass );

	std::vector<std::string> bSecurityClass = { BETTING_CLASS, "Security" };

	Add( &cfg::Get( ).Betting.security.PreventDownPeaks , B , "PreventDownPeaks" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.MinimumPeakValue , F , "MinimumPeakValue" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.MinimumPeakDistance , I , "MinimumPeakDistance" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.WaitingTime , I , "WaitingTime" , bSecurityClass );

	Add( &cfg::Get( ).Betting.security.ProtectProfit , B , "ProtectProfit" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.ProtectIfProfit ,F , "ProtectIfProfit" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.ProfitProtectPercentage , F , "ProfitProtectPercentage" , bSecurityClass );

	Add( &cfg::Get( ).Betting.security.WaitIfLosing , B , "WaitIfLosing" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.ResetCountIfWin , B , "ResetCountIfWin" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.WaitAgainIfLose , B , "WaitAgainIfLose" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.WaitAfterXLose , I , "WaitAfterXLose" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.WaitAmount , I , "WaitAmount" , bSecurityClass );


	Add( &cfg::Get( ).Betting.security.ProtectCapital , B , "ProtectCapital" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.PredictDownPeaks , B , "PredictDownPeaks" , bSecurityClass );
	Add( &cfg::Get( ).Betting.security.CapitalProtectPercentage , F , "CapitalProtectPercentage" , bSecurityClass );

	std::vector<std::string> bAutomaticClass = { BETTING_CLASS, "Automatic" };

	Add( &cfg::Get( ).Betting.automatic.SimulateBet , B , "SimulateBet" , bAutomaticClass );
	Add( &cfg::Get( ).Betting.automatic.AutoBet , B , "AutoBet" , bAutomaticClass );

	Add( &cfg::Get( ).Betting.automatic.RedPoint , P , "RedPoint" , bAutomaticClass );
	Add( &cfg::Get( ).Betting.automatic.BlackPoint , P , "BlackPoint" , bAutomaticClass );
	Add( &cfg::Get( ).Betting.automatic.WhitePoint , P , "WhitePoint" , bAutomaticClass );
	Add( &cfg::Get( ).Betting.automatic.InputPoint , P , "InputPoint" , bAutomaticClass );
	Add( &cfg::Get( ).Betting.automatic.BetPoint , P , "BetPoint" , bAutomaticClass );


	for ( int i = 0; i < 2; i++ ) {

		std::string Type = "";

		switch ( i )
		{
		case 0:
			Type = "LOSE";
			break;
		case 1:
			Type = "WIN";
			break;
		}

		std::vector<std::string> bTypeClass = { BETTING_CLASS, Type };

		Add( &cfg::Get( ).Betting.type[i].MultiplyBet , B , "MultiplyBet" , bTypeClass );
		Add( &cfg::Get( ).Betting.type[ i ].IncrementMinimum , B , "IncrementMinimum" , bTypeClass );
		Add( &cfg::Get( ).Betting.type[ i ].ResetIfDifferent , B , "ResetIfDifferent" , bTypeClass );
		Add( &cfg::Get( ).Betting.type[ i ].BetMultiplier , F , "BetMultiplier" , bTypeClass );
		Add( &cfg::Get( ).Betting.type[ i ].MultiplyAfterX ,I , "MultiplyAfterX" , bTypeClass );
		Add( &cfg::Get( ).Betting.type[ i ].MaxMultiplierTimes , I , "MaxMultiplierTimes" , bTypeClass );

	}

	std::vector<std::string> bPredictingClass = { PREDICTING_CLASS };


	Add( &cfg::Get( ).Prediction.MinPatterSize, I , "MinPatterSize" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.IgnoreStreakAfter , I , "IgnoreStreakAfter" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.StreakBettingSpacing , I , "StreakBettingSpacing" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.MinimumPercentage , F , "MinimumPercentage" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.MaximumPercentage , F , "MaximumPercentage" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.InvertIfMissing , B , "InvertIfMissing" , bPredictingClass );
	Add( &cfg::Get( ).Prediction.InvertIfMissingAbove , F , "InvertIfMissingAbove" , bPredictingClass );


	std::vector<std::string> bGameClass = { GAME_CLASS };

	Add( &cfg::Get( ).Game.BalanceHistory , V , "BalanceHistory" , bGameClass );
	Add( &cfg::Get( ).Game.FullBalanceHistory , V , "FullBalanceHistory" , bGameClass );
	Add( &cfg::Get( ).Game.InitialBalance , F, "InitialBalance" , bGameClass );
	Add( &cfg::Get( ).Game.CurrentBalance , F , "CurrentBalance" , bGameClass );

	//////////////////////// aim ///////////////

	//Add( &cfg::Get( ).aim_assist.Enable , B , "Enable" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.pSilent , B , "pSilent" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.pSilentHitbox , I , "pSilentHitbox" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.pSilentFOV , I , "pSilentFOV" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.pSilentMinBulletDistance , I , "pSilentMinBulletDistance" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.pSilentHitchance , I , "pSilentHitchance" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.DrawpSilentFov , B , "DrawpSilentFov" , bAimClass );
	//
	//Add( &cfg::Get( ).aim_assist.Aimbot , B , "Aimbot" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.AimbotHitbox , I , "AimbotHitbox" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.AimbotFOV , I , "AimbotFOV" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.DrawAimbotFov , B , "DrawAimbotFov" , bAimClass );
	//
	//
	//Add( &cfg::Get( ).aim_assist.DynamicFov , B , "DynamicFov" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.Hitmarker , B , "Hitmarker" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.HitmarkerSize , F , "HitmarkerSize" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.MarkerThickness , I , "MarkerThickness" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.MarkerStyle , I , "MarkerStyle" , bAimClass );
	//Add( &cfg::Get( ).aim_assist.MarkerSpeed , F , "MarkerSpeed" , bAimClass );
	//
	//Add( &cfg::Get( ).aim_assist.Friends , B , "Friends" , bAimClass );
	//
	//AddColor( &cfg::Get( ).aim_assist.FriendsColor , C , "FriendsColor" , bAimClass );
	//AddColor( &cfg::Get( ).aim_assist.HitmarkerColor , C , "HitmarkerColor" , bAimClass );
	//AddColor( &cfg::Get( ).aim_assist.pSilentColor , C , "pSilentColor" , bAimClass );
	//AddColor( &cfg::Get( ).aim_assist.AimbotColor , C , "AimbotColor" , bAimClass );
	//
	/////////////////////////////////////////////////
	//
	//std::vector<std::string> bEspClass = { "esp" };
	//
	//Add( &cfg::Get( ).esp.Enable , B , "Enable" , bEspClass );

	/////////////Entities///////////////////////////
}



template<typename T>
T * GetVarValue( VARIABLE & var ) {

	switch ( var ) {
	case B:
		return var.GetAdressAsBool( );
	case I:
		return var.GetAdressAsInt( );
	case F:
		return var.GetAdressAsFloat( );
	}
}

void SetVars( json * js ) {

	for ( auto & var : variables )
	{
		Load( var , js );
	}
}


void SaveVars( json * js )
{
	for ( auto & var : variables )
	{
		Save( &var , js );
	}
}





std::string GetPath( )
{
	return "C:\\Blaze\\";
}

#include <filesystem>
namespace fs = std::filesystem;

std::vector<std::string> GetFilesInPath( )
{
	std::vector<std::string> files;

	for ( const auto & entry : fs::directory_iterator( GetPath( ) ) )
	{
		files.emplace_back( entry.path( ).filename( ).string( ) );
	}

	return files;
}

void CreateConfig( std::string name )
{
	auto Folder = GetPath( );

	if ( Folder.empty( ) )
		return;

	std::string path = Folder + name + ".json";
	std::ofstream outfile( path.c_str( ) );
	outfile.close( );
}

void RemoveConfig( std::string name )
{
	auto Folder = GetPath( );

	if ( Folder.empty( ) )
		return;

	std::remove( std::string( Folder + name ).c_str( ) );
}

bool DoOnce = false;

bool SaveCFG( std::string name )
{
	auto Folder = GetPath( );

	if ( Folder.empty( ) )
		return false;

	_mkdir( Folder.c_str( ) );

	json cfgjson;


	if ( !DoOnce )
	{
		DoOnce = true;
		AddVars( );
	}

	SaveVars( &cfgjson );

	std::string path = Folder + name;
	std::ofstream outfile( path.c_str( ) );
	outfile << cfgjson;
	outfile.close( );

	return true;
}

bool LoadCfg( std::string name )
{
	auto Folder = GetPath( );

	if ( Folder.empty( ) )
		return false;

	_mkdir( Folder.c_str( ) );

	std::string CFGPath = Folder + name;

	json cfgjson;

	std::ifstream outfile( CFGPath.c_str( ) );
	outfile >> cfgjson;
	outfile.close( );

	if ( !DoOnce )
	{
		DoOnce = true;
		AddVars( );
	}

	SetVars( &cfgjson );

	return true;
}