#ifndef BETMANAGER_H
#define BETMANAGER_H

#include "..\Predictor\Predictor.h"
#include "..\Colors\ColorManagement\ColorManagement.h"

#include <string>
#include <vector>
#include "../json.hpp"

using json = nlohmann::json;

enum METHODS
{
    MARTINGALE,
    FIBONACCI,
    MATH
};

enum BETRESULT
{
    PREDICTION,
    WON,
    LOSE
};


class Bet {

    Color color;
    double chance = -1;
    int Correct = PREDICTION;
    int Method = -1;
    float BetValue = 0;

public:
    Bet( )
    {
        this->color = Null;
        this->chance = 0;
    }

    Bet( Color color , double chance, int correct, int method )
    {
        this->color = color;
        this->chance = chance;
        this->Correct = correct;
        this->Method = method;
    }

    void DoBet( float value, float currency) {
        if ( value <= currency )
            this->BetValue = value;
        else 
            this->BetValue = currency;
    }

    bool DidBet( ) {
        return (bool)this->BetValue;
    }

    float GetBetAmount( ) {
        return this->BetValue;
    }


    void SetCorrect(int correct ) {
        this->Correct = correct;
    }

    void SetMethod( int method  ) {
        this->Method = method;
    }

    int GetBetResult( ) {
        return this->Correct;
    }

    int GetMethod( ) {
        return this->Method;
    }

    Color GetColor( ) {
        return this->color;
    }

    double GetChance( ) {
        return this->chance;
    }

};

class Player {
    float StartMoney = -1;
    float CurrentMoney = -1;
public:
    Player( float money ) {
        this->CurrentMoney = money;
        this->StartMoney = money;
    } //Settup money

    float GetProfit( ) {
        return CurrentMoney - StartMoney;
    }

    float GetBalance( ) { return this->CurrentMoney; }
    float GetInitialMoney( ) { return this->StartMoney; }
    void IncreaseBalance( float value ) { this->CurrentMoney = this->CurrentMoney + value; }
    void DecreaseBalance( float value ) { this->CurrentMoney = this->CurrentMoney - value; }

    bool IsSetupped( ) {
        return this->CurrentMoney != -1;
    }

};

class BetManager {
private:
    DoublePredictor* predictor;
    std::vector<Bet> bets;
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

    Player CurrentPlayer = Player(-1);

public:

    BetManager( const std::string & filename , DoublePredictor * predictor );

    Bet GetCurrentPrediction( );
    Bet GetLastPrediction( );

    Bet PredictBets( Color c, double chance, float current_balance );

    int Consecutive( Color c ) {

    }

    bool OnGameMode( bool * set = nullptr );
    void setupData( );
    void addColor( json play , bool write = true);   
    void ClearData( );
    void SetCurrentPrediction( Bet bet );

    Color nextBet( Color prediction , float predictionChance , double * success );

    static std::vector<json> blazeAPI( );
    const std::vector<Bet> & getBets( ) const;

    int getReds( ) const;
    int getBlues( ) const;
    int getWhites( ) const;
    int getCorrects( ) const;
    int getWrongs( ) const;
    int getSafeCorrects( ) const;
    int getSafeWrongs( ) const;
};

#endif // BETMANAGER_H