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

#include "BetManagement/BetManagement.h"
#include "Predictor/Predictor.h"
#include "Colors/ColorManagement/ColorManagement.h"
#include "Utils/Utils.h"


#include "Utils/Bytes.h"
#include "globals.h"



#pragma warning(disable:4996)

std::string filename = "Scan.json";
std::string Folder = "C:\\Blaze\\";
std::string IAPATH = "IA.pt";
std::string JSName = "js.js";
std::string HistoryName = "history.json";
std::string ImportName = "import.json";
bool Stop = false;

DoublePredictor predictor;

BetManager betManager( filename , &predictor );
Utils usefull;

Globals g_globals;

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

void save_cfg( bool read ) {

	json cfg;

	std::string DIR = Folder + "config.json";
	static bool settuped_initial = false;

	if ( read ) {

		std::fstream arquivo( DIR );// abre o arquivo para leitura
		if ( arquivo.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso

			cfg = json::parse( arquivo );
			arquivo.close( ); // fecha o arquivo
		}
		else {
			std::cout << "Não foi possível abrir o arquivo" << std::endl;
			return;
		}

		betManager.SetRedPosition( GetPoint( "RedPos" , cfg ) );
		betManager.SetBlackPosition( GetPoint( "BlackPos" , cfg ) );
		betManager.SetWhitePosition( GetPoint( "WhitePos" , cfg ) );
		betManager.SetInputPosition( GetPoint( "InputPos" , cfg ) );
		betManager.SetStartPosition( GetPoint( "StartPos" , cfg ) );
		betManager.CurrentPlayer = Player( cfg[ "InitialBalance" ] );
		betManager.CurrentPlayer.SetBalance( cfg[ "Balance" ] );
		betManager.BalanceHistory.emplace_back( betManager.CurrentPlayer.GetBalance( ) );
		if ( cfg[ "BalanceGraph" ].is_array( ) )
		{
			std::vector<float> balance_g;

			for ( auto & value : cfg[ "BalanceGraph" ] )
			{
				balance_g.emplace_back( value );
			}

			betManager.BalanceHistory = balance_g;
		}

		g_globals.Prediction.IgnoreStreakAfter = cfg[ "Config" ][ "Prediction" ][ "IgnoreStreakAfter" ];
		g_globals.Prediction.StreakBettingSpacing = cfg[ "Config" ][ "Prediction" ][ "StreakBettingSpacing" ];
		g_globals.Prediction.InvertIfMissing = cfg[ "Config" ][ "Prediction" ][ "InvertIfMissing" ];
		g_globals.Prediction.InvertIfMissingAbove = cfg[ "Config" ][ "Prediction" ][ "InvertIfMissingAbove" ];
		g_globals.Prediction.MaximumPercentage = cfg[ "Config" ][ "Prediction" ][ "MaximumPercentage" ];
		g_globals.Prediction.MinimumPercentage = cfg[ "Config" ][ "Prediction" ][ "MinimumPercentage" ];
		g_globals.Prediction.MinPatterSize = cfg[ "Config" ][ "Prediction" ][ "MinPatterSize" ];

		g_globals.Betting.AutoBet = cfg[ "Config" ][ "Betting" ][ "AutoBet" ];
		g_globals.Betting.LoseBetMultiplier = cfg[ "Config" ][ "Betting" ][ "LoseBetMultiplier" ];
		g_globals.Betting.MaxBetPercentage = cfg[ "Config" ][ "Betting" ][ "MaxBetPercentage" ];
		g_globals.Betting.MaxMultiplierTimesOnLose = cfg[ "Config" ][ "Betting" ][ "MaxMultiplierTimesOnLose" ];
		g_globals.Betting.MaxMultiplierTimesOnWin = cfg[ "Config" ][ "Betting" ][ "MaxMultiplierTimesOnWin" ];
		g_globals.Betting.MinBetPercentage = cfg[ "Config" ][ "Betting" ][ "MinBetPercentage" ];
		g_globals.Betting.MultiplyBetAfterXLose = cfg[ "Config" ][ "Betting" ][ "MultiplyBetAfterXLose" ];
		g_globals.Betting.MultiplyBetAfterXWin = cfg[ "Config" ][ "Betting" ][ "MultiplyBetAfterXWin" ];
		g_globals.Betting.MultiplyBetOnLose = cfg[ "Config" ][ "Betting" ][ "MultiplyBetOnLose" ];
		g_globals.Betting.MultiplyBetOnWin = cfg[ "Config" ][ "Betting" ][ "MultiplyBetOnWin" ];
		g_globals.Betting.ResetNextBetMultiplierOnLose = cfg[ "Config" ][ "Betting" ][ "ResetNextBetMultiplierOnLose" ];
		g_globals.Betting.ResetNextBetMultiplierOnWin = cfg[ "Config" ][ "Betting" ][ "ResetNextBetMultiplierOnWin" ];
		g_globals.Betting.WaitAfterXLose = cfg[ "Config" ][ "Betting" ][ "WaitAfterXLose" ];
		g_globals.Betting.WaitAmount = cfg[ "Config" ][ "Betting" ][ "WaitAmount" ];
		g_globals.Betting.WaitIfLosing = cfg[ "Config" ][ "Betting" ][ "WaitIfLosing" ];
		g_globals.Betting.WonBetMultiplier = cfg[ "Config" ][ "Betting" ][ "WonBetMultiplier" ];

	}
	else {
		SavePoint( "RedPos" , cfg , betManager.GetRedPosition( ) );
		SavePoint( "BlackPos" , cfg , betManager.GetBlackPosition( ) );
		SavePoint( "WhitePos" , cfg , betManager.GetWhitePosition( ) );
		SavePoint( "InputPos" , cfg , betManager.GetInputPosition( ) );
		SavePoint( "StartPos" , cfg , betManager.GetStartPosition( ) );
		cfg[ "Balance" ] = betManager.CurrentPlayer.GetBalance( );
		cfg[ "BalanceGraph" ] = betManager.BalanceHistory;
		cfg[ "InitialBalance" ] = betManager.CurrentPlayer.GetInitialMoney( );

		cfg[ "Config" ][ "Prediction" ][ "IgnoreStreakAfter" ] = g_globals.Prediction.IgnoreStreakAfter;
		cfg[ "Config" ][ "Prediction" ][ "StreakBettingSpacing" ] = g_globals.Prediction.StreakBettingSpacing;
		cfg[ "Config" ][ "Prediction" ][ "InvertIfMissing" ] = g_globals.Prediction.InvertIfMissing;
		cfg[ "Config" ][ "Prediction" ][ "InvertIfMissingAbove" ] = g_globals.Prediction.InvertIfMissingAbove;
		cfg[ "Config" ][ "Prediction" ][ "MaximumPercentage" ] = g_globals.Prediction.MaximumPercentage;
		cfg[ "Config" ][ "Prediction" ][ "MinimumPercentage" ] = g_globals.Prediction.MinimumPercentage;
		cfg[ "Config" ][ "Prediction" ][ "MinPatterSize" ] = g_globals.Prediction.MinPatterSize;

		cfg[ "Config" ][ "Betting" ][ "AutoBet" ] = g_globals.Betting.AutoBet;
		cfg[ "Config" ][ "Betting" ][ "LoseBetMultiplier" ] = g_globals.Betting.LoseBetMultiplier;
		cfg[ "Config" ][ "Betting" ][ "MaxBetPercentage" ] = g_globals.Betting.MaxBetPercentage;
		cfg[ "Config" ][ "Betting" ][ "MaxMultiplierTimesOnLose" ] = g_globals.Betting.MaxMultiplierTimesOnLose;
		cfg[ "Config" ][ "Betting" ][ "MaxMultiplierTimesOnWin" ] = g_globals.Betting.MaxMultiplierTimesOnWin;
		cfg[ "Config" ][ "Betting" ][ "MinBetPercentage" ] = g_globals.Betting.MinBetPercentage;
		cfg[ "Config" ][ "Betting" ][ "MultiplyBetAfterXLose" ] = g_globals.Betting.MultiplyBetAfterXLose;
		cfg[ "Config" ][ "Betting" ][ "MultiplyBetAfterXWin" ] = g_globals.Betting.MultiplyBetAfterXWin;
		cfg[ "Config" ][ "Betting" ][ "MultiplyBetOnLose" ] = g_globals.Betting.MultiplyBetOnLose;
		cfg[ "Config" ][ "Betting" ][ "MultiplyBetOnWin" ] = g_globals.Betting.MultiplyBetOnWin;
		cfg[ "Config" ][ "Betting" ][ "ResetNextBetMultiplierOnLose" ] = g_globals.Betting.ResetNextBetMultiplierOnLose;
		cfg[ "Config" ][ "Betting" ][ "ResetNextBetMultiplierOnWin" ] = g_globals.Betting.ResetNextBetMultiplierOnWin;
		cfg[ "Config" ][ "Betting" ][ "WaitAfterXLose" ] = g_globals.Betting.WaitAfterXLose;
		cfg[ "Config" ][ "Betting" ][ "WaitAmount" ] = g_globals.Betting.WaitAmount;
		cfg[ "Config" ][ "Betting" ][ "WaitIfLosing" ] = g_globals.Betting.WaitIfLosing;
		cfg[ "Config" ][ "Betting" ][ "WonBetMultiplier" ] = g_globals.Betting.WonBetMultiplier;


		std::ofstream  arquivo( DIR );// abre o arquivo para leitura
		if ( arquivo.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso

			arquivo << cfg;
			arquivo.close( ); // fecha o arquivo
		}
		else {
			std::cout << "Não foi possível abrir o arquivo" << std::endl;
		}
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
		}
		else
		{
			ShowWindow( ::GetConsoleWindow( ) , SW_SHOW );

			float balance;

			std::cout << "Insert Your Current Balance: ";
			std::cin >> balance;
			betManager.CurrentPlayer = Player( balance );

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


	io.Fonts->AddFontFromFileTTF( "c:\\Windows\\Fonts\\TT Firs Neue Trl.ttf" , 25.f );


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

			ImGui::Checkbox( "Information Window" , &g_globals.Windows.InformationWindow );

			ImGui::Checkbox( "Game History" , &g_globals.Windows.HistoryWindow );

			ImGui::Checkbox( "Accurracy Window" , &g_globals.Windows.AccurracyWindow );

			ImGui::Checkbox( "Balance graph" , &g_globals.Windows.BalanceWindow );

			ImGui::Checkbox( "Betting Options" , &g_globals.Windows.ShowBetsWindow );

			ImGui::Checkbox( "Predicting Options" , &g_globals.Windows.ShowPredictionWindow );


			if ( g_globals.Windows.AccurracyWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 250 , 220 ) );
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


						std::string Hits = "Result" + Method + ": ( " + std::to_string( BeatsSeparated.beats ) + "/" + std::to_string( BeatsSeparated.misses ) + " ) - " + Col;

						ImGui::Text( Hits.c_str( ) );
					}
				}

				ImGui::End( );

			}


			if ( g_globals.Windows.InformationWindow ) {
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
					ImGui::Text( Str( "Bet Value: " , std::to_string( current_prediction.GetBetAmount( )  ) ).c_str( ) );
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

			if ( g_globals.Windows.HistoryWindow ) {

				ImGui::SetNextWindowSize( ImVec2( 450 , 73 ) );
				ImGui::Begin( "Game History" , NULL , ImGuiWindowFlags_NoResize );

				ImDrawList * drawList = ImGui::GetWindowDrawList( );
				float pos = 0;

				float size = 19;
				pos += 15;

				int start = -1;

				auto Hist = predictor.GetHistory( );

				for ( int i = Hist.size( ) - 18; i < Hist.size( ); i++ ) {

					auto Color = Hist.at( i ).GetColor( );
					auto Number = Hist.at( i ).GetRoll( );

					int Number_Spacing;
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
					center = ImVec2 { center.x - Number_Spacing, center.y - 6 };
					drawList->AddText( NULL , 0 , center , text_col , std::to_string( Number ).c_str( ) , NULL , 0.0f , NULL );
					start = 1;
				}



				ImGui::End( );
			}

			if ( g_globals.Windows.BalanceWindow ) {
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

			if ( g_globals.Windows.ShowBetsWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 450 , 280 ) );
				ImGui::Begin( "Betting Options" , NULL , ImGuiWindowFlags_NoResize );

				ImGui::Checkbox( "Multiply Bet on lose" , &g_globals.Betting.MultiplyBetOnLose );
				if ( g_globals.Betting.MultiplyBetOnLose )
				{
					ImGui::SliderInt( "Multiply after - lose" , &g_globals.Betting.MultiplyBetAfterXLose , 1 , 10 );
					ImGui::SliderInt( "Max multiplier times - lose" , &g_globals.Betting.MaxMultiplierTimesOnLose , 1 , 10 );
					ImGui::SliderFloat( "Lose multiplier" , &g_globals.Betting.LoseBetMultiplier , 0 , 200 );

					ImGui::Checkbox( "Reset count on win" , &g_globals.Betting.ResetNextBetMultiplierOnWin );
					ImGui::NewLine( );
				}



				ImGui::Checkbox( "MultiplyBetOnWin" , &g_globals.Betting.MultiplyBetOnWin );
				if ( g_globals.Betting.MultiplyBetOnWin )
				{
					ImGui::SliderInt( "Multiply after - win" , &g_globals.Betting.MultiplyBetAfterXWin , 1 , 10 );
					ImGui::SliderInt( "Max multiplier times - win" , &g_globals.Betting.MaxMultiplierTimesOnWin , 1 , 10 );
					ImGui::SliderFloat( "Win multiplier" , &g_globals.Betting.WonBetMultiplier , 0 , 200 );

					ImGui::Checkbox( "Reset count on lose" , &g_globals.Betting.ResetNextBetMultiplierOnLose );
					ImGui::NewLine( );
				}

				ImGui::Checkbox( "Wait if losing" , &g_globals.Betting.WaitIfLosing );
				if ( g_globals.Betting.WaitIfLosing )
				{
					ImGui::SliderInt( "Wait after" , &g_globals.Betting.WaitAfterXLose , 1 , 10 );
					ImGui::SliderInt( "Wait amount" , &g_globals.Betting.WaitAmount , 1 , 10 );
					ImGui::NewLine( );
				}

				ImGui::SliderFloat( "Target Percentage" , &g_globals.Betting.TargetParcentage , 0 , 100 );
				ImGui::SliderFloat( "Stop if lower than %" , &g_globals.Betting.StopPercentage , 0 , 100 );

				ImGui::SliderFloat( "Max Bet percentage" , &g_globals.Betting.MaxBetPercentage , 0 , 100 );
				ImGui::SliderFloat( "Min Bet percentage" , &g_globals.Betting.MinBetPercentage , 0 , 20 );

				ImGui::End( );
			}

			if ( g_globals.Windows.ShowPredictionWindow ) {
				ImGui::SetNextWindowSize( ImVec2( 400 , 200 ) );
				ImGui::Begin( "Predicting Options" , NULL , ImGuiWindowFlags_NoResize );

				ImGui::SliderInt( "Min Pattern size" , &g_globals.Prediction.MinPatterSize , 2 , 20 );
				ImGui::SliderInt( "Ignore streaks after" , &g_globals.Prediction.IgnoreStreakAfter , 2 , 20 );
				ImGui::SliderInt( "Streak Betting Spacing" , &g_globals.Prediction.StreakBettingSpacing , 1 , 10 );

				ImGui::SliderFloat( "Minimum Chance" , &g_globals.Prediction.MinimumPercentage , 0 , 100 );
				ImGui::SliderFloat( "Maximum Chance" , &g_globals.Prediction.MaximumPercentage , 0 , 100 );

				ImGui::End( );
			}


			ImGui::End( );
		}

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
