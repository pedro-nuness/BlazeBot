#ifndef UTILS_H
#define UTILS_H

#include <string>
#include "Singleton.h"
#include "..\BetManagement\BetManagement.h"

enum CMD_COLORS
{
    DARK_BLUE = 1 ,
    GREEN ,
    BLUE ,
    RED ,
    PURPLE ,
    YELLOW ,
    WHITE ,
    GRAY ,
    LIGHT_BLUE ,
    LIGHT_GREEN ,
    LIGHTER_BLUE ,
    LIGHT_RED ,
    PINK ,
    LIGHT_YELLOW ,
    LIGHT_WHITE
};

namespace maths {

    template<typename T>
    T Sqr( T number ) {

        return number * number;
    }

    template<typename T>
    T Delta( T a, T b ) {

        return a - b;
    }

    template<typename T>
    T fDelta( T a , T b ) {

        return fabs(a - b);
    }

    template <typename T>
    T nClamp( T Value , T min , T max )
    {
        if ( Value > max )
            return max;
        else if ( Value < min )
            return min;

        return Value;
    }

}

class Utils : public CSingleton<Utils>
{
public:
    static void ColoredText( std::string text , CMD_COLORS color );
    void WriteData( std::string file , std::string data, bool DeleteContent );
    int aproximaFloat( float numero );
    std::vector<ColorManagement> GetNodeOutput( std::string seed , int amount );
};


#endif // UTILS_H