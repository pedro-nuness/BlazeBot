#include "BetManagement.h"


#include <fstream>
#include <iostream>
#include <WinInet.h>
#include "..\Utils\Config\Config.h"
#include "..\Utils\Utils.h"
#pragma comment(lib , "wininet.lib")


using namespace std;

extern std::string filename;
extern std::string Folder;
extern std::string JSName;
extern std::string HistoryName;
extern std::string ImportName;

extern void save_cfg( bool read );

bool Exists( const std::string & name ) {
	return std::filesystem::exists( name );
}

bool Bigger( int a , int b )
{
	return a > b;
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

	Color c = ( Color ) play[ "color" ];

	predictor->addColor( play );

	std::cout << "Add: " << "c:" << c << ", r: " << play[ "roll" ] << ", id: " << play[ "server_seed" ] << std::endl;
	LastColorAddTime = std::chrono::high_resolution_clock::now( );
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
	return this->CurrentPlayer.Bets;
}

Bet BetManager::GetCurrentPrediction( ) {
	return this->CurrentBet;
}

Bet BetManager::GetLastPrediction( ) {
	return this->LastBet;
}

bool BetManager::OnGameMode( bool * set ) {

	if ( set != nullptr )
		GameMode = *set;

	return GameMode;
}



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
	js[ "balancehistory" ] = bet->CurrentPlayer.BalanceHistory;

	std::ofstream arquivo( Folder + ImportName , std::ios::trunc );

	if ( arquivo.is_open( ) ) {
		arquivo << js;
	}
	else {
		std::cout << "Erro ao abrir o arquivo." << std::endl;
	}
}

void BetManager::StartGameSimulation( BetManager TrueBetManager ) {

	//Settup bets vec
	for ( int i = 0; i < NONE; i++ )
		SeparatedBeats.emplace_back( Beats( i ) );

	std::string LastSedd = "";

	while ( true ) {

		while ( !cfg::Get( ).Windows.SimulationGraph )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
		}

		std::string Seed;

		if ( LastSedd == "" )
			Seed = TrueBetManager.predictor->GetHistory( )[ TrueBetManager.predictor->GetHistory( ).size( ) - 1 ].GetServerID( );
		else {
			Seed = LastSedd;
		}

		std::vector<ColorManagement> Game = Utils::Get( ).GetNodeOutput( Seed , 30000 );

		LastSedd = Game[ 0 ].GetServerID( );

		std::vector<ColorManagement> Data;
		if ( this->predictor->GetHistory( ).empty( ) ) {
			Data = Utils::Get( ).GetNodeOutput( LastSedd , 20000 );
			LastSedd = Data[ 0 ].GetServerID( );
		}

		CurrentPlayer = Player( TrueBetManager.CurrentPlayer.GetInitialMoney( ) );

		SimulateGame( Game , Data );

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
	}
}

void BetManager::SetupBet( Bet bet , Player * player , ColorManagement NextResult )
{
	if ( bet.DidBet( ) )
	{
		Bet bet_push;
		bet_push = bet;
		bet_push.SetCorrect( PREDICTION );

		if ( bet.GetColor( ) == NextResult.GetColor( ) )
		{
			bet_push.SetCorrect( WON );
		}
		else {
			bet_push.SetCorrect( LOSE );
		}

		if ( bet.GetWhiteBet( ) )
		{
			if ( NextResult.GetColor( ) == White ) {
				bet_push.GotWhite = true;
				float WhiteProfit = bet.GetWhiteBet( ) * 14;
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Hit white" , 3000 ) );

				player->IncreaseBalance( WhiteProfit );

				if ( WhiteProfit > bet.GetBetAmount( ) * 2 ) {
					bet_push.SetCorrect( WON );
				}
				else
					cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "White proffit can't cover bet" , 3000 ) );
			}
			else {
				player->DecreaseBalance( bet.GetWhiteBet( ) );
			}
		}
		else if ( NextResult.GetColor( ) == White )
		{
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Missed white" , 3000 ) );
		}

		//We did a bet
		if ( bet.GetColor( ) == NextResult.GetColor( ) )
		{
			player->IncreaseBalance( bet.GetBetAmount( ) );
		}
		else {
			player->DecreaseBalance( bet.GetBetAmount( ) );

		}

		player->Bets.push_back( bet_push );
		player->FullBets.emplace_back( bet_push );
	}
}


void BetManager::SimulateGame( std::vector<ColorManagement> GameHistory , std::vector<ColorManagement> Data )
{
	if ( this->predictor->GetHistory( ).empty( ) ) {
		for ( auto Play : Data )
		{
			addColor( Play.GetJson( ) );
		}
	}

	std::vector<Bet> SimulatedBets;

	for ( int i = 0; i < GameHistory.size( ) - 2; i++ )
	{
		while ( !cfg::Get( ).Windows.SimulationGraph )
		{
			std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
		}

		while ( GetAsyncKeyState( VK_SPACE ) )
		{
			std::this_thread::sleep_for( std::chrono::seconds( 5 ) );
		}

		RequiredPlays++;

		auto Play = GameHistory[ i ];

		addColor( Play.GetJson( ) );

		//Predict && DoBet
		auto Prediction = predictor->predictNext( );
		Bet RawBet;
		auto NextBet = PredictBets( Prediction , &RawBet );

		SetCurrentPrediction( NextBet );

		auto NextResult = GameHistory[ i + 1 ];

		if ( !predictor->SeparatedPrediction.empty( ) ) {

			for ( int i = 0; i < NONE; i++ )
			{
				auto prediction = predictor->SeparatedPrediction[ i ];

				SeparatedBeats[ i ].SetupBeat( prediction , NextResult.GetColor( ) );
			}

			this->predictor->SeparatedBeats = this->SeparatedBeats;
		}

		SetupBet( this->CurrentBet , &CurrentPlayer , NextResult );
		SetupBet( RawBet , &RawPlayer , NextResult );
	}
}

void BetManager::ManageBets( ) {

	int last_history_size = -1;

	bool pressed_del = false;
	bool pressed_home = false;

	bool PressedNum1 = false;
	bool PressedNum2 = false;
	bool PressedNum3 = false;
	bool PressedNum4 = false;
	bool PressedNum5 = false;

	while ( true )
	{
		if ( StartBets )
		{
			while ( CurrentPlayer.BalanceHistory.size( ) > 300 )
			{
				CurrentPlayer.BalanceHistory.erase( CurrentPlayer.BalanceHistory.begin( ) , CurrentPlayer.BalanceHistory.begin( ) + 1 );
			}

			auto hist = predictor->GetHistory( );

			if ( last_history_size == -1 )
				last_history_size = hist.size( );

			if ( hist.size( ) != last_history_size )
			{
				save_cfg( false );

				auto Prediction = predictor->predictNext( );
				auto NextBet = PredictBets( Prediction );
				SetCurrentPrediction( NextBet );

				std::cout << "Corrects: " << Corrects << std::endl;
				std::cout << "Wrongs: " << Wrongs << std::endl;

				ImportToBot( this );

				if ( cfg::Get( ).Betting.automatic.AutoBet && NextBet.DidBet( ) )
				{
					DoBet( );
				}

				last_history_size = hist.size( );
			}

			if ( GetAsyncKeyState( VK_NUMLOCK ) )
			{
				pressed_del = true;
			}
			else if ( pressed_del )
			{
				ClearData( );
				pressed_del = false;
			}

			if ( GetAsyncKeyState( VK_HOME ) && GetAsyncKeyState( VK_LSHIFT ) )
			{
				pressed_home = true;
			}
			else if ( pressed_home )
			{
				CurrentPlayer.FullBalanceHistory.clear( );
				pressed_home = false;
			}

			if ( GetKeyState( VK_NUMPAD1 ) )
			{
				PressedNum1 = true;
			}
			else if ( PressedNum1 )
			{
				PressedNum1 = false;
				SetCursorPos( this->RedPos.x , this->RedPos.y );
			}

			if ( GetKeyState( VK_NUMPAD2 ) )
			{
				PressedNum2 = true;
			}
			else if ( PressedNum2 )
			{
				PressedNum2 = false;
				SetCursorPos( this->BlackPos.x , this->BlackPos.y );
			}

			if ( GetKeyState( VK_NUMPAD3 ) )
			{
				PressedNum3 = true;
			}
			else if ( PressedNum3 )
			{
				PressedNum3 = false;
				SetCursorPos( this->WhitePos.x , this->WhitePos.y );
			}

			if ( GetKeyState( VK_NUMPAD4 ) )
			{
				PressedNum4 = true;
			}
			else if ( PressedNum4 )
			{
				PressedNum4 = false;
				SetCursorPos( this->InputPos.x , this->InputPos.y );
			}

			if ( GetKeyState( VK_NUMPAD5 ) )
			{
				PressedNum5 = true;
			}
			else if ( PressedNum5 )
			{
				PressedNum5 = false;
				SetCursorPos( this->StartBetPos.x , this->StartBetPos.y );
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
		SeparatedBeats.emplace_back( Beats( i ) );

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

			RequiredPlays++;

			APIHistory.emplace_back( ColorManagement( MostRecentPlay ) );
			predictor->LastColor = MostRecentPlay.GetColor( );


			if ( !predictor->SeparatedPrediction.empty( ) ) {

				for ( int i = 0; i < NONE; i++ )
				{
					auto prediction = predictor->SeparatedPrediction[ i ];

					SeparatedBeats[ i ].SetupBeat( prediction , MostRecentPlay.GetColor( ) );
				}

				this->predictor->SeparatedBeats = this->SeparatedBeats;
			}

			Bet bet_push;
			bet_push = this->CurrentBet;
			bet_push.SetCorrect( PREDICTION );

			bool push_bet = true;

			if ( this->CurrentBet.GetColor( ) != Null && this->CurrentBet.GetChance( ) )
			{
				if ( MostRecentPlay.GetColor( ) == this->CurrentBet.GetColor( ) )
					bet_push.SetCorrect( WON );
				else
					bet_push.SetCorrect( LOSE );

				if ( this->CurrentBet.GetPrediction( ).PossibleWhite
					&& this->CurrentBet.GetWhiteBet( ) && this->CurrentBet.DidBet( ) )
				{
					if ( MostRecentPlay.GetColor( ) == White ) {

						float WhiteProfit = this->CurrentBet.GetWhiteBet( ) * 14;

						CurrentPlayer.IncreaseBalance( WhiteProfit );
						if ( WhiteProfit > this->CurrentBet.GetBetAmount( ) * 2 ) {
							bet_push.SetCorrect( WON );
						}
						else {
							push_bet = false;
						}
					}
					else
						CurrentPlayer.DecreaseBalance( this->CurrentBet.GetWhiteBet( ) );
				}

				if ( MostRecentPlay.GetColor( ) == this->CurrentBet.GetColor( ) ) {

					Corrects++;
					if ( this->CurrentBet.GetChance( ) * 100 > 50 )
						SafeCorrects++;

					int losestreak = 0;

					for ( int i = CurrentPlayer.Bets.size( ) - 1; i >= 0; i-- )
					{
						auto bet = CurrentPlayer.Bets[ i ];

						if ( bet.GetBetResult( ) == LOSE )
						{
							losestreak++;
						}
						else
							break;
					}

					if ( losestreak && losestreak <= 3 ) // We just lost 2, small lose amount
					{
						Wrongs -= losestreak;
					}

					if ( this->CurrentBet.DidBet( ) ) {

						CurrentPlayer.IncreaseBalance( this->CurrentBet.GetBetAmount( ) );

						if ( push_bet ) {
							CurrentPlayer.Bets.push_back( bet_push );
							CurrentPlayer.FullBets.emplace_back( bet_push );
						}
					}
				}
				else {

					Wrongs++;
					if ( this->CurrentBet.GetChance( ) * 100 > 50 )
						SafeWrongs++;

					if ( this->CurrentBet.DidBet( ) ) {

						CurrentPlayer.DecreaseBalance( this->CurrentBet.GetBetAmount( ) );

						if ( push_bet ) {
							CurrentPlayer.Bets.push_back( bet_push );
							CurrentPlayer.FullBets.emplace_back( bet_push );
						}
					}
				}
			}

			cfg::Get( ).Game.InitialBalance = CurrentPlayer.GetInitialMoney( );
			cfg::Get( ).Game.CurrentBalance = CurrentPlayer.GetBalance( );
			cfg::Get( ).Game.BalanceHistory = CurrentPlayer.BalanceHistory;
			cfg::Get( ).Game.FullBalanceHistory = CurrentPlayer.FullBalanceHistory;

			addColor( GameColors.front( ) );
		}

		this->LastBet = this->CurrentBet;
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

int BET_ID = 0;

void BetManager::EndBet( bool Leave )
{
	json result;

	BET_ID++;
	if ( BET_ID > 1500 )
		BET_ID = 0;

	if ( cfg::Get( ).Betting.automatic.AutoBet )
		cfg::Get( ).Betting.automatic.AutoBet = false;

	result[ "balance" ] = CurrentPlayer.GetBalance( );
	result[ "initialbalance" ] = CurrentPlayer.GetInitialMoney( );
	result[ "profit" ] = CurrentPlayer.GetProfit( );
	result[ "exit" ] = Leave;
	result[ "id" ] = BET_ID;

	std::string result_str = result.dump( );

	Utils::Get( ).WriteData( "result_import.json" , result_str, false );

	json graph;
	graph[ "balance" ] = CurrentPlayer.BalanceHistory;
	graph[ "won" ] = result[ "profit" ] > 0;

	int ElapsedTime = 0;

	//Minutes, each play require 30 seconds, so 2 plays = 1 minute
	ElapsedTime = Utils::Get( ).aproximaFloat( ( RequiredPlays / 2.f ) / 60.f );

	RequiredPlays = 0;

	graph[ "timing" ] = ElapsedTime;

	std::string graph_str = graph.dump( );

	if ( CurrentPlayer.GetProfit( ) > 0 )
	{
		cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Added " + std::to_string( ( int ) ( CurrentPlayer.GetProfit( ) / 2 ) ) + " into the bank!" , 10000 ) );
		BankProfit += CurrentPlayer.GetProfit( ) / 2;
		CurrentPlayer.FinishBets( CurrentPlayer.GetInitialMoney( ) + ( CurrentPlayer.GetProfit( ) / 2 ) );
	}
	else
	{
		CurrentPlayer.FinishBets( CurrentPlayer.GetBalance( ) );
		if ( !CurrentPlayer.GetBalance( ) || CurrentPlayer.GetBalance( ) < 0 )
		{
			CurrentPlayer.FinishBets( CurrentPlayer.GetDepositedMoney( ) );
		}
	}

	Utils::Get( ).WriteData( "GameHistory.json" , graph_str , false );

	if ( !Leave ) {

		ClearData( );

		StartingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
	}
	else
		exit( 0 );
}


int aproximaFloat( float numero ) {
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

bool BetManager::NeedToWait( )
{
	bool * Wait = &WaitController.Wait;
	int * WaitStartingPosition = &WaitController.WaitStartPos;
	int * WaitAmount = &WaitController.WaitAmount;

	if ( cfg::Get( ).Betting.security.WaitIfLosing ) {

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

			for ( int i = CurrentPlayer.Bets.size( ) - 1; i > 0; i-- )
			{
				auto bet = CurrentPlayer.Bets.at( i );

				if ( bet.GetBetResult( ) == LOSE )
					LoseCount++;
				else
					break;
			}

			if ( LoseCount >= cfg::Get( ).Betting.security.WaitAfterXLose )
			{
				//Já esperamos 1
				*Wait = true;
				*WaitAmount = cfg::Get( ).Betting.security.WaitAmount - 1;
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


int Tolerance = 30;

bool verificaQuedaSemSubida( const std::vector<float> & valores ) {

	for ( size_t i = valores.size( ) - 2; i >= 0; i-- ) {

		if ( valores[ i ] + Tolerance < valores[ i - 1 ] && valores[ i + 1 ] + Tolerance < valores[ i - 1 ] )
			return true;

		return false;
	}

	return false;
}

int calcularTempoSubida( const std::vector<float> & valores , int start_pos = 1 ) {
	int contadorSubida = 0;

	for ( int i = valores.size( ) - start_pos; i > 0; i-- ) {

		if ( valores[ i ] + Tolerance > valores[ i - 1 ] || valores[ i ] + Tolerance > valores[ i - 2 ] ) {
			contadorSubida++;
		}
		else {

			if ( valores[ i + 1 ] + Tolerance > valores[ i - 1 ] )
				contadorSubida++;
			else
				break;
		}
	}

	return contadorSubida;
}


std::vector<float> filtrarPerdas( const std::vector<float> & grafico ) {
	std::vector<float> novoGrafico;

	// Verifica cada valor do gráfico e adiciona apenas os que não são perdas
	for ( int i = 0; i < grafico.size( ) - 1; i++ ) {
		if ( i == 0 ) {
			novoGrafico.push_back( grafico[ i ] );
		}
		else {
			if ( grafico[ i ] < grafico[ i + 1 ] )
			{
				if ( novoGrafico[ novoGrafico.size( ) - 1 ] != grafico[ i ] ) {
					novoGrafico.push_back( grafico[ i ] );
				}
				novoGrafico.push_back( grafico[ i + 1 ] );
			}
		}
	}

	return novoGrafico;
}


void BetManager::EndRawBet( ) {
	json result;


	result[ "balance" ] = RawPlayer.GetBalance( );
	result[ "initialbalance" ] = RawPlayer.GetInitialMoney( );
	result[ "profit" ] = RawPlayer.GetProfit( );
	result[ "exit" ] = false;

	std::string result_str = result.dump( );

	Utils::Get( ).WriteData( "Raw_result_import.json" , result_str , true );

	json graph;
	graph[ "balance" ] = RawPlayer.BalanceHistory;
	graph[ "won" ] = result[ "profit" ] > 0;

	int ElapsedTime = 0;

	//Minutes, each play require 30 seconds, so 2 plays = 1 minute
	ElapsedTime = Utils::Get( ).aproximaFloat( ( RequiredPlays / 2.f ) / 60.f );

	graph[ "timing" ] = ElapsedTime;

	std::string graph_str = graph.dump( );

	if ( RawPlayer.GetProfit( ) > 0 )
	{
		cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Added " + std::to_string( ( int ) ( CurrentPlayer.GetProfit( ) / 2 ) ) + " into the bank!" , 10000 ) );
		RawPlayer.FinishBets( RawPlayer.GetInitialMoney( ) + ( RawPlayer.GetProfit( ) / 2 ) );
	}
	else
	{
		RawPlayer.FinishBets( RawPlayer.GetBalance( ) );
		if ( !RawPlayer.GetBalance( ) || RawPlayer.GetBalance( ) <= 10 )
		{
			RawPlayer.FinishBets( CurrentPlayer.GetDepositedMoney( ) );
		}
	}

	Utils::Get( ).WriteData( "Raw_GameHistory.json" , graph_str , false );

	ClearRawData( );
}


Bet BetManager::Martingalle( Prediction predict , Player player , NextBetManagement * Control )
{
	Bet NextBet;
	NextBet = Bet( predict , PREDICTION , MINIMUM ); //Default Allocator

	auto cBets = player.Bets;

	float CurrentBalance = player.GetBalance( );
	float InitialBalance = player.GetInitialMoney( );
	float DepositedBalance = player.GetDepositedMoney( );
	float Profit = player.GetProfit( );
	float CurrentBet = 0.f;
	float WhiteBet = 0.f;
	float MaximumBet = ( InitialBalance * ( cfg::Get( ).Betting.MaxBetPercentage / 100.f ) );
	float MinimumBet = 2;

	if ( InitialBalance < DepositedBalance )
	{
		MinimumBet = ( InitialBalance * ( cfg::Get( ).Betting.MinBetPercentage / 100.f ) );
	}
	else {
		MinimumBet = ( DepositedBalance * ( cfg::Get( ).Betting.MinBetPercentage / 100.f ) );
	}

	int TargetPercentage = int( InitialBalance * ( cfg::Get( ).Betting.TargetParcentage / 100 ) );
	int StopPercentage = int( InitialBalance * ( ( cfg::Get( ).Betting.StopPercentage ) / 100 ) );


	if ( Profit <= -( StopPercentage ) || Profit >= ( TargetPercentage ) )
	{
		if ( Profit > 0 )
		{
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Finished betting, profitted!" , 3000 ) );
		}
		else if ( Profit < 0 ) {
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Finished betting, non profit!" , 3000 ) );
		}
		else {
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Finished betting, neutral result!" , 3000 ) );
		}

		EndRawBet( );
		Control->Reset( );
		return NextBet;
	}

	if ( cBets.empty( ) && ( predict.color != Null ) ) {

		NextBet.SetMethod( MINIMUM );

		if ( predict.color != Null )
			CurrentBet = MinimumBet;

		if ( predict.PossibleWhite ) {

			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
			CurrentBet += WhiteBet;
			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
		}

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
			if ( predict.color != Null ) {

				int CurrentLoseStreak = 0;

				if ( !cBets.empty( ) ) {

					for ( int i = cBets.size( ) - 1; i >= 0; i-- )
					{
						auto bBet = cBets[ i ];

						if ( bBet.GetBetResult( ) == LOSE )
						{
							CurrentLoseStreak++;
						}
						else {
							break;
						}
					}
				}

				auto & LastBet = cBets.at( cBets.size( ) - 1 );

				float WinMultipliedValue = ( LastBet.GetBetAmount( ) * ( cfg::Get( ).Betting.type[ WON ].BetMultiplier / 100 ) );
				float LoseMultipliedValue = ( LastBet.GetBetAmount( ) * ( cfg::Get( ).Betting.type[ LOSE ].BetMultiplier / 100 ) );

				if ( WinMultipliedValue >= 1.0f && cfg::Get( ).Betting.type[ WON ].BetMultiplier >= 1.0f )
					WinMultipliedValue = Utils::Get( ).aproximaFloat( WinMultipliedValue );
				if ( LoseMultipliedValue >= 1.0f && cfg::Get( ).Betting.type[ LOSE ].BetMultiplier >= 1.0f )
					LoseMultipliedValue = Utils::Get( ).aproximaFloat( LoseMultipliedValue );

				if ( cfg::Get( ).Betting.type[ WON ].IncrementMinimum )
					WinMultipliedValue += MinimumBet;
				if ( cfg::Get( ).Betting.type[ LOSE ].IncrementMinimum )
					LoseMultipliedValue += MinimumBet;

				if ( LastBet.GetBetResult( ) == LOSE ) //We lose the LastBet
				{
					//Se a quantidade de multiplicador de ganho for maior que zero (Multiplicamos a aposta na vitoria)
					if ( Control->MultiplierWinCount > 0 ) {
						if ( cfg::Get( ).Betting.type[ WON ].ResetIfDifferent ) //Se estiver ligado, resete, senao, remova 1
						{
							Control->MultiplierWinCount = 0;
						}
						else
							Control->MultiplierWinCount--;
					}

					//Se quisermos multiplicar a aposta na perda
					if ( cfg::Get( ).Betting.type[ LOSE ].MultiplyBet )
					{
						if ( cfg::Get( ).Betting.security.ProtectProfit )
						{
							int PeakProfit = this->CurrentPlayer.GetPeakBalance( ) - this->CurrentPlayer.GetInitialMoney( );
							float ProtectPercentage = InitialBalance * ( cfg::Get( ).Betting.security.ProtectIfProfit / 100 );

							if ( PeakProfit > ProtectPercentage )
							{
								float MaximumMultiplier = PeakProfit * ( cfg::Get( ).Betting.security.ProfitProtectPercentage / 100 );

								if ( ( Profit - LoseMultipliedValue ) <= MaximumMultiplier )
								{
									cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Identified risky situation!" , 3000 ) );
									EndRawBet( );
									Control->Reset( );
									return NextBet;
								}
							}
						}

						if ( cfg::Get( ).Betting.security.ProtectCapital )
						{
							int ProtectPercentage = InitialBalance * ( cfg::Get( ).Betting.security.CapitalProtectPercentage / 100 );

							if ( CurrentBalance - LoseMultipliedValue < ProtectPercentage )
							{
								cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Identified risky situation!" , 3000 ) );
								LoseMultipliedValue = MinimumBet;
							}
						}

						if ( CurrentLoseStreak >= cfg::Get( ).Betting.type[ LOSE ].MultiplyAfterX )
						{
							Control->MultiplierLoseCount++;

							if ( Control->MultiplierLoseCount <= cfg::Get( ).Betting.type[ LOSE ].MaxMultiplierTimes )
							{
								CurrentBet = maths::nClamp( LoseMultipliedValue , MinimumBet , MaximumBet );
							}
						}
					}
				}
				else if ( LastBet.GetBetResult( ) == WON )
				{
					if ( Control->MultiplierLoseCount > 0 ) {

						if ( cfg::Get( ).Betting.type[ LOSE ].ResetIfDifferent )
						{
							Control->MultiplierLoseCount = 0;
						}
						else
							Control->MultiplierLoseCount--;
					}

					if ( cfg::Get( ).Betting.type[ WON ].MultiplyBet ) {

						float WinStreak = 0;

						for ( int i = cBets.size( ) - 1; i > 0; i-- )
						{
							auto Bet = cBets.at( i );

							if ( Bet.GetBetResult( ) == WON )
							{
								WinStreak++;
							}
							else {
								break;
							}

						}

						if ( WinStreak >= cfg::Get( ).Betting.type[ WON ].MultiplyAfterX )
						{
							Control->MultiplierWinCount++;

							if ( Control->MultiplierWinCount <= cfg::Get( ).Betting.type[ WON ].MaxMultiplierTimes )
							{
								CurrentBet = maths::nClamp( WinMultipliedValue , MinimumBet , MaximumBet );
							}
						}
					}

					NextBet.SetMethod( FIBONACCI );
				}

				if ( !CurrentBet || CurrentBet < MinimumBet )
					CurrentBet = MinimumBet;


				if ( CurrentLoseStreak <= 1 && CurrentBet >= 10 && LastBet.GetBetResult( ) == LOSE )
				{
					cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Anomaly detected on bet system!" , 3000 ) );
				}

				if ( CurrentBet > MinimumBet && LastBet.GetBetResult( ) == WON )
				{
					cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Anomaly detected on bet system!" , 3000 ) );
				}

				if ( predict.PossibleWhite ) {
					WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
					CurrentBet += WhiteBet;
					WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
				}

				NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );

			}
		}
	}

	return NextBet;
}

int BetManager::GetCurrentLoseStreak( std::vector<Bet> bet )
{
	int streak = 0;

	if ( !bet.empty( ) ) {

		for ( int i = bet.size( ) - 1; i >= 0; i-- )
		{
			auto bBet = bet[ i ];

			if ( bBet.GetBetResult( ) == LOSE )
			{
				streak++;
			}
			else {
				break;
			}
		}
	}

	return streak;
}

Bet BetManager::PredictBets( Prediction predict , Bet * RawBet ) {

	Bet NextBet;
	NextBet = Bet( predict , PREDICTION , MINIMUM ); //Default Allocator

	if ( !CurrentPlayer.IsSetupped( ) )
		return NextBet;

	if ( !CurrentPlayer.IsPeakSettuped )
	{
		CurrentPlayer.SettupPeak( CurrentPlayer.BalanceHistory );
	}

	static int OldSize = -1;

	float CurrentBalance = CurrentPlayer.GetBalance( );
	float InitialBalance = CurrentPlayer.GetInitialMoney( );
	float DepositedBalance = CurrentPlayer.GetDepositedMoney( );
	float PeakBalance = CurrentPlayer.GetPeakBalance( );
	float Profit = CurrentPlayer.GetProfit( );
	float MinimumPercentage = cfg::Get( ).Betting.MinBetPercentage / 100;
	float CurrentBet = 0.f;
	float WhiteBet = 0.f;
	float MaximumBet = ( InitialBalance * ( cfg::Get( ).Betting.MaxBetPercentage / 100.f ) );
	float MinimumBet = 0.0f;

	if ( InitialBalance < DepositedBalance )
	{
		MinimumBet = ( InitialBalance * ( cfg::Get( ).Betting.MinBetPercentage / 100.f ) );
	}
	else {
		MinimumBet = ( DepositedBalance * ( cfg::Get( ).Betting.MinBetPercentage / 100.f ) );
	}

	auto FullBalanceHistory = CurrentPlayer.FullBalanceHistory;
	auto BalanceHistory = CurrentPlayer.BalanceHistory;

	static NextBetManagement Controls;
	static NextBetManagement RawControls;

	auto bets = CurrentPlayer.Bets;
	auto complete_bets = CurrentPlayer.FullBets;

	if ( RawBet != nullptr )
	{
		if ( !RawPlayer.IsSetupped( ) )
		{
			RawPlayer = Player( InitialBalance );
		}

		*RawBet = Martingalle( predict , RawPlayer , &RawControls );
	}

	static bool OnRecovery = false;
	static int LoseCount = 0;
	int TargetPercentage = int( InitialBalance * ( cfg::Get( ).Betting.TargetParcentage / 100 ) );
	int StopPercentage = int( InitialBalance * ( ( cfg::Get( ).Betting.StopPercentage ) / 100 ) );

	static bool Waited = false;

	if ( StartingBetTime == ( std::chrono::high_resolution_clock::time_point::min ) ( ) ) {
		StartingBetTime = std::chrono::high_resolution_clock::now( ); // Salva o tempo atual
	}


	int FullLoseStreak = GetCurrentLoseStreak( complete_bets );
	int CurrentLoseStreak = GetCurrentLoseStreak(bets);

	static auto FinishBet = [& ] ( )->void {
		EndBet( );
		Controls.Reset( );
	};


	if ( Profit <= -( StopPercentage ) || Profit >= ( TargetPercentage ) )
	{
		if ( Profit > 0 )
		{
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Finished betting, profitted!" , 3000 ) );
		}
		else if ( Profit < 0 ) {
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Finished betting, non profit!" , 3000 ) );
		}
		else {
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Finished betting, neutral result!" , 3000 ) );
		}
		FinishBet( );
		return NextBet;
	}


	if ( FullBalanceHistory.size( ) > 30 ) {

		std::vector<float> FilteredGraph = filtrarPerdas( FullBalanceHistory );

		int TempoSubida = calcularTempoSubida( FilteredGraph );

		if ( TempoSubida >= 30 )
		{
			if ( CurrentLoseStreak > 4 )
			{
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Possible pattern breaker!" , 3000 ) );
				FinishBet( );
				return NextBet;
			}
			else if ( CurrentLoseStreak > 3 ) {

				cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Possible pattern breaker!" , 3000 ) );

				if ( Profit > 0 )
				{
					FinishBet( );
					return NextBet;
				}
			}
		}

		if ( OldSize != FilteredGraph.size( ) ) {
			OldSize = FilteredGraph.size( );

			if ( verificaQuedaSemSubida( FilteredGraph ) )
			{
				//Aconteceu uma queda, e nao subiu, vamos guardar a distancia da subida

				cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Couldn't prevent down peak!" , 3000 ) );

				int TempoSubidaAnterior = calcularTempoSubida( FilteredGraph , 3 );
				json new_js;
				new_js[ "size" ] = TempoSubidaAnterior;

				Utils::Get( ).WriteData( "Ups.json" , new_js.dump( ) , false );
			}
			else
			{
				static int LastSubida = -1;

				if ( LastSubida != TempoSubida )
				{
					if ( maths::fDelta( LastSubida , TempoSubida ) >= 10 ) {
						cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Up hills reached " + std::to_string( ( int ) TempoSubida ) , 3000 ) );
						LastSubida = TempoSubida;
					}
				}
			}
		}

		int BalanceSize = FullBalanceHistory.size( );
		int DifferenceFromLastBalance = FullBalanceHistory[ BalanceSize - 2 ] - FullBalanceHistory[ BalanceSize - 1 ];
		float Tolerance = InitialBalance * ( cfg::Get( ).Betting.security.MinimumPeakValue / 100 );

		if ( DifferenceFromLastBalance < 0 && fabs( DifferenceFromLastBalance >= Tolerance ) )
		{
			cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Recovered from a down peak!" , 3000 ) );

			//We just Recovered from a down peak;
			//Let's check if there's others down peaks near this one

			std::vector<int> PeakDownPositions;

			for ( int i = FullBalanceHistory.size( ) - 1; i > 1; i-- )
			{
				int Difference = FullBalanceHistory[ i - 1 ] - FullBalanceHistory[ i ];
				if ( ( fabs( Difference ) > Tolerance ) && Difference < 0 )
				{
					PeakDownPositions.emplace_back( i - 1 );
				}
			}

			if ( PeakDownPositions.size( ) >= 2 )
			{
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Found multiples down peaks!" , 3000 ) );

				int DistanceBetweenPeaks = fabs( PeakDownPositions[ 0 ] - PeakDownPositions[ 1 ] );

				//They are near?
				if ( DistanceBetweenPeaks <= cfg::Get( ).Betting.security.MinimumPeakDistance )
				{
					cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Found near down peak!" , 3000 ) );

					FinishBet( );

					/*WAIT SYSTEM*/
					//if ( StartWaitingTime == ( std::chrono::high_resolution_clock::time_point::min ) ( ) ) {
					//	StartWaitingTime = std::chrono::high_resolution_clock::now( );
					//	//Let's wait
					//}
					//else if ( !Waited ) {
					//	//We're already waiting
					//	float ElapsedTime = std::chrono::duration_cast< std::chrono::minutes >( std::chrono::high_resolution_clock::now( ) - StartWaitingTime ).count( );
					//
					//	if ( ElapsedTime >= cfg::Get( ).Betting.security.WaitingTime )
					//	{
					//		StartWaitingTime = ( std::chrono::high_resolution_clock::time_point::min ) ( );
					//		Waited = true;
					//	}
					//	else {
					//		NextBet.SetMethod( STANDBY );
					//		return NextBet;
					//	}
					//}

				}
			}
		}
	}

	bool RealBet = true;

	if ( bets.empty( ) && ( predict.color != Null ) ) {

		NextBet.SetMethod( MINIMUM );

		if ( predict.color != Null )
			CurrentBet = MinimumBet;

		if ( predict.PossibleWhite ) {

			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
			CurrentBet += WhiteBet;
			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 20.f ) );
		}

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
			if ( cfg::Get( ).Betting.security.PlayOnlyOnStableMoments )
			{
				if ( !SeparatedBeats[ GENERAL ].StableMoment( ) )
				{
					NextBet.SetMethod( STANDBY );
					return NextBet;
				}
			}

			if ( cfg::Get( ).Betting.security.PredictDownPeaks )
			{
				if ( complete_bets.size( ) > 30 ) {

					static int WaitCount = 0;

					if ( FullLoseStreak ) {

						int LoseStreak = 0;

						int max_lose_streak = 0;

						float MediumLose = 0.0f;
						float Rolls = 0;

						for ( int i = complete_bets.size( ) - FullLoseStreak; i > complete_bets.size( ) - ( 30 + FullLoseStreak ); i-- )
						{
							if ( complete_bets[ i ].GetBetResult( ) == LOSE )
								LoseStreak++;
							else {
								if ( LoseStreak > max_lose_streak ) {
									max_lose_streak = LoseStreak;
								}

								MediumLose += LoseStreak;

								Rolls++;

								LoseStreak = 0;
							}
						}

						MediumLose = Utils::Get( ).aproximaFloat( MediumLose / Rolls );
						max_lose_streak = maths::nClamp( max_lose_streak , 1 , 5 );

						if ( FullLoseStreak ) {

							int MaxStreakOnThisBet = 0;
							int CurrRoll = 0;

							for ( int i = 0; i < bets.size( ) - 1; i++ )
							{
								if ( bets[ i ].GetBetResult( ) == LOSE )
									CurrRoll++;
								else {
									if ( CurrRoll > MaxStreakOnThisBet )
										MaxStreakOnThisBet = CurrRoll;

									CurrRoll = 0;
								}
							}

							//if ( ( Profit >= InitialBalance * 0.02 ) && FullLoseStreak >= MaxStreakOnThisBet ) {
							//	cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Ending betting due to prediction" , 3000 ) );
							//	EndBet( );
							//	return NextBet;
							//}

							if ( CurrentLoseStreak > MediumLose ) {

								std::cout << "[SECURITY]: Predicting down peak!\n";

								if ( !SeparatedBeats[ GENERAL ].OnBetPattern( ) ) {
									NextBet.SetMethod( STANDBY );
									return NextBet;
								}
								else {
									cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Found pattern" , 3000 ) );
									predict.PossibleWhite = true;
								}
							}
						}
					}
				}
			}

			if ( cfg::Get( ).Betting.security.RecoveryModeIfDownPeak )
			{
				if ( CurrentBalance >= InitialBalance && OnRecovery )
				{
					cfg::Get( ).Game.Notifications.emplace_back( Notification( Success , "Sucessfully recovered capital!" , 3000 ) );
					FinishBet( );
					OnRecovery = false;
					return NextBet;
				}

				if ( CurrentPlayer.GetLowestMoney( ) <= ( InitialBalance * ( cfg::Get( ).Betting.security.DownPercentage / 100 ) ) )
				{
					/*TODO: Recode recovery mode*/
				}
			}

			if ( NeedToWait( ) )
			{
				NextBet.SetMethod( STANDBY );
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Info , "Currently on wait mode!" , 3000 ) );
				return NextBet;
			}

			auto & LastBet = bets.at( bets.size( ) - 1 );

			float WinMultipliedValue = ( LastBet.GetBetAmount( ) * ( cfg::Get( ).Betting.type[ WON ].BetMultiplier / 100.f ) );
			float LoseMultipliedValue = ( LastBet.GetBetAmount( ) * ( cfg::Get( ).Betting.type[ LOSE ].BetMultiplier / 100.f ) );

			if ( WinMultipliedValue >= 1.0f && cfg::Get( ).Betting.type[ WON ].BetMultiplier >= 1.0f )
				WinMultipliedValue = Utils::Get( ).aproximaFloat( WinMultipliedValue );
			if ( LoseMultipliedValue >= 1.0f && cfg::Get( ).Betting.type[ LOSE ].BetMultiplier >= 1.0f )
				LoseMultipliedValue = Utils::Get( ).aproximaFloat( LoseMultipliedValue );

			if ( cfg::Get( ).Betting.type[ WON ].IncrementMinimum )
				WinMultipliedValue += MinimumBet;
			if ( cfg::Get( ).Betting.type[ LOSE ].IncrementMinimum )
				LoseMultipliedValue += MinimumBet;

			if ( CurrentBalance != InitialBalance )
			{
				if ( CurrentBalance + MinimumBet >= PeakBalance )
				{
					//Current Max
					LoseMultipliedValue = MinimumBet;
					WinMultipliedValue = MinimumBet;
					CurrentBet = MinimumBet;
				}
			}

			if ( LastBet.GetBetResult( ) == LOSE ) //We lose the LastBet
			{
				if ( WaitController.Wait )
				{
					LoseCount++;
					if ( cfg::Get( ).Betting.security.WaitAgainIfLose ) {
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
				if ( Controls.MultiplierWinCount > 0 ) {
					if ( cfg::Get( ).Betting.type[ WON ].ResetIfDifferent ) //Se estiver ligado, resete, senao, remova 1
					{
						Controls.MultiplierWinCount = 0;
					}
					else
						Controls.MultiplierWinCount--;
				}

				//Se quisermos multiplicar a aposta na perda
				if ( cfg::Get( ).Betting.type[ LOSE ].MultiplyBet )
				{
					if ( cfg::Get( ).Betting.security.ProtectProfit )
					{
						int PeakProfit = this->CurrentPlayer.GetPeakBalance( ) - this->CurrentPlayer.GetInitialMoney( );
						float ProtectPercentage = InitialBalance * ( cfg::Get( ).Betting.security.ProtectIfProfit / 100 );

						if ( PeakProfit > ProtectPercentage )
						{
							float MaximumMultiplier = PeakProfit * ( cfg::Get( ).Betting.security.ProfitProtectPercentage / 100 );

							if ( ( Profit - LoseMultipliedValue ) <= MaximumMultiplier )
							{
								cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Identified risky situation!" , 3000 ) );
								FinishBet( );
								return NextBet;
							}
						}
					}

					if ( cfg::Get( ).Betting.security.ProtectCapital )
					{
						int ProtectPercentage = InitialBalance * ( cfg::Get( ).Betting.security.CapitalProtectPercentage / 100 );

						if ( CurrentBalance - LoseMultipliedValue < ProtectPercentage )
						{
							cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Identified risky situation!" , 3000 ) );
							LoseMultipliedValue = MinimumBet;
						}
					}

					if ( CurrentLoseStreak >= cfg::Get( ).Betting.type[ LOSE ].MultiplyAfterX )
					{
						Controls.MultiplierLoseCount++;

						if ( Controls.MultiplierLoseCount <= cfg::Get( ).Betting.type[ LOSE ].MaxMultiplierTimes )
						{
							CurrentBet = maths::nClamp( LoseMultipliedValue , MinimumBet , MaximumBet );
						}
					}
				}
			}
			else if ( LastBet.GetBetResult( ) == WON )
			{
				if ( cfg::Get( ).Betting.security.ResetCountIfWin )
				{
					ResetWaitControler( );
				}
				else {

					if ( WaitController.WaitAmount >= 2 )
						WaitController.WaitAmount--;
				}

				if ( Controls.MultiplierLoseCount > 0 ) {

					if ( cfg::Get( ).Betting.type[ LOSE ].ResetIfDifferent )
					{
						Controls.MultiplierLoseCount = 0;
					}
					else
						Controls.MultiplierLoseCount--;
				}

				if ( cfg::Get( ).Betting.type[ WON ].MultiplyBet ) {

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

					if ( WinStreak >= cfg::Get( ).Betting.type[ WON ].MultiplyAfterX )
					{
						Controls.MultiplierWinCount++;

						if ( Controls.MultiplierWinCount <= cfg::Get( ).Betting.type[ WON ].MaxMultiplierTimes )
						{
							CurrentBet = maths::nClamp( WinMultipliedValue , MinimumBet , MaximumBet );
						}
					}
				}

				NextBet.SetMethod( FIBONACCI );
			}

			if ( !CurrentBet || CurrentBet < MinimumBet )
				CurrentBet = MinimumBet;


			if ( CurrentLoseStreak <= 1 && CurrentBet >= 10 && LastBet.GetBetResult( ) == LOSE )
			{
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Anomaly detected on bet system!" , 3000 ) );
			}

			if ( CurrentBet > MinimumBet + WhiteBet && LastBet.GetBetResult( ) == WON )
			{
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Warning , "Anomaly detected on bet system!" , 3000 ) );
			}
		}

		Waited = false;

		if ( predict.PossibleWhite ) {
			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 10.f ) );
			CurrentBet += WhiteBet;
			WhiteBet = aproximaFloat( maths::nClamp( ( CurrentBet / 14 ) * 2 , MinimumBet / 2 , 20.f ) );
		}

		if ( CurrentBalance >= PeakBalance )
		{
			if ( CurrentBet > MinimumBet + WhiteBet || WhiteBet > CurrentBet * 0.7 )
			{
				cfg::Get( ).Game.Notifications.emplace_back( Notification( Error , "Anomaly detected on bet system!" , 3000 ) );
			}
		}

		NextBet.DoBet( CurrentBet , WhiteBet , CurrentBalance );
	}

	return NextBet;
}

void BetManager::ClearData( ) {
	this->CurrentPlayer.Reset( );
}

void BetManager::ClearRawData( ) {
	this->RawPlayer.Reset( );
}

void BetManager::SetCurrentPrediction( Bet  bet )
{
	this->CurrentBet = bet;
}

void MouseClick( )
{
	mouse_event( MOUSEEVENTF_LEFTDOWN , 0 , 0 , 0 , 0 );
	Sleep( 50 );
	mouse_event( MOUSEEVENTF_LEFTUP , 0 , 0 , 0 , 0 );
}

void SetAndClick( POINT pos ) {
	Sleep( 500 );
	SetCursorPos( pos.x , pos.y );
	Sleep( 500 );
	MouseClick( );
}

bool isDecimalNumber( const std::string & s ) {

	size_t pos = s.find( '.' );
	if ( pos == std::string::npos )
		return false;  // Nenhum ponto na string.

	// Verifique se as partes antes e depois do ponto são números.
	try {
		std::stod( s.substr( 0 , pos ) );
		double fracionario = std::stod( s.substr( pos + 1 ) );

		// Verifique se a parte fracionária é diferente de zero.
		if ( fracionario != 0 )
			return true;
		else
			return false;

	}
	catch ( const std::invalid_argument & ia ) {
		// Se a substring não é um número, stod lançará uma exceção.
		return false;
	}
}

void typeNumber( double num ) {
	// Converte o número para uma string
	std::string strNum = std::to_string( num );

	bool Decimal = isDecimalNumber( strNum );

	// Remove os zeros adicionais à direita
	strNum.erase( strNum.find_last_not_of( '0' ) + 1 , std::string::npos );

	if ( !Decimal )
		strNum.erase( std::remove( strNum.begin( ) , strNum.end( ) , '.' ) , strNum.end( ) );
	else
		// Substitui o ponto decimal por uma vírgula
		std::replace( strNum.begin( ) , strNum.end( ) , '.' , ',' );

	// Itera sobre os dígitos do número e simula a entrada no teclado
	for ( char & c : strNum ) {
		if ( c == ',' ) {
			keybd_event( VK_OEM_COMMA , 0 , 0 , 0 ); // Pressiona a tecla de vírgula
			keybd_event( VK_OEM_COMMA , 0 , KEYEVENTF_KEYUP , 0 ); // Libera a tecla de vírgula
		}
		else {
			int keyCode = c - '0'; // Converte o caractere para o código da tecla numérica correspondente
			keybd_event( 0x30 + keyCode , 0 , 0 , 0 ); // Pressiona a tecla correspondente ao dígito
			keybd_event( 0x30 + keyCode , 0 , KEYEVENTF_KEYUP , 0 ); // Libera a tecla correspondente ao dígito
		}
	}
}

int DelayPerAct = 200;

void BetManager::BetOnColor( Color c , float amount )
{
	POINT colorpos = { 0,0 };

	switch ( c )
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


	//Clica na cor
	SetAndClick( colorpos );

	//Coloca o valor da aposta
	SetAndClick( this->InputPos );
	std::this_thread::sleep_for( std::chrono::milliseconds( DelayPerAct ) );
	typeNumber( amount );

	//Aposta
	std::this_thread::sleep_for( std::chrono::milliseconds( DelayPerAct ) );
	SetAndClick( this->StartBetPos );
}

void BetManager::DoBet( ) {

	float ElapsedTime = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::high_resolution_clock::now( ) - LastColorAddTime ).count( );

	while ( ElapsedTime < 4.0 )
	{
		std::this_thread::sleep_for( std::chrono::milliseconds( DelayPerAct ) );
		ElapsedTime = std::chrono::duration_cast< std::chrono::seconds >( std::chrono::high_resolution_clock::now( ) - LastColorAddTime ).count( );
	}

	if ( this->CurrentBet.DidBet( ) )
	{
		BetOnColor( this->CurrentBet.GetColor( ) , this->CurrentBet.GetBetAmount( ) );

		if ( this->CurrentBet.GetWhiteBet( ) )
		{
			BetOnColor( White , this->CurrentBet.GetWhiteBet( ) );
		}
	}

	std::this_thread::sleep_for( std::chrono::milliseconds( DelayPerAct ) );
	SetCursorPos( 0 , 0 );

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

// Definições dos métodos

void BetManager::SetRedPosition( POINT pos ) {
	this->RedPos = pos;
}

void BetManager::SetWhitePosition( POINT pos ) {
	this->WhitePos = pos;
}

void BetManager::SetBlackPosition( POINT pos ) {
	this->BlackPos = pos;
}

void BetManager::SetInputPosition( POINT pos ) {
	this->InputPos = pos;
}

void BetManager::SetStartPosition( POINT pos ) {
	this->StartBetPos = pos;
}

POINT BetManager::GetRedPosition( ) {
	return this->RedPos;
}

POINT BetManager::GetWhitePosition( ) {
	return this->WhitePos;
}

POINT BetManager::GetBlackPosition( ) {
	return this->BlackPos;
}

POINT BetManager::GetInputPosition( ) {
	return this->InputPos;
}

POINT BetManager::GetStartPosition( ) {
	return this->StartBetPos;
}

DoublePredictor * BetManager::GetPredictor( ) {
	return this->predictor;
}

void BetManager::ResetWaitControler( ) {
	WaitController.Wait = false;
	WaitController.WaitAmount = 0;
	WaitController.WaitStartPos = 0;
}