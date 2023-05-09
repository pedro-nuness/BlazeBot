#pragma once

struct Globals{

	struct {
		bool InformationWindow = false;
		bool BalanceWindow = false;
		bool HistoryWindow = false;
		bool AccurracyWindow = false;
		bool ShowBetsWindow = false;
		bool ShowPredictionWindow = false;
	}Windows;

	//Betting
	struct {

		bool MultiplyBetOnWin = false;
		bool ResetNextBetMultiplierOnLose = false;
		float WonBetMultiplier = 40;
		int MultiplyBetAfterXWin = 2;
		int MaxMultiplierTimesOnWin = 2;

	
		bool MultiplyBetOnLose = false;
		bool ResetNextBetMultiplierOnWin = false;
		int MaxMultiplierTimesOnLose = 2;
		int MultiplyBetAfterXLose = 2;
		float LoseBetMultiplier = 50;
	

		float MinBetPercentage = 2;
		float MaxBetPercentage = 50;

		float TargetParcentage = 50;
		float StopPercentage = 50;

		bool SimulateBet = false;
		bool AutoBet = false;

		bool WaitIfLosing = false;
		int WaitAfterXLose = 3;
		int WaitAmount = 5;
		bool ResetCountIfWin = false;
		bool WaitAgainIfLose = false;

		bool PreventDownPeaks = false;
		int MinimumPeakDistance = 10;
		float PreventIfAbove = 20;
		int WaitingTime = 30;


		bool ProtectProfit = false;
		float ProtectAfter = 30;
		float ProtectPercentage = 40;


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

};
extern Globals g_globals;