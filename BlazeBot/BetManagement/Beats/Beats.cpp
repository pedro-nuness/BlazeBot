#include "..\..\Utils\Config\Config.h"
#include "..\BetManagement.h"

#include "..\..\Utils\Utils.h"

Beats::Beats( int Method ) {
	this->Hits = 0;
	this->Misses = 0;
	this->MaxRollLoseAmount = 0;
	this->CurrentRollLose = 0;
	this->WasLoosing = false;
	this->WasWinning = false;
	RollLoses.clear( );
	this->Method = Method;
}

Beats::Beats( ) {
	this->Hits = 0;
	this->Misses = 0;
	this->MaxRollLoseAmount = 0;
	this->CurrentRollLose = 0;
	this->WasLoosing = false;
	this->WasWinning = false;
	RollLoses.clear( );
	this->Method = -1;
}


std::vector<int> Beats::GetRollLoses( ) {
	return this->RollLoses;
}

int Beats::GetRollLosesAmount( ) {
	return RollLoses.size( );
}

int Beats::GetMaximumRollLoseAmount( ) {
	return MaxRollLoseAmount;
}

std::vector<int> Beats::GetResults( ) {
	return this->Results;
}

int Beats::GetMediumRollLoseAmount( ) {
	float Sum = 0.f;

	if ( !RollLoses.empty( ) ) {
		for ( auto roll : RollLoses ) {
			Sum += roll;
		}

		Sum /= RollLoses.size( );
	}

	return int( Sum );
}

void Beats::PrintBetLose( int prediction ) {

	std::vector < Color > CurrentGame;




}

void Beats::SetupBeat( int ColorPrediction , int TrueColor )
{
	if ( ColorPrediction != Null ) {
		if ( ColorPrediction == TrueColor ) {
			WasWinning = true;
			Hits++;

			Results.emplace_back( WON );

			if ( WasLoosing ) {
				RollLoses.emplace_back( CurrentRollLose );

				if ( CurrentRollLose && CurrentRollLose <= cfg::Get( ).Betting.type[ LOSE ].MaxMultiplierTimes )
				{
					Misses -= CurrentRollLose;
				}

				CurrentRollLose = 0;
				WasLoosing = false;
			}
		}
		else {
			WasWinning = false;

			Results.emplace_back( LOSE );

			PrintBetLose( ColorPrediction );

			if ( !WasWinning )
			{
				WasLoosing = true;

				CurrentRollLose++;


				if ( CurrentRollLose > MaxRollLoseAmount )
				{
					MaxRollLoseAmount = CurrentRollLose;
				}
			}
			Misses++;
		}
	}
}

bool Beats::OnBadTrip( ) {

	int ConsecutiveLoses = 0;

	if ( !Results.empty( ) ) {

		for ( int i = 0; i < Results.size( ) - 1; i++ )
		{
			if ( Results[ i ] == LOSE )
				ConsecutiveLoses++;
			else
				break;
		}

		if ( ConsecutiveLoses > GetMediumRollLoseAmount( ) + 3 )
		{
			return true;
		}
		else {
			return false;
		}
	}

	return false;
}

template<typename T>
T Sqr( T number ) {

	return number * number;
}

bool Beats::OnBetPattern( int startpos ) {

	int max = 30;
	int count = 0;
	int Roll = 0;
	float MedRoll = 0;
	int MaxRoll = 0;

	std::vector<float> roll_loses;

	if ( Results.size( ) <= max )
		return false;

	for ( int i = Results.size( ) - 1; i > Results.size( ) - max; i-- )
	{
		if ( Results[ i ] == LOSE )
		{
			Roll++;
		}
		else if ( Roll ) {
			if ( Roll > MaxRoll )
				MaxRoll = Roll;

			roll_loses.emplace_back( Roll );

			MedRoll += Roll;
			Roll = 0;
		}

	}

	MedRoll = Utils::Get( ).aproximaFloat( MedRoll / roll_loses.size( ) );

	float variancy = 0.0f;

	for ( auto roll : roll_loses )
	{
		//Squared
		variancy += Sqr( fabs( roll - MedRoll ) );
	}

	variancy = Utils::Get( ).aproximaFloat( variancy / roll_loses.size( ) );

	Roll = 0;

	if ( MedRoll <= 2 && MaxRoll <= 4 && variancy <= 2 ) {

		for ( int i = Results.size( ) - 1; i > 0; i-- )
		{
			if ( Results[ i ] == LOSE )
			{
				Roll++;
			}
			else
				break;
		}

		if ( Roll > MedRoll )
			return true;
	}
	return false;
}

bool Beats::StableMoment( ) {

	int max = 30;
	int count = 0;
	int Roll = 0;
	float MedRoll = 0;
	int MaxRoll = 0;

	std::vector<float> roll_loses;

	if ( Results.size( ) <= max )
		return false;

	for ( int i = Results.size( ) - 1; i > Results.size( ) - max; i-- )
	{
		if ( Results[ i ] == LOSE )
		{
			Roll++;
		}
		else if ( Roll ) {
			if ( Roll > MaxRoll )
				MaxRoll = Roll;

			roll_loses.emplace_back( Roll );

			MedRoll += Roll;
			Roll = 0;
		}

	}

	MedRoll = Utils::Get( ).aproximaFloat( MedRoll / roll_loses.size( ) );

	float variancy = 0.0f;

	for ( auto roll : roll_loses )
	{
		//Squared
		variancy += Sqr( fabs( roll - MedRoll ) );
	}

	variancy = Utils::Get( ).aproximaFloat( variancy / roll_loses.size( ) );

	Roll = 0;

	if ( MedRoll <= 2 && MaxRoll <= 4 && variancy <= 2 )
		return true;

	return false;
}



int Beats::DistanceBetweenRolls( ) {

	float sum = 0;

	if ( !Results.empty( ) ) {

		std::vector<int> LosesPosistions;
		int LoseStreak = 0;
		int medium_roll = GetMediumRollLoseAmount( );

		for ( int i = 1; i < Results.size( ) - 1; i++ )
		{
			if ( Results[ i ] == LOSE ) {
				LoseStreak++;

				if ( LoseStreak > medium_roll )
				{
					LosesPosistions.emplace_back( i );
				}
			}
			else {
				LoseStreak = 0;
			}
		}

		if ( !LosesPosistions.empty( ) ) {

			for ( int i = 1; i < LosesPosistions.size( ) - 1; i++ )
			{
				float Diffence = fabs( LosesPosistions[ i ] - LosesPosistions[ i - 1 ] );
				sum += Diffence;
			}

			sum /= LosesPosistions.size( );
		}
	}

	return ( int ) sum;
}

int Beats::GetHits( ) {
	return this->Hits;
}

int Beats::GetMisses( ) {
	return this->Misses;
}

int Beats::GetHitsPercentage( ) {
	float Percentage = 0.0;

	if ( ( Hits + Misses ) > 0 ) {
		float Hit = Hits;
		float Miss = Misses;
		float Total = Hit + Miss;
		Percentage = ( Hit / Total ) * 100;
	}

	return int( Percentage );
}

int Beats::GetTotal( ) {
	return Hits + Misses;
}

int Beats::GetMissesPercentage( ) {
	float Percentage = 0.0;

	if ( ( Hits + Misses ) > 0 ) {
		float Hit = Hits;
		float Miss = Misses;
		float Total = Hit + Miss;
		Percentage = ( Miss / Total ) * 100;
	}

	return int( Percentage );
}