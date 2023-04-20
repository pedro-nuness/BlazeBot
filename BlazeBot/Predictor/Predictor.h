#ifndef DOUBLEPREDICTOR_H
#define DOUBLEPREDICTOR_H

#include <vector>
#include <unordered_map>
#include <map>
#include <random>
#include <stdexcept>
#include <cmath>

#include "..\Colors\Colors.h"

namespace std {
    template <>
    struct hash<Color> {
        size_t operator()( const Color & c ) const {
            return static_cast< size_t >( c );
        }
    };

    template <>
    struct hash<std::pair<Color , Color>> {
        size_t operator()( const std::pair<Color , Color> & p ) const {
            return static_cast< size_t >( p.first ) ^ static_cast< size_t >( p.second );
        }
    };

    template <>
    struct equal_to<std::pair<Color , Color>> {
        bool operator()( const std::pair<Color , Color> & lhs , const std::pair<Color , Color> & rhs ) const {
            return lhs.first == rhs.first && lhs.second == rhs.second;
        }
    };
}

class Transition {
   
public:
    std::vector<Color> Colors;
    double Weight = 0.0;
};

class Prediction {
public:
    Color color;
    float chance;
};

class Streak {
public:
    Color color;
    int StreakSize;
};


class DoublePredictor {
private:
    std::vector<Color> history;
    std::unordered_map<Color , int> counts;
    std::unordered_map<std::pair<Color , Color> , int> transitions;
    std::map<std::pair<Color , std::pair<Color , Color>> , double> transitions_three;

    double decay_factor = 0.9;
    double laplace_constant = 1.0;
    double red_chance = 0.46;
    double blue_chance = 0.46;
    double white_chance = 0.06;

public:
    int Beats;
    int Misses;
    Color LastColor;
  

    DoublePredictor( );

    double get_next_color_probability( Color color );

    void AddHistory( Color col );
    std::vector<Color> GetHistory();

    void clearResults( );
    int Total( );
    void addColor( Color c );
    std::vector<double> crossValidate( int k = 5 );
    double getCertainty( Color c);
    double getCertaintyRecoded( Color c );
    std::vector<Transition> GetTransitions( int size );
    double GetTransitionWeight( std::vector<Transition> Transitions , int position );
    Color getNextColorWithPreference( Color next_color );
    Prediction predictNext( );
    Streak isStreak( );
    std::pair<int , double> getStreakProbability( Color c, double bayes);
    void updateTransitionsThree( );
    Transition GetGameTransition( );
};

#endif // DOUBLEPREDICTOR_H