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

class Utils : public CSingleton<Utils>
{
public:
    static void ColoredText( std::string text , CMD_COLORS color );

    void WriteData( std::string file , std::string data, bool DeleteContent );
    int aproximaFloat( float numero );
    std::vector<ColorManagement> GetNodeOutput( std::string seed , int amount );

};


#endif // UTILS_H