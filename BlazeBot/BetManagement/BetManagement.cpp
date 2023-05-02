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
				last_history_size = hist.size( );

				std::cout << "Corrects: " << Corrects << std::endl;
				std::cout << "Wrongs: " << Wrongs << std::endl;

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

	for ( int i = content.size( ) - 1; i > - 1; i-- )
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
		SeparatedBeats.emplace_back( Beats( 0 , 0 ) );

	while ( true ) {

		if ( OnGameMode( nullptr ) )
			continue;

		std::vector<json> GameColors = blazeAPI( );

		if ( GameColors.empty( ) )
			continue;

		if ( !LastColorSetup.IsSetupped( ) ) {
			
			LastColorSetup = GameColors.front( );
			StartupNode( LastColorSetup.GetServerID( ) , 15000 , this );

			std::vector<json> DelayedColors = blazeAPI( );
			GameColors = DelayedColors;

			if ( !DelayedColors.empty( ) )
			{
				ColorManagement LastPlay = ColorManagement( LastColorSetup );

				bool Found = false;

				for ( int i = DelayedColors.size() - 1; i > -1; i-- )
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

			Bet pBetPush;
			pBetPush = this->LastPrediction;
			pBetPush.SetCorrect( STANDBY );
			bets_display.push_back( pBetPush );

			if ( !predictor->SeparatedPrediction.empty( ) ) {

				for ( int i = 0; i < NONE; i++ )
				{
					auto prediction = predictor->SeparatedPrediction[ i ];

					if ( prediction != Null ) {
						if ( prediction == MostRecentPlay.GetColor( ) )
							SeparatedBeats[ i ].beats++;
						else
							SeparatedBeats[ i ].misses++;
					}
				}
			}

			Bet bet_push;
			bet_push = this->LastPrediction;

			if ( this->LastPrediction.GetPrediction( ).PossibleWhite )
			{
				if ( MostRecentPlay.GetColor( ) == White )
					CurrentPlayer.IncreaseBalance( this->LastPrediction.GetWhiteBet( ) * 14 );
				else
					CurrentPlayer.DecreaseBalance( this->LastPrediction.GetWhiteBet( ) );
			}

			if ( this->LastPrediction.GetColor( ) != Null && this->LastPrediction.GetChance( ) )
			{
				if ( MostRecentPlay.GetColor( ) == this->LastPrediction.GetColor( ) ) {

					bet_push.SetCorrect( WON );

					Corrects++;
					if ( this->LastPrediction.GetChance( ) * 100 > 50 )
						SafeCorrects++;

					if ( this->LastPrediction.DidBet( ) ) {

						CurrentPlayer.IncreaseBalance( this->LastPrediction.GetBetAmount( ) );

						BalanceHistory.emplace_back( CurrentPlayer.GetBalance( ) );
						bets.push_back( bet_push );
					}
				}
				else {

					bet_push.SetCorrect( LOSE );

					Wrongs++;
					if ( this->LastPrediction.GetChance( ) * 100 > 50 )
						SafeWrongs++;

					if ( this->LastPrediction.DidBet( ) ) {

						CurrentPlayer.DecreaseBalance( this->LastPrediction.GetBetAmount( ) );
						BalanceHistory.emplace_back( CurrentPlayer.GetBalance( ) );
						bets.push_back( bet_push );
					}
				}

				bets_display.push_back( bet_push );
			}

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
				return false;
			}
			else {
				return true;
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




Bet BetManager::PredictBets( Prediction predict ) {


	Bet NextBet;
	NextBet = Bet( predict , PREDICTION , MINIMUM ); //Default Allocator

	if ( !CurrentPlayer.IsSetupped( ) )
		return NextBet;

	float CurrentBalance = CurrentPlayer.GetBalance( );
	float InitialBalance = CurrentPlayer.GetInitialMoney( );
	float Profit = CurrentPlayer.GetProfit( );
	float MinimumPercentage = g_globals.Betting.MinBetPercentage / 100;
	float CurrentBet = 0.f;
	float WhiteBet = 0.f;
	bool Betted = false;
	static int MultiplierLoseCount = 0;
	static int MultiplierWinCount = 0;
	static int LoseCount = 0;

	int TargetPercentage = int( CurrentPlayer.GetInitialMoney( ) * ( g_globals.Betting.TargetParcentage / 100 ) );
	int StopPercentage = int( CurrentPlayer.GetInitialMoney( ) * ( ( g_globals.Betting.StopPercentage ) / 100 ) );

	float MaximumBet = ( CurrentPlayer.GetInitialMoney( ) * ( g_globals.Betting.MaxBetPercentage / 100.f ) );
	float MinimumBet = clamp_value( CurrentPlayer.GetInitialMoney( ) * ( g_globals.Betting.MinBetPercentage / 100.f ) , 2.f , MaximumBet );

	if ( StartingBetTime == ( std::chrono::high_resolution_clock::time_point::min ) ( ) ) {
		StartingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
	}

	if ( Profit <= -( StopPercentage ) || Profit >= ( TargetPercentage ) )
	{
		PrintBet( this );
		return NextBet;
	}

	if ( predict.PossibleWhite )
		WhiteBet = MinimumBet / 2;

	if ( bets.empty( ) ) {

		NextBet.SetMethod( MINIMUM );
		CurrentBet = MinimumBet;

		NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );
		Betted = true;
	}
	else {

		if ( predict.color == Null )
		{
			NextBet.SetMethod( MINIMUM );
		}
		else {

			int ConsecutiveLoses = 0;

			for ( int i = bets.size( ) - 1; i > 0; i-- )
			{
				auto Bet = bets.at( i );

				if ( i == bets.size( ) - 1 )
				{
					if ( Bet.GetBetResult( ) != WON )
						break;
				}
				else {

					if ( Bet.GetBetResult( ) == LOSE )
					{
						ConsecutiveLoses++;
					}
					else {
						break;
					}
				}
			}

			if ( ConsecutiveLoses >= 3 )
			{
				auto CurrentGame = predictor->GetGameTransition( );
				std::string Output;

				for ( auto Col : CurrentGame.Colors )
				{
					switch ( Col )
					{
					case Red:
						Output += "Red ";
						break;
					case Blue:
						Output += "Black ";
						break;
					case White:
						Output += "White ";
						break;
					default:
						break;
					}
				}

				Output += "( " + std::to_string( ConsecutiveLoses ) + " Loses )\n";

				WriteData( Folder + "Loses.txt" , Output );
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
					if ( g_globals.Betting.LoseAgainIfLose ) {
						if ( LoseCount >= 2 )
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
						if ( MultiplierLoseCount <= g_globals.Betting.MaxMultiplierTimesOnLose )
						{
							CurrentBet = clamp_value( LoseMultipliedValue , MinimumBet , MaximumBet );
							MultiplierLoseCount++;
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
						if ( MultiplierWinCount <= g_globals.Betting.MaxMultiplierTimesOnWin )
						{
							CurrentBet = clamp_value( WinMultipliedValue , MinimumBet , MaximumBet );
							MultiplierWinCount++;
						}
					}
				}

				NextBet.SetMethod( FIBONACCI );
			}

			if ( !CurrentBet || CurrentBet < 2 )
				CurrentBet = MinimumBet;

			if ( NeedToWait( ) )
			{
				NextBet.SetMethod( STANDBY );
				return NextBet;
			}
		}

		NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );
		Betted = true;
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

