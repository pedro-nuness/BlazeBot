#ifndef PLAYER_H
#define PLAYER_H

#include <vector>

class Player {
    float StartMoney = -1;
    float CurrentMoney = -1;
    float PeakMoney = -1;
    float LowestMoney = -1;

public:
    bool IsPeakSettuped = false;

    Player( float money );

    void SettupPeak( std::vector<float> historybalance );
    float GetProfit( );
    float GetPeakBalance( );
    float GetBalance( );
    float GetInitialMoney( );
    float GetLowestMoney( );
    void IncreaseBalance( float value );
    void DecreaseBalance( float value );
    void SetBalance( float balance );
    void Reset( );
    bool IsSetupped( );
};

#endif // PLAYER_H