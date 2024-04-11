#ifndef PLAYER_H
#define PLAYER_H

#include "..\Bet\Bets.h"
#include <vector>



class Player {
    float StartMoney = -1;
    float DepositedMoney = -1;
    float CurrentMoney = -1;
    float PeakMoney = -1;
    float LowestMoney = -1;

public:
    bool IsPeakSettuped = false;

    Player( float money );
    std::vector<float> BalanceHistory;
    std::vector<float> FullBalanceHistory;
    std::vector<Bet> Bets;
    std::vector<Bet> FullBets;
    void SettupPeak( std::vector<float> historybalance );
    void FinishBets(float new_money );
    float GetProfit( );
    float GetPeakBalance( );
    float GetBalance( );
    float GetDepositedMoney( );
    float GetInitialMoney( );
    float GetLowestMoney( );
    void IncreaseBalance( float value );
    void DecreaseBalance( float value );
    void SetBalance( float balance );
    void Reset( );
    bool IsSetupped( );
};

#endif // PLAYER_H