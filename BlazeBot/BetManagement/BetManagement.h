#ifndef BETMANAGER_H
#define BETMANAGER_H

#include "..\Predictor\Predictor.h"
#include "..\Colors\ColorManagement\ColorManagement.h"

#include <string>
#include <vector>
#include "../json.hpp"

using json = nlohmann::json;

class Bet {

    Color color;
    double chance;

public:
    Bet( )
    {
        this->color = Null;
        this->chance = 0;
    }

    Bet( Color color, double chance)
    {
        this->color = color;
        this->chance = chance;
    }

    Color GetColor( ) {
        return this->color;
    }

    double GetChance( ) {
        return this->chance;
    }

};

class BetManager {
private:
    DoublePredictor* predictor;
    std::vector<ColorManagement> bets;
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

public:

    BetManager( const std::string & filename , DoublePredictor * predictor );

    Bet GetCurrentPrediction( );
    Bet GetLastPrediction( );

    void PredictBets( );

    int Consecutive( Color c ) {

    }

    bool OnGameMode( bool * set = nullptr );
    void setupData( );
    void addColor( int color , bool write = true);   
    void ClearData( );
    void SetCurrentPrediction( Color col, double chance );

    Color nextBet( Color prediction , float predictionChance , double * success );

    static std::vector<json> blazeAPI( );
    const std::vector<ColorManagement> & getBets( ) const;


    int getReds( ) const;
    int getBlues( ) const;
    int getWhites( ) const;
    int getCorrects( ) const;
    int getWrongs( ) const;
    int getSafeCorrects( ) const;
    int getSafeWrongs( ) const;
};

#endif // BETMANAGER_H