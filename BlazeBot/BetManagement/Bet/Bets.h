#ifndef BET_H
#define BET_H

#include "../../Predictor\Predictor.h"
#include <string>

#include "../../json.hpp"

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
	LOSE ,
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

	void DoBet( float value , float white_bet , float currency );
	bool DidBet( );
	float GetBetAmount( );
	Prediction GetPrediction( );
	float GetWhiteBet( );
	void SetCorrect( int correct );
	void SetMethod( int method );
	int GetBetResult( );
	int GetMethod( );
	Color GetColor( );
	void SetColor( Color c );
	double GetChance( );
	PREDICTOR_METHOD GetPredictionMethod( );
	bool GotWhite = false;
};

#endif // BET_H
