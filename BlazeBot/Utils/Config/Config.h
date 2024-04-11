#pragma once

#include "..\Singleton.h"
#include <vector>
#include <string>

class Vector2D {

public:
	
	int x , y;

	Vector2D( ) {
		x = 0;
		y = 0;
	}

	Vector2D(int x, int y ) {
		this->x = x;
		this->y = y;
	}

};

class Notification {
public:
	int Type = -1;
	std::string Message = "";
	int Timing = 3000;

};

enum NotificationType
{
	None ,
	Success ,
	Warning ,
	Error ,
	Info ,
	COUNT
};

class cfg : public CSingleton<cfg> {
public:

	struct 
	{
		bool InformationWindow = false;
		bool FullGraph = false;
		bool BalanceWindow = false;
		bool SimulationGraph = false;
		bool HistoryWindow = false;
		bool AccurracyWindow = false;
		bool ShowBetsWindow = false;
		bool ShowPredictionWindow = false;
	}Windows;

	//Betting
	struct {

		struct {

			bool MultiplyBet = false;
			bool ResetIfDifferent = false;
			float BetMultiplier = 40;
			int MultiplyAfterX = 2;
			int MaxMultiplierTimes = 2;
			bool IncrementMinimum = false;

		}type[2];

		struct {

			bool PreventDownPeaks = false;
			float MinimumPeakValue = 20;
			int MinimumPeakDistance = 10;			
			int WaitingTime = 30;


			bool ProtectProfit = false;
			float ProtectIfProfit = 30;
			float ProfitProtectPercentage = 40;

			bool ProtectCapital = true;
			float CapitalProtectPercentage = 60;

			bool PredictDownPeaks = false;

			bool PlayOnlyOnStableMoments = false;

			bool RecoveryModeIfDownPeak = false;
			float DownPercentage = 30.0f;

			bool WaitIfLosing = false;
			int WaitAfterXLose = 3;
			int WaitAmount = 5;
			bool ResetCountIfWin = false;
			bool WaitAgainIfLose = false;

			int StabilityParameterWindow = 30;
			float MaxMedRoll = 2.f;
			float MaxRoll = 3.f;
			float MaxVariancy = 1.0f;
			int RollLimit = 3;


		}security;

		struct {

			bool SimulateBet = false;
			bool AutoBet = false;

			Vector2D RedPoint;
			Vector2D BlackPoint;
			Vector2D WhitePoint;
			Vector2D InputPoint;
			Vector2D BetPoint;


		}automatic;


		float MinBetPercentage = 2;
		float MaxBetPercentage = 50;

		float TargetParcentage = 50;
		float StopPercentage = 50;
	
	}Betting;

	struct {
		int MinPatterSize = 4;
		int IgnoreStreakAfter = 3;
		int StreakBettingSpacing = 1;
		float MinimumPercentage = 50;
		float MaximumPercentage = 70;
		bool InvertIfMissing = false;
		float InvertIfMissingAbove = 50;

	}Prediction;

	struct {
		
		std::vector<float> BalanceHistory;
		std::vector<float> FullBalanceHistory;
		float InitialBalance;
		float CurrentBalance;
		float FullBalance;

		std::vector<Notification> Notifications;


	}Game;

};
