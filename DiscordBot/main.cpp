


#include <dpp/dpp.h>
#include <matplot/matplot.h>
#include <filesystem>

#pragma comment(lib, "matplot.lib")
#pragma comment(lib, "Matplot++\\nodesoup.lib")


//C:\Program Files\matplotplusplus 1.1.0\lib\Matplot++

using namespace matplot;

const std::string    BOT_TOKEN = "OTgxMzM0OTEwMDAwMzA4MjM0.G2c2lU.UetyElkiLj-e5Glcd6oi8XwgFcTHQ8nR27xUA8";

dpp::cluster bot( BOT_TOKEN );

bool StartedPrediction = false;
dpp::snowflake PredictChannel;
dpp::snowflake ResultsChannel;

using json = nlohmann::json;
namespace fs = std::filesystem;

bool CheckFile( std::string file , json & js )
{
	if ( fs::exists( file ) ) {
		Sleep( 3000 );

		std::ifstream input_file( file );

		input_file >> js;
		return true;
	}

	return false;
}

void ReadConfig( ) {

	json js;

	if ( !CheckFile( "C:\\Blaze\\BotConfig.json" , js ) )
		return;

	StartedPrediction = js[ "started" ];
	int PredC = js[ "predict" ];
	int ResC = js[ "results" ];

	PredictChannel = PredC;
	ResultsChannel = ResC;

}


void SaveConfig( ) {

	json js;

	int PredC = PredictChannel;
	int ResC = ResultsChannel;

	js[ "predict" ] = PredC;
	js[ "results" ] = ResC;
	js[ "started" ] = StartedPrediction;

	std::ofstream  arquivo( "C:\\Blaze\\BotConfig.json" );// abre o arquivo para leitura
	if ( arquivo.is_open( ) ) { // verifica se o arquivo foi aberto com sucesso

		arquivo << js;
		arquivo.close( ); // fecha o arquivo
	}
	else {
		std::cout << "Não foi possível abrir o arquivo" << std::endl;
	}
}






void SetupAutoMessages( ) {

	bool possible_white = false;

	std::string old_seed;

	while ( true ) {

		std::this_thread::sleep_for( std::chrono::seconds( 1 ) );

		SaveConfig( );

		if ( StartedPrediction ) {

			if ( PredictChannel ) {

				json js;

				if ( !CheckFile( "C:\\Blaze\\import.json" , js ) ) {
					continue;
				}

				std::string seed = js[ "server_seed" ];

				bool ItsBet = js[ "betted" ];
				bool CoverWhite = js[ "coverwhite" ];

				if ( seed != old_seed/* && ( ItsBet || CoverWhite )*/ ) {

					old_seed = seed;

					int color = js[ "color" ];
					float balance = js[ "balance" ];
					float initbalance = js[ "initialbalance" ];
					float BetValue = js[ "betvalue" ];
					float WhiteBetValue = js[ "whitebetvalue" ];
					std::vector<int>history = js[ "history" ];;
					float profit = balance - initbalance;
					int Accuracy = js[ "accuracy" ];


					std::string prediction_colors = "";
					std::string bet_value = "";
					std::string history_str = "";
					std::string balance_str;
					std::string accuracy_str = std::to_string( Accuracy ) + "%";

					balance_str = "Balance: " + std::to_string( int( balance ) ) + "\n";
					balance_str += "Initial Balance: " + std::to_string( int( initbalance ) ) + "\n";
					balance_str += "Profit: " + std::to_string( int( profit ) ) + "\n";

					for ( auto col : history )
					{
						switch ( col )
						{
						case 0:
							history_str += ":white_circle:";
							break;
						case 1:
							history_str += ":red_circle:";
							break;
						case 2:
							history_str += ":black_circle:";
							break;
						}
					}


					if ( ItsBet )
					{
						switch ( color )
						{
						case 0:
							prediction_colors += ":white_circle:";
							break;
						case 1:
							prediction_colors += ":red_circle:";
							break;
						case 2:
							prediction_colors += ":black_circle:";
							break;
						}

						bet_value += "Bet: " + std::to_string( int( BetValue ) ) + "\n";

					}

					if ( CoverWhite ) {
						prediction_colors += ":white_circle:";
						bet_value += "White Bet: " + std::to_string( int( WhiteBetValue ) ) + "\n";
					}

					std::vector<float> BalanceHistory;

					for ( auto value : js[ "balancehistory" ] ) {
						BalanceHistory.emplace_back( value );
					}

					plot( BalanceHistory , "k" );

					save( "graph.jpg" );
					std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

					dpp::embed embed = dpp::embed( ).
						set_color( dpp::colors::red ).
						set_title( "Prediction" ).
						set_description( "Created by Kun#6394" ).
						set_thumbnail( "https://blaze.com/images/logo-icon.png" ).
						add_field(
							"Color Prediction" ,
							prediction_colors
						).

						add_field(
							"Game History" ,
							history_str ,
							true
						).
						add_field(
							"Accuracy" ,
							accuracy_str ,
							true
						).
						add_field(
							"Bet value" ,
							bet_value ,
							false
						).
						add_field(
							"Balance" ,
							balance_str ,
							false
						).						
						//set_image( "https://dpp.dev/DPP-Logo.png" ).
						set_footer( dpp::embed_footer( ).set_text( "Creation Time " ).set_icon( "https://blaze.com/images/logo-icon.png" ) ).
						set_timestamp( time( 0 ) );

			
					dpp::message msg( PredictChannel , embed );

					// attach the image to the message
					msg.add_file( "image.jpg" , dpp::utility::read_file( "graph.jpg" ) );				
					embed.set_image( "attachment://image.jpg" ); // reference to the attached file
			
					// send the message
					bot.message_create( msg );
				}
			}

			if ( ResultsChannel )
			{
				/*
					result[ "balance" ] = bet->CurrentPlayer.GetBalance( );
					result[ "initialbalance" ] = bet->CurrentPlayer.GetInitialMoney( );
					result[ "profit" ] = bet->CurrentPlayer.GetProfit( );
					result[ "exit" ] = Leave;
				*/

				json js;


				if ( !CheckFile( "C:\\Blaze\\result_import.json" , js ) )
					continue;


				float Balance = js[ "balance" ];
				float InitBalance = js[ "initialbalance" ];
				float Profit = js[ "profit" ];
				int ProfitPercentage = int( ( Profit / InitBalance ) * 100 );
				bool Exit = js[ "exit" ];

				std::string BalanceStr = "Final balance: " + std::to_string( ( int ) Balance ) + "\n";
				BalanceStr += "Start balance: " + std::to_string( ( int ) InitBalance ) + "\n";

				std::string ProfitStr = "Profit: " + std::to_string( ( int ) Profit ) + " ( " + std::to_string( ProfitPercentage ) + "% )";
				std::string Leave = Exit ? "Yes" : "No";

				uint32_t Color;

				if ( Profit > 0 )
				{
					Color = dpp::colors::green;
				}
				else if ( Profit < 0 )
					Color = dpp::colors::red;
				else
					Color = dpp::colors::yellow;



				dpp::embed embed = dpp::embed( ).
					set_color( Color ).
					set_title( "Finish betting" ).
					set_description( "Created by Kun#6394" ).
					set_thumbnail( "https://blaze.com/images/logo-icon.png" ).
					add_field(
						"Balance" ,
						BalanceStr ,
						true
					).
					add_field(
						"Profit" ,
						ProfitStr ,
						true
					).
					add_field(
						"Leave" ,
						Leave ,
						false
					).

					//set_image( "https://dpp.dev/DPP-Logo.png" ).
					set_footer( dpp::embed_footer( ).set_text( "Creation Time " ).set_icon( "https://blaze.com/images/logo-icon.png" ) ).
					set_timestamp( time( 0 ) );

				bot.message_create( dpp::message( ResultsChannel , embed ) );

				fs::remove( "C:\\Blaze\\result_import.json" );
			}
		}
	}
}


int main( ) {

	//ReadConfig( );

	bot.on_log( dpp::utility::cout_logger( ) );

	bot.on_slashcommand( [ & ] ( const dpp::slashcommand_t & event ) {

		if ( event.command.get_command_name( ) == "clear" ) {

			event.reply( "Clearing messages!" );
			dpp::command_completion_event_t completed;

			bot.messages_get( event.command.channel_id , 0 , 0 , 0 , 100 , [ & ] ( const dpp::confirmation_callback_t & callback ) {
				if ( callback.is_error( ) ) {
					std::cerr << "Erro ao obter as mensagens: " << callback.http_info.body << '\n';
					return;
				}

			std::unordered_map<dpp::snowflake , dpp::message> message_map = callback.get<std::unordered_map<dpp::snowflake , dpp::message>>( );
			std::vector<dpp::snowflake> messages_id;

			auto get_messages_id = [ & ] ( const auto & key , const auto & value )
			{
				messages_id.emplace_back( key );
			};

			for ( const std::pair<dpp::snowflake , dpp::message> & n : message_map )
			{
				get_messages_id( n.first , n.second );
			}

			bot.message_delete_bulk( messages_id , event.command.channel_id );

				} );
		}

	if ( event.command.get_command_name( ) == "shutdown" ) {
		event.reply( "Shutting down computer!" );
		Sleep( 6000 );
		system( "shutdown /s /t 0" );
	}

	if ( event.command.get_command_name( ) == "exit" ) {
		event.reply( "Exiting bot!" );
		Sleep( 6000 );
		exit( 0 );
	}

	if ( event.command.get_command_name( ) == "start" ) {
		event.reply( "Starting predicting in this channel!" );
		StartedPrediction = !StartedPrediction;
		PredictChannel = event.command.channel_id;
	}

	if ( event.command.get_command_name( ) == "results" ) {
		event.reply( "Starting printing results in this channel!" );
		ResultsChannel = event.command.channel_id;
	}



		} );



	bot.on_message_create( [ & ] ( const dpp::message_create_t & message ) {

		} );

	/* Create command handler, and specify prefixes */
	dpp::commandhandler command_handler( &bot );
	/* Specifying a prefix of "/" tells the command handler it should also expect slash commands */
	command_handler.add_prefix( "." ).add_prefix( "/" );


	bot.on_ready( [ & ] ( const dpp::ready_t & event ) {
		if ( dpp::run_once<struct register_bot_commands>( ) ) {


			//Create new command
			bot.global_command_create(
				dpp::slashcommand( "clear" , "clear the chat" , bot.me.id )
			);

			bot.global_command_create(
				dpp::slashcommand( "shutdown" , "shutdown the computer" , bot.me.id )
			);

			bot.global_command_create(
				dpp::slashcommand( "exit" , "exit bot" , bot.me.id )
			);

			bot.global_command_create(
				dpp::slashcommand( "start" , "start predicting in this channel" , bot.me.id )
			);

			bot.global_command_create(
				dpp::slashcommand( "results" , "print the results in this channel" , bot.me.id )
			);

			//bot.global_command_create(
			//	dpp::slashcommand( "start" , "start predicting on the current channel" , bot.me.id )
			//);
		}

		} );


	std::thread( SetupAutoMessages ).detach( );


	bot.start( dpp::st_wait );

 // Dados do gráfico
	

}