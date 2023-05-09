#include "BetManagement.h"


#include <fstream>
#include <iostream>
#include <WinInet.h>
#include "..\globals.h"
#pragma comment(lib , "wininet.lib")


using namespace std;

extern std::string filename;
extern std::string Folder;
extern std::string JSName;
extern std::string HistoryName;
extern std::string ImportName;

bool Exists( const std::string & name ) {
	return std::filesystem::exists( name );
}


template<typename T>
void WriteData( std::string file , T data )
{
	std::string dir = "C:\\Blaze\\" + file;

	ofstream arquivo( dir , ios::app );

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

void Beats::PrintBetLose( int prediction ) {

	std::vector < Color > CurrentGame;

	for ( int i = BetPtr->APIHistory.size( ) - 18; i < BetPtr->APIHistory.size( ); i++ )
	{
		auto c = BetPtr->APIHistory[ i ].GetColor( );
		CurrentGame.emplace_back( c );
	}

	std::string data;

	for ( int i = 0; i < CurrentGame.size( ) - 2; i++ )
	{
		auto c = CurrentGame[ i ];
		switch ( c )
		{
		case Red:
			data += "R";
			break;
		case Blue:
			data += "B";
			break;
		case White:
			data += "W";
			break;
		default:
			data += "0";
			break;
		}

		data += " ";
	}

	data += "-> ";
	int latestcol = CurrentGame[ CurrentGame.size( ) - 1 ];

	switch ( latestcol )
	{
	case Red:
		data += "R";
		break;
	case Blue:
		data += "B";
		break;
	case White:
		data += "W";
		break;
	default:
		data += "0";
		break;
	}

	data += " Prediction: ";

	switch ( prediction )
	{
	case Red:
		data += "R";
		break;
	case Blue:
		data += "B";
		break;
	case White:
		data += "W";
		break;
	default:
		data += "0";
		break;
	}


	std::string dir = "C:\\Blaze\\Loses";

	switch ( Method )
	{
	case FOUND_PATTERN:
		dir += " ( PATTERN )";
		break;
	case CERTAINTY:
		dir += " ( MATH )";
		break;
	case IA:
		dir += " ( I.A )";
		break;
	case STREAK:
		dir += " ( STREAK )";
		break;
	case SEQUENCE:
		dir += " ( SEQUENCE )";
		break;
	case GENERAL:
		dir += " ( GENERAL )";
		break;
	default:
		dir += " ( NONE )";
		break;
	}


	dir += ".txt";

	ofstream arquivo( dir , ios::app );

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

METHODS BetPredictor::ChooseBetStrategy( )
{



}


BetManager::BetManager( const std::string & filename , DoublePredictor * PredPtr )
{
	this->filename = filename;
	this->predictor = PredPtr;
	this->BetPredicton = BetPredictor( PredPtr , &this->bets );
	ClearData( );
}

void BetManager::addColor( json play , bool write ) {
	//if ( write )
	//	WriteData( filename , play );

	Color c = ( Color ) play[ "color" ];

	predictor->addColor( play );


	std::cout << "Add: " << "c:" << c << ", r: " << play[ "roll" ] << ", id: " << play[ "server_seed" ] << std::endl;
	switch ( c ) {
	case Red:
		reds++;
		break;
	case Blue:
		blues++;
		break;
	case White:
		whites++;
		break;
	default:
		break;
	}

}

// Função que faz a conexão com a API Blaze e retorna o array de cores resultante
std::vector<json> BetManager::blazeAPI( ) {
	auto res = DownloadString( "https://blaze.com/api/roulette_games/recent" );

	std::vector<json> resp_json;
	if ( !res.empty( ) ) {
		resp_json = json::parse( res );
	}
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

std::vector<Bet> & BetManager::getBets( ) {
	return this->bets;
}

std::vector<Bet> & BetManager::getBetsDisplay( ) {
	return this->bets_display;
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


extern void save_cfg( bool read );

void ImportToBot( BetManager * bet ) {

	json js;

	js[ "balance" ] = bet->CurrentPlayer.GetBalance( );
	js[ "initialbalance" ] = bet->CurrentPlayer.GetInitialMoney( );
	js[ "server_seed" ] = bet->APIHistory.back( ).GetServerID( );
	js[ "color" ] = bet->GetCurrentPrediction( ).GetColor( );
	js[ "betted" ] = bet->GetCurrentPrediction( ).DidBet( );
	js[ "coverwhite" ] = bet->GetCurrentPrediction( ).GetPrediction( ).PossibleWhite;
	js[ "betvalue" ] = bet->GetCurrentPrediction( ).GetBetAmount( );
	js[ "whitebetvalue" ] = bet->GetCurrentPrediction( ).GetWhiteBet( );
	js[ "history" ] = bet->GetPredictor( )->GetGameTransition( 13 ).Colors;
	js[ "accuracy" ] = bet->SeparatedBeats[ GENERAL ].GetHitsPercentage( );

	std::ofstream arquivo( Folder + ImportName , std::ios::trunc );

	if ( arquivo.is_open( ) ) {
		arquivo << js;
	}
	else {
		std::cout << "Erro ao abrir o arquivo." << std::endl;
	}
}


void BetManager::ManageBets( ) {

	int last_history_size = -1;

	while ( true )
	{
		if ( StartBets )
		{
			while ( BalanceHistory.size( ) > 300 )
			{
				BalanceHistory.erase( BalanceHistory.begin( ) , BalanceHistory.begin( ) + 1 );
			}

			auto hist = predictor->GetHistory( );

			if ( last_history_size == -1 )
				last_history_size = hist.size( );

			if ( hist.size( ) != last_history_size )
			{
				auto Prediction = predictor->predictNext( );

				auto NextBet = PredictBets( Prediction );
				SetCurrentPrediction( NextBet );

				save_cfg( false );

				std::cout << "Corrects: " << Corrects << std::endl;
				std::cout << "Wrongs: " << Wrongs << std::endl;

				ImportToBot( this );

				last_history_size = hist.size( );
			}

			Sleep( 500 );
		}
	}

}

void StartupNode( std::string seed , int amount , BetManager * bets ) {

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
		return;
	}

	std::string Command = "node ";
	Command += Folder + JSName;

	system( Command.c_str( ) );

	Sleep( 1000 );

	while ( !std::filesystem::exists( HistoryName ) )
		Sleep( 3000 );


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
		return;
	}

	json curjs;

	for ( int i = content.size( ) - 1; i > -1; i-- )
	{
		auto c = content.at( i );

		curjs = json::parse( c );

		bets->addColor( curjs , false );
		bets->APIHistory.emplace_back( ColorManagement( curjs ) );

	}

	std::remove( ( HistoryName ).c_str( ) );
	std::remove( ( Folder + JSName ).c_str( ) );
}


void BetManager::setupData( ) {
	ColorManagement LastColorSetup;

	//Settup bets vec
	for ( int i = 0; i < NONE; i++ )
		SeparatedBeats.emplace_back( Beats( this , i ) );

	while ( true ) {

		if ( OnGameMode( nullptr ) )
			continue;

		std::vector<json> GameColors = blazeAPI( );

		if ( GameColors.empty( ) )
			continue;

		if ( !LastColorSetup.IsSetupped( ) ) {

			LastColorSetup = GameColors.front( );
			StartupNode( LastColorSetup.GetServerID( ) , 20000 , this );

			std::vector<json> DelayedColors = blazeAPI( );
			GameColors = DelayedColors;

			if ( !DelayedColors.empty( ) )
			{
				ColorManagement LastPlay = ColorManagement( LastColorSetup );

				bool Found = false;

				for ( int i = DelayedColors.size( ) - 1; i > -1; i-- )
				{
					auto JsonColor = DelayedColors[ i ];

					ColorManagement Play( JsonColor );

					if ( Play.GetID( ) == LastPlay.GetID( ) )
					{
						Found = true;
						continue;
					}

					if ( Found )
					{
						addColor( JsonColor );
					}
				}

				LastColorSetup = GameColors.front( );
			}
		}
		else
			StartBets = true;

		auto MostRecentPlay = ColorManagement( GameColors.front( ) );

		if ( MostRecentPlay.GetID( ) != LastColorSetup.GetID( ) ) {

			APIHistory.emplace_back( ColorManagement( MostRecentPlay ) );
			this->LastPrediction = this->CurrentPrediction;
			predictor->LastColor = MostRecentPlay.GetColor( );


			if ( !predictor->SeparatedPrediction.empty( ) ) {

				for ( int i = 0; i < NONE; i++ )
				{
					auto prediction = predictor->SeparatedPrediction[ i ];

					SeparatedBeats[ i ].SetupBeat( prediction , MostRecentPlay.GetColor( ) );
				}
			}

			Bet bet_push;
			bet_push = this->CurrentPrediction;
			bet_push.SetCorrect( PREDICTION );

			if ( this->CurrentPrediction.GetColor( ) != Null && this->CurrentPrediction.GetChance( ) )
			{
				if ( MostRecentPlay.GetColor( ) == this->CurrentPrediction.GetColor( ) )
					bet_push.SetCorrect( WON );
				else
					bet_push.SetCorrect( LOSE );


				if ( this->CurrentPrediction.GetPrediction( ).PossibleWhite && this->CurrentPrediction.GetWhiteBet( ) )
				{
					if ( MostRecentPlay.GetColor( ) == White ) {
						CurrentPlayer.IncreaseBalance( this->CurrentPrediction.GetWhiteBet( ) * 14 );
						bet_push.SetCorrect( WON );
					}
					else
						CurrentPlayer.DecreaseBalance( this->CurrentPrediction.GetWhiteBet( ) );
				}

				if ( MostRecentPlay.GetColor( ) == this->CurrentPrediction.GetColor( ) ) {

					Corrects++;
					if ( this->CurrentPrediction.GetChance( ) * 100 > 50 )
						SafeCorrects++;

					int losestreak = 0;

					for ( int i = bets.size( ) - 1; i > 0; i-- )
					{
						auto bet = bets[ i ];

						if ( bet.GetBetResult( ) == LOSE )
						{
							losestreak++;
						}
						else
							break;
					}

					if ( losestreak && losestreak <= 2 ) // We just lost 2, small lose amount
					{
						Wrongs -= losestreak;
					}

					if ( this->CurrentPrediction.DidBet( ) ) {

						CurrentPlayer.IncreaseBalance( this->CurrentPrediction.GetBetAmount( ) );

						BalanceHistory.emplace_back( CurrentPlayer.GetBalance( ) );
						bets.push_back( bet_push );
					}
				}
				else {

					Wrongs++;
					if ( this->CurrentPrediction.GetChance( ) * 100 > 50 )
						SafeWrongs++;

					if ( this->CurrentPrediction.DidBet( ) ) {

						CurrentPlayer.DecreaseBalance( this->CurrentPrediction.GetBetAmount( ) );
						BalanceHistory.emplace_back( CurrentPlayer.GetBalance( ) );
						bets.push_back( bet_push );
					}
				}
			}


			bets_display.push_back( bet_push );

			addColor( GameColors.front( ) );
		}

		LastColorSetup = MostRecentPlay;

		Sleep( 2000 );
	}
}


template<typename T>
void WriteOnFile( std::string file , T data )
{
	std::string dir = "C:\\Blaze\\" + file;

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

void PrintBet( BetManager * bet )
{
	std::string PrintFile;
	PrintFile += "Balance: " + std::to_string( bet->CurrentPlayer.GetBalance( ) ) + "\n";
	PrintFile += "InitialMoney: " + std::to_string( bet->CurrentPlayer.GetInitialMoney( ) ) + "\n";
	PrintFile += "Profit: " + std::to_string( bet->CurrentPlayer.GetProfit( ) ) + "\n";
	if ( bet->CurrentPlayer.GetProfit( ) > 0 ) {
		PrintFile += "Profit!\n";
	}
	else
		PrintFile += "Non Profit!\n";

	bet->EndingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
	std::chrono::minutes diferenca = std::chrono::duration_cast< std::chrono::minutes >( bet->EndingBetTime - bet->StartingBetTime ); // Calcula a diferença entre os tempos em milissegundos

	PrintFile += "Elapsed time betting: " + std::to_string( float( diferenca.count( ) ) ) + " minutes \n";

	WriteData( "BetResults.txt" , PrintFile );

	bet->ClearData( );

	bet->StartingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
}

template <typename T>
T clamp_value( T Value , T min , T max )
{
	if ( Value > max )
		return max;
	else if ( Value < min )
		return min;

	return Value;
}



bool BetManager::NeedToWait( )
{
	bool * Wait = &WaitController.Wait;
	int * WaitStartingPosition = &WaitController.WaitStartPos;
	int * WaitAmount = &WaitController.WaitAmount;

	if ( g_globals.Betting.WaitIfLosing ) {

		if ( *Wait )
		{
			if ( WaitAmount > 0 )
			{
				WaitAmount--;
				return true;
			}
			else {
				return false;
			}
		}
		else {

			int LoseCount = 0;

			for ( int i = bets.size( ) - 1; i > 0; i-- )
			{
				auto bet = bets.at( i );

				if ( bet.GetBetResult( ) == LOSE )
					LoseCount++;
				else
					break;
			}

			if ( LoseCount >= g_globals.Betting.WaitAfterXLose )
			{
				//Já esperamos 1
				*Wait = true;
				*WaitAmount = g_globals.Betting.WaitAmount - 1;
				*WaitStartingPosition = predictor->GetHistory( ).size( ) - 1;
				return true;
			}
		}
	}
	else
	{
		*Wait = false;
		*WaitAmount = 0;
		*WaitStartingPosition = 0;
	}

	return false;
}


void Beats::SetupBeat( int ColorPrediction , int TrueColor )
{
	if ( ColorPrediction != Null ) {
		if ( ColorPrediction == TrueColor ) {
			WasWinning = true;
			Hits++;

			Results.emplace_back( WON );

			if ( WasLoosing ) {
				RollLoses.emplace_back( CurrentRollLose );

				if ( CurrentRollLose && CurrentRollLose <= g_globals.Betting.MaxMultiplierTimesOnLose )
				{
					Misses -= CurrentRollLose;
				}

				CurrentRollLose = 0;
				WasLoosing = false;
			}
		}
		else {
			WasWinning = false;

			Results.emplace_back( LOSE );

			PrintBetLose( ColorPrediction );

			if ( !WasWinning )
			{
				WasLoosing = true;

				CurrentRollLose++;


				if ( CurrentRollLose > MaxRollLoseAmount )
				{
					MaxRollLoseAmount = CurrentRollLose;
				}
			}
			Misses++;
		}
	}
}

Bet BetManager::PredictBets( Prediction predict ) {


	Bet NextBet;
	NextBet = Bet( predict , PREDICTION , MINIMUM ); //Default Allocator

	if ( !CurrentPlayer.IsSetupped( ) )
		return NextBet;

	float CurrentBalance = CurrentPlayer.GetBalance( );
	float InitialBalance = CurrentPlayer.GetInitialMoney( );
	float PeakBalance = CurrentPlayer.GetPeakBalance( );
	float Profit = CurrentPlayer.GetProfit( );
	float MinimumPercentage = g_globals.Betting.MinBetPercentage / 100;
	float CurrentBet = 0.f;
	float WhiteBet = 0.f;
	float MaximumBet = ( InitialBalance * ( g_globals.Betting.MaxBetPercentage / 100.f ) );
	float MinimumBet = clamp_value( InitialBalance * ( g_globals.Betting.MinBetPercentage / 100.f ) , 2.f , MaximumBet );

	static int MultiplierLoseCount = 0;
	static int MultiplierWinCount = 0;
	static int LoseCount = 0;
	int TargetPercentage = int( InitialBalance * ( g_globals.Betting.TargetParcentage / 100 ) );
	int StopPercentage = int( InitialBalance * ( ( g_globals.Betting.StopPercentage ) / 100 ) );

	static bool Waited = false;


	if ( BalanceHistory.size( ) > 10 ) {
		int BalanceSize = BalanceHistory.size( );
		int DifferenceFromLastBalance = BalanceHistory[ BalanceSize - 2 ] - BalanceHistory[ BalanceSize - 1 ];
		float Tolerance = InitialBalance * ( g_globals.Betting.PreventIfAbove / 100 );

		if ( DifferenceFromLastBalance < 0 && fabs( DifferenceFromLastBalance >= Tolerance ) )
		{
			//We just Recovered from a down peak;
			//Let's check if there's others down peaks near this one

			std::vector<int> PeakDownPositions;

			for ( int i = BalanceHistory.size( ) - 1; i > 1; i-- )
			{
				if ( PeakDownPositions.size( ) >= 2 )
					break;

				int Difference = BalanceHistory[ i - 1 ] - BalanceHistory[ i ];
				if ( ( fabs( Difference ) > Tolerance ) && Difference < 0 )
				{
					PeakDownPositions.emplace_back( i - 1 );
				}
			}

			if ( PeakDownPositions.size( ) >= 2 )
			{
				int DistanceBetweenPeaks = fabs( PeakDownPositions[ 0 ] - PeakDownPositions[ 1 ] );

				//They are near?
				if ( DistanceBetweenPeaks <= g_globals.Betting.MinimumPeakDistance )
				{
					if ( StartWaitingTime == ( std::chrono::high_resolution_clock::time_point::min ) ( ) ) {
						StartWaitingTime = std::chrono::high_resolution_clock::now( );
						//Let's wait
					}
					else if ( !Waited ) {
						//We're already waiting
						float ElapsedTime = std::chrono::duration_cast< std::chrono::minutes >( std::chrono::high_resolution_clock::now( ) - StartWaitingTime ).count( );

						if ( ElapsedTime >= g_globals.Betting.WaitingTime )
						{
							StartWaitingTime = ( std::chrono::high_resolution_clock::time_point::min ) ( );
							Waited = true;
						}
						else {
							NextBet.SetMethod( STANDBY );
							return NextBet;
						}
					}

				}
			}
		}

	}

	if ( StartingBetTime == ( std::chrono::high_resolution_clock::time_point::min ) ( ) ) {
		StartingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
	}

	if ( Profit <= -( StopPercentage ) || Profit >= ( TargetPercentage ) )
	{
		PrintBet( this );
		return NextBet;
	}

	if ( bets.empty( ) && ( predict.color != Null ) ) {

		NextBet.SetMethod( MINIMUM );

		if ( predict.color != Null )
			CurrentBet = MinimumBet;

		if ( predict.PossibleWhite )
			WhiteBet = MinimumBet / 2;

		NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );
	}
	else {

		if ( predict.color == Null )
		{
			NextBet.SetMethod( MINIMUM );
			CurrentBet = 0.0f;
		}
		else
		{
			if ( predict.PossibleWhite )
				WhiteBet = MinimumBet / 2;

			if ( NeedToWait( ) )
			{
				NextBet.SetMethod( STANDBY );
				return NextBet;
			}

			CurrentBet = MinimumBet / 2;

			auto & LastBet = bets.at( bets.size( ) - 1 );

			float WinMultipliedValue = ( LastBet.GetBetAmount( ) * ( g_globals.Betting.WonBetMultiplier / 100 ) );
			float LoseMultipliedValue = ( LastBet.GetBetAmount( ) * ( g_globals.Betting.LoseBetMultiplier / 100 ) );


			if ( LastBet.GetBetResult( ) == LOSE ) //We lose the LastBet
			{
				if ( WaitController.Wait )
				{
					LoseCount++;
					if ( g_globals.Betting.WaitAgainIfLose ) {
						if ( LoseCount >= 3 )
						{
							//Wait again if we lose once
							WaitController.WaitStartPos = predictor->GetHistory( ).size( ) - 1;
							NextBet.SetMethod( STANDBY );
							return NextBet;
						}
					}
					else {
						ResetWaitControler( );
					}
				}

				//Se a quantidade de multiplicador de ganho for maior que zero (Multiplicamos a aposta na vitoria)
				if ( MultiplierWinCount > 0 ) {
					if ( g_globals.Betting.ResetNextBetMultiplierOnLose ) //Se estiver ligado, resete, senao, remova 1
					{
						MultiplierWinCount = 0;
					}
					else
						MultiplierWinCount--;
				}

				//Se a quantidade de multiplicacao for 
				if ( g_globals.Betting.MultiplyBetOnLose )
				{
					int LoseStreak = 0;

					for ( int i = bets.size( ) - 1; i > 0; i-- )
					{
						auto Bet = bets.at( i );

						if ( Bet.GetBetResult( ) == LOSE )
						{
							LoseStreak++;
						}
						else {
							break;
						}
					}

					if ( LoseStreak >= g_globals.Betting.MultiplyBetAfterXLose )
					{
						MultiplierLoseCount++;

						if ( MultiplierLoseCount <= g_globals.Betting.MaxMultiplierTimesOnLose )
						{
							CurrentBet = clamp_value( LoseMultipliedValue , MinimumBet , MaximumBet );	

							if ( g_globals.Betting.ProtectProfit )
							{
								float ProtectPercentage = InitialBalance * ( g_globals.Betting.ProtectAfter / 100 );

								if ( Profit > ProtectPercentage )
								{
									float MaximumMultiplier = Profit * ( g_globals.Betting.ProtectPercentage / 100 );

									if ( LoseMultipliedValue > MaximumMultiplier )
									{
										CurrentBet = MinimumBet;
									}
								}
							}						
						}
					}
				}
			}
			else if ( LastBet.GetBetResult( ) == WON )
			{
				if ( g_globals.Betting.ResetCountIfWin )
				{
					ResetWaitControler( );
				}
				else {

					if ( WaitController.WaitAmount >= 2 )
						WaitController.WaitAmount--;
				}

				if ( MultiplierLoseCount > 0 ) {

					if ( g_globals.Betting.ResetNextBetMultiplierOnWin )
					{
						MultiplierLoseCount = 0;
					}
					else
						MultiplierLoseCount--;
				}

				if ( g_globals.Betting.MultiplyBetOnWin ) {

					float WinStreak = 0;

					for ( int i = bets.size( ) - 1; i > 0; i-- )
					{
						auto Bet = bets.at( i );

						if ( Bet.GetBetResult( ) == WON )
						{
							WinStreak++;
						}
						else {
							break;
						}

					}

					if ( WinStreak >= g_globals.Betting.MultiplyBetAfterXWin )
					{
						MultiplierWinCount++;

						if ( MultiplierWinCount <= g_globals.Betting.MaxMultiplierTimesOnWin )
						{
							CurrentBet = clamp_value( WinMultipliedValue , MinimumBet , MaximumBet );						
						}
					}
				}

				NextBet.SetMethod( FIBONACCI );
			}

			if ( !CurrentBet || CurrentBet < 2 )
				CurrentBet = MinimumBet;
		}

		Waited = false;
		NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );
	}

	return NextBet;
}

void BetManager::ClearData( ) {
	this->bets.clear( );
	//this->predictor->clearResults( );
	this->CurrentPlayer.Reset( );
	//this->reds = 0;
	//this->blues = 0;
	//this->whites = 0;
}

void BetManager::SetCurrentPrediction( Bet  bet )
{
	this->CurrentPrediction = bet;
}

void MouseClick( )
{
	mouse_event( MOUSEEVENTF_LEFTDOWN , 0 , 0 , 0 , 0 );
	Sleep( 50 );
	mouse_event( MOUSEEVENTF_LEFTUP , 0 , 0 , 0 , 0 );
}

void typeNumber( int num ) {
	// Converte o número para uma string
	std::string strNum = std::to_string( num );

	// Itera sobre os dígitos do número e simula a entrada no teclado
	for ( char & c : strNum ) {
		int keyCode = c - '0'; // Converte o caractere para o código da tecla numérica correspondente
		keybd_event( 0x30 + keyCode , 0 , 0 , 0 ); // Pressiona a tecla correspondente ao dígito
		keybd_event( 0x30 + keyCode , 0 , KEYEVENTF_KEYUP , 0 ); // Libera a tecla correspondente ao dígito
	}
}

void SetAndClick( POINT pos ) {
	Sleep( 800 );
	SetCursorPos( pos.x , pos.y );
	Sleep( 800 );
	MouseClick( );
}

void BetManager::DoBet( ) {
	POINT colorpos = { 0,0 };

	switch ( this->CurrentPrediction.GetColor( ) )
	{
	case Red:
		colorpos = this->RedPos;
		break;
	case Blue:
		colorpos = this->BlackPos;
		break;
	case White:
		colorpos = this->WhitePos;
		break;
	default:
		return;
		break;
	}

	SetAndClick( colorpos );
	SetAndClick( this->InputPos );
	Sleep( 1000 );
	typeNumber( this->CurrentPrediction.GetBetAmount( ) );
	Sleep( 1000 );
	SetAndClick( this->StartBetPos );
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

