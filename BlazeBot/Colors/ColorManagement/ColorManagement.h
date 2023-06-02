#ifndef COLORMANAGEMENT_H
#define COLORMANAGEMENT_H

#include "..\Colors.h"
#include <string>

#include "..\..\json.hpp"

using json = nlohmann::json;

class ColorManagement {
private:
    std::string ID;
    std::string Creation;
    std::string ServerSeed;
    int IntColor;
    int Roll;
    json FullJson;
    bool Setupped;

public:
    ColorManagement( );
    ColorManagement( json col );

    std::string GetID( );
    std::string GetCreation( );
    std::string GetServerID( );
    Color GetColor( );
    int GetRoll( );
    json GetJson( );
    bool IsSetupped( );
};

#endif // COLORMANAGEMENT_H
