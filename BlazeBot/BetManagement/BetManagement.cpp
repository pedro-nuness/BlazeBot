#include "BetManagement.h"


#include <fstream>
#include <iostream>
#include <Windows.h>
#include <WinInet.h>
#pragma comment(lib , "wininet.lib")


using namespace std;

bool Exists( const std::string & name ) {
	return std::filesystem::exists( name );
}


template<typename T>
void WriteData( std::string file , T data )
{
	ofstream arquivo( file , ios::app );

	// Verificar se o arquivo foi aberto corretamente
	if ( !arquivo.is_open( ) ) {
		cout << "Erro ao abrir o arquivo" << endl;
		return;
	}

	// Adicionar texto ao arquivo
	arquivo << data << "\n";

	// Fechar o arquivo
	arquivo.close( );
}

std::string replaceAllString( std::string subject , const std::string & search ,
	const std::string & replace ) {
	size_t pos = 0;
	while ( ( pos = subject.find( search , pos ) ) != std::string::npos ) {
		subject.replace( pos , search.length( ) , replace );
		pos += replace.length( );
	}
	return subject;
}



std::string DownloadString( std::string URL ) {
	HINTERNET interwebs = InternetOpenA( "Mozilla/5.0" , INTERNET_OPEN_TYPE_DIRECT , NULL , NULL , NULL );
	HINTERNET urlFile;
	std::string rtn;
	if ( interwebs ) {
		urlFile = InternetOpenUrlA( interwebs , URL.c_str( ) , NULL , NULL , NULL , NULL );
		if ( urlFile ) {
			char buffer[ 2000 ];
			DWORD bytesRead;
			do {
				InternetReadFile( urlFile , buffer , 2000 , &bytesRead );
				rtn.append( buffer , bytesRead );
				memset( buffer , 0 , 2000 );
			} while ( bytesRead );
			InternetCloseHandle( interwebs );
			InternetCloseHandle( urlFile );
			std::string p = replaceAllString( rtn , "|n" , "\r\n" );
			return p;
		}
	}
	InternetCloseHandle( interwebs );
	std::string p = replaceAllString( rtn , "|n" , "\r\n" );
	return p;
}

BetManager::BetManager( const std::string & filename, DoublePredictor * PredPtr )
{
	this->filename = filename;
	this->predictor = PredPtr;
	ClearData( );
}

void BetManager::addColor( int color , bool write) {
	if ( write )
		WriteData( filename , color );

	Color c = ( Color ) color;

	switch ( color ) {
	case Red:
		predictor->addColor( Red );
		reds++;
		break;
	case Blue:
		predictor->addColor( Blue );
		blues++;
		break;
	case White:
		predictor->addColor( White );
		whites++;
		break;
	default:
		break;
	}

}

// Função que faz a conexão com a API Blaze e retorna o array de cores resultante
std::vector<json> BetManager::blazeAPI( ) {
	auto res = DownloadString( "https://blaze.com/api/roulette_games/recent" );
	std::vector<json> resp_json = json::parse( res );
	return resp_json;
}

int BetManager::getBlues( ) const {
	return this->blues;
}

int BetManager::getReds( ) const {
	return this->reds;
}

int BetManager::getWhites( ) const {
	return this->whites;
}

int BetManager::getCorrects( ) const {
	return this->Corrects;
}

int BetManager::getWrongs( ) const {
	return this->Wrongs;
}

int BetManager::getSafeCorrects( ) const {
	return this->SafeCorrects;
}

int BetManager::getSafeWrongs( ) const {
	return this->SafeWrongs;
}

const std::vector<ColorManagement> & BetManager::getBets( ) const {
	return this->bets;
}

Bet BetManager::GetCurrentPrediction( ) {
	return this->CurrentPrediction;
}

Bet BetManager::GetLastPrediction( ) {
	return this->LastPrediction;
}

bool BetManager::OnGameMode( bool * set ) {

	if ( set != nullptr )
		GameMode = *set;

	return GameMode;
}

void BetManager::setupData( ) {
	json last_setup;

	while ( true ) {

		if ( OnGameMode( nullptr ) )
			continue;

		std::vector<json> colors = blazeAPI( );

		if ( colors.empty( ) )
			continue;

		if ( last_setup.empty( ) ) {
			
			for ( int i = colors.size( ) - 1; i >= 0; i-- ) {
				auto color = colors.at( i );
				addColor( color[ "color" ].get<int>( ) , !Exists( filename ) );
			}
			last_setup = colors.front( );
		}

		auto newest_color = colors.front( );

		if ( newest_color[ "id" ] != last_setup[ "id" ] ) {
			int color = newest_color[ "color" ].get<int>( );

			this->LastPrediction = this->CurrentPrediction;
			predictor->newest_color = ( Color ) color ;

			if ( predictor->newest_color == this->LastPrediction.GetColor() ) {
				bets.push_back( ColorManagement( this->LastPrediction.GetColor( ) , true ) );
				Corrects++;

				if ( this->LastPrediction.GetChance( ) * 100 > 50 )
					SafeCorrects++;
			}
			else if(this->LastPrediction.GetColor() != Null && this->LastPrediction.GetChance()){
				bets.push_back( ColorManagement( this->LastPrediction.GetColor( ) , false ) );
				Wrongs++;

				if ( this->LastPrediction.GetChance( ) * 100 > 50 )
					SafeWrongs++;
			}

			addColor( color );
		}

		last_setup = newest_color;

		Sleep( 2000 );
	}
}

int ConsecutiveLoses( ) {

}


void BetManager::PredictBets( ) {





}

void BetManager::ClearData( ) {
	this->bets.clear( );
	this->reds = 0;
	this->blues = 0;
	this->whites = 0;
}

void BetManager::SetCurrentPrediction( Color col, double chance ) {
	this->CurrentPrediction = Bet( col , chance );
}


Color BetManager::nextBet( Color prediction , float predictionChance , double * success ) {
	Color next_bet = Null;
	if ( prediction != Red && prediction != Blue && prediction != White )
		return next_bet;

	double red_chance = 4.5;
	double blue_chance = 4.5;
	double white_chance = 1.0;

	double total_chance = red_chance + blue_chance + white_chance;
	double red_prob = red_chance / total_chance * ( 100 - predictionChance ) / 100;
	double blue_prob = blue_chance / total_chance * ( 100 - predictionChance ) / 100;
	double white_prob = white_chance / total_chance * ( 100 - predictionChance ) / 100;
	double prediction_prob = predictionChance / 100.0;



}

