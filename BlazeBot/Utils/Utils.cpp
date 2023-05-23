#include "Utils.h"
#include <Windows.h>
#include <iostream>
#include <fstream>

void Utils::ColoredText( std::string text , CMD_COLORS color )
{
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	SetConsoleTextAttribute( hConsole , color );
	std::cout << text;
	SetConsoleTextAttribute( hConsole , WHITE );
}

void Utils::WriteData( std::string file , std::string data , bool DeleteContent ) {
	std::string dir = "C:\\Blaze\\" + file;
	
	if ( !DeleteContent )
	{
		std::ofstream arquivo( dir , std::ios::app );

		// Verificar se o arquivo foi aberto corretamente
		if ( !arquivo.is_open( ) ) {
			std::cout << "Erro ao abrir o arquivo" << std::endl;
			return;
		}

		// Adicionar texto ao arquivo
		arquivo << data << "\n";

		// Fechar o arquivo
		arquivo.close( );

	}
	else {
		std::ofstream arquivo( dir , std::ios::trunc );

		// Verificar se o arquivo foi aberto corretamente
		if ( !arquivo.is_open( ) ) {
			std::cout << "Erro ao abrir o arquivo" << std::endl;
			return;
		}

		// Adicionar texto ao arquivo
		arquivo << data << "\n";

		// Fechar o arquivo
		arquivo.close( );
	}

}