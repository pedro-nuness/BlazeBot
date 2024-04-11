#ifndef BETMANAGER_H
#define BETMANAGER_H

#include "Bet/Bets.h"
#include "Player/Player.h"

#include <string>
#include <vector>
#include "../json.hpp"

#include <Windows.h>

using json = nlohmann::json;

class Waiting {
public:
	bool Wait = false;
	int WaitStartPos = 0;
	int WaitAmount = 0;
};


class BetManager;
struct NextBetManagement {
	int MultiplierLoseCount = 0;
	int MultiplierWinCount = 0;

	void Reset( ) {
		MultiplierLoseCount = 0;
		MultiplierWinCount = 0;
	}
};

class BetManager {
private:
	DoublePredictor * predictor;
	std::string filename;

	int reds;
	int blues;
	int whites;

	int Corrects;
	int Wrongs;

	int SafeCorrects;
	int SafeWrongs;

	Bet CurrentBet;
	Bet LastBet;

	bool GameMode;

	POINT RedPos = { 0, 0 };
	POINT BlackPos = { 0, 0 };
	POINT WhitePos = { 0, 0 };
	POINT InputPos = { 0, 0 };
	POINT StartBetPos = { 0,0 };

	void BetOnColor( Color c , float amount );
	int RequiredPlays = 0;
	void EndBet( bool Leave = false );

	Bet Martingalle( Prediction predict , Player player , NextBetManagement * Control );
	void EndRawBet( );
public:
	bool StartBets = false;
	BetManager( const std::string & filename , DoublePredictor * predictor );
	std::chrono::high_resolution_clock::time_point StartingBetTime;
	std::chrono::high_resolution_clock::time_point EndingBetTime;
	std::chrono::high_resolution_clock::time_point StartWaitingTime;
	std::chrono::high_resolution_clock::time_point LastColorAddTime;

	float BankProfit = -1000.f;
	Player CurrentPlayer = Player( -1 );
	Player RawPlayer = Player( -1 );

	std::vector<Beats> SeparatedBeats { Beats( ) };

	Bet GetCurrentPrediction( );
	Bet GetLastPrediction( );
	Bet PredictBets( Prediction predict , Bet * RawBet = nullptr );
	int GetCurrentLoseStreak( std::vector<Bet> bet );
	Bet PredictRawBets( Prediction predict );

	DoublePredictor * GetPredictor( );

	void SetRedPosition( POINT pos );
	void SetWhitePosition( POINT pos );
	void SetBlackPosition( POINT pos );
	void SetInputPosition( POINT pos );
	void SetStartPosition( POINT pos );

	POINT GetRedPosition( );
	POINT GetWhitePosition( );
	POINT GetBlackPosition( );
	POINT GetInputPosition( );
	POINT GetStartPosition( );

	void ResetWaitControler( );

	Waiting WaitController;
	bool OnGameMode( bool * set = nullptr );
	bool NeedToWait( );
	void setupData( );
	void ManageBets( );
	void SimulateGame( std::vector<ColorManagement> GameHistory , std::vector<ColorManagement> Data );
	void SetupBet( Bet bet , Player * player , ColorManagement NextResult );
	void StartGameSimulation( BetManager TrueBetManager );
	void addColor( json play , bool write = true );
	void ClearData( );
	void ClearRawData( );
	void SetCurrentPrediction( Bet bet );
	void DoBet( );

	Color nextBet( Color prediction , float predictionChance , double * success );

	static std::vector<json> blazeAPI( );
	std::vector<Bet> & getBets( );
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
