#include "Player.h"

Player::Player( float money ) {
	this->CurrentMoney = money;
	this->StartMoney = money;
	this->PeakMoney = money;
	this->DepositedMoney = money;
}

void Player::SettupPeak( std::vector<float> historybalance ) {
	for ( auto money : historybalance ) {
		if ( money > PeakMoney )
			PeakMoney = money;
	}

	IsPeakSettuped = true;
}

float Player::GetProfit( ) {
	return CurrentMoney - StartMoney;
}

float Player::GetPeakBalance( ) {
	return this->PeakMoney;
}

float Player::GetDepositedMoney( ) {
	return this->DepositedMoney;
}

float Player::GetBalance( ) {
	return this->CurrentMoney;
}

float Player::GetInitialMoney( ) {
	return this->StartMoney;
}

void Player::IncreaseBalance( float value ) {
	this->CurrentMoney = this->CurrentMoney + value;
	if ( this->CurrentMoney > PeakMoney )
		PeakMoney = this->CurrentMoney;

	BalanceHistory.emplace_back( this->CurrentMoney );
	if ( !FullBalanceHistory.empty( ) )
		FullBalanceHistory.emplace_back( FullBalanceHistory[ FullBalanceHistory.size( ) - 1 ] + value );
	else
		FullBalanceHistory.emplace_back( value );
}

void Player::FinishBets( float new_money )
{
	this->CurrentMoney = new_money;
	this->StartMoney = new_money;
	this->PeakMoney = new_money;
}

void Player::DecreaseBalance( float value ) {
	this->CurrentMoney = this->CurrentMoney - value;
	if ( this->CurrentMoney < this->LowestMoney )
		this->LowestMoney = CurrentMoney;

	BalanceHistory.emplace_back( this->CurrentMoney );
	if ( !FullBalanceHistory.empty( ) )
		FullBalanceHistory.emplace_back( FullBalanceHistory[ FullBalanceHistory.size( ) - 1 ] - value );
	else
		FullBalanceHistory.emplace_back(  -value );
}

float Player::GetLowestMoney( ) {
	return this->LowestMoney;
}

void Player::SetBalance( float balance ) {
	this->CurrentMoney = balance;
	if ( this->CurrentMoney > PeakMoney )
		PeakMoney = this->CurrentMoney;
}

void Player::Reset( ) {
	this->CurrentMoney = this->StartMoney;
	this->PeakMoney = CurrentMoney;
	this->BalanceHistory.clear( );
	this->Bets.clear( );
}

bool Player::IsSetupped( ) {
	return this->CurrentMoney != -1;
}