#include "Utils.h"
#include <Windows.h>
#include <iostream>

void Utils::ColoredText( std::string text , CMD_COLORS color )
{
    HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
    SetConsoleTextAttribute( hConsole , color );
    std::cout << text;
    SetConsoleTextAttribute( hConsole , WHITE );
}