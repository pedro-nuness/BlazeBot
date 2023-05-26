#include "Utils.h"
#include <Windows.h>
#include <iostream>
#include <fstream>

extern bool Exist( const std::string & name );
extern std::string Folder;
extern std::string IAModel;
extern std::string JSName;
extern std::string HistoryName;

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

int Utils::aproximaFloat( float numero ) {
	if ( numero >= 0 ) {
		int parteInteira = static_cast< int >( numero );
		float parteDecimal = numero - parteInteira;

		if ( parteDecimal > 0.5 )
			return parteInteira + 1;
		else
			return parteInteira;
	}
	else {
		int parteInteira = static_cast< int >( numero );
		float parteDecimal = parteInteira - numero;

		if ( parteDecimal > 0.5 )
			return parteInteira - 1;
		else
			return parteInteira;
	}
}

std::vector<ColorManagement> Utils::GetNodeOutput( std::string seed , int amount )
{
	std::vector<ColorManagement> history;

	std::string JS = R"(
const fs = require("fs");
const crypto = require("crypto");

const TILES = [ { number: 0, color: 0 }, 
  { number: 11, color: 2 }, 
  { number: 5, color: 1 },
  { number: 10, color: 2 },
  { number: 6, color: 1 },
  { number: 9, color: 2 },
  { number: 7, color: 1 },
  { number: 8, color: 2 },
  { number: 1, color: 1 },
  { number: 14, color: 2 },
  { number: 2, color: 1 },
  { number: 13, color: 2 },
  { number: 3, color: 1 },
  { number: 12, color: 2 },
  { number: 4, color: 1 }
];
)";

	JS += "const serverSeed = ";

	JS += R"(")";
	JS += seed;
	JS += R"(")";

	JS += ";\n";
	JS += "const amount = " + std::to_string( amount ) + ";\n";

	JS += R"(
const chain = [serverSeed];
for (let i = 0; i < amount; i++) {
  chain.push(
    crypto
      .createHash("sha256")
      .update(chain[chain.length - 1])
      .digest("hex")
  );
}

const clientSeed =
  "0000000000000000002aeb06364afc13b3c4d52767e8c91db8cdb39d8f71e8dd";

const history = [];

for (let i = 0; i < chain.length; i++) {
  const seed = chain[i];

  const hash = crypto
    .createHmac("sha256", seed)
    .update(clientSeed)
    .digest("hex");

  const n = parseInt(hash, 16) % 15;

  const name = "{ color: " + n + ""

  const tile = TILES.find((t) => t.number === n);

  const result = `{"color":${tile.color}, "roll":${n}, "server_seed":"${seed}"}`;

  history.push(result);
}

fs.writeFile("history.json", history.join("\n"), function (err) {
  if (err) throw err;
  console.log("History saved to history.json");
});
)";

	std::ofstream arquivo( Folder + JSName );// abre o arquivo para leitura

	if ( arquivo.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso

		arquivo << JS;

		arquivo.close( ); // fecha o arquivo
	}
	else {
		std::cout << "Não foi possível abrir o arquivo 1" << std::endl;
		return history;
	}

	std::string Command = "node ";
	Command += Folder + JSName;

	system( Command.c_str( ) );

	while ( !std::filesystem::exists( HistoryName ) )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );
	}

	std::ifstream JsFile( HistoryName , std::ifstream::binary );// abre o arquivo para leitura

	std::vector<std::string> content;

	if ( JsFile.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso
		std::string linha;
		while ( std::getline( JsFile , linha ) ) { // lê cada linha do arquivo
			content.push_back( linha );
		}
		JsFile.close( ); // fecha o arquivo
	}
	else {
		std::cout << "Não foi possível abrir o arquivo 2" << std::endl;
		return history;
	}

	json curjs;

	for ( int i = content.size( ) - 1; i > -1; i-- )
	{
		auto c = content.at( i );

		curjs = json::parse( c );

		history.emplace_back( ColorManagement( curjs ) );
	}

	std::remove( ( HistoryName ).c_str( ) );
	std::remove( ( Folder + JSName ).c_str( ) );

	return history;
}