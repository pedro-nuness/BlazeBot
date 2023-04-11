#include "ColorManagement.h"

ColorManagement::ColorManagement( Color color , bool won ) {
    this->ColorGuessed = color;
    this->Won = won;
}

bool ColorManagement::getWon( ) const {
    return this->Won;
}

Color ColorManagement::getColor( ) const {
    return this->ColorGuessed;
}