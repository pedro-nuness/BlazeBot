#include <torch/torch.h>
#include <torch/script.h>
#include "Predictor.h"
#include <numeric>
#include "IA\IA.h"

#include "..\Utils\Config\Config.h"

#include "..\Utils\Utils.h"

#include <filesystem>
namespace fs = std::filesystem;

extern bool Exist( const std::string & name );
extern std::string Folder;
extern std::string IAModel;
extern std::string JSName;
extern std::string HistoryName;

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

double DoublePredictor::getCertainty( Color c ) {
	if ( history.empty( ) ) {
		return 0.0;
	}

	int size = 15;

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

	double bayesian_probability = 0.0;
	if ( predicted_probability > 0 ) {
		double prior_mean = ColorPresence;
		double prior_var = 0.1;
		double likelihood_mean = predicted_probability;
		double likelihood_var = 0.01;
		double posterior_var = 1.0 / ( 1.0 / prior_var + 1.0 / likelihood_var );
		double posterior_mean = posterior_var * ( prior_mean / prior_var + likelihood_mean / likelihood_var );
		bayesian_probability = std::clamp( posterior_mean , 0.0 , 1.0 );
	}

	return bayesian_probability;
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

int IASize = 17;
ColorPredictor ColorIA( IASize );

float BetterAccurracy = 0.0f;

float TotalAttemps = 0;
float Hits = 0;
float RollMed = 0;
float Accurracy = 0;

int RollLoses = 0;
int MaxRolLose = 0;

float Variancy = 0;

int FirstSize = 0;

std::string results_str = "";

std::vector<int> Rolls;

#define TRAINING_MODE false

bool loaded = false;

Color DoublePredictor::IAPrediction( ) {

#if TRAINING_MODE == true
	if ( !loaded ) {
		if ( ColorIA.TraningExist( ) )
		{
			ColorIA.LoadModel( Folder + IAModel );
		}

		loaded = true;
	}

	std::cout << results_str;
	results_str.clear( );

	TrainingData trainingData = ColorIA.CreateExampleData( history , 200000 );

	TrainingData ParameterExample = ColorIA.CreateExampleData( history );

	if ( !trainingData.empty( ) && !ParameterExample.empty( ) )
		// Treina o modelo	
		ColorIA.Train( trainingData , ParameterExample );

	ColorIA.SaveModel( Folder + IAModel );
	std::cout << "Saved model, path: " << Folder + IAModel << std::endl;

	Accurracy = 0;
	RollMed = 0;
	Hits = 0;
	TotalAttemps = 0;
	Rolls.clear( );
	RollLoses = 0;
	MaxRolLose = 0;
	Variancy = 0;

	if ( !FirstSize )
		FirstSize = history.size( );

	for ( int j = FirstSize - 2; j > IASize + 1; j-- ) {

		TotalAttemps++;

		std::vector<ColorManagement> label;

		for ( int i = j; i > j - IASize; i-- )
		{
			label.emplace_back( history[ i ] );
		}

		// Faz uma previsão
		auto predictedColor = ColorIA.Predict( label );
		if ( predictedColor != White && predictedColor != Null && history[ j + 1 ].GetColor( ) != White ) {

			if ( predictedColor == history[ j + 1 ].GetColor( ) ) {
				Hits++;
				if ( RollLoses )
				{
					Rolls.emplace_back( RollLoses );
					RollLoses = 0;
				}

			}
			else {
				RollLoses++;
			}
		}
		else
			TotalAttemps--;
	}

	for ( auto Roll : Rolls )
	{
		if ( Roll > MaxRolLose )
			MaxRolLose = Roll;

		RollMed += Roll;

	}

	RollMed /= Rolls.size( );

	Accurracy = ( Hits / TotalAttemps ) * 100;

	for ( auto roll : Rolls )
	{
		//Squared
		Variancy += maths::Sqr( fabs( roll - RollMed ) );
	}

	Variancy = Variancy / Rolls.size( );

	results_str += "\nAccurracy: " + std::to_string( Accurracy ) + "\n";
	results_str += "Med Roll: " + std::to_string( RollMed ) + "\n";
	results_str += "Max Roll: " + std::to_string( MaxRolLose ) + "\n";
	results_str += "Num Rolls: " + std::to_string( Rolls.size( ) ) + "\n";
	results_str += "Variancy: " + std::to_string( Variancy ) + "\n";

	if ( Accurracy > BetterAccurracy ) {
		BetterAccurracy = Accurracy;

		std::string FolderName = "Model-" + std::to_string( BetterAccurracy ) + "\\";
		std::string nFolder = Folder + FolderName;

		fs::create_directory( nFolder );

		ColorIA.SaveModel( nFolder + IAModel );
		std::cout << "Saved better model, path: " << nFolder + IAModel << std::endl;

		Utils::Get( ).WriteData( FolderName + "info.txt" , results_str , true );
		json results_js;
		results_js[ "accurracy" ] = Accurracy;
		results_js[ "med_roll" ] = RollMed;
		results_js[ "max_roll" ] = MaxRolLose;
		results_js[ "variancy" ] = Variancy;
		results_js[ "rolls" ] = Rolls.size( );

		std::string Js_Str = results_js.dump( );

		Utils::Get( ).WriteData( FolderName + "info.json" , Js_Str , true );
	}

	results_str += "Better Accurracy: " + std::to_string( BetterAccurracy ) + "\n\n";

#else

	if ( !ColorIA.trained ) {

		TrainingData trainingData;

		if ( ColorIA.TraningExist( ) ) {
			std::cout << "Previous model found!\n";
			ColorIA.LoadModel( Folder + IAModel );
			ColorIA.trained = true;
		}
		else {

			trainingData = ColorIA.CreateTrainingData( history );
			TrainingData ParameterExample = ColorIA.CreateExampleData( this->history );

			if ( !ParameterExample.empty( ) ) {

				// Treina o modelo	
				ColorIA.Train( trainingData , ParameterExample );

				ColorIA.SaveModel( Folder + IAModel );
				std::cout << "Saved model, path: " << Folder + IAModel << std::endl;
				ColorIA.trained = true;
			}
		}
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

#endif

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

Color DoublePredictor::StreakSolve( ) {

	//Historico presente no site da blaze, ultima jogadas
	Transition GameTransition = GetGameTransition( );

	Streak streak = isStreak( );

	int Points[ 3 ] { 0 };


	if ( streak.StreakSize >= 2 && streak.color != White )
	{
		for ( int i = history.size( ) - 1; i > history.size( ) - 15; i-- ) {

			switch ( history[ i ].GetColor( ) )
			{
			case Red:
				Points[ Red ]++;
				break;
			case  Blue:
				Points[ Blue ]++;
				break;
			}
		}

		if ( Points[ streak.color ] >= 9 )
		{
			return streak.color;
		}
		else if ( Points[ InverseColor( streak.color ) ] >= 9 )
		{
			return InverseColor( streak.color );
		}
	}

	return Null;

	//Streak BeforeStreak = isStreak( streak.StreakSize );
	//if ( streak.StreakSize >= 2 ) {
	//	// We have a streak, let's see it
	//	Color inversecolor = InverseColor( streak.color );
	//
	//	if ( streak.StreakSize >= cfg::Get( ).Prediction.IgnoreStreakAfter ) {
	//		// We have a big streak, don't bet anything
	//		// R, R, R, R -> ?
	//		return Null;
	//	}
	//	else {
	//
	//		static int StartPos = 0;
	//
	//		if ( streak.StreakSize == 2 )
	//			StartPos = 0;
	//
	//		Color OldColor = Null;
	//		std::vector<int> PresentStreaks;
	//		int Size;
	//		//Get Most Present Streak size
	//		for ( int i = GameTransition.Colors.size( ) - streak.StreakSize + 1; i > 0; i-- )
	//		{
	//			auto color = GameTransition.Colors.at( i );
	//
	//			if ( OldColor != Null )
	//			{
	//				if ( color == OldColor ) {
	//					Size++;
	//				}
	//				else if ( Size ) {
	//					PresentStreaks.emplace_back( Size + 1 );
	//					Size = 0;
	//				}
	//			}
	//
	//			OldColor = color;
	//		}
	//
	//		if ( !PresentStreaks.empty( ) )
	//		{
	//			int MostPresentStreak = findMostFrequent( PresentStreaks );
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
	//						StartPos = streak.StreakSize;
	//
	//						return inversecolor;
	//					}
	//
	//				}
	//			}
	//		}
	//
	//		return Null;
	//	}
	//}
	//else if ( BeforeStreak.StreakSize >= 4 )
	//{
	//
	//
	//}
}


Color DoublePredictor::SearchPattern( int window_size )
{
	if ( history.size( ) >= window_size * 5 ) {

		std::vector<bool> Matches;

		std::vector<Color> LastThree;

		for ( int i = history.size( ) - window_size; i < history.size( ); i++ )
		{
			if ( history[ i ].GetColor( ) == White )
				return Null;

			LastThree.emplace_back( history[ i ].GetColor( ) );
		}

		int Presence[ 3 ] { 0 };
		bool Correpondente[ 3 ] { false };

		for ( auto col : LastThree )
		{
			Presence[ ( int ) col ] ++;
		}

		if ( Presence[ Red ] > Presence[ Blue ] )
		{
			Correpondente[ Red ] = true;
			Correpondente[ Blue ] = false;
		}
		else if ( Presence[ Blue ] > Presence[ Red ] )
		{
			Correpondente[ Blue ] = true;
			Correpondente[ Red ] = false;
		}

		for ( auto col : LastThree )
		{
			Matches.emplace_back( Correpondente[ ( int ) col ] );
		}

		std::vector<bool> Predictions;


		std::vector<std::pair<std::vector<bool> , bool>> Pairs;

		for ( int i = history.size( ) - ( 15 - window_size ); i < history.size( ) - window_size; i++ )
		{
			std::vector<int> Three;

			if ( history[ i - 2 ].GetColor( ) != White ) {
				Three.emplace_back( history[ i - 2 ].GetColor( ) );
			}
			else
				continue;

			if ( history[ i - 1 ].GetColor( ) != White ) {
				Three.emplace_back( history[ i - 1 ].GetColor( ) );
			}
			else
				continue;

			if ( history[ i ].GetColor( ) != White ) {
				Three.emplace_back( history[ i ].GetColor( ) );
			}
			else
				continue;

			int ColorPresence[ 3 ] { 0 };
			bool Respective[ 3 ] { false };

			for ( auto col : Three )
			{
				ColorPresence[ col ]++;
			}

			std::vector<bool> ThreeM;
			if ( ColorPresence[ Red ] > ColorPresence[ Blue ] )
			{
				Respective[ Red ] = true;
				Respective[ Blue ] = false;
			}
			else if ( ColorPresence[ Blue ] > ColorPresence[ Red ] )
			{
				Respective[ Blue ] = true;
				Respective[ Red ] = false;
			}

			for ( auto col : Three )
			{
				ThreeM.emplace_back( Respective[ col ] );
			}

			if ( history[ i + 1 ].GetColor( ) != White ) {

				Pairs.emplace_back( ThreeM , Respective[ history[ i + 1 ].GetColor( ) ] );
			}
		}

		if ( Pairs.empty( ) )
			return Null;

		for ( auto pair : Pairs )
		{
			if ( pair.first == Matches )
			{
				//Equal Sequence
				Predictions.emplace_back( pair.second );
			}
		}

		if ( !Predictions.empty( ) ) {

			int Votes[ 3 ];

			for ( auto prediction : Predictions )
			{
				if ( Correpondente[ Red ] == prediction )
					Votes[ Red ]++;
				else if ( Correpondente[ Blue ] == prediction )
					Votes[ Blue ]++;
			}

			if ( Votes[ Red ] > Votes[ Blue ] )
				return Red;
			else if ( Votes[ Blue ] > Votes[ Red ] )
				return Blue;
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



Color DoublePredictor::PredictLogic( ) {

	std::vector<Color> Last7;
	int RedPresence = 0;
	int BlackPresence = 0;

	for ( int i = history.size( ) - 1; i > history.size( ) - 8; i-- )
	{
		Color col = history[ i ].GetColor( );

		switch ( col )
		{
		case Red:
			RedPresence++;
			break;
		case Blue:
			BlackPresence++;
			break;
		default:
			return Null;
			break;
		}
	}

	if ( RedPresence > BlackPresence )
		return Blue;
	else
		return Red;
}

void DoublePredictor::SetupVote( Color c , int ID ) {

	std::string Name;

	switch ( ID )
	{
	case SEQUENCE:
		Name = "SEQUENCE";
		break;
	case IA:
		Name = "IA";
		break;
	case FOUND_PATTERN:
		Name = "PATTERN";
		break;
	case CERTAINTY:
		Name = "MATH";
		break;
	case STREAK:
		Name = "STREAK";
		break;
	case GENERAL:
		Name = "GENERAL";
		break;
		//case LOGIC:
		//	Name = "LOGIC";
		//	break;
	}

	if ( c != Null )
	{
		if ( SeparatedBeats.empty( ) ) {
			if ( c == Red ) {
				RedPoints++;
				std::cout << Name << " vote: red\n";
			}
			else if ( c == Blue ) {
				BlackPoints++;
				std::cout << Name << " vote: black \n";
			}
		}
		else if ( !SeparatedBeats[ ID ].OnBadTrip( ) )
		{
			if ( c == Red ) {
				RedPoints++;
				std::cout << Name << " vote: red ( " << SeparatedBeats[ ID ].GetRawHitsPercentage( ) << " )\n";
			}
			else if ( c == Blue ) {
				BlackPoints++;
				std::cout << Name << " vote: black ( " << SeparatedBeats[ ID ].GetRawHitsPercentage( ) << " )\n";
			}
		}
		else {
			std::cout << Name << " is on a bad trip!\n";
		}

		SeparatedPrediction[ ID ] = c;
	}
	else {
		std::cout << Name << " didn't voted!\n";
	}

}

Prediction DoublePredictor::predictNext( ) {

	Prediction FinalPrediction;
	RedPoints = 0;
	BlackPoints = 0;
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

		int PositionDifference = ( history.size( ) - 1 ) - LastWhitePos;
		int WhitePositionGame = ( history.size( ) - 1 ) - PositionDifference;

		if ( LastWhitePos != -1 && AverageDistance >= 1 ) {

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
	else if ( WhitePos.size( ) )
	{
		//We have at least one white
		int LastWhitePos = -1;

		for ( int i = WhitePos[ 0 ]; i > 0; i-- )
		{
			if ( history[ i ].GetColor( ) == White )
			{
				LastWhitePos = i;
				break;
			}
		}

		int Diff = fabs( WhitePos[ 0 ] - LastWhitePos );
		if ( Diff <= 16 && Diff >= 1 )
		{
			//We had two whites
			int NextWhitePos = WhitePos[ 0 ] + Diff;
			int ProbablyNextWhite = NextWhitePos - history.size( );

			std::cout << "Average WhiteDistance: " << Diff << std::endl;
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

	std::cout << "\n";

#if  TRAINING_MODE == true

	Color IAPred = IAPrediction( );

	SetupVote( IAPred , IA );

	RedPoints = 0;
	BlackPoints = 0;
	ResetPrediction( &FinalPrediction );
	return FinalPrediction;

#else


	Streak streak = isStreak( );
	if ( streak.StreakSize >= cfg::Get( ).Prediction.IgnoreStreakAfter )
	{
		RedPoints = 0;
		BlackPoints = 0;
		ResetPrediction( &FinalPrediction );
		return FinalPrediction;
	}

	Color IAPred = IAPrediction( );

	SetupVote( IAPred , IA );

	Color StreakPred = StreakSolve( );

	SetupVote( StreakPred , STREAK );

	Color SequencePrediction = PredictSequence( );

	SetupVote( SequencePrediction , SEQUENCE );

	Color PatterPrediction = SearchPattern( cfg::Get( ).Prediction.MinPatterSize );

	SetupVote( PatterPrediction , FOUND_PATTERN );

	//Color CertaintyPred = CertaintyPrediction( );

	//SetupVote( CertaintyPred , CERTAINTY );

	std::cout << "\nRed votes: " << RedPoints << std::endl;
	std::cout << "Black votes: " << BlackPoints << std::endl;

	Color GeneralPred = Null;

	if ( RedPoints + BlackPoints >= 2 && RedPoints != BlackPoints )
	{
		if ( RedPoints > BlackPoints )
		{
			GeneralPred = Red;
		}
		else if ( BlackPoints > RedPoints ) {

			GeneralPred = Blue;
		}

		SetupVote( GeneralPred , GENERAL );

		if ( GeneralPred != Null )
		{
			FinalPrediction.color = GeneralPred;
			FinalPrediction.chance = getCertainty( GeneralPred );
			FinalPrediction.method = GENERAL;
		}

		return FinalPrediction;
	}
	else {
		RedPoints = 0;
		BlackPoints = 0;
		ResetPrediction( &FinalPrediction );
		return FinalPrediction;
	}

#endif //  TRAINING_MODE == true

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
		start += startpos;

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