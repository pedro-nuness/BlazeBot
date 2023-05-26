#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx9.h"
#include "ImGui/imgui_impl_win32.h"
#include <d3dx9.h>
#include <d3d9.h>
#include <tchar.h>
#include <math.h>
#include <cmath>
#include <sstream>
#include <vector>
#include <iostream>
#include <numeric>
#include <thread>
#include <fstream>

#include "ImGui/imgui_notify.h"

#include "BetManagement/BetManagement.h"
#include "Predictor/Predictor.h"
#include "Colors/ColorManagement/ColorManagement.h"
#include "Utils/Utils.h"


#include "Utils/Bytes.h"
#include "Utils/Config/Config.h"

extern bool LoadCfg( std::string name );
extern bool SaveCFG( std::string name );


#pragma warning(disable:4996)

std::string filename = "Scan.json";
std::string Folder = "C:\\Blaze\\";
std::string IAPATH = "IA.pt";
std::string JSName = "js.js";
std::string HistoryName = "history.json";
std::string ImportName = "import.json";
std::string IAModel = "IA";
bool Stop = false;

DoublePredictor predictor;
DoublePredictor spredictor;
BetManager betManager( filename , &predictor );

BetManager SimulationbetManager( "Simulated" + filename , &spredictor );

Utils usefull;

static LPDIRECT3D9              g_pD3D = NULL;
static LPDIRECT3DDEVICE9        g_pd3dDevice = NULL;
static D3DPRESENT_PARAMETERS    g_d3dpp = {};

bool CreateDeviceD3D( HWND hWnd );
void CleanupDeviceD3D( );
void ResetDevice( );
LRESULT WINAPI WndProc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

static float Sin( void * , int i ) {
	return sinf( i * 0.1f );
}

static float Cos( void * , int i ) {
	return cosf( i * 0.1f );
}

static float Sqrrt( void * , int i ) {
	return sqrtf( i );
}

static float Saw( void * , int i ) {
	return ( i & 1 ) ? 1.0f : -1.0f;
}

static float Tan( void * , int i ) {
	return tanf( i * 0.1f );
}

static float Log2( void * , int i ) {
	return log2f( i * 0.1f );
}

ImVec2 sum_vec( ImVec2 a , ImVec2 b ) {
	return ImVec2 { a.x + b.x, a.y + b.y };
}

ImVec2 RemoveVec( ImVec2 a , ImVec2 b ) {
	return ImVec2 { a.x - b.x, a.y - b.y };
}

bool settuped = false;

void AddToString( std::string content , std::string * strptr )
{
	*strptr += content;
}

void GetSavedGame( ) {
	std::vector<std::string> content;

	std::ifstream arquivo( Folder + filename , std::ifstream::binary );// abre o arquivo para leitura

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

	json curjs;

	for ( auto c : content )
	{
		curjs = json::parse( c );

		betManager.addColor( curjs , false );
		betManager.APIHistory.emplace_back( ColorManagement( curjs ) );
	}
}

void SavePoint( std::string name , json & js , POINT pos )
{
	js[ name ][ "x" ] = pos.x;
	js[ name ][ "y" ] = pos.y;
}

POINT GetPoint( std::string name , json & js ) {
	return POINT( js[ name ][ "x" ] , js[ name ][ "y" ] );
}

POINT TransformVec( Vector2D vec ) {
	return POINT( vec.x , vec.y );
}

Vector2D TransformPoint( POINT p ) {
	return Vector2D( p.x , p.y );
}

void save_cfg( bool read ) {

	json cfg;

	std::string DIR = Folder + "config.json";
	static bool settuped_initial = false;

	if ( read ) {
		LoadCfg( "config.json" );

		if ( !betManager.CurrentPlayer.IsSetupped( ) )
		{
			betManager.CurrentPlayer = Player( cfg::Get( ).Game.InitialBalance );
			betManager.FullPlayer = Player( cfg::Get( ).Game.InitialBalance );
			betManager.CurrentPlayer.SetBalance( cfg::Get( ).Game.CurrentBalance );
			betManager.FullPlayer.SetBalance( cfg::Get( ).Game.FullBalance );
			betManager.BalanceHistory = cfg::Get( ).Game.BalanceHistory;
			betManager.FullBalanceHistory = cfg::Get( ).Game.FullBalanceHistory;

			betManager.SetRedPosition( TransformVec( cfg::Get( ).Betting.automatic.RedPoint ) );
			betManager.SetBlackPosition( TransformVec( cfg::Get( ).Betting.automatic.BlackPoint ) );
			betManager.SetWhitePosition( TransformVec( cfg::Get( ).Betting.automatic.WhitePoint ) );
			betManager.SetInputPosition( TransformVec( cfg::Get( ).Betting.automatic.InputPoint ) );
			betManager.SetStartPosition( TransformVec( cfg::Get( ).Betting.automatic.BetPoint ) );
		}
	}
	else {

		cfg::Get( ).Betting.automatic.RedPoint = TransformPoint( betManager.GetRedPosition( ) );
		cfg::Get( ).Betting.automatic.BlackPoint = TransformPoint( betManager.GetBlackPosition( ) );
		cfg::Get( ).Betting.automatic.WhitePoint = TransformPoint( betManager.GetWhitePosition( ) );
		cfg::Get( ).Betting.automatic.InputPoint = TransformPoint( betManager.GetInputPosition( ) );
		cfg::Get( ).Betting.automatic.BetPoint = TransformPoint( betManager.GetStartPosition( ) );

		SaveCFG( "config.json" );
	}
}


bool Exist( const std::string & name ) {
	return std::filesystem::exists( name );
}
template < typename T>
std::string Str( std::string label , T data ) {
	std::string st = label;
	st += data;
	return st;
}

bool pressed_insert = false;

void SetPoint( POINT * point ) {
	while ( true ) {

		if ( GetAsyncKeyState( VK_INSERT ) )
		{
			pressed_insert = true;
		}
		else if ( pressed_insert ) {
			pressed_insert = false;
			GetCursorPos( point );
			break;
		}

		std::this_thread::sleep_for( std::chrono::milliseconds( 30 ) );
	}
}

void SetupPoints( BetManager * bet ) {

	std::cout << "PRESS INSERT TO SET RED POSITION!\n";
	std::cout << "Waiting VK_INSERT!\n";

	POINT pos;
	SetPoint( &pos );
	bet->SetRedPosition( pos );
	pos.x = 0;
	pos.y = 0;


	std::cout << "PRESS INSERT TO SET BLACK POSITION!\n";
	std::cout << "Waiting VK_INSERT!\n";

	SetPoint( &pos );
	bet->SetBlackPosition( pos );
	pos.x = 0;
	pos.y = 0;


	std::cout << "PRESS INSERT TO SET WHITE POSITION!\n";
	std::cout << "Waiting VK_INSERT!\n";

	SetPoint( &pos );
	bet->SetWhitePosition( pos );
	pos.x = 0;
	pos.y = 0;

	std::cout << "PRESS INSERT TO SET INPUT POSITION!\n";
	std::cout << "Waiting VK_INSERT!\n";

	SetPoint( &pos );
	bet->SetInputPosition( pos );
	pos.x = 0;
	pos.y = 0;

	std::cout << "PRESS INSERT TO SET START POSITION!\n";
	std::cout << "Waiting VK_INSERT!\n";

	SetPoint( &pos );
	bet->SetStartPosition( pos );
	pos.x = 0;
	pos.y = 0;

	system( "cls" );

	std::cout << "Sucessfully settuped bet points!\n";
}

bool StartedSimulation = false;

int main( int , char ** )
{
	ShowWindow( ::GetConsoleWindow( ) , SW_SHOW );

	if ( !settuped )
	{
		_wmkdir( L"C:\\Blaze\\" );

		std::thread( &BetManager::ManageBets , &betManager ).detach( );
		std::thread( &BetManager::setupData , &betManager ).detach( );

		while ( !betManager.StartBets )
		{
			Sleep( 1500 );
		}

		if ( Exist( Folder + "config.json" ) ) {
			save_cfg( true );

			if ( !betManager.GetRedPosition( ).x )
				SetupPoints( &betManager );

			save_cfg( false );
		}
		else
		{
			ShowWindow( ::GetConsoleWindow( ) , SW_SHOW );

			float balance;

			std::cout << "Insert Your Current Balance: ";
			std::cin >> balance;
			betManager.CurrentPlayer = Player( balance );

			SetupPoints( &betManager );

			save_cfg( false );
		}

		settuped = true;
	}



	WNDCLASSEXW wc = { sizeof( wc ), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle( NULL ), NULL, NULL, NULL, NULL, L"ImGui Example", NULL };
	::RegisterClassExW( &wc );
	HWND hwnd = ::CreateWindowW( wc.lpszClassName , L"BlazeBot" , WS_POPUP | WS_VISIBLE , 0 , 0 , 1366 , 768 , NULL , NULL , NULL , NULL );

	if ( !CreateDeviceD3D( hwnd ) )
	{
		CleanupDeviceD3D( );
		::UnregisterClassW( wc.lpszClassName , wc.hInstance );
		return 1;
	}


	//SetWindowLongPtr( hwnd , GWL_EXSTYLE , WS_EX_TRANSPARENT );


	//SetWindowLong( hwnd , GWL_EXSTYLE , GetWindowLong( hwnd , GWL_EXSTYLE ) | WS_EX_LAYERED | WS_EX_TRANSPARENT );
	//SetLayeredWindowAttributes( hwnd , 0 , 255 , LWA_ALPHA );

	::ShowWindow( hwnd , SW_SHOWDEFAULT );
	::UpdateWindow( hwnd );

	IMGUI_CHECKVERSION( );
	ImGui::CreateContext( );
	ImGuiIO & io = ImGui::GetIO( ); ( void ) io;

	ImGui::StyleColorsDark( );

	ImGui_ImplWin32_Init( hwnd );
	ImGui_ImplDX9_Init( g_pd3dDevice );

	ImGuiIO * ioptr = &ImGui::GetIO( );

	ioptr->Fonts->AddFontFromMemoryTTF( ( void * ) new_font , sizeof( new_font ) , 17.f );

	// Initialize notify
	ImGui::MergeIconsWithLatestFont( 16.f , false );


	LPDIRECT3DTEXTURE9 Background = nullptr;

	if ( Background == nullptr )
		D3DXCreateTextureFromFileInMemoryEx( g_pd3dDevice , &BgBytes , sizeof( BgBytes ) , 1920 , 1080 , D3DX_DEFAULT , D3DUSAGE_DYNAMIC , D3DFMT_UNKNOWN , D3DPOOL_DEFAULT , D3DX_DEFAULT , D3DX_DEFAULT , 0 , NULL , NULL , &Background );

	ImVec4 clear_color = ImVec4( 0.45f , 0.55f , 0.60f , 1.00f );

	bool done = false;
	while ( !done )
	{
		MSG msg;
		while ( ::PeekMessage( &msg , NULL , 0U , 0U , PM_REMOVE ) )
		{
			::TranslateMessage( &msg );
			::DispatchMessage( &msg );
			if ( msg.message == WM_QUIT )
				done = true;
		}
		if ( done )
			break;

		ImGui_ImplDX9_NewFrame( );
		ImGui_ImplWin32_NewFrame( );
		ImGui::NewFrame( );

		//ShowWindow( ::GetConsoleWindow( ) , SW_HIDE );

		{
			static float display_count = 2;
			static float display_count1 = 2;
			static float display_count2 = 2;
			static float display_count3 = 2;
			static float display_count4 = 2;

			static bool animate = false;
			static float values[ 90 ] = {};
			static int values_offset = 0;
			static double refresh_time = 0.0;
			char overlay[ 32 ];
			float average = 0.0f;

			if ( !animate || refresh_time == 0.0 ) {
				refresh_time = ImGui::GetTime( );
			}

			while ( refresh_time < ImGui::GetTime( ) ) {
				static float phase = 0.0f;
				values[ values_offset ] = cosf( phase );
				values_offset = ( values_offset + 1 ) % IM_ARRAYSIZE( values );
				phase += 0.10f * values_offset;
				refresh_time += 1.0f / 60.0f;
			}

			for ( int n = 0; n < IM_ARRAYSIZE( values ); n++ ) {
				average += values[ n ];
			}

			average /= ( float ) IM_ARRAYSIZE( values );

			ImDrawList * draw_list = ImGui::GetBackgroundDrawList( ); // Obtem uma referência ao ImDrawList de fundo
			ImVec2 window_size = ImGui::GetIO( ).DisplaySize; // Obtem o tamanho da janela
			//draw_list->AddRectFilled( ImVec2( 0 , 0 ) , window_size , ImColor( 10 , 10 , 10 ) ); // Desenha um retângulo preenchido com a cor cinza escuro

			draw_list->AddImage( Background , ImVec2( 0 , 0 ) , ImVec2( window_size ) );


			ImGui::SetNextWindowSize( ImVec2( 250 , 220 ) );
			ImGui::Begin( "Windows Option" , NULL , ImGuiWindowFlags_NoResize );                          // Create a window called "Hello, world!" and append into it.
			ImGui::PushItemWidth( ImGui::GetFontSize( ) * -12 );

			ImGui::Text( "%.3f ms/frame (%.1f FPS)" , 1000.0f / ImGui::GetIO( ).Framerate , ImGui::GetIO( ).Framerate );

			//ImGui::Checkbox( "Average Window" , &show_avg_window );
			//
			//ImGui::Checkbox( "Sin Window" , &show_sin_window );
			//
			//ImGui::Checkbox( "Saw Window" , &show_saw_window );

			ImGui::Checkbox( "Information Window" , &cfg::Get( ).Windows.InformationWindow );

			ImGui::Checkbox( "Game History" , &cfg::Get( ).Windows.HistoryWindow );

			ImGui::Checkbox( "Accurracy Window" , &cfg::Get( ).Windows.AccurracyWindow );

			ImGui::Checkbox( "Balance graph" , &cfg::Get( ).Windows.BalanceWindow );

			ImGui::Checkbox( "Simulation graph" , &cfg::Get( ).Windows.SimulationGraph );

			ImGui::Checkbox( "Full Balance graph" , &cfg::Get( ).Windows.FullGraph );

			ImGui::Checkbox( "Betting Options" , &cfg::Get( ).Windows.ShowBetsWindow );

			ImGui::Checkbox( "Predicting Options" , &cfg::Get( ).Windows.ShowPredictionWindow );

			if ( cfg::Get( ).Windows.SimulationGraph )
			{
				if ( !StartedSimulation )
				{
					std::thread( &BetManager::StartGameSimulation , &SimulationbetManager , betManager ).detach( );
					StartedSimulation = true;
				}


				ImGui::SetNextWindowSize( ImVec2( 300 , 150 ) );
				ImGui::Begin( "Simulation Balance graph" , NULL , ImGuiWindowFlags_NoResize );

				//PlotLines( const char * label , const float * values ,
				//int values_count , int values_offset = 0 , 
				//const char * overlay_text = NULL , float scale_min = FLT_MAX , 
				//float scale_max = FLT_MAX , ImVec2 graph_size = ImVec2( 0 , 0 ) , int stride = sizeof( float ) );

				ImGui::PlotLines( "##simulatedgraph" , SimulationbetManager.BalanceHistory.data( ) , SimulationbetManager.BalanceHistory.size( ) , 0 , NULL , FLT_MAX , FLT_MAX , ImVec2( 280 , 105 ) );

				ImGui::End( );



				ImGui::SetNextWindowSize( ImVec2( 300 , 150 ) );
				ImGui::Begin( "Full Simulation Balance graph" , NULL , ImGuiWindowFlags_NoResize );

				//PlotLines( const char * label , const float * values ,
				//int values_count , int values_offset = 0 , 
				//const char * overlay_text = NULL , float scale_min = FLT_MAX , 
				//float scale_max = FLT_MAX , ImVec2 graph_size = ImVec2( 0 , 0 ) , int stride = sizeof( float ) );

				ImGui::PlotLines( "##fullsimulatedgraph" , SimulationbetManager.FullBalanceHistory.data( ) , SimulationbetManager.FullBalanceHistory.size( ) , 0 , NULL , FLT_MAX , FLT_MAX , ImVec2( 280 , 105 ) );

				ImGui::End( );

			}

			if ( cfg::Get( ).Windows.AccurracyWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 300 , 300 ) );
				ImGui::Begin( "Acurracy" , NULL , ImGuiWindowFlags_NoResize );
				ImGui::SetNextItemWidth( ImGui::GetFontSize( ) * -12 );

				auto Beats = betManager.SeparatedBeats;
				auto SeparatedPrediction = predictor.SeparatedPrediction;

				if ( !Beats.empty( ) )
				{
					for ( int i = 0; i < NONE; i++ )
					{
						std::string Method;
						std::string Col;

						auto BeatsSeparated = Beats[ i ];


						switch ( i )
						{
						case FOUND_PATTERN:
							Method += " ( PATTERN )";
							break;
						case CERTAINTY:
							Method += " ( MATH )";
							break;
						case IA:
							Method += " ( I.A )";
							break;
						case STREAK:
							Method += " ( STREAK )";
							break;
						case SEQUENCE:
							Method += " ( SEQUENCE )";
							break;
						case GENERAL:
							Method += " ( GENERAL )";
							break;
							//case LOGIC:
							//	Method += " ( LOGIC )";
							//	break;
						default:
							Method += " ( NONE )";
							break;
						}

						Col = "0";

						if ( !SeparatedPrediction.empty( ) )
						{
							auto Prediction = SeparatedPrediction[ i ];
							switch ( Prediction )
							{
							case White:
								Col = "W";
								break;
							case Blue:
								Col = "B";
								break;
							case Red:
								Col = "R";
								break;
							default:
								Col = "0";
								break;
							}
						}

						int HitsPercentage = BeatsSeparated.GetHitsPercentage( );
						int TotalBets = BeatsSeparated.GetTotal( );

						std::string Hits = "Result" + Method + ": ( " + std::to_string( HitsPercentage ) + " ) - " + Col;
						std::string Total = "Total" + Method + ": " + std::to_string( TotalBets );
						std::string Roll = "Roll" + Method + ": " + std::to_string( BeatsSeparated.GetRollLosesAmount( ) );
						std::string MaxRoll = "Max Roll" + Method + ": " + std::to_string( BeatsSeparated.GetMaximumRollLoseAmount( ) );
						std::string MedRoll = "Med Roll" + Method + ": " + std::to_string( BeatsSeparated.GetMediumRollLoseAmount( ) );
						std::string RollDistance = "Roll Distance" + Method + ": " + std::to_string( BeatsSeparated.DistanceBetweenRolls( ) );

						ImGui::Text( Hits.c_str( ) );
						ImGui::Text( Total.c_str( ) );
						ImGui::Text( Roll.c_str( ) );
						ImGui::Text( MaxRoll.c_str( ) );
						ImGui::Text( MedRoll.c_str( ) );
						ImGui::Text( RollDistance.c_str( ) );
						ImGui::NewLine( );
					}
				}

				ImGui::End( );

			}


			if ( cfg::Get( ).Windows.InformationWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 250 , 220 ) );
				ImGui::Begin( "Informations" , NULL , ImGuiWindowFlags_NoResize );
				ImGui::SetNextItemWidth( ImGui::GetFontSize( ) * -12 );

				auto current_prediction = betManager.GetCurrentPrediction( );
				float chance = current_prediction.GetChance( ) * 100;

				std::string PredColor = "Color Prediction: ";
				switch ( current_prediction.GetColor( ) )
				{
				case Red:
					PredColor += "Red";
					break;
				case Blue:
					PredColor += "Black";
					break;
				case White:
					PredColor += "White";
					break;
				case Null:
					PredColor += "None";
					break;
				}

				switch ( current_prediction.GetPredictionMethod( ) )
				{
				case FOUND_PATTERN:
					PredColor += " ( PATTERN )";
					break;
				case CERTAINTY:
					PredColor += " ( MATH )";
					break;
				case IA:
					PredColor += " ( I.A )";
					break;
				case STREAK:
					PredColor += " ( STREAK )";
					break;
				case SEQUENCE:
					PredColor += " ( SEQUENCE )";
					break;
				case GENERAL:
					PredColor += " ( GENERAL )";
					break;
					//case LOGIC:
					//	PredColor += " ( LOGIC )";
					//	break;
				default:
					PredColor += " ( NONE )";
					break;
				}


				ImGui::Text( PredColor.c_str( ) );
				ImGui::Text( Str( "Certainty: " , std::to_string( int( chance ) ) ).c_str( ) );
				ImGui::Text( Str( "Balance: " , std::to_string( ( int ) betManager.CurrentPlayer.GetBalance( ) ) ).c_str( ) );
				ImGui::Text( Str( "Initial Balance: " , std::to_string( ( int ) betManager.CurrentPlayer.GetInitialMoney( ) ) ).c_str( ) );
				ImGui::Text( Str( "Profit: " , std::to_string( ( int ) betManager.CurrentPlayer.GetProfit( ) ) ).c_str( ) );

				if ( current_prediction.DidBet( ) ) {

					ImGui::TextColored( ImVec4( 1.0f , 0.f , 0.f , 1.0f ) , "It's a bet!" );
					ImGui::Text( Str( "Bet Value: " , std::to_string( current_prediction.GetBetAmount( ) ) ).c_str( ) );
				}
				if ( current_prediction.GetPrediction( ).PossibleWhite ) {

					ImGui::TextColored( ImVec4( 1.0f , 0.f , 0.f , 1.0f ) , "Cover White!" );
					ImGui::Text( Str( "White Bet Value: " , std::to_string( current_prediction.GetWhiteBet( ) ) ).c_str( ) );
				}
				else if ( current_prediction.GetMethod( ) == STANDBY )
				{
					ImGui::TextColored( ImVec4( 0.59f , 0.27f , 0.60f , 1.0f ) , "Stand Bying!" );
				}

				ImGui::End( );
			}

			if ( cfg::Get( ).Windows.HistoryWindow ) {

				ImGui::SetNextWindowSize( ImVec2( 450 , 73 ) );
				ImGui::Begin( "Game History" , NULL , ImGuiWindowFlags_NoResize );

				ImDrawList * drawList = ImGui::GetWindowDrawList( );
				float pos = 0;

				float size = 19;
				pos += 15;

				int start = -1;

				auto Hist = predictor.GetHistory( );

				for ( int i = Hist.size( ) - 18; i < Hist.size( ); i++ ) {

					auto Color = Hist[ i ].GetColor( );
					auto Number = Hist[ i ].GetRoll( );

					int Number_Spacing = 0;
					if ( Number >= 10 )
					{
						Number_Spacing = 6;
					}
					else
						Number_Spacing = 4;


					ImColor col;
					ImColor text_col = ImColor( 1.0f , 1.0f , 1.0f );

					switch ( Color )
					{
					case Red:
						col = ImColor( 241.f / 255.f , 44.f / 255.f , 76.f / 255.f );
						break;
					case Blue:
						col = ImColor( 38.f / 255.f , 47.f / 255.f , 60.f / 255.f );
						break;
					case White:
						col = ImColor( 1.0f , 1.0f , 1.0f );
						text_col = ImColor( 1.0f , 0.f , 0.f );
						break;
					}

					//Red : 241, 44, 76
					//Black: 38, 47, 60

					if ( start != -1 )
						pos += size * 1.3;

					ImVec2 center = ImVec2( ImGui::GetWindowPos( ).x + pos , ImGui::GetWindowPos( ).y + size * 2.5 );
					ImVec2 less = RemoveVec( center , ImVec2( size / 2 , size / 2 ) );
					ImVec2 sum = sum_vec( center , ImVec2( size / 2 , size / 2 ) );

					drawList->AddRectFilled( less , sum , col );
					center = ImVec2 { center.x - Number_Spacing, center.y - 8 };
					drawList->AddText( NULL , 0 , center , text_col , std::to_string( Number ).c_str( ) , NULL , 0.0f , NULL );
					start = 1;
				}



				ImGui::End( );
			}

			if ( cfg::Get( ).Windows.BalanceWindow ) {
				// iterate over the array using the pointer

				ImGui::SetNextWindowSize( ImVec2( 300 , 150 ) );
				ImGui::Begin( "Balance graph" , NULL , ImGuiWindowFlags_NoResize );

				//PlotLines( const char * label , const float * values ,
				//int values_count , int values_offset = 0 , 
				//const char * overlay_text = NULL , float scale_min = FLT_MAX , 
				//float scale_max = FLT_MAX , ImVec2 graph_size = ImVec2( 0 , 0 ) , int stride = sizeof( float ) );

				ImGui::PlotLines( "##graph" , betManager.BalanceHistory.data( ) , betManager.BalanceHistory.size( ) , 0 , NULL , FLT_MAX , FLT_MAX , ImVec2( 280 , 105 ) );

				ImGui::End( );
			}


			if ( cfg::Get( ).Windows.FullGraph ) {
				// iterate over the array using the pointer

				ImGui::SetNextWindowSize( ImVec2( 300 , 150 ) );
				ImGui::Begin( "Full Balance graph" , NULL , ImGuiWindowFlags_NoResize );

				//PlotLines( const char * label , const float * values ,
				//int values_count , int values_offset = 0 , 
				//const char * overlay_text = NULL , float scale_min = FLT_MAX , 
				//float scale_max = FLT_MAX , ImVec2 graph_size = ImVec2( 0 , 0 ) , int stride = sizeof( float ) );

				ImGui::PlotLines( "##fullgraph" , betManager.FullBalanceHistory.data( ) , betManager.FullBalanceHistory.size( ) , 0 , NULL , FLT_MAX , FLT_MAX , ImVec2( 280 , 105 ) );

				ImGui::End( );
			}


			if ( cfg::Get( ).Windows.ShowBetsWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 450 , 280 ) );
				ImGui::Begin( "Betting Options" , NULL , ImGuiWindowFlags_NoResize );

				ImGui::Checkbox( "Auto bet" , &cfg::Get( ).Betting.automatic.AutoBet );

				for ( int i = 0; i < 2; i++ )
				{
					std::string type;

					switch ( i )
					{
					case WON:
						type = " - win";
						break;
					case LOSE:
						type = " - lose";
						break;
					default:
						type = " - ?";
						break;
					}

					ImGui::Checkbox( std::string( "Multiply Bet" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].MultiplyBet );
					if ( cfg::Get( ).Betting.type[ i ].MultiplyBet ) {
						ImGui::Checkbox( std::string( "Reset if different" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].ResetIfDifferent );
						ImGui::SliderFloat( std::string( "Multiplier" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].BetMultiplier , 0 , 300 );
						ImGui::SliderInt( std::string( "Max Times" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].MaxMultiplierTimes , 0 , 10 );
						ImGui::SliderInt( std::string( "Multiply after" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].MultiplyAfterX , 0 , 10 );
						ImGui::Checkbox( std::string( "Increment Minimum" + type ).c_str( ) , &cfg::Get( ).Betting.type[ i ].IncrementMinimum );

						if ( i == LOSE )
						{
							ImGui::Checkbox( "Protect Profit" , &cfg::Get( ).Betting.security.ProtectProfit );
							if ( cfg::Get( ).Betting.security.ProtectProfit )
							{
								ImGui::SliderFloat( "Protect if profit" , &cfg::Get( ).Betting.security.ProtectIfProfit , 1 , 100 );
								ImGui::SliderFloat( "Protect percentage" , &cfg::Get( ).Betting.security.ProfitProtectPercentage , 1 , 100 );
							}

							ImGui::Checkbox( "Protect Capital" , &cfg::Get( ).Betting.security.ProtectCapital );
							if ( cfg::Get( ).Betting.security.ProtectCapital )
							{
								ImGui::SliderFloat( "Protect percentage - C" , &cfg::Get( ).Betting.security.CapitalProtectPercentage , 1 , 100 );
							}

							ImGui::Checkbox( "Predict down peaks" , &cfg::Get( ).Betting.security.PredictDownPeaks );
						}
					}

					ImGui::NewLine( );
				}

				ImGui::Checkbox( "Prevent from down peaks" , &cfg::Get( ).Betting.security.PreventDownPeaks );

				ImGui::Checkbox( "Play only on stable moments" , &cfg::Get( ).Betting.security.PlayOnlyOnStableMoments );

				if ( cfg::Get( ).Betting.security.PreventDownPeaks )
				{
					ImGui::SliderInt( "Peek Distance" , &cfg::Get( ).Betting.security.MinimumPeakDistance , 1 , 50 );
					ImGui::SliderFloat( "Prevent if peek is (%)" , &cfg::Get( ).Betting.security.MinimumPeakValue , 0 , 100 );
					ImGui::SliderInt( "Waiting Time (minutes)" , &cfg::Get( ).Betting.security.WaitingTime , 1 , 120 );
				}
				ImGui::NewLine( );

				ImGui::SliderFloat( "Target Percentage" , &cfg::Get( ).Betting.TargetParcentage , 0 , 100 );
				ImGui::SliderFloat( "Stop if lower than %" , &cfg::Get( ).Betting.StopPercentage , 0 , 100 );

				ImGui::SliderFloat( "Max Bet percentage" , &cfg::Get( ).Betting.MaxBetPercentage , 0 , 100 );
				ImGui::SliderFloat( "Min Bet percentage" , &cfg::Get( ).Betting.MinBetPercentage , 0 , 20 );



				//ImGui::Checkbox( "Multiply Bet on lose" , &g_globals.Betting.MultiplyBetOnLose );
				//if ( g_globals.Betting.MultiplyBetOnLose )
				//{
				//	ImGui::SliderInt( "Multiply after - lose" , &g_globals.Betting.MultiplyBetAfterXLose , 1 , 10 );
				//	ImGui::SliderInt( "Max multiplier times - lose" , &g_globals.Betting.MaxMultiplierTimesOnLose , 1 , 10 );
				//	ImGui::SliderFloat( "Lose multiplier" , &g_globals.Betting.LoseBetMultiplier , 0 , 200 );
				//	ImGui::Checkbox( "Reset count on win" , &g_globals.Betting.ResetNextBetMultiplierOnWin );
				//
				//	ImGui::Checkbox( "Protect Profit" , &g_globals.Betting.ProtectProfit );
				//	if ( g_globals.Betting.ProtectProfit )
				//	{
				//		ImGui::SliderFloat( "Protect if profit" , &g_globals.Betting.ProtectAfter , 1 , 100 );
				//		ImGui::SliderFloat( "Protect percentage" , &g_globals.Betting.ProtectPercentage , 1 , 100 );
				//	}
				//
				//	ImGui::NewLine( );
				//}
				//
				//
				//
				//ImGui::Checkbox( "MultiplyBetOnWin" , &g_globals.Betting.MultiplyBetOnWin );
				//if ( g_globals.Betting.MultiplyBetOnWin )
				//{
				//	ImGui::SliderInt( "Multiply after - win" , &g_globals.Betting.MultiplyBetAfterXWin , 1 , 10 );
				//	ImGui::SliderInt( "Max multiplier times - win" , &g_globals.Betting.MaxMultiplierTimesOnWin , 1 , 10 );
				//	ImGui::SliderFloat( "Win multiplier" , &g_globals.Betting.WonBetMultiplier , 0 , 200 );
				//
				//	ImGui::Checkbox( "Reset count on lose" , &g_globals.Betting.ResetNextBetMultiplierOnLose );
				//	ImGui::NewLine( );
				//}
				//
				//ImGui::Checkbox( "Wait if losing" , &g_globals.Betting.WaitIfLosing );
				//if ( g_globals.Betting.WaitIfLosing )
				//{
				//	ImGui::SliderInt( "Wait after" , &g_globals.Betting.WaitAfterXLose , 1 , 10 );
				//	ImGui::SliderInt( "Wait amount" , &g_globals.Betting.WaitAmount , 1 , 10 );
				//	ImGui::Checkbox( "Reset Stand If win" , &g_globals.Betting.ResetCountIfWin );
				//	ImGui::Checkbox( "Wait again if lose" , &g_globals.Betting.WaitAgainIfLose );
				//	ImGui::NewLine( );
				//}
				//
				//ImGui::SliderFloat( "Target Percentage" , &g_globals.Betting.TargetParcentage , 0 , 100 );
				//ImGui::SliderFloat( "Stop if lower than %" , &g_globals.Betting.StopPercentage , 0 , 100 );
				//
				//ImGui::SliderFloat( "Max Bet percentage" , &g_globals.Betting.MaxBetPercentage , 0 , 100 );
				//ImGui::SliderFloat( "Min Bet percentage" , &g_globals.Betting.MinBetPercentage , 0 , 20 );
				//
				//ImGui::Checkbox( "Prevent from down peaks" , &g_globals.Betting.PreventDownPeaks );
				//if ( g_globals.Betting.PreventDownPeaks )
				//{
				//	ImGui::SliderInt( "Peek Distance" , &g_globals.Betting.MinimumPeakDistance , 1 , 50 );
				//	ImGui::SliderFloat( "Prevent if peek is (%)" , &g_globals.Betting.PreventIfAbove , 0 , 100 );
				//	ImGui::SliderInt( "Waiting Time (minutes)" , &g_globals.Betting.WaitingTime , 1 , 120 );
				//}

				for ( auto notification : cfg::Get( ).Game.Notifications )
				{
					ImGui::InsertNotification( { notification.Type, notification.Timing,notification.Message.c_str( ) } );
				}

				cfg::Get( ).Game.Notifications.clear( );


				ImGui::End( );
			}

			if ( cfg::Get( ).Windows.ShowPredictionWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 400 , 200 ) );
				ImGui::Begin( "Predicting Options" , NULL , ImGuiWindowFlags_NoResize );

				ImGui::SliderInt( "Min Pattern size" , &cfg::Get( ).Prediction.MinPatterSize , 2 , 20 );
				ImGui::SliderInt( "Ignore streaks after" , &cfg::Get( ).Prediction.IgnoreStreakAfter , 2 , 20 );
				ImGui::SliderInt( "Streak Betting Spacing" , &cfg::Get( ).Prediction.StreakBettingSpacing , 1 , 10 );

				ImGui::SliderFloat( "Minimum Chance" , &cfg::Get( ).Prediction.MinimumPercentage , 0 , 100 );
				ImGui::SliderFloat( "Maximum Chance" , &cfg::Get( ).Prediction.MaximumPercentage , 0 , 100 );

				ImGui::End( );
			}


			ImGui::End( );
		}

		ImGui::RenderNotifications( );

		ImGui::EndFrame( );
		g_pd3dDevice->SetRenderState( D3DRS_ZENABLE , FALSE );
		g_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE , FALSE );
		g_pd3dDevice->SetRenderState( D3DRS_SCISSORTESTENABLE , FALSE );
		D3DCOLOR clear_col_dx = D3DCOLOR_RGBA( ( int ) ( clear_color.x * clear_color.w * 255.0f ) , ( int ) ( clear_color.y * clear_color.w * 255.0f ) , ( int ) ( clear_color.z * clear_color.w * 255.0f ) , ( int ) ( clear_color.w * 255.0f ) );
		g_pd3dDevice->Clear( 0 , NULL , D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER , clear_col_dx , 1.0f , 0 );
		if ( g_pd3dDevice->BeginScene( ) >= 0 )
		{
			ImGui::Render( );
			ImGui_ImplDX9_RenderDrawData( ImGui::GetDrawData( ) );
			g_pd3dDevice->EndScene( );
		}
		HRESULT result = g_pd3dDevice->Present( NULL , NULL , NULL , NULL );

		if ( result == D3DERR_DEVICELOST && g_pd3dDevice->TestCooperativeLevel( ) == D3DERR_DEVICENOTRESET )
			ResetDevice( );
	}

	ImGui_ImplDX9_Shutdown( );
	ImGui_ImplWin32_Shutdown( );
	ImGui::DestroyContext( );

	CleanupDeviceD3D( );
	::DestroyWindow( hwnd );
	::UnregisterClassW( wc.lpszClassName , wc.hInstance );

	return 0;
}


bool CreateDeviceD3D( HWND hWnd )
{
	if ( ( g_pD3D = Direct3DCreate9( D3D_SDK_VERSION ) ) == NULL )
		return false;

	ZeroMemory( &g_d3dpp , sizeof( g_d3dpp ) );
	g_d3dpp.Windowed = TRUE;
	g_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	g_d3dpp.BackBufferFormat = D3DFMT_UNKNOWN;
	g_d3dpp.EnableAutoDepthStencil = TRUE;
	g_d3dpp.AutoDepthStencilFormat = D3DFMT_D16;
	g_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_ONE;
	if ( g_pD3D->CreateDevice( D3DADAPTER_DEFAULT , D3DDEVTYPE_HAL , hWnd , D3DCREATE_HARDWARE_VERTEXPROCESSING , &g_d3dpp , &g_pd3dDevice ) < 0 )
		return false;

	return true;
}

void CleanupDeviceD3D( )
{
	if ( g_pd3dDevice ) { g_pd3dDevice->Release( ); g_pd3dDevice = NULL; }
	if ( g_pD3D ) { g_pD3D->Release( ); g_pD3D = NULL; }
}

void ResetDevice( )
{
	ImGui_ImplDX9_InvalidateDeviceObjects( );
	HRESULT hr = g_pd3dDevice->Reset( &g_d3dpp );
	if ( hr == D3DERR_INVALIDCALL )
		IM_ASSERT( 0 );
	ImGui_ImplDX9_CreateDeviceObjects( );
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam );

LRESULT WINAPI WndProc( HWND hWnd , UINT msg , WPARAM wParam , LPARAM lParam )
{
	if ( ImGui_ImplWin32_WndProcHandler( hWnd , msg , wParam , lParam ) )
		return true;

	switch ( msg )
	{
	case WM_SIZE:
		if ( g_pd3dDevice != NULL && wParam != SIZE_MINIMIZED )
		{
			g_d3dpp.BackBufferWidth = LOWORD( lParam );
			g_d3dpp.BackBufferHeight = HIWORD( lParam );
			ResetDevice( );
		}
		return 0;
	case WM_SYSCOMMAND:
		if ( ( wParam & 0xfff0 ) == SC_KEYMENU )
			return 0;
		break;
	case WM_DESTROY:
		::PostQuitMessage( 0 );
		return 0;
	}
	return ::DefWindowProc( hWnd , msg , wParam , lParam );
}
