#ifndef COLORPREDICTOR_H
#define COLORPREDICTOR_H

#include <vector>
#include <utility>
#include <torch/torch.h>
#include <torch/cuda.h>
#include "..\..\Colors\ColorManagement\ColorManagement.h"

// Define o tipo de dado do conjunto de treinamento
using TrainingData = std::vector<std::pair<std::vector<int> , Color>>;

class ColorPredictor {
private:
    torch::nn::Linear inputLayer { nullptr };
    torch::nn::Linear hiddenLayer { nullptr };
    torch::nn::Linear outputLayer { nullptr };
    int window_size = 4;
    int multiplier = 3;
    int Neurons = 64;

    std::string LastSeed = "";

public:
    bool trained = false;

    ColorPredictor( int window_size = 4 );
    bool TraningExist( );
    std::vector<ColorManagement> GetNodeOutput( std::string seed , int amount );
    void SaveModel( const std::string & file_path );
    void LoadModel( const std::string & file_path );
    void Train( const TrainingData & trainingData , const TrainingData & validationData );
    Color Predict( std::vector<ColorManagement> history );
    TrainingData CreateTrainingData( std::vector<ColorManagement> & history );
    TrainingData CreateLastColHistory( std::vector<ColorManagement> & history );
    TrainingData CreateExampleData( std::vector<ColorManagement> history, int amount = 20000);
   
private:
    torch::Tensor Forward( torch::Tensor input );
};

#endif // COLORPREDICTOR_H