#include <torch/torch.h>
#include <torch/script.h>
#include "Predictor.h"
#include <numeric>

#include "..\Utils\Config\Config.h"


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


double DoublePredictor::getCertaintyRecoded( Color c )
{


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
	pred->PossibleWhite = false;
}


// Define o tipo de dado do conjunto de treinamento
using TrainingData = std::vector<std::pair<std::vector<int> , Color>>;

class ColorPredictor {
private:
	torch::nn::Linear inputLayer { nullptr };
	torch::nn::Linear hiddenLayer { nullptr };
	torch::nn::Linear outputLayer { nullptr };
	int window_size = 4;

public:

	bool trained = false;

	ColorPredictor( int window_size = 4 ) {
		this->window_size = window_size;
		inputLayer = torch::nn::Linear( torch::nn::LinearOptions( window_size * 2 , 32 ) );
		hiddenLayer = torch::nn::Linear( torch::nn::LinearOptions( 32 , 16 ) );
		outputLayer = torch::nn::Linear( torch::nn::LinearOptions( 16 , 3 ) );
	}

	void SaveModel( const std::string & file_path ) {
		torch::serialize::OutputArchive output_archive;
		inputLayer->save( output_archive );
		hiddenLayer->save( output_archive );
		outputLayer->save( output_archive );
		output_archive.save_to( file_path );
	}

	void LoadModel( const std::string & file_path ) {
		torch::serialize::InputArchive input_archive;
		input_archive.load_from( file_path );
		inputLayer->load( input_archive );
		hiddenLayer->load( input_archive );
		outputLayer->load( input_archive );
	}

	void Train( const TrainingData & trainingData ) {
		// Verificar se a GPU (CUDA) está disponível e selecionar o dispositivo
		torch::Device device = torch::cuda::is_available( ) ? torch::kCUDA : torch::kCPU;

		// Mover o modelo para o dispositivo selecionado (GPU, se disponível)
		inputLayer->to( device );

		torch::optim::Adam optimizer( inputLayer->parameters( ) , torch::optim::AdamOptions( 0.001 ) );
		torch::nn::CrossEntropyLoss lossFunction {};

		for ( size_t epoch = 0; epoch < 200; ++epoch ) {
			optimizer.zero_grad( );
			auto inputs = torch::empty( { static_cast< int >( trainingData.size( ) ), window_size * 2 } ).to( device );
			auto targets = torch::empty( static_cast< int >( trainingData.size( ) ) , torch::TensorOptions( ).dtype( torch::kLong ) ).to( device );

			for ( size_t i = 0; i < trainingData.size( ); ++i ) {
				for ( size_t j = 0; j < window_size * 2; ++j ) {
					inputs[ i ][ j ] = trainingData[ i ].first[ j ];
				}
				targets[ i ] = static_cast< int >( trainingData[ i ].second );
			}

			auto output = Forward( inputs );
			auto loss = lossFunction( output , targets );
			loss.backward( );
			optimizer.step( );
		}
	}

	// Faz uma previsão com base em uma cor e um rolo
	Color Predict( std::vector<ColorManagement> history ) {

		auto input = torch::empty( { 1, window_size * 2 } );
		for ( size_t i = 0; i < window_size; ++i ) {
			input[ 0 ][ i * 2 ] = static_cast< int >( history[ i ].GetColor( ) );
			input[ 0 ][ i * 2 + 1 ] = history[ i ].GetRoll( );
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

	TrainingData CreateTrainingData( std::vector<ColorManagement> & history ) {
		TrainingData trainingData;
		for ( size_t i = window_size; i < history.size( ); ++i ) {
			auto inputData = std::vector<int>( window_size * 2 );
			for ( int j = 0; j < window_size; ++j ) {
				inputData[ j * 2 ] = static_cast< int >( history[ i - window_size + j ].GetColor( ) );
				inputData[ j * 2 + 1 ] = history[ i - window_size + j ].GetRoll( );
			}
			auto currColor = history[ i ].GetColor( );
			trainingData.push_back( { inputData, currColor } );
		}
		return trainingData;
	}


	TrainingData CreateLastColHistory( std::vector<ColorManagement> & history ) {
		TrainingData trainingData;

		if ( history.size( ) < window_size + 1 ) {
			return trainingData;
		}

		auto inputData = std::vector<int>( window_size * 2 );
		for ( int j = 0; j < window_size; ++j ) {
			inputData[ j * 2 ] = static_cast< int >( history[ history.size( ) - ( window_size + 1 ) + j ].GetColor( ) );
			inputData[ j * 2 + 1 ] = history[ history.size( ) - ( window_size + 1 ) + j ].GetRoll( );
		}
		auto currColor = history.back( ).GetColor( );
		trainingData.push_back( { inputData, currColor } );

		return trainingData;
	}



private:
	// Executa o passo para frente da rede neural
	torch::Tensor Forward( torch::Tensor input ) {
		auto x = torch::relu( inputLayer->forward( input ) );
		x = torch::relu( hiddenLayer->forward( x ) );
		x = outputLayer->forward( x );
		return torch::log_softmax( x , 1 );
	}

};



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

int IASize = 4;
ColorPredictor ColorIA( IASize );

extern bool Exist( const std::string & name );
extern std::string Folder;
extern std::string IAModel;

Color DoublePredictor::IAPrediction( ) {

	//Simple I.A
	TrainingData trainingData;
	if ( !ColorIA.trained ) {
		//if ( Exist( Folder + IAModel ) ) {
		//	std::cout << "Previous model found!\n";
		//	ColorIA.LoadModel( Folder + IAModel );
		//	trainingData = ColorIA.CreateLastColHistory( history );
		//}
		//else
		trainingData = ColorIA.CreateTrainingData( history );

		ColorIA.trained = true;

		// Treina o modelo	
		ColorIA.Train( trainingData );

		ColorIA.SaveModel( Folder + IAModel );
		std::cout << "Saved model, path: " << Folder + IAModel << std::endl;
	}
	
	std::vector<ColorManagement> label;

	for ( int i = history.size( ) - ( IASize + 1 ); i < history.size( ); i++ )
	{
		label.emplace_back( history[ i ] );
	}

	// Faz uma previsão
	auto predictedColor = ColorIA.Predict( label );
	if ( predictedColor != White ) {

		return predictedColor;
	}

	return Null;
}

Color DoublePredictor::SearchPattern( int window_size )
{
	if ( history.size( ) >= window_size * 5 ) {

		std::vector<Color> LastThree;
		for ( int i = history.size( ) - window_size; i < history.size( ); i++ ) {
			LastThree.emplace_back( history.at( i ).GetColor( ) );
		}

		std::vector<Color> Predictions;

		int Match = 0;
		for ( int i = history.size( ) - 19; i < history.size( ) - window_size; i++ )
		{
			auto col = history.at( i ).GetColor( );

			if ( Match == LastThree.size( ) )
			{
				Predictions.emplace_back( col );
				Match = 0;
			}

			if ( col == LastThree.at( Match ) )
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

	for ( int i = history.size( ) - 1; i > 0; i-- )
	{
		if ( WhitePos.size( ) >= 2 )
			break;

		if ( history[ i ].GetColor( ) == White )
		{
			WhitePos.emplace_back( i );
		}
	}

	int WindowTrying = 3;

	if ( WhitePos.size( ) >= 2 ) {
		//We have 2+ Whites on the roll

		AverageDistance = WhitePos.at( 0 ) - WhitePos.at( 1 );

		int LastWhitePos = WhitePos.front( );
		
		int PositionDifference = (history.size( ) - 1) - LastWhitePos;
		int WhitePositionGame = ( history.size( ) - 1 ) - PositionDifference;
		



		if ( LastWhitePos != -1 && AverageDistance >= 3 ) {

			int NextWhitePos = LastWhitePos + AverageDistance;
			int ProbablyNextWhite = NextWhitePos - history.size( );

			if ( fabs( ProbablyNextWhite ) <= WindowTrying ) {

				std::cout << "Average WhiteDistance: " << AverageDistance << std::endl;
				std::cout << "NextWhitePos: " << NextWhitePos << std::endl;
				std::cout << "CurrentPos: " << history.size( ) << std::endl;
				std::cout << "Probably next white in: " << ProbablyNextWhite << std::endl;

				if ( ( history.size( ) ) >= NextWhitePos - WindowTrying
					&& ( history.size( ) ) <= NextWhitePos + WindowTrying
					) {
					//Probably white
					FinalPrediction.PossibleWhite = true;
				}
			}
		}
	}


	if ( !FinalPrediction.PossibleWhite )
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

			float AverageDistance = 0.0f;

			for ( int i = 0; i < WhitePos.size( ) - 1; i++ )
			{
				AverageDistance += WhitePos[ i + 1 ] - WhitePos[ i ];
			}

			AverageDistance = fabs( AverageDistance );
			AverageDistance /= WhitePos.size( ) - 1;
			AverageDistance = int( AverageDistance );

			int LasWhitePos = WhitePos.back( );
			int NextWhitePos = LasWhitePos + AverageDistance;
			int ProbablyNextWhite = NextWhitePos - history.size( );
			int LasWhiteDistance = history.size( ) - LasWhitePos;

			if ( float( LasWhitePos ) / ( AverageDistance ) >= 2 ) {
				WindowTrying++;
			}

			if ( ProbablyNextWhite <= -fabs( WindowTrying + 1 ) )
			{
				while ( ProbablyNextWhite <= -fabs( WindowTrying + 1 ) )
				{
					NextWhitePos += AverageDistance;
					ProbablyNextWhite = NextWhitePos - history.size( );
				}
			}

			std::cout << "Average WhiteDistance: " << AverageDistance << "\n";
			std::cout << "NextWhitePos: " << NextWhitePos << std::endl;
			std::cout << "CurrentPos: " << history.size( ) << std::endl;
			std::cout << "Probably next white in: " << ProbablyNextWhite << std::endl;

			if ( ( history.size( ) ) >= NextWhitePos - WindowTrying
				&& ( history.size( ) ) <= NextWhitePos + WindowTrying
				) {
				//Probably white
				FinalPrediction.PossibleWhite = true;
			}
		}
	}

	int RedPoints = 0;
	int BlackPoints = 0;


	Streak streak = isStreak( );
	if ( streak.StreakSize >= 2 ) {
		// We have a streak, let's see it
		Color inversecolor = InverseColor( streak.color );

		if ( streak.StreakSize >= cfg::Get().Prediction.IgnoreStreakAfter ) {
			// We have a big streak, don't bet anything
			// R, R, R, R -> ?
			ResetPrediction( &FinalPrediction );
			return FinalPrediction;
		}
		else {

			static int StartPos = 0;

			if ( streak.StreakSize == 2 )
				StartPos = 0;

			Color OldColor = Null;
			std::vector<int> PresentStreaks;
			int Size;
			//Get Most Present Streak size
			for ( int i = GameTransition.Colors.size( ) - streak.StreakSize + 1; i > 0; i-- )
			{
				auto color = GameTransition.Colors.at( i );

				if ( OldColor != Null )
				{
					if ( color == OldColor ) {
						Size++;
					}
					else if ( Size ) {
						PresentStreaks.emplace_back( Size + 1 );
						Size = 0;
					}
				}

				OldColor = color;
			}

			if ( !PresentStreaks.empty( ) )
			{
				int MostPresentStreak = findMostFrequent( PresentStreaks );

				if ( MostPresentStreak >= 2 ) {

					std::cout << "Streak BetPos: " << MostPresentStreak << std::endl;

					// Small streak, try inverse color every 2 streaks
					// R, R -> B, R, R, R, R -> B
					if ( streak.StreakSize == StartPos + MostPresentStreak )
					{
						if ( inversecolor != Null ) {
							StartPos = streak.StreakSize;
							SeparatedPrediction[ STREAK ] = inversecolor;
							if ( inversecolor == Red ) {
								RedPoints++;
								std::cout << "streak vote: red\n";
							}
							else if ( inversecolor == Blue ) {
								BlackPoints++;
								std::cout << "streak vote: black\n";
							}
						}
					}
				}
			}

			ResetPrediction( &FinalPrediction );
			return FinalPrediction;
		}
	}

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
	}

	Color SequencePrediction = PredictSequence( );

	if ( SequencePrediction != Null )
	{
		if ( SequencePrediction == Red ) {
			RedPoints++;
			std::cout << "Sequence vote: red\n";
		}
		else if ( SequencePrediction == Blue ) {
			BlackPoints++;
			std::cout << "Sequence vote: black\n";
		}
		SeparatedPrediction[ SEQUENCE ] = SequencePrediction;
	}

	Color PatterPrediction = SearchPattern( cfg::Get( ).Prediction.MinPatterSize );

	if ( PatterPrediction != Null )
	{
		if ( PatterPrediction == Red ) {
			RedPoints++;
			std::cout << "Pattern vote: red\n";
		}
		else if ( PatterPrediction == Blue ) {
			BlackPoints++;
			std::cout << "Pattern vote: black\n";
		}

		SeparatedPrediction[ FOUND_PATTERN ] = PatterPrediction;
	}

	Color CertaintyPred = CertaintyPrediction( );

	if ( CertaintyPred != Null )
	{
		if ( CertaintyPred == Red ) {
			RedPoints++;
			std::cout << "Certainty vote: red\n";
		}
		else if ( CertaintyPred == Blue ) {
			BlackPoints++;
			std::cout << "Certainty vote: black\n";
		}

		SeparatedPrediction[ CERTAINTY ] = CertaintyPred;
	}

	std::cout << "Red votes: " << RedPoints << std::endl;
	std::cout << "Black votes: " << BlackPoints << std::endl;

	if ( RedPoints + BlackPoints > 1 && RedPoints != BlackPoints )
	{
		if ( BlackPoints > RedPoints )
		{
			FinalPrediction.color = Blue;
			FinalPrediction.chance = getCertainty( Blue );
			FinalPrediction.method = FOUND_PATTERN;
			SeparatedPrediction[ GENERAL ] = Blue;
			return FinalPrediction;
		}
		else if ( RedPoints > BlackPoints )
		{
			FinalPrediction.color = Red;
			FinalPrediction.chance = getCertainty( Red );
			SeparatedPrediction[ GENERAL ] = Red;
			return FinalPrediction;
		}
	}
	else {
		ResetPrediction( &FinalPrediction );
		return FinalPrediction;
	}


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