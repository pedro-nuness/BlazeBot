#include "Bets.h"

void Bet::DoBet( float value , float white_bet , float currency ) {
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

bool Bet::DidBet( ) {
	return ( bool ) this->BetValue;
}

float Bet::GetBetAmount( ) {
	return this->BetValue;
}

Prediction Bet::GetPrediction( ) {
	return this->prediction;
}

float Bet::GetWhiteBet( ) {
	return this->WhiteBet;
}

void Bet::SetCorrect( int correct ) {
	this->Correct = correct;
}

void Bet::SetMethod( int method ) {
	this->Method = method;
}

int Bet::GetBetResult( ) {
	return this->Correct;
}

int Bet::GetMethod( ) {
	return this->Method;
}

Color Bet::GetColor( ) {
	return this->prediction.color;
}

void Bet::SetColor( Color c ) {
	this->prediction.color = c;
}

double Bet::GetChance( ) {
	return this->prediction.chance;
}

PREDICTOR_METHOD Bet::GetPredictionMethod( ) {
	return this->prediction.method;
}
