#include <torch/torch.h>
#include <torch/script.h>
#include "Predictor.h"
#include <numeric>

#include "..\globals.h"


DoublePredictor::DoublePredictor( ) {}

void DoublePredictor::clearResults( ) {
	history.clear( );
}

int DoublePredictor::Total( ) {
	return history.size( );
}

void DoublePredictor::addColor( ColorManagement c ) {

	if ( !history.empty( ) ) {
		Color prev = history.back( ).GetColor( );
		std::pair<Color , Color> transition = std::make_pair( prev , c.GetColor( ) );
		transitions[ transition ]++;
	}

	auto Value = ColorManagement( c );
	auto Col = Value.GetColor( );

	history.push_back( Value );
	switch ( Value.GetColor( ) ) {
	case Red:
		counts[ Col ] += red_chance;
		break;
	case Blue:
		counts[ Col ] += blue_chance;
		break;
	case White:
		counts[ Col ] += white_chance;
		break;
	}
	updateTransitionsThree( );

}

void DoublePredictor::updateTransitionsThree( ) {

	if ( history.size( ) > 5 ) {

		for ( size_t i = 2; i < history.size( ); ++i ) {
			std::pair<Color , std::pair<Color , Color>> key = { history[ i - 2 ].GetColor( ), {history[ i - 1 ].GetColor( ), history[ i ].GetColor( )} };
			if ( transitions_three.find( key ) == transitions_three.end( ) ) {
				transitions_three[ key ] = 1;
			}
			else {
				transitions_three[ key ] += 1;
			}
		}
	}
}

std::vector<double> DoublePredictor::crossValidate( int k ) {
	std::vector<double> fold_accuracies;

	if ( k < 2 ) {
		return fold_accuracies;
	}

	int fold_size = history.size( ) / k;
	if ( fold_size < 2 ) {
		return fold_accuracies;
	}

	for ( int fold = 0; fold < k; fold++ ) {
		int fold_start = fold * fold_size;
		int fold_end = ( fold + 1 ) * fold_size;
		DoublePredictor classifier;

		for ( int i = 0; i < history.size( ); i++ ) {
			if ( i < fold_start || i >= fold_end ) {
				classifier.addColor( history[ i ] );
			}
		}

		int correct = 0;
		int total = history.size( ) - fold_size;

		for ( int i = fold_start; i < fold_end; i++ ) {
			Color true_label = history[ i ].GetColor( );
			Color predicted_label = Red;
			double max_certainty = 0.0;

			Prediction Pred = classifier.predictNext( );

			if ( Pred.chance && Pred.color != Null ) {
				if ( Pred.color == true_label ) {
					correct++;
				}
			}
			else
				total--;
		}

		double accuracy = ( double ) correct / total;
		fold_accuracies.push_back( accuracy );
	}

	return fold_accuracies;
}



double DoublePredictor::GetTransitionWeight( std::vector<Transition> Transitions , int position ) {

	if ( position > Transitions.size( ) - 1 )
		return 0.0f;

	if ( position < 0 )
		return 0.0f;

	if ( Transitions.empty( ) )
		return 0.0f;

	//Peso da transicao mais recente
	double MaxWeight = 1.0f;
	//Peso da ultima transicao
	double MinWeight = 0.3;

	double totalPositions = Transitions.size( ) - 1;
	double positionFactor = ( double ) position / totalPositions;

	double weight = MinWeight + ( MaxWeight - MinWeight ) * positionFactor;

	return std::clamp( weight , MinWeight , MaxWeight );
}

struct SequenceStats {
	size_t sameColorStreak;
	size_t diffColorStreak;
	double sameColorProb;
	double diffColorProb;
};

SequenceStats detectSequences( const std::vector<Color> & history , Color targetColor ) {
	if ( history.empty( ) ) {
		return { 0, 0, 0, 0 };
	}

	size_t sameCount = 0;
	size_t diffCount = 0;
	size_t maxSameColorStreak = 0;
	size_t maxDiffColorStreak = 0;
	size_t currentStreak = 1;

	for ( size_t i = 1; i < history.size( ); ++i ) {
		if ( history[ i - 1 ] == history[ i ] ) {
			sameCount++;
			currentStreak++;
			maxSameColorStreak = ( std::max ) ( maxSameColorStreak , currentStreak );
		}
		else {
			diffCount++;
			currentStreak = 1;
			maxDiffColorStreak = ( std::max ) ( maxDiffColorStreak , currentStreak );
		}
	}

	double sameProb = static_cast< double >( sameCount ) / ( sameCount + diffCount );
	double diffProb = static_cast< double >( diffCount ) / ( sameCount + diffCount );

	return { maxSameColorStreak, maxDiffColorStreak, sameProb, diffProb };
}


double evaluateTransitions( const std::vector<Color> & history , Color targetColor , size_t windowSize , double weightDecay ) {
	if ( history.size( ) < windowSize ) {
		return 0;
	}

	double weightedSum = 0;
	double totalWeight = 0;
	double weight = 1;

	for ( size_t i = history.size( ) - 1; i >= windowSize - 1; --i ) {
		size_t matches = 0;
		for ( size_t j = 0; j < windowSize; ++j ) {
			if ( history[ i - j ] == targetColor ) {
				matches++;
			}
		}
		weightedSum += matches * weight;
		totalWeight += windowSize * weight;

		weight *= weightDecay;
		if ( i < windowSize ) {
			break;
		}
	}

	return weightedSum / totalWeight;
}

std::vector<Transition> DoublePredictor::GetTransitions( int size ) {
	std::vector<Transition> SortedColors;

	if ( history.size( ) < size ) {
		return SortedColors;
	}
	//Pegar todas as transicoes
	std::vector<Color> TempCol;
	for ( auto color : history )
	{
		if ( TempCol.size( ) < size )
		{
			TempCol.emplace_back( color.GetColor( ) );
		}
		else {
			SortedColors.emplace_back( TempCol );
			TempCol.clear( );
		}
	}

	//Pegar o peso de cada uma
	for ( int i = 0; i < SortedColors.size( ); i++ )
	{
		auto & transition = SortedColors.at( i );

		transition.Weight = GetTransitionWeight( SortedColors , i );
	}

	return SortedColors;
}


double analyzeTrends( const std::vector<Color> & history , Color targetColor ) {
	// Implemente a análise de tendências aqui
	// Esta é uma implementação básica que compara a frequência recente da cor alvo com sua frequência geral.
	// Você pode aprimorar essa função de acordo com suas necessidades.
	if ( history.empty( ) ) {
		return 0;
	}

	size_t targetCount = 0;
	size_t recentCount = 0;
	size_t recentWindowSize = ( std::min <size_t> )( history.size( ) , 10 );

	for ( size_t i = 0; i < history.size( ); ++i ) {
		if ( history[ i ] == targetColor ) {
			targetCount++;
			if ( i >= history.size( ) - recentWindowSize ) {
				recentCount++;
			}
		}
	}

	double recentFrequency = static_cast< double >( recentCount ) / recentWindowSize;
	double overallFrequency = static_cast< double >( targetCount ) / history.size( );

	return recentFrequency / overallFrequency;
}

std::map<Color , double> computeMarkovChains( const std::vector<Color> & history ) {
	// Implemente o cálculo das cadeias de Markov aqui
	// Esta é uma implementação básica que constrói uma matriz de transição a partir do histórico e calcula as probabilidades de estado estacionário.
	if ( history.size( ) < 2 ) {
		return {};
	}

	std::map<Color , std::map<Color , size_t>> transitionMatrix;

	for ( size_t i = 1; i < history.size( ); ++i ) {
		transitionMatrix[ history[ i - 1 ] ][ history[ i ] ]++;
	}

	size_t iterationCount = 20;
	std::map<Color , double> currentState;
	for ( const auto & entry : transitionMatrix ) {
		currentState[ entry.first ] = 1.0 / NUM_COLORS;
	}

	for ( size_t i = 0; i < iterationCount; ++i ) {
		std::map<Color , double> nextState;
		for ( const auto & entry : transitionMatrix ) {
			Color fromColor = entry.first;
			double probSum = 0;
			for ( const auto & destEntry : entry.second ) {
				Color toColor = destEntry.first;
				size_t transitionCount = destEntry.second;
				probSum += currentState[ toColor ] * transitionCount;
			}
			nextState[ fromColor ] = probSum;
		}
		currentState = nextState;
	}

	return currentState;
}

double get_probability_of_last_N_plus_1_jogadas( const std::vector<Color> & history , const std::vector<int> & last_N_jogadas , double prior_probability ) {
	std::vector<double> probabilities( Color::NUM_COLORS , prior_probability );

	auto last_N_jogadas_begin = history.end( ) - last_N_jogadas.size( ) - 1;
	auto last_N_jogadas_end = history.end( ) - 1;
	auto range = std::search( history.begin( ) , history.end( ) , last_N_jogadas_begin , last_N_jogadas_end );

	int count_last_N_jogadas = 0;
	while ( range != history.end( ) ) {
		count_last_N_jogadas++;
		range = std::search( range + 1 , history.end( ) , last_N_jogadas_begin , last_N_jogadas_end );
	}

	for ( int i = 0; i < Color::NUM_COLORS; i++ ) {
		int count_i = std::count( history.begin( ) , history.end( ) - 1 , static_cast< Color >( i ) );
		probabilities[ i ] = ( count_i + prior_probability ) / ( history.size( ) + Color::NUM_COLORS * prior_probability - count_last_N_jogadas );
	}

	double probability_last_N_plus_1_jogadas = 1.0;
	for ( int i = 0; i < last_N_jogadas.size( ); i++ ) {
		probability_last_N_plus_1_jogadas *= probabilities[ last_N_jogadas[ i ] ];
	}
	probability_last_N_plus_1_jogadas *= prior_probability;

	return probability_last_N_plus_1_jogadas;
}

double get_probability_of_last_N_jogadas( const std::vector<int> & last_N_jogadas ) {
	if ( last_N_jogadas.empty( ) ) {
		return 1.0;
	}

	std::vector<double> probabilities( Color::NUM_COLORS , 0.0 );
	for ( int i = 0; i < last_N_jogadas.size( ); i++ ) {
		probabilities[ last_N_jogadas[ i ] ] += 1.0;
	}
	for ( int i = 0; i < Color::NUM_COLORS; i++ ) {
		probabilities[ i ] /= last_N_jogadas.size( );
	}

	double probability_last_N_jogadas = 1.0;
	for ( int i = 0; i < Color::NUM_COLORS; i++ ) {
		probability_last_N_jogadas *= std::pow( probabilities[ i ] , ( last_N_jogadas.size( ) * 1.0 / Color::NUM_COLORS ) );
	}

	return probability_last_N_jogadas;
}

double get_probability_of_color( const std::vector<Color> & history , Color color ) {
	const int N = 5; // Número de jogadas anteriores a serem consideradas
	const double prior_probability = 1.0 / Color::NUM_COLORS; // Probabilidade a priori de uma cor ser sorteada (1/3)

	int total_jogadas = history.size( );
	int count_cor = std::count( history.begin( ) , history.end( ) , color );
	double probability_cor = static_cast< double >( count_cor ) / total_jogadas;

	// Probabilidade de que as últimas N jogadas foram de certas cores, independentemente da cor da próxima jogada
	std::vector<int> last_N_jogadas;
	for ( int i = total_jogadas - 1; i >= ( std::max ) ( total_jogadas - N , 0 ); i-- ) {
		last_N_jogadas.push_back( static_cast< int >( history[ i ] ) );
	}
	std::reverse( last_N_jogadas.begin( ) , last_N_jogadas.end( ) );
	double probability_last_N_jogadas = get_probability_of_last_N_jogadas( last_N_jogadas );

	// Probabilidade de que a próxima jogada será de uma cor específica, dado que as últimas N jogadas foram de certas cores
	double probability_next_cor = probability_last_N_jogadas * prior_probability / get_probability_of_last_N_plus_1_jogadas( history , last_N_jogadas , prior_probability );

	return probability_next_cor;
}


double DoublePredictor::getCertaintyRecoded( Color c )
{
	//double transitionWeight = 0.25;
	//double sequenceWeight = 0.25;
	//double trendWeight = 0.25;
	//
	//double transitionProb = evaluateTransitions( history , c , 10 , 0.3 );
	//
	//SequenceStats sequenceStats = detectSequences( history , c );
	//double sameColorProb = sequenceStats.sameColorProb;
	//double diffColorProb = sequenceStats.diffColorProb;
	//double trendProb = analyzeTrends( history , c );
	//
	//double sequenceProb = ( c == history.back( ) ) ? sameColorProb : diffColorProb;
	//
	//double weightedProb = 
	//	transitionWeight * transitionProb +
	//	sequenceWeight * sequenceProb +
	//	trendWeight * trendProb;
	//
	//
	//return weightedProb; 0.481865

}


std::pair<int , double> DoublePredictor::getStreakProbability( Color c , double bayes ) {
	int count = 0;
	for ( int i = history.size( ) - 1; i >= 0; i-- ) {
		if ( history[ i ].GetColor( ) == c ) {
			count++;
		}
		else {
			break;
		}
	}

	if ( count >= 2 ) {
		// calcular a probabilidade de continuar a streak com a cor atual
		double color_count = 0.0;
		double transition_count = 0.0;
		double total_weight = 0.0;
		for ( size_t i = 1; i < history.size( ); i++ ) {
			if ( history[ i - 1 ].GetColor( ) == c && history[ i ].GetColor( ) == c ) {
				count++;
			}
			else if ( history[ i - 1 ].GetColor( ) == c && history[ i ].GetColor( ) != c ) {
				color_count++;
				double weight = pow( decay_factor , history.size( ) - i - 1 );
				double transition_prob = transitions[ {c , history[ i ].GetColor( )} ] / ( color_count + laplace_constant );
				double streak_prob = ( count - 1 + laplace_constant ) / ( history.size( ) - 1 + laplace_constant * 2 );
				double adjusted_prob = streak_prob * transition_prob + ( 1 - streak_prob ) * ( 1 - transition_prob );
				transition_count += adjusted_prob * weight;
				total_weight += weight;
			}
		}
		double color_chance = color_count / history.size( );
		double streak_prob = pow( color_chance , count );
		double adjusted_p_c_bayes = bayes;
		if ( transition_count > 0 ) {
			double transition_prob = transition_count / total_weight;
			adjusted_p_c_bayes = streak_prob * transition_prob + ( 1 - streak_prob ) * ( 1 - color_chance );
		}
		return { count, adjusted_p_c_bayes };
	}
	else {
		return { 0, 0.0 };
	}
}

Color DoublePredictor::getNextColorWithPreference( Color next_color ) {
	// Define os pesos para cada cor
	std::map<Color , double> color_weights = {
		{Red, red_chance},
		{Blue, blue_chance},
		{White, white_chance}
	};

	// Remove a cor 'next_color' da lista de possíveis cores
	color_weights.erase( next_color );

	// Calcula a soma dos pesos restantes
	double total_weight = 0.0;
	for ( const auto & pair : color_weights ) {
		total_weight += pair.second;
	}

	// Gera um número aleatório ponderado
	std::random_device rd;
	std::mt19937 gen( rd( ) );
	std::uniform_real_distribution<> dist( 0.0 , total_weight );
	double random_weight = dist( gen );

	// Seleciona a cor com base no número aleatório ponderado
	Color selected_color;
	double cumulative_weight = 0.0;
	for ( const auto & pair : color_weights ) {
		cumulative_weight += pair.second;
		if ( random_weight <= cumulative_weight ) {
			selected_color = pair.first;
			break;
		}
	}

	return selected_color;
}


Transition DoublePredictor::GetGameTransition( int size ) {

	Transition FinalTransition;

	if ( history.size( ) < 20 )
		return FinalTransition;

	for ( int i = history.size( ) - size; i < history.size( ); i++ )
	{
		auto color = history.at( i ).GetColor( );

		FinalTransition.Colors.emplace_back( color );
	}

	return FinalTransition;
}

Color InverseColor( Color col ) {

	switch ( col )
	{
	case Red:
		return Blue;
	case Blue:
		return Red;
	default:
		return Null;
	}

}

bool i_sort( int a , int b )
{
	return a > b;
}

void ResetPrediction( Prediction * pred ) {
	pred->color = Null;
	pred->chance = 0.0f;
	pred->method = NONE;
}


std::vector<int> generateLabels( std::vector<ColorManagement> & colorData ) {
	std::vector<int> labels;
	for ( auto color : colorData ) {
		labels.push_back( ( color.GetColor( ) ) );
	}
	return labels;
}


void trainModel( std::vector<ColorManagement> & data , int numEpochs , float learningRate ) {
	// Define o modelo da rede neural
	auto model = torch::nn::Sequential(
		torch::nn::Linear( 3 , 64 ) ,
		torch::nn::ReLU( ) ,
		torch::nn::Linear( 64 , 32 ) ,
		torch::nn::ReLU( ) ,
		torch::nn::Linear( 32 , 3 ) ,
		torch::nn::LogSoftmax( 1 )
	);

	// Define a função de perda e o otimizador
	auto lossFunc = torch::nn::NLLLoss( );
	auto optimizer = torch::optim::Adam( model->parameters( ) , torch::optim::AdamOptions( learningRate ) );

	// Define o dispositivo de processamento (CPU ou GPU)
	torch::Device device( torch::kCPU );

	// Verifica se a GPU está disponível e usa-a, se possível
	if ( torch::cuda::is_available( ) ) {
		device = torch::Device( torch::kCUDA );
		model->to( device );
	}

	// Treina o modelo
	for ( int epoch = 0; epoch < numEpochs; ++epoch ) {
		for ( auto & cm : data ) {
			// Prepara os dados de entrada para o modelo
			std::vector<float> input( 2 );
			input[ 0 ] = cm.GetRoll( ) - data[ data.size( ) - 2 ].GetRoll( ); // diferença entre os rolls da última e penúltima jogada
			input[ 1 ] = static_cast< float >( static_cast< int >( cm.GetColor( ) ) ); // cor da última jogada
			//input[ 2 ] = static_cast< float >( std::stoi( cm.GetCreation( ).substr( 11 , 2 ) ) ); // hora da última jogada

			// Converte os dados de entrada em um tensor do PyTorch e envia para o dispositivo de processamento
			auto inputTensor = torch::from_blob( input.data( ) , { 1, 2 } ).to( torch::kFloat32 ).to( device );

			// Faz a previsão usando o modelo e calcula a perda
			auto outputTensor = model->forward( inputTensor );
			auto loss = lossFunc( outputTensor , torch::tensor( { static_cast< int >( static_cast< int >( cm.GetColor( ) ) ) } ).to( device ) );

			// Faz a retropropagação do erro e atualiza os pesos da rede neural
			optimizer.zero_grad( );
			loss.backward( );
			optimizer.step( );
		}
	}

	// Salva o modelo em disco
	torch::save( model , "model.pt" );
}

// Define o tipo de dado do conjunto de treinamento
using TrainingData = std::vector<std::pair<std::pair<int , int> , Color>>;

extern bool Exist( const std::string & name );

class ColorPredictor {
private:
	torch::nn::Sequential model { nullptr };
	std::shared_ptr<torch::optim::Adam> optimizer;
	double learning_rate = 0.001;
	size_t epochs = 200;
	size_t batch_size = 16;

public:

	bool trained = false;

	void SaveModel( const std::string & file_path ) {
		torch::serialize::OutputArchive output_archive;
		model->save( output_archive );
		output_archive.save_to( file_path );
	}

	bool LoadModel( const std::string & file_path ) {
		if ( Exist( file_path ) ) {
			torch::serialize::InputArchive input_archive;
			input_archive.load_from( file_path );
			model->load( input_archive );
			return true;
		}
		else
			return false;
	}

	ColorPredictor( ) {
		model = torch::nn::Sequential(
			torch::nn::Linear( torch::nn::LinearOptions( 2 , 64 ) ) ,
			torch::nn::ReLU( ) ,
			torch::nn::Linear( torch::nn::LinearOptions( 64 , 32 ) ) ,
			torch::nn::ReLU( ) ,
			torch::nn::Linear( torch::nn::LinearOptions( 32 , 3 ) )
		);

		optimizer = std::make_shared<torch::optim::Adam>( model->parameters( ) , torch::optim::AdamOptions( learning_rate ) );
	}


	void Train( const TrainingData & trainingData ) {
		torch::Device device = torch::cuda::is_available( ) ? torch::kCUDA : torch::kCPU;
		model->to( device );

		torch::nn::CrossEntropyLoss lossFunction {};

		for ( size_t epoch = 0; epoch < epochs; ++epoch ) {
			// Dividir dados em mini-batches
			for ( size_t batch_start = 0; batch_start < trainingData.size( ); batch_start += batch_size ) {
				optimizer->zero_grad( );

				auto inputs = torch::empty( { static_cast< int >( std::min( batch_size, trainingData.size( ) - batch_start ) ), 2 } ).to( device );
				auto targets = torch::empty( static_cast< int >( std::min( batch_size , trainingData.size( ) - batch_start ) ) , torch::TensorOptions( ).dtype( torch::kLong ) ).to( device );

				for ( size_t i = batch_start; i < std::min( batch_start + batch_size , trainingData.size( ) ); ++i ) {
					inputs[ i - batch_start ][ 0 ] = trainingData[ i ].first.first;
					inputs[ i - batch_start ][ 1 ] = trainingData[ i ].first.second;
					targets[ i - batch_start ] = static_cast< int >( trainingData[ i ].second );
				}

				auto output = model->forward( inputs );
				auto loss = lossFunction( output , targets );
				loss.backward( );
				optimizer->step( );
			}

			std::cout << "Finished epoch " << epoch << std::endl;
		}
	}

	// Faz uma previsão com base em uma cor e um rolo
	Color Predict( int roll , Color color ) {
		auto input = torch::empty( { 1, 2 } );
		input[ 0 ][ 0 ] = roll;

		switch ( color ) {
		case Color::White:
			input[ 0 ][ 1 ] = 0;
			break;
		case Color::Red:
			input[ 0 ][ 1 ] = 1;
			break;
		case Color::Blue:
			input[ 0 ][ 1 ] = 2;
			break;
		default:
			break;
		}

		auto output = Forward( input );
		auto predicted = output.argmax( ).item<int>( );

		switch ( predicted ) {
		case 0:
			return Color::White;
		case 1:
			return Color::Red;
		case 2:
			return Color::Blue;
		default:
			return Color::Null;
		}
	}

private:
	// Executa o passo para frente da rede neural
	torch::Tensor Forward( torch::Tensor input ) {
		auto x = model->forward( input );
		return torch::log_softmax( x , 1 );
	}

};


TrainingData CreateTrainingData( std::vector<ColorManagement> & history , size_t lookback ) {
	TrainingData trainingData;

	for ( size_t i = lookback; i < history.size( ); ++i ) {
		for ( size_t j = 1; j <= lookback; ++j ) {
			auto prevColor = history[ i - j ].GetColor( );
			auto currColor = history[ i ].GetColor( );
			auto roll = history[ i ].GetRoll( );
			if ( prevColor != Color::Null ) {
				trainingData.push_back( { {static_cast< int >( prevColor ), roll}, currColor } );
			}
		}
	}

	std::cout << "Sucessfully created Training Data!\n";

	return trainingData;
}


TrainingData CreateLastColHistory( std::vector<ColorManagement> & history , size_t lookback ) {
	TrainingData trainingData;

	size_t i = history.size( ) - 1;
	for ( size_t j = 1; j <= lookback; ++j ) {
		auto prevColor = history[ i - j ].GetColor( );
		auto currColor = history[ i ].GetColor( );
		auto roll = history[ i ].GetRoll( );
		if ( prevColor != Color::Null ) {
			trainingData.push_back( { {static_cast< int >( prevColor ), roll}, currColor } );
		}
	}
	return trainingData;
}


double DoublePredictor::getCertainty( Color c ) {
	if ( history.empty( ) ) {
		return 0.0;
	}


	int size = 5;

	size_t window_size = size;
	double alpha = 0.5;
	double color_chance = 0.0;

	switch ( c ) {
	case White:
		color_chance = white_chance;
		break;
	case Red:
		color_chance = red_chance;
		break;
	case Blue:
		color_chance = blue_chance;
		break;
	}


	float ColorPresence = 0.0f;

	for ( auto color : history )
	{
		if ( color.GetColor( ) == c )
			ColorPresence++;
	}
	ColorPresence /= history.size( );


	//Fixed entropy and color presence
	auto ColorEntropy = -ColorPresence * log2( ColorPresence ) - ( 1 - ColorPresence ) * log2( 1 - ColorPresence );

	std::vector<double> SmoothedHistory( history.size( ) , 0 );
	SmoothedHistory[ 0 ] = ( history[ 0 ].GetColor( ) == c ) ? 1.0 : 0.0;
	for ( size_t i = 1; i < history.size( ); i++ ) {

		double base_value = history[ i ].GetColor( ) == c ? 1.0 : 0.0;
		double reverse_value = 1.0 - alpha;

		SmoothedHistory[ i ] = ( alpha * base_value ) + ( reverse_value * SmoothedHistory[ i - 1 ] );
	}

	double predicted_probability = 0.0;
	if ( history.size( ) >= window_size ) {
		std::vector<double> probabilities_vector( window_size , 0 );
		for ( size_t i = 0; i < window_size; i++ ) {
			probabilities_vector[ i ] = SmoothedHistory[ history.size( ) - window_size + i ];
		}
		predicted_probability = std::accumulate( probabilities_vector.begin( ) , probabilities_vector.end( ) , 0.0 ) / probabilities_vector.size( );
	}

	size_t season_length = window_size;
	if ( season_length > 0 && history.size( ) >= season_length ) {
		size_t num_seasons = history.size( ) / season_length;
		double total_occurrences = 0.0;
		for ( size_t i = 0; i < num_seasons; i++ ) {
			size_t start_index = i * season_length;
			size_t end_index = ( std::min ) ( ( i + 1 ) * season_length , history.size( ) );
			double event_occurrences = 0.0;
			double total_events = 0.0;
			for ( size_t j = start_index; j < end_index; j++ ) {
				if ( history[ j ].GetColor( ) == c ) {
					event_occurrences += ( 1.0 );
				}
				total_events += 1.0;
			}
			total_occurrences += event_occurrences / total_events;
		}
		double average_occurrences = total_occurrences / num_seasons;
		predicted_probability = predicted_probability + average_occurrences;
	}

	double laplaceConstant = 1.0;

	double weightedCountCC = 0.0;
	double weightedCountCCC = 0.0;
	double totalWeight = 0.0;
	for ( size_t i = 1; i < history.size( ); i++ ) {
		double weight = pow( alpha , history.size( ) - i - 1 );
		totalWeight += weight;

		if ( history[ i - 1 ].GetColor( ) == c && history[ i ].GetColor( ) == c ) {
			double transitionCountCC = transitions[ {history[ i - 1 ].GetColor( ) , history[ i ].GetColor( )} ];
			transitionCountCC *= color_chance;
			weightedCountCC += ( transitionCountCC + laplaceConstant ) * weight;
		}

		if ( i > 1 && history[ i - 2 ].GetColor( ) == c && history[ i - 1 ].GetColor( ) == c && history[ i ].GetColor( ) == c ) {
			double transitionCountCCC = transitions_three[ {history[ i - 2 ].GetColor( ) , { history[ i - 1 ].GetColor( ), history[ i ].GetColor( ) }} ];
			transitionCountCCC *= color_chance;
			weightedCountCCC += ( transitionCountCCC + laplaceConstant ) * weight;
		}
	}

	double transitionProbCC = weightedCountCC / ( totalWeight * ( history.size( ) - 1 + 2 * laplaceConstant ) );
	double transitionProbCCC = weightedCountCCC / ( totalWeight * ( history.size( ) - 2 + 3 * laplaceConstant ) );
	double combinedTransitionProb = 0.5 * transitionProbCC + 0.5 * transitionProbCCC;

	if ( combinedTransitionProb ) {
		predicted_probability += combinedTransitionProb;
		predicted_probability /= 3;
	}

	double bayesian_probability = ColorPresence;
	if ( predicted_probability > 0 ) {
		double prior_mean = ColorPresence;
		double prior_var = 0.1;
		double likelihood_mean = predicted_probability;
		double likelihood_var = 0.01;
		double posterior_var = 1.0 / ( 1.0 / prior_var + 1.0 / likelihood_var );
		double posterior_mean = posterior_var * ( prior_mean / prior_var + likelihood_mean / likelihood_var );
		bayesian_probability = std::clamp( posterior_mean , 0.0 , 1.0 );
	}

	return std::clamp( predicted_probability , 0.0 , 1.0 );
}

Color DoublePredictor::CertaintyPrediction( ) {

	double max_certainty = -1.0;
	Color next_color = Color::Red;
	for ( int i = 0; i < NUM_COLORS; i++ ) {
		Color c = static_cast< Color >( i );
		double certainty = getCertainty( c );
		if ( certainty > max_certainty ) {
			max_certainty = certainty;
			next_color = c;
		}
	}

	return InverseColor( next_color );
}

ColorPredictor ColorIA {};

extern std::string Folder;
extern std::string IAPATH;

Color DoublePredictor::IAPrediction( ) {

	size_t lookback = 4;

	//Simple I.A
	TrainingData trainingData;
	if ( !ColorIA.trained ) {
		std::cout << "I.A Training started, it may take a while!\n";

		if ( ColorIA.LoadModel( Folder + IAPATH ) ) {
			std::cout << "Found a previous model of I.A!\n";
		}
		else
			std::cout << "Can't find a previous save of the I.A!\n";

		trainingData = CreateTrainingData( history , 4 );
		ColorIA.trained = true;
	}
	else
		trainingData = CreateLastColHistory( history , 4 );

	
	// Treina o modelo
	ColorIA.Train( trainingData );
	// Faz uma previsão
	auto predictedColor = ColorIA.Predict( history.back( ).GetRoll( ) , history.back( ).GetColor( ) );

	ColorIA.SaveModel( Folder + IAPATH );

	if ( predictedColor != White ) {

		return predictedColor;
	}

	return Null;
}

Color DoublePredictor::SearchPattern( int window_size )
{
	if ( history.size( ) >= window_size * 5 ) {

		std::vector<Color> LastResults;
		for ( int i = history.size( ) - window_size; i < history.size( ); i++ ) {
			LastResults.emplace_back( history[ i ].GetColor( ) );
		}

		std::vector<Color> Predictions;

		int Match = 0;
		for ( int i = history.size( ) - 19; i < history.size( ) - window_size; i++ )
		{
			auto col = history.at( i ).GetColor( );

			if ( Match == LastResults.size( ) )
			{
				Predictions.emplace_back( col );
				Match = 0;
			}

			if ( col == LastResults[ Match ] )
				Match++;
			else
				Match = 0;
		}

		if ( !Predictions.empty( ) ) {

			float RedPresence = 0;
			float BlackPresence = 0;

			for ( auto Pred : Predictions )
			{
				if ( Pred == Red )
					RedPresence++;
				else if ( Pred == Blue )
					BlackPresence++;
			}

			if ( RedPresence > BlackPresence ) {
				return Red;
			}
			else if ( BlackPresence > RedPresence ) {
				return Blue;
			}
		}
	}

	return Null;
}

Color DoublePredictor::PredictSequence( )
{
	std::vector<Color> Sequence;
	int Unmatches = 1;
	int Count = 0;

	for ( int i = history.size( ) - 3; i < history.size( ); i++ )
	{
		auto color = history.at( i ).GetColor( );

		if ( color == White )
		{
			Sequence.clear( );
			Unmatches = 0;
			Count = 0;
			break;
		}

		Sequence.emplace_back( color );
		if ( Count ) {
			if ( Sequence.at( Count - 1 ) != color )
				Unmatches++;
		}
		//B, R, B -> R
		Count++;
	}

	if ( Unmatches == Count )
	{
		if ( !Sequence.empty( ) ) {
			return Sequence.at( 1 );
		}
	}

	return Null;
}


int findMostFrequent( std::vector<int> & nums ) {
	std::unordered_map<int , int> freqMap;

	// Conta a frequência de cada número no vetor.
	for ( int num : nums ) {
		freqMap[ num ]++;
	}

	int mostFrequentNum = 0;
	int maxFreq = 0;

	// Encontra o número com a maior frequência.
	for ( auto & p : freqMap ) {
		if ( p.second > maxFreq ) {
			maxFreq = p.second;
			mostFrequentNum = p.first;
		}
	}

	// Retorna o número com a maior frequência.
	return mostFrequentNum;
}

Prediction DoublePredictor::predictNext( ) {

	Prediction FinalPrediction;
	ResetPrediction( &FinalPrediction );
	FinalPrediction.PossibleWhite = false;

	system( "cls" );

	if ( history.empty( ) ) {
		return FinalPrediction;
	}

	if ( SeparatedPrediction.empty( ) ) {
		for ( int i = 0; i < NONE; i++ )
			SeparatedPrediction.emplace_back( Null );

		std::cout << "Setupped prediction vector to null!\n";
	}
	else
		for ( auto & pred : SeparatedPrediction )
			pred = Null;

	std::vector<int> WhitePositions;
	for ( auto i = 0; i < history.size( ); i++ )
	{
		if ( history[ i ].GetColor( ) == White ) {
			WhitePositions.push_back( i );
		}
	}
	//White prediction 
	//W,R,R,B,R,B,R,B,W, + 7 -> W

	std::vector<int> WhitePos;
	float AverageDistance = 0;


	//Historico presente no site da blaze, ultima jogadas
	Transition GameTransition = GetGameTransition( );

	for ( int i = history.size( ) - 1; i > history.size( ) - 18; i-- )
	{
		if ( WhitePos.size( ) >= 2 )
			break;

		if ( history[ i ].GetColor( ) == White )
		{
			WhitePos.emplace_back( i );
		}
	}
	if ( WhitePos.size( ) >= 2 ) {
		//We have 2+ Whites on the roll

		int LastWhitePos = WhitePos.front( );
		AverageDistance = WhitePos.at( 0 ) - WhitePos.at( 1 );

		if ( LastWhitePos != -1 && AverageDistance >= 3 ) {

			int NextWhitePos = LastWhitePos + AverageDistance;
			int ProbablyNextWhite = NextWhitePos - history.size( );

			if ( fabs( ProbablyNextWhite ) <= 3 ) {

				std::cout << "Average WhiteDistance: " << AverageDistance << std::endl;
				std::cout << "NextWhitePos: " << NextWhitePos << std::endl;
				std::cout << "CurrentPos: " << history.size( ) << std::endl;
				std::cout << "Probably next white in: " << ProbablyNextWhite << std::endl;

				if ( ( history.size( ) ) >= NextWhitePos - 3
					&& ( history.size( ) ) <= NextWhitePos + 3
					) {
					//Probably white
					FinalPrediction.PossibleWhite = true;
				}
			}
		}
	}
	else
	{
		//We don't have any white

		WhitePos.clear( );

		for ( int i = 0; i < history.size( ); i++ )
		{
			auto col = history.at( i ).GetColor( );

			if ( col == White )
			{
				WhitePos.emplace_back( i );
			}
		}

		if ( !WhitePos.empty( ) && WhitePos.size( ) > 2 ) {

			float sum = 0.0f;

			for ( int i = 0; i < WhitePos.size( ) - 1; i++ )
			{
				sum += WhitePos[ i + 1 ] - WhitePos[ i ];
			}

			sum = fabs( sum );

			sum /= WhitePos.size( ) - 1;
			sum = int( sum );

			int LasWhitePos = WhitePos.back( );
			int NextWhitePos = LasWhitePos + sum;
			int ProbablyNextWhite = NextWhitePos - history.size( );

			std::cout << "Average WhiteDistance: " << sum << "\n";
			std::cout << "NextWhitePos: " << NextWhitePos << std::endl;
			std::cout << "CurrentPos: " << history.size( ) << std::endl;
			std::cout << "Probably next white in: " << ProbablyNextWhite << std::endl;

			if ( ( history.size( ) ) >= NextWhitePos - 3
				&& ( history.size( ) ) <= NextWhitePos + 3
				) {
				//Probably white
				FinalPrediction.PossibleWhite = true;
			}
		}
	}

	int RedPoints = 0;
	int BlackPoints = 0;


	//Streak streak = isStreak( );
	//if ( streak.StreakSize >= 2 ) {
	//	// We have a streak, let's see it
	//	Color inversecolor = InverseColor( streak.color );
	//
	//	Color OldColor = Null;
	//	std::vector<int> PresentStreaks;
	//	int Size;
	//	//Get Most Present Streak size
	//	for ( int i = GameTransition.Colors.size( ) - streak.StreakSize + 1; i > 0; i-- )
	//	{
	//		auto color = GameTransition.Colors.at( i );
	//
	//		if ( OldColor != Null )
	//		{
	//			if ( color == OldColor ) {
	//				Size++;
	//			}
	//			else if ( Size ) {
	//				PresentStreaks.emplace_back( Size + 1 );
	//				Size = 0;
	//			}
	//		}
	//
	//		OldColor = color;
	//	}
	//
	//	if ( !PresentStreaks.empty( ) )
	//	{
	//		int MostPresentStreak = findMostFrequent( PresentStreaks );
	//
	//		if ( streak.StreakSize > MostPresentStreak ) {
	//			// We have a big streak, don't bet anything
	//			// R, R, R, R -> ?
	//			ResetPrediction( &FinalPrediction );
	//			return FinalPrediction;
	//		}
	//		else {
	//
	//			static int StartPos = 0;
	//
	//			if ( streak.StreakSize == 2 )
	//				StartPos = 0;
	//
	//			if ( MostPresentStreak >= 2 ) {
	//
	//				std::cout << "Streak BetPos: " << MostPresentStreak << std::endl;
	//
	//				// Small streak, try inverse color every 2 streaks
	//				// R, R -> B, R, R, R, R -> B
	//				if ( streak.StreakSize == StartPos + MostPresentStreak )
	//				{
	//					if ( inversecolor != Null ) {
	//						FinalPrediction.color = inversecolor;
	//						FinalPrediction.chance = getCertainty( inversecolor );
	//						FinalPrediction.method = STREAK;
	//						StartPos = streak.StreakSize;
	//						SeparatedPrediction[ STREAK ] = inversecolor;
	//
	//						if ( inversecolor == Red ) {
	//							RedPoints++;
	//							std::cout << "Streak vote: red\n";
	//						}
	//						else if ( inversecolor == Blue ) {
	//							BlackPoints++;
	//							std::cout << "Streak vote: black\n";
	//						}
	//					}
	//				}
	//			}
	//		}
	//	}
	//}

	std::chrono::high_resolution_clock::time_point Timing = std::chrono::high_resolution_clock::now( );

	//Color SequencePrediction = PredictSequence( );
	//
	//if ( SequencePrediction != Null )
	//{
	//	if ( SequencePrediction == Red ) {
	//		RedPoints++;
	//		std::cout << "Sequence vote: red\n";
	//	}
	//	else if ( SequencePrediction == Blue ) {
	//		BlackPoints++;
	//		std::cout << "Sequence vote: black\n";
	//	}
	//	SeparatedPrediction[ SEQUENCE ] = SequencePrediction;
	//}
	//
	auto SequenceTiming = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - Timing ); // Calcula a diferença entre os tempos em milissegundos
	//std::cout << "Sequence timing: " << SequenceTiming << " ms\n";
	//
	//Timing = std::chrono::high_resolution_clock::now( );
	//
	//Color PatterPrediction = SearchPattern( g_globals.Prediction.MinPatterSize );
	//
	//if ( PatterPrediction != Null )
	//{
	//	if ( PatterPrediction == Red ) {
	//		RedPoints++;
	//		std::cout << "Pattern vote: red\n";
	//	}
	//	else if ( PatterPrediction == Blue ) {
	//		BlackPoints++;
	//		std::cout << "Pattern vote: black\n";
	//	}
	//
	//	SeparatedPrediction[ FOUND_PATTERN ] = PatterPrediction;
	//}
	//
	//SequenceTiming = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - Timing ); // Calcula a diferença entre os tempos em milissegundos
	//std::cout << "Pattern timing: " << SequenceTiming << " ms\n";
	//
	//Timing = std::chrono::high_resolution_clock::now( );

	Color IAPred = IAPrediction( );

	if ( IAPred != Null )
	{
		if ( IAPred == Red ) {
			RedPoints++;
			std::cout << "IA vote: red\n";
		}
		else if ( IAPred == Blue ) {
			BlackPoints++;
			std::cout << "IA vote: black\n";
		}

		SeparatedPrediction[ IA ] = IAPred;

		SequenceTiming = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - Timing ); // Calcula a diferença entre os tempos em milissegundos
		std::cout << "IA timing: " << SequenceTiming << " ms\n";
		Timing = std::chrono::high_resolution_clock::now( );

		FinalPrediction.color = IAPred;
		FinalPrediction.chance = getCertainty( IAPred );
		return FinalPrediction;
	
	}

	ResetPrediction( &FinalPrediction );
	return FinalPrediction;

	

	//Color CertaintyPred = CertaintyPrediction( );
	//
	//if ( CertaintyPred != Null )
	//{
	//	if ( CertaintyPred == Red ) {
	//		RedPoints++;
	//		std::cout << "Certainty vote: red\n";
	//	}
	//	else if ( CertaintyPred == Blue ) {
	//		BlackPoints++;
	//		std::cout << "Certainty vote: black\n";
	//	}
	//
	//	SeparatedPrediction[ CERTAINTY ] = CertaintyPred;
	//}
	//
	//SequenceTiming = std::chrono::duration_cast< std::chrono::milliseconds >( std::chrono::high_resolution_clock::now( ) - Timing ); // Calcula a diferença entre os tempos em milissegundos
	//std::cout << "MATH timing: " << SequenceTiming << " ms\n";
	//Timing = std::chrono::high_resolution_clock::now( );

	//std::cout << "Red votes: " << RedPoints << std::endl;
	//std::cout << "Black votes: " << BlackPoints << std::endl;
	//
	//if ( RedPoints + BlackPoints > 1 && RedPoints != BlackPoints )
	//{
	//	if ( BlackPoints > RedPoints )
	//	{
	//		FinalPrediction.color = Blue;
	//		FinalPrediction.chance = getCertainty( Blue );
	//		FinalPrediction.method = FOUND_PATTERN;
	//		return FinalPrediction;
	//	}
	//	else if ( RedPoints > BlackPoints )
	//	{
	//		FinalPrediction.color = Red;
	//		FinalPrediction.chance = getCertainty( Red );
	//		FinalPrediction.method = FOUND_PATTERN;
	//		return FinalPrediction;
	//	}
	//}
	//else {
	//	ResetPrediction( &FinalPrediction );
	//	return FinalPrediction;
	//}


	//if ( g_globals.Prediction.InvertIfMissing ) {
	//
	//	auto total = float( this->Beats + this->Misses );
	//
	//	if ( ( float( this->Misses ) / total ) > ( g_globals.Prediction.InvertIfMissingAbove / 100 ) )
	//	{
	//		FinalPrediction.color = InverseColor( FinalPrediction.color );
	//		FinalPrediction.chance = getCertainty( FinalPrediction.color );
	//	}
	//}
	//
	//float MinChance = ( g_globals.Prediction.MinimumPercentage / 100 );
	//float MaxChance = ( g_globals.Prediction.MaximumPercentage / 100 );
	//
	//if ( FinalPrediction.chance > MinChance && FinalPrediction.chance < MaxChance )
	//	return FinalPrediction;
	//else
	//{
	//	ResetPrediction( &FinalPrediction );
	//	return FinalPrediction;
	//}
}
Streak DoublePredictor::isStreak( int startpos ) {
	int count = 0;

	int start = 1;

	if ( startpos )
		start = startpos;

	Color c = Red;
	for ( int i = history.size( ) - start; i >= 0; i-- ) {
		if ( i == history.size( ) - start )
			c = history[ i ].GetColor( );

		if ( history[ i ].GetColor( ) == c ) {
			count++;
		}
		else {
			break;
		}
	}
	Streak str;
	str.color = c;
	str.StreakSize = count;
	return str;
}

void DoublePredictor::AddHistory( json c ) {
	history.push_back( ColorManagement( c ) );
}

std::vector<ColorManagement> DoublePredictor::GetHistory( ) {
	return this->history;
}