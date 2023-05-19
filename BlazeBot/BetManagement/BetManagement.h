#ifndef BETMANAGER_H
#define BETMANAGER_H

#include "..\Predictor\Predictor.h"
#include "..\Colors\ColorManagement\ColorManagement.h"

#include <string>
#include <vector>
#include "../json.hpp"

#include <Windows.h>

using json = nlohmann::json;

enum METHODS
{
	MARTINGALE ,
	FIBONACCI ,
	MINIMUM ,
	STANDBY
};

enum BETRESULT
{
	WON ,
	LOSE,
	PREDICTION 
};




class Bet {

	Prediction prediction;
	int Correct = PREDICTION;
	int Method = -1;
	float BetValue = 0.f;
	float WhiteBet = 0.f;

public:
	Bet( )
	{
		this->prediction.color = Null;
		this->prediction.chance = 0;
		this->prediction.method = NONE;
	}

	Bet( Prediction pred , int correct , int method )
	{
		this->prediction = pred;
		this->Correct = correct;
		this->Method = method;
	}

	void DoBet( float value , float white_bet , float currency ) {
		if ( value ) {
			if ( value <= currency )
				this->BetValue = value;
			else
				this->BetValue = currency;
		}
		else
			this->BetValue = 0.0f;

		if ( white_bet ) {
			if ( white_bet <= currency )
				this->WhiteBet = white_bet;
			else
				this->WhiteBet = currency;
		}
		else
			this->WhiteBet = 0.0f;
	}

	bool DidBet( ) {
		return ( bool ) this->BetValue;
	}

	float GetBetAmount( ) {
		return this->BetValue;
	}

	Prediction GetPrediction( ) {
		return this->prediction;
	}

	float GetWhiteBet( ) {
		return this->WhiteBet;
	}


	void SetCorrect( int correct ) {
		this->Correct = correct;
	}

	void SetMethod( int method ) {
		this->Method = method;
	}

	int GetBetResult( ) {
		return this->Correct;
	}

	int GetMethod( ) {
		return this->Method;
	}

	Color GetColor( ) {
		return this->prediction.color;
	}

	void SetColor( Color c ) {
		this->prediction.color = c;
	}

	double GetChance( ) {
		return this->prediction.chance;
	}

	PREDICTOR_METHOD GetPredictionMethod( ) {
		return this->prediction.method;
	}

};

class Player {
	float StartMoney = -1;
	float CurrentMoney = -1;
	float PeakMoney = -1;
public:
	bool IsPeakSettuped = false;

	void SettupPeak( std::vector<float> historybalance )
	{
		for ( auto money : historybalance )
		{
			if ( money > PeakMoney )
				PeakMoney = money;
		}

		IsPeakSettuped = true;
	}

	Player( float money ) {
		this->CurrentMoney = money;
		this->StartMoney = money;
		this->PeakMoney = money;
	} //Settup money

	float GetProfit( ) {
		return CurrentMoney - StartMoney;
	}

	float GetPeakBalance( ) { return this->PeakMoney; }
	float GetBalance( ) { return this->CurrentMoney; }
	float GetInitialMoney( ) { return this->StartMoney; }
	void IncreaseBalance( float value ) {
		this->CurrentMoney = this->CurrentMoney + value;
		if ( this->CurrentMoney > PeakMoney )
			PeakMoney = this->CurrentMoney;
	}
	void DecreaseBalance( float value ) {
		this->CurrentMoney = this->CurrentMoney - value;

	}

	void SetBalance( float balance ) {
		this->CurrentMoney = balance;
		if ( this->CurrentMoney > PeakMoney )
			PeakMoney = this->CurrentMoney;
	}

	void Reset( ) {
		this->CurrentMoney = this->StartMoney;
		this->PeakMoney = CurrentMoney;
	}

	bool IsSetupped( ) {
		return this->CurrentMoney != -1;
	}

};

class Waiting {

public:
	bool Wait = false;
	int WaitStartPos = 0;
	int WaitAmount = 0;
};


class BetPredictor {

	DoublePredictor * Predictor;
	std::vector<Bet> * Bets;

public:
	BetPredictor( ) {
		this->Predictor = nullptr;
		this->Bets = nullptr;
	}

	BetPredictor( DoublePredictor * predictor , std::vector<Bet> * bets )
	{
		this->Predictor = predictor;
		this->Bets = bets;
	}

	METHODS ChooseBetStrategy( );


};


class BetManager;

class Beats {


	int Hits = 0;
	int Misses = 0;
	int MaxRollLoseAmount = 0;
	int CurrentRollLose;
	bool WasWinning = false;
	bool WasLoosing = false;
	std::vector<int> RollLoses;
	std::vector<int> Results;
	BetManager * BetPtr;
	int Method;


public:

	Beats( BetManager * bet_ptr , int Method ) {
		this->Hits = 0;
		this->Misses = 0;
		this->MaxRollLoseAmount = 0;
		this->CurrentRollLose = 0;
		this->WasLoosing = false;
		this->WasWinning = false;
		RollLoses.clear( );
		this->Method = Method;
		this->BetPtr = bet_ptr;
	}

	void PrintBetLose( int prediction );


	void SetupBeat( int ColorPrediction , int TrueColor );


	std::vector<int> GetRollLoses( ) {
		return this->RollLoses;
	}

	int GetRollLosesAmount( ) {
		return RollLoses.size( );
	}

	int GetMaximumRollLoseAmount( ) {
		return MaxRollLoseAmount;
	}

	int GetMediumRollLoseAmount( ) {

		float Sum = 0.f;

		if ( !RollLoses.empty( ) ) {

			for ( auto roll : RollLoses )
			{
				Sum += roll;
			}

			Sum /= RollLoses.size( );
		}

		return int( Sum );
	}

	int GetHits( ) {
		return this->Hits;
	}

	int GetMisses( ) {
		return this->Misses;
	}

	int GetHitsPercentage( ) {
		float Percentage = 0.0;

		if ( ( Hits + Misses ) > 0 ) {
			float Hit = Hits;
			float Miss = Misses;
			float Total = Hit + Miss;
			Percentage = ( Hit / Total ) * 100;
		}

		return int( Percentage );
	}

	int GetTotal( ) {
		return Hits + Misses;
	}


	int GetMissesPercentage( ) {

		float Percentage = 0.0;

		if ( ( Hits + Misses ) > 0 ) {
			float Hit = Hits;
			float Miss = Misses;
			float Total = Hit + Miss;
			Percentage = ( Miss / Total ) * 100;
		}

		return int( Percentage );
	}

};



class BetManager {
private:
	DoublePredictor * predictor;
	std::vector<Bet> bets;
	std::vector<Bet> bets_display;
	std::string filename;

	int reds;
	int blues;
	int whites;

	int Corrects;
	int Wrongs;

	int SafeCorrects;
	int SafeWrongs;

	Bet CurrentPrediction;
	Bet LastPrediction;

	bool GameMode;

	POINT RedPos = { 0, 0 };
	POINT BlackPos = { 0, 0 };
	POINT WhitePos = { 0, 0 };
	POINT InputPos = { 0, 0 };
	POINT StartBetPos = { 0,0 };
	BetPredictor BetPredicton;

public:
	bool StartBets = false;
	BetManager( const std::string & filename , DoublePredictor * predictor );

	std::chrono::high_resolution_clock::time_point StartingBetTime;
	std::chrono::high_resolution_clock::time_point EndingBetTime;
	std::chrono::high_resolution_clock::time_point StartWaitingTime;

	Player CurrentPlayer = Player( -1 );
	std::vector<float> BalanceHistory;
	std::vector<Beats> SeparatedBeats;

	Bet GetCurrentPrediction( );
	Bet GetLastPrediction( );
	Bet PredictBets( Prediction predict );


	DoublePredictor * GetPredictor( )
	{
		return this->predictor;
	}

	void SetRedPosition( POINT pos ) {
		this->RedPos = pos;
	}
	void SetWhitePosition( POINT pos ) {
		this->WhitePos = pos;
	}
	void SetBlackPosition( POINT pos ) {
		this->BlackPos = pos;
	}
	void SetInputPosition( POINT pos ) {
		this->InputPos = pos;
	}
	void SetStartPosition( POINT pos ) {
		this->StartBetPos = pos;
	}

	POINT GetRedPosition( ) {
		return this->RedPos;
	}
	POINT GetWhitePosition( ) {
		return this->WhitePos;
	}
	POINT GetBlackPosition( ) {
		return this->BlackPos;
	}
	POINT GetInputPosition( ) {
		return this->InputPos;
	}
	POINT GetStartPosition( ) {
		return this->StartBetPos;
	}

	void ResetWaitControler( ) {
		WaitController.Wait = false;
		WaitController.WaitAmount = 0;
		WaitController.WaitStartPos = 0;
	}


	Waiting WaitController;
	bool OnGameMode( bool * set = nullptr );
	bool NeedToWait( );
	void setupData( );
	void ManageBets( );
	void addColor( json play , bool write = true );
	void ClearData( );
	void SetCurrentPrediction( Bet bet );
	void DoBet( );


	Color nextBet( Color prediction , float predictionChance , double * success );

	static std::vector<json> blazeAPI( );
	std::vector<Bet> & getBets( );
	std::vector<Bet> & getBetsDisplay( );
	std::vector<ColorManagement> APIHistory;

	int getReds( ) const;
	int getBlues( ) const;
	int getWhites( ) const;
	int getCorrects( ) const;
	int getWrongs( ) const;
	int getSafeCorrects( ) const;
	int getSafeWrongs( ) const;
};

#endif // BETMANAGER_H