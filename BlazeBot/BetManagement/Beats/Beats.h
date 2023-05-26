#ifndef BEATS_H
#define BEATS_H

#include <vector>

class Beats {
    int Hits = 0;
    int Misses = 0;
    int MaxRollLoseAmount = 0;
    int CurrentRollLose;
    bool WasWinning = false;
    bool WasLoosing = false;
    std::vector<int> RollLoses;
    std::vector<int> Results;
    bool BadTrip = false;
    int Method;

public:
    Beats(  int Method );
    Beats(  );

    void PrintBetLose( int prediction );
    void SetupBeat( int ColorPrediction , int TrueColor );
    int DistanceBetweenRolls( );
    bool StableMoment( );
    bool OnBadTrip( );
    bool OnBetPattern( int startpos = 1);

    std::vector<int> GetRollLoses( );
    int GetRollLosesAmount( );
    int GetMaximumRollLoseAmount( );
    std::vector<int> GetResults( );
    int GetMediumRollLoseAmount( );
    int GetHits( );
    int GetMisses( );
    int GetHitsPercentage( );
    int GetTotal( );
    int GetMissesPercentage( );
};

#endif // BEATS_H