#include "Player.h"

Player::Player( float money ) {
    this->CurrentMoney = money;
    this->StartMoney = money;
    this->PeakMoney = money;
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
}

void Player::DecreaseBalance( float value ) {
    this->CurrentMoney = this->CurrentMoney - value;
}

void Player::SetBalance( float balance ) {
    this->CurrentMoney = balance;
    if ( this->CurrentMoney > PeakMoney )
        PeakMoney = this->CurrentMoney;
}

void Player::Reset( ) {
    this->CurrentMoney = this->StartMoney;
    this->PeakMoney = CurrentMoney;
}

bool Player::IsSetupped( ) {
    return this->CurrentMoney != -1;
}