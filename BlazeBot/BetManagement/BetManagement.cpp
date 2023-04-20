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

BetManager::BetManager( const std::string & filename , DoublePredictor * PredPtr )
{
	this->filename = filename;
	this->predictor = PredPtr;
	ClearData( );
}

void BetManager::addColor( json play , bool write ) {
	if ( write )
		WriteData( filename , play );

	Color c = (Color)play["color" ];

	switch ( c ) {
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

const std::vector<Bet> & BetManager::getBets( ) const {
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
	ColorManagement LastColorSetup;

	while ( true ) {

		if ( OnGameMode( nullptr ) )
			continue;

		std::vector<json> GameColors = blazeAPI( );

		if ( GameColors.empty( ) )
			continue;

		if ( !LastColorSetup.IsSetupped( ) ) {

			for ( int i = GameColors.size( ) - 1; i >= 0; i-- ) { //Inverse sequence in API
				addColor( GameColors.at( i ), !Exists( filename ) );
			}
			LastColorSetup = GameColors.front( );
		}

		auto MostRecentPlay = ColorManagement(GameColors.front( ));


		if ( MostRecentPlay.GetID() != LastColorSetup.GetID() ) {
		

			this->LastPrediction = this->CurrentPrediction;
			predictor->LastColor = MostRecentPlay.GetColor( );

			if ( predictor->LastColor == this->LastPrediction.GetColor( ) ) {

				Bet bet_push;
				bet_push = this->LastPrediction;
				bet_push.SetCorrect( WON );

				bets.push_back( bet_push );

				if ( this->LastPrediction.DidBet( ) ) {
					CurrentPlayer.IncreaseBalance( this->LastPrediction.GetBetAmount( ) );
				}

				Corrects++;
				if ( this->LastPrediction.GetChance( ) * 100 > 50 )
					SafeCorrects++;
			}
			else if ( this->LastPrediction.GetColor( ) != Null && this->LastPrediction.GetChance( ) ) {

				Bet bet_push;
				bet_push = this->LastPrediction;
				bet_push.SetCorrect( LOSE );

				bets.push_back( bet_push );

				if ( this->LastPrediction.DidBet( ) ) {
					CurrentPlayer.DecreaseBalance( this->LastPrediction.GetBetAmount( ) );
				}

				Wrongs++;
				if ( this->LastPrediction.GetChance( ) * 100 > 50 )
					SafeWrongs++;
			}

			addColor(GameColors.front() );
		}

		LastColorSetup = MostRecentPlay;

		Sleep( 2000 );
	}
}



Bet BetManager::PredictBets( Color c , double chance , float balance ) {


	Bet NextBet;
	NextBet = Bet( c , chance , PREDICTION , MATH ); //Default Allocator

	if ( !CurrentPlayer.IsSetupped( ) )
		CurrentPlayer = Player( balance ); //Settup our balance to play the game

	auto CurrentBalance = CurrentPlayer.GetBalance( );
	auto Profit = CurrentPlayer.GetProfit( );
	bool Betted = false;

	float CurBet = 0;

	static bool WonLastBet = false;

	if ( bets.empty( ) || c == Null )
	{
		NextBet.SetMethod( MATH );
	}
	else {

		auto & LastBet = bets.at( bets.size( ) - 1 );

		//Martingalle

		//if ( LastBet.DidBet( ) ) { //We did a bet last round

		if ( LastBet.GetBetResult( ) == LOSE ) //We lose the LastBet
		{
			if ( LastBet.DidBet( ) ) {
				CurBet = ( LastBet.GetBetAmount( ) * 2 ); // 2x bet
				//Martingalle
			}
			else
				CurBet = balance * 0.02; // 2% of balance
			NextBet.SetMethod( MARTINGALE );
		}
		else if ( LastBet.GetBetResult( ) == WON )
		{
			if ( LastBet.DidBet( ) ) {

				if ( ( LastBet.GetBetAmount( ) / 2 ) >= balance * 0.02 )
				{
					CurBet = LastBet.GetBetAmount( ) / 2; // 0.5x bet
					NextBet.SetMethod( FIBONACCI );
				}
				else {
					CurBet = LastBet.GetBetAmount( ); // 0.5x bet
					NextBet.SetMethod( FIBONACCI );
				}
			}
			else {
				CurBet = balance * 0.02; // 2% of balance
			}
		}

		NextBet.DoBet( CurBet , CurrentBalance );
		Betted = true;
	}

	std::cout << "Current Balance: R$" << CurrentBalance << "\n";
	std::cout << "Initial investment: R$" << CurrentPlayer.GetInitialMoney() << "\n";
	std::cout << "Profit: R$" << Profit << "\n";

	if ( Betted )
	{

		std::cout << "It's a bet!\n";
		std::cout << "CurrentBet: R$" << CurBet;
		std::cout << " ( ";
		switch ( NextBet.GetMethod( ) )
		{
		case MARTINGALE:
			std::cout << "MARTINGALE";
			break;
		case FIBONACCI:
			std::cout << "FIBONACCI";
			break;
		default:
			std::cout << "?";
			break;
		}
		std::cout << " )\n ";

	}


	return NextBet;
}

void BetManager::ClearData( ) {
	this->bets.clear( );
	this->reds = 0;
	this->blues = 0;
	this->whites = 0;
}

void BetManager::SetCurrentPrediction( Bet  bet )
{
	this->CurrentPrediction = bet;
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

