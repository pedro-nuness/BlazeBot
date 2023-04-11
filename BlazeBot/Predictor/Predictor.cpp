#include "Predictor.h"
#include <numeric>

DoublePredictor::DoublePredictor( ) {}

void DoublePredictor::clearResults( ) {
	history.clear( );
}

int DoublePredictor::Total( ) {
	return history.size( );
}

void DoublePredictor::addColor( Color c ) {
	if ( !history.empty( ) ) {
		Color prev = history.back( );
		std::pair<Color , Color> transition = std::make_pair( prev , c );
		transitions[ transition ]++;
	}

	history.push_back( c );
	switch ( c ) {
	case Red:
		counts[ c ] += red_chance;
		break;
	case Blue:
		counts[ c ] += blue_chance;
		break;
	case White:
		counts[ c ] += white_chance;
		break;
	}
	updateTransitionsThree( );
}

void DoublePredictor::updateTransitionsThree( ) {
	for ( size_t i = 2; i < history.size( ); ++i ) {
		std::pair<Color , std::pair<Color , Color>> key = { history[ i - 2 ], {history[ i - 1 ], history[ i ]} };
		if ( transitions_three.find( key ) == transitions_three.end( ) ) {
			transitions_three[ key ] = 1;
		}
		else {
			transitions_three[ key ] += 1;
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
			Color true_label = history[ i ];
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

double DoublePredictor::getCertainty( Color c ) {
	if ( history.empty( ) ) {
		return 0.0;
	}


	int size = history.size( ) / 5;

	size_t window_size = size;
	double alpha = 0.7;
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
		if ( color == c )
			ColorPresence++;
	}
	ColorPresence /= history.size( );

	//Fixed entropy and color presence
	auto ColorEntropy = -ColorPresence * log2( ColorPresence ) - ( 1 - ColorPresence ) * log2( 1 - ColorPresence );

	std::vector<double> SmoothedHistory( history.size( ) , 0 );
	SmoothedHistory[ 0 ] = ( history[ 0 ] == c ) ? 1.0 : 0.0;
	for ( size_t i = 1; i < history.size( ); i++ ) {
		SmoothedHistory[ i ] = alpha * ( ( history[ i ] == c ) ? 1.0 : 0.0 ) + ( 1 - alpha ) * SmoothedHistory[ i - 1 ];
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
			size_t end_index = std::min( ( i + 1 ) * season_length , history.size( ) );
			double event_occurrences = 0.0;
			double total_events = 0.0;
			for ( size_t j = start_index; j < end_index; j++ ) {
				if ( history[ j ] == c ) {
					event_occurrences += ( 1.0 );
				}
				total_events += 1.0;
			}
			total_occurrences += event_occurrences / total_events;
		}
		double average_occurrences = total_occurrences / num_seasons;
		predicted_probability = predicted_probability + average_occurrences;
	}

	double laplaceConstant = 1.5;

	double weightedCountCC = 0.0;
	double weightedCountCCC = 0.0;
	double totalWeight = 0.0;
	for ( size_t i = 1; i < history.size( ); i++ ) {
		double weight = pow( alpha , history.size( ) - i - 1 );
		totalWeight += weight;

		if ( history[ i - 1 ] == c && history[ i ] == c ) {
			double transitionCountCC = transitions[ {history[ i - 1 ] , history[ i ]} ];
			transitionCountCC *= color_chance;
			weightedCountCC += ( transitionCountCC + laplaceConstant ) * weight;
		}

		if ( i > 1 && history[ i - 2 ] == c && history[ i - 1 ] == c && history[ i ] == c ) {
			double transitionCountCCC = transitions_three[ {history[ i - 2 ] , { history[ i - 1 ], history[ i ] }} ];
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
			maxSameColorStreak = std::max( maxSameColorStreak , currentStreak );
		}
		else {
			diffCount++;
			currentStreak = 1;
			maxDiffColorStreak = std::max( maxDiffColorStreak , currentStreak );
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
			TempCol.emplace_back( color );
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
	size_t recentWindowSize = std::min<size_t>( history.size( ) , 10 );

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
	for ( int i = total_jogadas - 1; i >= std::max( total_jogadas - N , 0 ); i-- ) {
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

	return get_probability_of_color( history , c );
}


std::pair<int , double> DoublePredictor::getStreakProbability( Color c , double bayes ) {
	int count = 0;
	for ( int i = history.size( ) - 1; i >= 0; i-- ) {
		if ( history[ i ] == c ) {
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
			if ( history[ i - 1 ] == c && history[ i ] == c ) {
				count++;
			}
			else if ( history[ i - 1 ] == c && history[ i ] != c ) {
				color_count++;
				double weight = pow( decay_factor , history.size( ) - i - 1 );
				double transition_prob = transitions[ {c , history[ i ]} ] / ( color_count + laplace_constant );
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


Transition DoublePredictor::GetGameTransition( ) {

	Transition FinalTransition;

	if ( history.size( ) < 20 )
		return FinalTransition;

	for ( int i = history.size( ) - 17; i < history.size( ); i++ )
	{
		auto color = history.at( i );

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

Prediction DoublePredictor::predictNext( ) {

	Prediction Final_Pred;
	Final_Pred.color = Null;
	Final_Pred.chance = 0.0f;

	if ( history.empty( ) ) {
		return Final_Pred;
	}

	//Primeiramente, vamos verificar a existencia de cores sequenciais
	Streak streak = isStreak( );
	if ( streak.StreakSize ) {
		//We have a streak, let's see it
		if ( streak.StreakSize >= 4 ) {
			//We have a big streak, don't bet anything
			// R, R, R, R -> ?
			Final_Pred.color = Null;
			Final_Pred.chance = 0.0f;
			return Final_Pred;
		}
		else if ( streak.StreakSize >= 2 ) {
			//Small streak
			//R, R -> B
			Color inversecolor = InverseColor( streak.color );
			if ( inversecolor != Null ) {
				Final_Pred.color = inversecolor;
				Final_Pred.chance = getCertainty( inversecolor );
				return Final_Pred;
			}

		}

	}


	std::vector<Color> Sequence;
	int Unmatches = 0;
	int Count = 0;
	for ( int i = history.size( ) - 3; i < history.size( ); i++ )
	{
		auto color = history.at( i );
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
			Final_Pred.color = Sequence.at( 1 );
			Final_Pred.chance = getCertainty( Sequence.at( 1 ) );
			return Final_Pred;
		}
	}

	//Historico presente no site da blaze, ultima jogadas
	Transition GameTransition = GetGameTransition( );

	//Vamos colocar as presencas do gametransition neste vector
	std::vector<int> ColorsPresence;

	for ( int i = 0; i < 6; i++ )
		ColorsPresence.emplace_back( 0 );
	
	for ( auto color : GameTransition.Colors ) {
		ColorsPresence.at( color )++;
	}
	//Organiza o vector em ordem de presenca
	std::sort( ColorsPresence.begin( ) , ColorsPresence.end( ) , i_sort );
	
	//Cor mais presente
	Color MostPresentColor = ( Color ) ColorsPresence.front( );
	Color LessPresentColor = InverseColor( MostPresentColor );
	Color LastColor = history.front( );
	
	
	int MostPresentStreaks = 0;
	int LessPresentStreaks = 0;
	
	int PresentStreakSize = 0;
	int NonPresentStreak = 0;
	
	bool HasPresentStreak = false;
	bool HasLessStreak = false;
	
	//Vamos verificar se existe sequencias desta cor
	for ( auto color : GameTransition.Colors ) {
	
		if ( color == LessPresentColor )
			NonPresentStreak++;
		else {
	
			if ( HasLessStreak ) {
				LessPresentStreaks++;
				HasLessStreak = false;
			}
	
			NonPresentStreak = 0;
		}
	
		if ( color == MostPresentColor )
			PresentStreakSize++;
		else {
			if ( HasPresentStreak ) {
				MostPresentStreaks++;
				HasPresentStreak = false;
			}
	
			PresentStreakSize = 0;
		}
	
		//Achamos uma streak
		if ( PresentStreakSize >= 2 )
		{
			HasPresentStreak = true;
		}
	
		//Achamos uma streak
		if ( NonPresentStreak >= 2 )
		{
			HasLessStreak = true;
		}
	}
	
	//if ( LessPresentStreaks || MostPresentStreaks ) {
	//
	//	if ( LessPresentStreaks > MostPresentStreaks )
	//	{
	//		Final_Pred.color = LessPresentColor;
	//		Final_Pred.chance = getCertainty( LessPresentColor );
	//		return Final_Pred;
	//	}
	//	else if ( MostPresentStreaks > LessPresentStreaks ) {
	//		Final_Pred.color = MostPresentColor;
	//		Final_Pred.chance = getCertainty( MostPresentColor );
	//		return Final_Pred;
	//	}
	//}

	if ( history.size( ) < 70 )
		return Final_Pred;


	//Nao achamos nenhum padrao, vamos utilizar matematica
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
	if ( max_certainty >= 0.55 ) {
		Final_Pred.color = next_color;
		Final_Pred.chance = max_certainty;
	}

	return Final_Pred;
}

Streak DoublePredictor::isStreak( ) {
	int count = 0;
	Color c = Red;
	for ( int i = history.size( ) - 1; i >= 0; i-- ) {
		if ( i == history.size( ) - 1 )
			c = history[ i ];

		if ( history[ i ] == c ) {
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

void DoublePredictor::AddHistory( Color c ) {
	history.push_back( c );
}

std::vector<Color> DoublePredictor::GetHistory( ) {
	return this->history;
}