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
    ColorManagement( ) {
        this->ID = "";
        this->Creation = "";
        this->ServerSeed = "";
        this->IntColor = -1;
        this->Roll = -1;
        this->Setupped = false;
    }

    ColorManagement( json col  )
    {
        this->FullJson = col;

        if ( !col[ "id" ].empty( ) )
            this->ID = col[ "id" ];

        if ( !col[ "created_at" ].empty( ) )
            this->Creation = col[ "created_at" ];

        if ( !col[ "color" ].empty( ) )
            this->IntColor = col[ "color" ];

        if ( !col[ "roll" ].empty( ) )
            this->Roll = col[ "roll" ];

        if ( !col[ "server_seed" ].empty( ) )
            this->ServerSeed = col[ "server_seed" ];      

        this->Setupped = true;
    }

    std::string GetID( ) {
        return this->ID;
    }

    std::string GetCreation( ) {
        return this->Creation;
    }
    std::string GetServerID( ) {
        return this->ServerSeed;
    }

    Color GetColor( ) {
        return ( Color ) this->IntColor;
    }

    int GetRoll( ) {
        return this->Roll;
    }

    json GetJson( ) {
        return this->FullJson;
    }

    bool IsSetupped( ) {
        return this->Setupped;
    }
      
};

#endif // COLORMANAGEMENT_H