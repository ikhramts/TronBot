// MyTronBot.cc
//
// This is the code file that you will modify in order to create your entry.

#include "Map.h"
#include <string>
#include <vector>
#include <deque>
#include <set>

#include "MoveScore.h"
#include "StepEvaluator.h"
#include "Timer.h"

#ifdef TEST_ENVIRONMENT
#include <iostream>
#endif

/* Global variables. */
int gMoveNumber = 0;
StepEvaluator* gStepEvaluator = NULL;

/**
 * The main movement logic function.
 */
int MakeMove(const Map& map) {
	//ForceBreak();
	gMoveNumber++;

	if (1 == gMoveNumber) {
		SetTimeOut(2.8);
		gStepEvaluator = new StepEvaluator();
		gStepEvaluator->initialize(map);
	
	} else {
		SetTimeOut(0.8);
		gStepEvaluator->updateMoves(map);
	}
	
#ifdef TEST_ENVIRONMENT
	SetTimeOut(0.7);
//	SetTimeOut(3600);
#endif
	
	//ForceBreak();

	//Perform as many calculations as possible in the available time.
	while (!HasTimedOut()) {
		const bool hasMoreWork = gStepEvaluator->performEvaluations();
		
		if (!hasMoreWork) {
			break;
		}
	}

	const int bestMove = gStepEvaluator->getBestMove();
	
	//if (gMoveNumber == 2) {
	//	ForceBreak();
	//}
#ifdef TEST_ENVIRONMENT
	std::cerr << gStepEvaluator->getMaxDepth() << " " << gStepEvaluator->getNumEvaluations();
#endif
 

	return bestMove;
}

// Ignore this function. It is just handling boring stuff for you, like
// communicating with the Tron tournament engine.
int main() {
  while (true) {
    Map map;
    Map::MakeMove(MakeMove(map));
  }
  return 0;
}
