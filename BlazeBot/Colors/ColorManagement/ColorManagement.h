#ifndef COLORMANAGEMENT_H
#define COLORMANAGEMENT_H

#include "..\Colors.h"


class ColorManagement {
private:
    Color ColorGuessed;
    bool Won;

public:
    ColorManagement( Color color , bool won );
    bool getWon( ) const;
    Color getColor( ) const;
};

#endif // COLORMANAGEMENT_H