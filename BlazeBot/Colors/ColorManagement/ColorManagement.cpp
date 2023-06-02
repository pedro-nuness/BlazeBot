#include "Colormanagement.h"

ColorManagement::ColorManagement( ) {
    this->ID = "";
    this->Creation = "";
    this->ServerSeed = "";
    this->IntColor = -1;
    this->Roll = -1;
    this->Setupped = false;
}

ColorManagement::ColorManagement( json col ) {
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

std::string ColorManagement::GetID( ) {
    return this->ID;
}

std::string ColorManagement::GetCreation( ) {
    return this->Creation;
}

std::string ColorManagement::GetServerID( ) {
    return this->ServerSeed;
}

Color ColorManagement::GetColor( ) {
    return static_cast< Color >( this->IntColor );
}

int ColorManagement::GetRoll( ) {
    return this->Roll;
}

json ColorManagement::GetJson( ) {
    return this->FullJson;
}

bool ColorManagement::IsSetupped( ) {
    return this->Setupped;
}
