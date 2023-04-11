#include <iostream>
#include <numeric>
#include <thread>

#include "BetManagement/BetManagement.h"
#include "Predictor/Predictor.h"
#include "Colors/ColorManagement/ColorManagement.h"
#include "Utils/Utils.h"

#include <Windows.h>
#include <fstream>

std::chrono::system_clock::time_point CurrentTime( ) {
	return std::chrono::system_clock::now( );
}

std::time_t ToTime( std::chrono::system_clock::time_point point ) {
	return std::chrono::system_clock::to_time_t( point );
}

float DiffInSeconds( std::chrono::system_clock::time_point t1 , std::chrono::system_clock::time_point t2 ) {

	return std::chrono::duration_cast< std::chrono::milliseconds >( t1 - t2 ).count( );
}

std::string ColorToLetter( Color col ) {

	switch ( col ) {
	case Color::Red:
		return "R";
		break;
	case Color::Blue:
		return "B";
		break;
	case Color::White:
		return "W";
		break;
	default:
		return "NULL";
	}
}

std::string ColorToString( Color col ) {

	switch ( col ) {
	case Color::Red:
		return "Red";
		break;
	case Color::Blue:
		return "Black";
		break;
	case Color::White:
		return "White";
		break;
	default:
		return "NULL";
	}
}

void DrawPred( std::string pred , Color col , float chance )
{
	std::cout << pred;
	switch ( col ) {
	case Color::Red:
		std::cout << "Red";
		break;
	case Color::Blue:
		std::cout << "Black";
		break;
	case Color::White:
		std::cout << "White";
		break;
	case Color::Null:
		std::cout << "Null";
		break;
	}

	std::cout << " ( " << int( chance * 100 ) << "% )" << "\n\n";
}


bool Exist( const std::string & name ) {
	return std::filesystem::exists( name );
}



void WriteCorrects( BetManager BetManage ) {

	std::ofstream arquivo( "Results.txt" );

	// Verificar se o arquivo foi aberto corretamente
	if ( !arquivo.is_open( ) ) {
		std::cout << "Erro ao abrir o arquivo" << std::endl;
		return;
	}

	// Adicionar texto ao arquivo
	arquivo << "Hits: " << BetManage.getCorrects( ) << "\n";
	arquivo << "Misses: " << BetManage.getWrongs( ) << "\n";

	arquivo << "SafeHits: " << BetManage.getSafeCorrects( ) << "\n";
	arquivo << "SafeMisses: " << BetManage.getSafeWrongs( ) << "\n";


	// Fechar o arquivo
	arquivo.close( );

}


using namespace std;

bool Stop = false;

DoublePredictor predictor;

void SimulateGame(BetManager& bet ) {

	system( "cls" );

	bool Gamemode = true;
	bet.OnGameMode( &Gamemode );
	
	Prediction Prediction;
	float Hits = 0;
	float Misses = 0;
	float Total = 0;
	bet.ClearData( );
	predictor.clearResults( );

	std::cout << "--You joined Game Simulation Mode--\n\n";

	while ( true ) {
		
		if ( GetAsyncKeyState( VK_HOME ) ) {

			auto OldHistory = predictor.GetHistory( );

			system( "cls" );

			std::cout << "--You joined Game Simulation Mode--\n\n";


			while ( predictor.GetHistory( ).size( ) < OldHistory.size( ) + 500 )
			{
				Prediction = predictor.predictNext( );

				Color NewColor = predictor.getNextColorWithPreference( Null );

				if ( Prediction.chance > 0.55 && predictor.GetHistory( ).size() >= 100) {

					if ( Prediction.color == NewColor )
						Hits++;
					else
						Misses++;
				}

				Total = Hits + Misses;

				bet.addColor( NewColor , false );
			}

			std::cout << "Red: " << bet.getReds( ) << std::endl;
			std::cout << "Black: " << bet.getBlues( ) << std::endl;
			std::cout << "White: " << bet.getWhites( ) << std::endl;

			float HitsPercentage = ( Hits / Total ) * 100;
			float MissesPercentage =  (Misses / Total) * 100;

			std::cout << "Hits: " << Hits << "( " << HitsPercentage << "% )" << std::endl;
			std::cout << "Misses: " << Misses << "( " << MissesPercentage << "% )" << std::endl;

		}


	}

	return;
}

void PrintColor( Utils usefull, Color c )
{
	switch ( c ) {
	case Red:
		usefull.ColoredText( "O" , RED );
		break;
	case Blue:
		usefull.ColoredText( "O" , GRAY );
		break;
	case White:
		usefull.ColoredText( "O" , WHITE );
		break;
	}

}


int main( ) {
	srand( time( 0 ) ); // Semente para a função rand()


	std::string filename = "Scan.txt";

	BetManager betManager( filename , &predictor );
	Utils usefull;


	if ( Exist( filename ) ) {

		std::vector<std::string> content;

		std::ifstream arquivo( filename ); // abre o arquivo para leitura

		if ( arquivo.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso
			std::string linha;
			while ( std::getline( arquivo , linha ) ) { // lê cada linha do arquivo
				content.push_back( linha );
			}
			arquivo.close( ); // fecha o arquivo
		}
		else {
			std::cout << "Não foi possível abrir o arquivo" << std::endl;
		}

		for ( auto c : content )
		{
			betManager.addColor( stoi( c ) , false );
		}

	}
	

	bool setup = false;
	std::thread( &BetManager::setupData , &betManager ).detach( );


	std::chrono::system_clock::time_point OldTime;

	Color OldPrediction = Null;
	int OldChance = 999;

	int OldTotal = -1;

	double BetterResults = 0;


	while ( true ) {
		std::chrono::system_clock::time_point NowTime = CurrentTime( );

		if ( GetAsyncKeyState( VK_HOME ) ) {
			SimulateGame( betManager );
		}

		if ( GetAsyncKeyState( 'P' ) & 1 )
		{
			Stop = !Stop;
		}


		if ( GetAsyncKeyState( VK_DELETE ) )
		{
			betManager.ClearData( );
			predictor.clearResults( );
		}

		int Total = predictor.GetHistory( ).size( );


		if ( !setup || Total != OldTotal ) {
			OldTotal = Total;
			OldTime = NowTime;

			WriteCorrects( betManager );

			system( "cls" );

			int Reds = betManager.getReds( );
			int Blues = betManager.getBlues( );
			int Whites = betManager.getWhites( );

			double pRedChance = predictor.getCertainty( Color::Red );
			double pBlackChance = predictor.getCertainty( Color::Blue );
			double pWhiteChance = predictor.getCertainty( Color::White );

			//double rRedChance = predictor.getCertaintyRecoded( Color::Red );
			//double rBlackChance = predictor.getCertaintyRecoded( Color::Blue );
			//double rWhiteChance = predictor.getCertaintyRecoded( Color::White );

			auto Prediction = predictor.predictNext( );
			Color PredictedColor = Prediction.color;
			double PredictionAccurracy = Prediction.chance;

			double Corrects = betManager.getCorrects( );
			double Wrongs = betManager.getWrongs( );
			double SafeCorrects = betManager.getSafeCorrects( );
			double SafeWrongs = betManager.getSafeWrongs( );
			double TotalBets = Corrects + Wrongs;
			double TotalSafeBets = SafeCorrects + SafeWrongs;

			betManager.SetCurrentPrediction( PredictedColor , PredictionAccurracy );

			std::cout << "Red: " << Reds << " ( " << pRedChance << " )" << std::endl;
			std::cout << "Black: " << Blues << " ( " << pBlackChance << " )" << std::endl;
			std::cout << "White: " << Whites << " ( " << pWhiteChance << " )" << std::endl;

			//std::cout << "rRed: " << Reds << " ( " << rRedChance << " )" << std::endl;
			//std::cout << "rBlack: " << Blues << " ( " << rBlackChance << " )" << std::endl;
			//std::cout << "rWhite: " << Whites << " ( " << rWhiteChance << " )" << std::endl;

			if ( predictor.isStreak(  ).StreakSize >= 2)
				std::cout << "\nIs Streak!\n";

			DrawPred( "\nPrediction: " , PredictedColor , PredictionAccurracy );

			DrawPred( "Last Prediction: " , betManager.GetLastPrediction( ).GetColor( ) , betManager.GetLastPrediction( ).GetChance( ) );

			std::cout << "Last results:\n";
			auto history = predictor.GetHistory( );
			if ( history.size( ) > 17 ) {
				for ( int i = history.size( ) - 17; i < history.size( ) ; i++ )
				{
					auto color = history.at( i );
					PrintColor( usefull,  color );
					std::cout << " ";
				}
				std::cout << "\n\n";
			}
			



			if ( TotalBets ) {
				std::cout << "Hits: " << Corrects << "/" << TotalBets << " (" << ( Corrects / TotalBets ) * 100 << "% )\n\n";
			}

			if ( TotalSafeBets ) {
				std::cout << "SafeHits:  " << SafeCorrects << "/" << TotalSafeBets << " (" << ( SafeCorrects / TotalSafeBets ) * 100 << "% )\n\n";
			}
			vector<double> tAccurracy = predictor.crossValidate( 2 );
			double MeanAccurracy = accumulate( tAccurracy.begin( ) , tAccurracy.end( ) , 0.0 ) / tAccurracy.size( );

			std::cout << "Accuracy tests:" << endl;
			for ( int i = 0; i < tAccurracy.size( ); i++ )
				std::cout << "Accurracy [" << i << "]: " << tAccurracy[ i ] * 100 << "%" << endl;

			std::cout << "\nMean accurracy: ";
			if ( MeanAccurracy >= 0.5 )
				usefull.ColoredText( std::to_string( MeanAccurracy ) , GREEN );	
			else if ( MeanAccurracy <= 0.35 )		
				usefull.ColoredText( std::to_string( MeanAccurracy ) , RED );
			else
				usefull.ColoredText( std::to_string( MeanAccurracy ) , YELLOW );
			std::cout << std::endl;

			std::cout << "\nBetter accurracy: ";
			if ( BetterResults >= 0.5 )
				usefull.ColoredText( std::to_string( BetterResults ) , GREEN );
			else if ( BetterResults <= 0.35 )
				usefull.ColoredText( std::to_string( BetterResults ) , RED );
			else
				usefull.ColoredText( std::to_string( BetterResults ) , YELLOW );
			std::cout << std::endl;


			if ( MeanAccurracy > BetterResults )
			{
				BetterResults = MeanAccurracy;
			}


			int TransitionsWindow = 3;
			auto Transitions = predictor.GetTransitions( TransitionsWindow );
			std::cout << "Found " << Transitions.size( ) << " transitions of " <<TransitionsWindow << "!\n";

			OldPrediction = PredictedColor;
			OldChance = PredictionAccurracy;
		}

		if ( !setup )
		{
			setup = true;
			OldTime = CurrentTime( );
		}
	}
}