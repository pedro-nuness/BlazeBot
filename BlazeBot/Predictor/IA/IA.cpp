#include "IA.h"
#include "..\..\Utils\Utils.h"
#include <limits>
#include <torch/serialize/output-archive.h>
#include <torch/serialize/input-archive.h>



extern bool Exist( const std::string & name );
extern std::string Folder;
extern std::string IAModel;
extern std::string JSName;
extern std::string HistoryName;

ColorPredictor::ColorPredictor( int window_size ) {
	this->window_size = window_size;
	inputLayer = torch::nn::Linear( torch::nn::LinearOptions( window_size * multiplier , Neurons ) );
	hiddenLayer = torch::nn::Linear( torch::nn::LinearOptions( Neurons , Neurons / 2 ) );
	outputLayer = torch::nn::Linear( torch::nn::LinearOptions( Neurons / 2 , 3 ) );
}

void ColorPredictor::SaveModel( const std::string & file_path )
{
	torch::serialize::OutputArchive output_iLayer;
	torch::serialize::OutputArchive output_hLayer;
	torch::serialize::OutputArchive output_oLayer;

	inputLayer->save( output_iLayer );
	hiddenLayer->save( output_hLayer );
	outputLayer->save( output_oLayer );


	output_iLayer.save_to( file_path + "_iLayer.pt" );
	output_hLayer.save_to( file_path + "_hLayer.pt" );
	output_oLayer.save_to( file_path + "_oLayer.pt" );
}

void ColorPredictor::LoadModel( const std::string & file_path )
{
	torch::serialize::InputArchive input_iLayer;
	torch::serialize::InputArchive input_hLayer;
	torch::serialize::InputArchive input_oLayer;

	input_iLayer.load_from( file_path + "_iLayer.pt" );
	input_hLayer.load_from( file_path + "_hLayer.pt" );
	input_oLayer.load_from( file_path + "_oLayer.pt" );

	inputLayer->load( input_iLayer );
	hiddenLayer->load( input_hLayer );
	outputLayer->load( input_oLayer );
}

void ColorPredictor::Train( const TrainingData & trainingData , const TrainingData & validationData )
{
	// Verificar se a GPU (CUDA) está disponível e selecionar o dispositivo
	torch::Device device = torch::cuda::is_available( ) ? torch::kCUDA : torch::kCPU;

	// Mover o modelo para o dispositivo selecionado (GPU, se disponível)
	inputLayer->to( device );

	bool SetuppedLoss = false;
	double min_validation_loss = (std::numeric_limits<double>::min)( );
	double last_loss = (std::numeric_limits<double>::min)( );
	int epochs_no_improve = 0;
	int n_epochs_stop = 1; // número de épocas para parar o treinamento se não houver melhora

	torch::optim::Adam optimizer( inputLayer->parameters( ) , torch::optim::AdamOptions( 0.001 ) );
	torch::nn::CrossEntropyLoss lossFunction {};

	for ( size_t epoch = 0; epoch < 1; ++epoch ) {
		optimizer.zero_grad( );
		auto inputs = torch::empty( { static_cast< int >( trainingData.size( ) ), window_size * multiplier } ).to( device );
		auto targets = torch::empty( static_cast< int >( trainingData.size( ) ) , torch::TensorOptions( ).dtype( torch::kLong ) ).to( device );

		for ( size_t i = 0; i < trainingData.size( ); ++i ) {
			for ( size_t j = 0; j < window_size * multiplier; ++j ) {
				inputs[ i ][ j ] = trainingData[ i ].first[ j ];
			}
			targets[ i ] = static_cast< int >( trainingData[ i ].second );
		}

		auto output = Forward( inputs );
		auto loss = lossFunction( output , targets );
		loss.backward( );
		optimizer.step( );

		std::cout << "Epoch: " << epoch << ", Loss: " << int( loss.item<double>( ) ) << std::endl;

		// validate the model

		float sum = 0;
		float total = validationData.size( );

		for ( size_t i = 0; i < validationData.size( ) - 1; ++i ) {

			auto val_inputs = torch::empty( { 1, window_size * multiplier } );

			for ( size_t j = 0; j < window_size * multiplier; ++j ) {
				val_inputs[ 0 ][ j ] = validationData[ i ].first[ j ];
			}

			auto val_output = Forward( val_inputs );
			auto predicted = val_output.argmax( ).item<int>( );

			if ( static_cast< Color >(predicted) != Null ) {

				if ( static_cast< Color > (predicted ) == static_cast< Color >( trainingData[ i ].second ) )
					sum++;
			}
			else
				total--;
		}

		sum /= total;
		float val_loss = sum;

		std::cout << "Validation of Epoch: " << epoch << ", Loss: " << val_loss << std::endl;

		// check for improvement
		if ( val_loss > min_validation_loss ) {
			min_validation_loss = val_loss;
			epochs_no_improve = 0;
		}
		else {
			epochs_no_improve++;
			if ( epochs_no_improve == n_epochs_stop ) {
				std::cout << "Early stopping!" << std::endl;
				break; // early stop
			}
		}

		if ( last_loss  && SetuppedLoss)
		{
			if ( fabs( val_loss - last_loss ) < 0.05 )
			{
				std::cout << "Small improvement, stopping!" << std::endl;
				break; // early stop
			}
		}
		SetuppedLoss = true;
		last_loss = val_loss;
	}
}

#define SHA256_DIGEST_LENGTH 32

int ConvertSeed( std::string seed ) {

	std::string nSedd = seed.substr( 0 , 16 );

	std::stringstream ss;
	for ( int i = 0; i < SHA256_DIGEST_LENGTH; ++i ) {
		ss << std::hex << static_cast< int >( seed[ i ] );
		if ( i >= 4 )
			break;
	}
	std::string hashHex = ss.str( );

	return std::stoull( hashHex , nullptr , 16 );
}

Color ColorPredictor::Predict( std::vector<ColorManagement> history )
{
	auto input = torch::empty( { 1, window_size * multiplier } );
	for ( size_t i = 0; i < window_size; ++i ) {
		int val = ConvertSeed( history[ i ].GetServerID( ) );

		input[ 0 ][ i * multiplier ] = static_cast< int >( history[ i ].GetColor( ) );
		input[ 0 ][ i * multiplier + 1 ] = val;
		input[ 0 ][ i * multiplier + 2 ] = history[ i ].GetRoll( );
	}

	auto output = Forward( input );
	auto predicted = output.argmax( ).item<int>( );

	return ( Color ) predicted;
}

TrainingData ColorPredictor::CreateTrainingData( std::vector<ColorManagement> & history ) {
	TrainingData trainingData;
	for ( size_t i = window_size; i < history.size( ); ++i ) {
		auto inputData = std::vector<int>( window_size * multiplier );
		for ( int j = 0; j < window_size; ++j ) {

			int val = ConvertSeed( history[ i - window_size + j ].GetServerID( ) );
			inputData[ j * multiplier ] = static_cast< int >( history[ i - window_size + j ].GetColor( ) );
			inputData[ j * multiplier + 1 ] = val;
			inputData[ j * multiplier + 2 ] = history[ i - window_size + j ].GetRoll( );
		}
		auto currColor = history[ i ].GetColor( );

		if ( currColor == White )
			continue;

		trainingData.push_back( { inputData, currColor } );
	}
	return trainingData;
}

TrainingData ColorPredictor::CreateLastColHistory( std::vector<ColorManagement> & history ) {
	TrainingData trainingData;

	if ( history.size( ) < window_size + 1 ) {
		return trainingData;
	}

	auto inputData = std::vector<int>( window_size * multiplier );
	for ( int j = 0; j < window_size; ++j ) {
		int val = ConvertSeed( history[ history.size( ) - ( window_size + 1 ) + j ].GetServerID( ) );
		inputData[ j * multiplier ] = static_cast< int >( history[ history.size( ) - ( window_size + 1 ) + j ].GetColor( ) );
		inputData[ j * multiplier + 1 ] = val;
		inputData[ j * multiplier + 2 ] = history[ history.size( ) - ( window_size + 1 ) + j ].GetRoll( );
	}
	auto currColor = history.back( ).GetColor( );

	if ( currColor != White )
		trainingData.push_back( { inputData, currColor } );

	return trainingData;
}

std::vector<ColorManagement> ColorPredictor::GetNodeOutput( std::string seed , int amount )
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

bool ColorPredictor::TraningExist( ) {
	if ( !Exist( Folder + IAModel + "_iLayer.pt" ) )
		return false;

	if ( !Exist( Folder + IAModel + "_hLayer.pt" ) )
		return false;

	if ( !Exist( Folder + IAModel + "_oLayer.pt" ) )
		return false;

	return true;
}

TrainingData ColorPredictor::CreateExampleData( std::vector<ColorManagement> history, int amount ) {

	std::string Seed = "";

	if ( LastSeed == "" ) {
		Seed = history[ 0 ].GetServerID( );
	}
	else
		Seed = LastSeed;

	std::vector<ColorManagement> Output = GetNodeOutput( Seed , amount );

	if ( !Output.empty( ) ) {

		LastSeed = Output[ 0 ].GetServerID( );
		std::cout << "New last seed: " << LastSeed << "\n";

		return CreateTrainingData( Output );
	}
}


torch::Tensor ColorPredictor::Forward( torch::Tensor input ) {
	auto x = torch::relu( inputLayer->forward( input ) );
	x = torch::relu( hiddenLayer->forward( x ) );
	x = outputLayer->forward( x );
	return torch::log_softmax( x , 1 );
}