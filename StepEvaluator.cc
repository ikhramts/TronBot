
#include <vector>
#include <deque>
#include <list>
#include <bitset>
#include "Map.h"
#include "StepEvaluator.h"
#include "Timer.h"

/*****************************
    Class StepEvaluator
*****************************/
StepEvaluator::StepEvaluator()
: cCells_(NULL), iWidth_(0), iSize_(0), iMe_(0), iOpponent_(0), numCellsRemaining_(0), 
rootStep_(NULL), evaluationQue_(), removedCellIndexes_(), removedCells_(), 
branchingQue_(), maxDepth_(), currentDepth_(0), removedPathCells_(NULL), 
removedPathCellIndexes_(NULL) {
}

StepEvaluator::~StepEvaluator() {
	//No need to clean up, the program's done.
}

void StepEvaluator::initialize(const Map& map) {
	iWidth_ = static_cast<TCellIndex>(map.Width());
	TCellIndex iHeight = static_cast<TCellIndex>(map.Height());
	iSize_ = iWidth_ * iHeight;

	//Initialize the move score calculator.
	InitMoveScoreCalculator(iSize_, iWidth_);
	
	removedPathCells_ = new TCell[iSize_];
	removedPathCellIndexes_ = new TCellIndex[iSize_];

	//Initialize the edges per cell matrix.  Specifically,
	//for every non-wall cell, count the number of non-wall neighbours.
	bool* isWall = new bool[iSize_];
	TCellIndex offset = 0;

	for (TCellIndex y = 0; y < iHeight; ++y) {
		offset = y * iWidth_;

		for (int x = 0; x < iWidth_; ++x) {
			isWall[offset + x] = map.IsWall(x, y);
			
			if(!isWall[offset + x]) {
				numCellsRemaining_++;
			}
		}
	}
	
	cCells_ = new TCell[iSize_];
	
	for (TCellIndex cell = 0; cell < iSize_; ++cell) {
		if (!isWall[cell]) {
			TCell cEdges = 0;
			TCell cEdgeCount = 0;

			for (int direction = 1; direction <= 4; ++direction) {
				if (!isWall[GetNeighbour(cell, direction)]) {
					cEdges |= (EDGE_BEFORE_FIRST << direction);
					cEdgeCount++;
				}
			}

			cCells_[cell] = (cEdgeCount | cEdges | NOT_WALL);
		
		} else {
			cCells_[cell] = 0;
		}
	}

	delete[] isWall;
	isWall = NULL;

	//My and opponent's positions.
	iMe_ = static_cast<TCellIndex>(map.MyY() * iWidth_ + map.MyX());
	iOpponent_ = static_cast<TCellIndex>(map.OpponentY() * iWidth_ + map.OpponentX());
	RemoveCell(cCells_, iMe_, iWidth_);
	RemoveCell(cCells_, iOpponent_, iWidth_);
	numCellsRemaining_ -= 2;
	
	//Initialize the first step.
	rootStep_ = getStep();
	rootStep_->initialize(iMe_, iOpponent_, NULL, 0);
	rootStep_->setDepth(currentDepth_);
	rootStep_->setFarFromOpponent(true);
	rootStep_->setSeparatedFromOpponent(false);
	rootStep_->branch();

	numEvaluations_ = 0;
}

void StepEvaluator::updateMoves(const Map& map) {
	const TCellIndex iNewMe = static_cast<TCellIndex>(map.MyY() * iWidth_ + map.MyX());
	const TCellIndex iNewOpponent = static_cast<TCellIndex>(map.OpponentY() * iWidth_ + map.OpponentX());

	//Find out which direction me and opponent went.
	int myDirection = 0;
	int opponentDirection = 0;

	const TCellIndex iMyDifference = iNewMe - iMe_;
	const TCellIndex iOpponentDifference = iNewOpponent - iOpponent_;

	if (iMyDifference == 1) {
		myDirection = RIGHT - 1;

	} else if (iMyDifference == -1) {
		myDirection = LEFT - 1;

	} else if (iMyDifference == iWidth_) {
		myDirection = DOWN - 1;

	} else {
		myDirection = UP - 1;
	}

	if (iOpponentDifference == 1) {
		opponentDirection = RIGHT - 1;

	} else if (iOpponentDifference == -1) {
		opponentDirection = LEFT - 1;

	} else if (iOpponentDifference == iWidth_) {
		opponentDirection = DOWN - 1;

	} else {
		opponentDirection = UP - 1;
	}

	//Update the field map.
	RemoveCell(cCells_, iNewMe, iWidth_);
	RemoveCell(cCells_, iNewOpponent, iWidth_);
	numCellsRemaining_ -= 2;

	iMe_ = iNewMe;
	iOpponent_ = iNewOpponent;

	numEvaluations_ = 0;
	maxDepth_ = 0;

	//Update the root step.
	rootStep_ = rootStep_->advance(myDirection, opponentDirection);
	
	//Remove steps no longer under consideration from
	//the evaluation que.
	//EvaluationQue::iterator queElement = evaluationQue_.begin();
	//const EvaluationQue::iterator queEnd = evaluationQue_.end();

	//while (queEnd != queElement) {
	//	EvaluationQue::iterator currentElement = queElement;
	//	queElement++;

	//	if (!(*currentElement)->isInStepTree()) {
	//		(*currentElement)->removeFromEvaluationQue();
	//		evaluationQue_.erase(currentElement);
	//	}
	//}
	
	currentDepth_++;
}

bool StepEvaluator::performEvaluations() {
	//Get the next step from the que.
	Step* step = NULL;
	
	while (true) {
		if (evaluationQue_.empty()) {
			return false;
		}

		step = evaluationQue_.front();
		evaluationQue_.pop_front();
		
		//Check whether the step is no longer under consideration.
		if (step->isInStepTree()) {
			//Don't evaluate steps past certain depth.
			if (step->getDepth() > this->currentDepth_ + kDepthLimit) {
				//reappend the step back to the back of the que.
				this->addStepToQue(step);
				return true;
			} else {
				//Evaluate this step.
				break;
			}
		
		} else {
			step->removeFromEvaluationQue();
			step = NULL;
			//Return because we don't want to spend too long working on this.
			return true;
		}
	}
	
	Step* parentStep = step->getParent();

	while(NULL != parentStep) {
		removedCellIndexes_.push_back(parentStep->getMyPosition());
		removedCellIndexes_.push_back(parentStep->getOpponentPosition());

		parentStep = parentStep->getParent();
	}
	
	const int numCellsRemoved = static_cast<int>(removedCellIndexes_.size());

	for (int i = 0; i < numCellsRemoved; ++i) {
		removedCells_.push_back(RemoveCell(cCells_, removedCellIndexes_[i], iWidth_));
	}

	//Evaluate the score for this move.
	const TCellIndex iMe = step->getMyPosition();
	const TCellIndex iOpponent = step->getOpponentPosition();

	int moveScore = 0;

	if (iMe == iOpponent || (!cCells_[iMe] && !cCells_[iOpponent])) {
		moveScore = 0;
		step->setDeadEnd(true);
	
	} else if (!cCells_[iMe]) {
		moveScore = VERY_BAD;
		step->setDeadEnd(true);

	} else if (!cCells_[iOpponent]) {
		moveScore = VERY_GOOD;
		step->setDeadEnd(true);
	
	} else {
		numEvaluations_++;
		
		//Step* parent = step->getParent();
		//Step* parentParent = parent->getParent();

		//if (parentParent 
		//	&& iMe == 100 && iOpponent == 122
		//	&& parent->getMyPosition() == 116 && parent->getOpponentPosition() == 123
		//	&& parentParent->getMyPosition() == 132 && parentParent->getOpponentPosition() == 139) {
		//		int x = 2;
		//}

		//Perform full evaluation.
//		moveScore = GetCellBalance(cCells_, iMe, iOpponent, step->getPrevMyPosition(), step->getPrevOpponentPosition());
//		moveScore = GetPathScore(cCells_, iMe, iOpponent, step->getPrevMyPosition(), step->getPrevOpponentPosition());
		
		//Check whether we should switch to a 'near' strategy.
		//ForceBreak();
		
		//if (!step->isSeparatedFromOpponent() && step->isFarFromOpponent()) {
		//	const short distanceToOpponent = 
		//		GetOpponentDistance(cCells_, step->getMyPosition(), step->getOpponentPosition());

		//	if (distanceToOpponent < kMinFarDistance) {
		//		step->setFarFromOpponent(false);
		//	}
		//}

		moveScore = this->calculatePathScore(step);
	}

	//Place the removed cells back in the reverse order of removing them.
	for (int i = numCellsRemoved - 1; i >= 0; --i) {
		AddCell(cCells_, removedCells_[i], removedCellIndexes_[i], iWidth_);
	}

	removedCells_.clear();
	removedCellIndexes_.clear();

	//Update the step state.
	step->setScore(moveScore);
	step->removeFromEvaluationQue();
	
	const int depth = numCellsRemoved / 2;
	if (depth > maxDepth_) {
		maxDepth_ = depth - 1;
	}

	return true;
}

/**
 * Calculate the path score of a step, i.e. start from this step
 * and see how the game will play out.  The score will depend
 * on the final outcome.
 */
TMoveScore StepEvaluator::calculatePathScore(Step* step) {
	//Assume that the step passed checks in evaluateStep().
	const TCellIndex iMe = step->getMyPosition();
	const TCellIndex iOpponent = step->getOpponentPosition();
	const TCellIndex iPrevMe = step->getPrevMyPosition();
	const TCellIndex iPrevOpponent = step->getPrevOpponentPosition();
	
	//ForceBreak();

	//Find the move score for the current cells; 
	//If they are separated already, return that move score.
	TMoveScore basicMoveScore = GetCellBalance(cCells_, iMe, iOpponent, iPrevMe, iPrevOpponent, true /* use trees */);
	bool areAlreadySeparated = WereBotsSeparated();
	step->setSeparatedFromOpponent(areAlreadySeparated);

	int distanceToOpponent = LastDistanceToOpponent();
	bool isFar = ((distanceToOpponent > kMinFarDistance) && step->isFarFromOpponent());
	step->setFarFromOpponent(isFar);

	if (areAlreadySeparated || !isFar) {
		return basicMoveScore;
	}

	TCell* cRemovedCells = removedPathCells_;
	TCellIndex* iRemovedCellIndexes = removedPathCellIndexes_;
	int numCellsRemoved = 0;
	
	//ForceBreak();
	TCellIndex iMyPosition = iMe;
	TCellIndex iOpponentPosition = iOpponent;
	
	bool pathEnded = false;
	TMoveScore pathScore = 0;
	
	std::vector<bool> separated;		//Indicate which of the next moves will separate the bots.
	std::vector<TMoveScore> moveScores; //Move scores for the next moves.
	
	for (int i = 0; i < 16; ++i) {
		separated.push_back(false);
		moveScores.push_back(0);
	}
	
	//Initialize a place to store scores for possible moves
	//for me and opponent.
	std::vector<TMoveScore> myMoveScores;
	std::vector<TMoveScore> opponentMoveScores;

	for (int i = 0; i < 4; ++i) {
		myMoveScores.push_back(0);
		opponentMoveScores.push_back(0);
	}
	
	const int maxPathLengh = kMaxPathCalculationDepth;
	int pathLength = 0;
	pathScore = basicMoveScore;
	bool wasDeadEnd = false;
	
	do {
		pathLength++;

		const TCell cMyCell = cCells_[iMyPosition];
		const TCell cOpponentCell = cCells_[iOpponentPosition];

		if (iMyPosition == iOpponentPosition || (!cMyCell && !cOpponentCell)) {
			pathEnded = true;
			pathScore = 0;
			wasDeadEnd = true;
			break;
		
		} else if (!cMyCell) {
			pathEnded = true;
			pathScore = VERY_BAD;
			wasDeadEnd = true;
			break;

		} else if (!cOpponentCell) {
			pathEnded = true;
			pathScore = VERY_GOOD;
			wasDeadEnd = true;
			break;
		}
		
		//Reset the separation indicators.
		for (int i = 0; i < 16; ++i) {
			separated[i] = false;
		}
		
		//Apply the current moves.
		cRemovedCells[numCellsRemoved] = cMyCell;
		iRemovedCellIndexes[numCellsRemoved] = iMyPosition;
		numCellsRemoved++;
		cRemovedCells[numCellsRemoved] = cOpponentCell;
		iRemovedCellIndexes[numCellsRemoved] = iOpponentPosition;
		numCellsRemoved++;

		cCells_[iMyPosition] = WALL;
		cCells_[iOpponentPosition] = WALL;
	
		//Score the moves in each direction.
		//While we're doing that, figure out the scores in each direction
		//for my bot.
		TMoveScore myBestScore = VERY_BAD;

		for (int myDirection = 0; myDirection < 4; ++myDirection) {
			const TCellIndex iNewMe = GetNeighbour(iMyPosition, myDirection + 1);
			const TCell myOpenSpace = cCells_[iNewMe];
			TMoveScore myWorstScoreThisDirection = VERY_GOOD;

			for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
				const int moveIndex = myDirection + opponentDirection * 4;
				TMoveScore thisMoveScore = 0;
				
				//Check simple cases.
				const TCellIndex iNewOpponent = GetNeighbour(iOpponentPosition, opponentDirection + 1);
				const TCell opponentOpenSpace = cCells_[iNewOpponent];

				if (!opponentOpenSpace && !myOpenSpace) {
					separated[moveIndex] = true;
					thisMoveScore = 0;

				} else if (!opponentOpenSpace) {
					separated[moveIndex] = true;
					thisMoveScore = VERY_GOOD;

				} else if (!myOpenSpace) {
					separated[moveIndex] = true;
					thisMoveScore = VERY_BAD;
				
				} else {
					//Do the full calculation.
					
					
					thisMoveScore = GetCellBalance(cCells_, iNewMe, iNewOpponent, 
						iMyPosition, iOpponentPosition, true /* use trees */);
					separated[moveIndex] = WereBotsSeparated();
				}

				moveScores[moveIndex] = thisMoveScore;

				//Update my best mvoe score calculation.
				if (myWorstScoreThisDirection > thisMoveScore) {
					myWorstScoreThisDirection = thisMoveScore;
				}
			}

			myMoveScores[myDirection] = myWorstScoreThisDirection;

			if (myBestScore < myWorstScoreThisDirection) {
				myBestScore = myWorstScoreThisDirection;
			}
		}

		//Find best directions to go into for me.
		int myBestDirection = 0;
		int numMyBestDirections = 0;
		int maxNeighbourEdges = 0;

		for (int myDirection = 0; myDirection < 4; ++myDirection) {
			if (myBestScore == myMoveScores[myDirection]) {
				myBestDirection = myDirection;
				numMyBestDirections++;

				if (maxNeighbourEdges > GetEdgeCount(cCells_[GetNeighbour(iMyPosition, myDirection + 1)])) {
					maxNeighbourEdges = GetEdgeCount(cCells_[GetNeighbour(iMyPosition, myDirection + 1)]);
				}
			}
		}

		//If there are more than two good directions, pick one that
		//goes into a cell with fewest neighbours.
		//If there are several such cells, pick the first one.
		if (numMyBestDirections > 1) {
			for (int myDirection = 0; myDirection < 4; ++myDirection) {
				if (myBestScore == myMoveScores[myDirection] 
					&& maxNeighbourEdges == GetEdgeCount(cCells_[GetNeighbour(iMyPosition, myDirection + 1)])) {
					
					myBestDirection = myDirection;
					break;
				}
			}
		}

		//Pick best direction for the opponent to go into.
		TMoveScore opponentBestScore = VERY_GOOD;

		for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
			TMoveScore worstScoreThisDirection = VERY_BAD;

			for (int myDirection = 0; myDirection < 4; ++myDirection) {
				const int moveIndex = myDirection + opponentDirection * 4;

				//Update my best mvoe score calculation.
				if (worstScoreThisDirection < moveScores[moveIndex]) {
					worstScoreThisDirection = moveScores[moveIndex];
				}
			}

			opponentMoveScores[opponentDirection] = worstScoreThisDirection;

			if (opponentBestScore > worstScoreThisDirection) {
				opponentBestScore = worstScoreThisDirection;
			}
		}

		//Find best directions to go into for me.
		int opponentBestDirection = 0;
		int numOpponentBestDirections = 0;
		maxNeighbourEdges = 0;

		for (int direcion = 0; direcion < 4; ++direcion) {
			if (opponentBestScore == opponentMoveScores[direcion]) {
				opponentBestDirection = direcion;
				numOpponentBestDirections++;

				if (maxNeighbourEdges > GetEdgeCount(cCells_[GetNeighbour(iOpponentPosition, direcion + 1)])) {
					maxNeighbourEdges = GetEdgeCount(cCells_[GetNeighbour(iOpponentPosition, direcion + 1)]);
				}
			}
		}

		//If there are more than two good directions, pick one that
		//goes into a cell with fewest neighbours.
		//If there are several such cells, pick the first one.
		if (numOpponentBestDirections > 1) {
			for (int direcion = 0; direcion < 4; ++direcion) {
				if (opponentBestScore == opponentMoveScores[direcion] 
					&& maxNeighbourEdges == GetEdgeCount(cCells_[GetNeighbour(iOpponentPosition, direcion + 1)])) {
					
					opponentBestDirection = direcion;
					break;
				}
			}
		}

		//Check whether the chosen pair of moves will separate the
		//two bots.  If it does, we're done calculating the score.
		if (separated[myBestDirection + opponentBestDirection * 4] 
		  || pathLength >= maxPathLengh 
		  || HasTimedOut()) {
			pathScore = myBestScore;
			pathEnded = true; //break from the outer while loop.
		
		} else {
			//Advance to the next cells.
			iMyPosition = GetNeighbour(iMyPosition, myBestDirection + 1);
			iOpponentPosition = GetNeighbour(iOpponentPosition, opponentBestDirection + 1);
		}
	} while (!pathEnded);
	
	//If a dead end was not reached, calculate the final path score.
	//if (!wasDeadEnd) {
	//	int pathScore = GetCellBalance(cCells_, iMyPosition, iOpponentPosition, 
	//		iRemovedCellIndexes[numCellsRemoved - 1], iRemovedCellIndexes[numCellsRemoved - 2]);
	//}
	
	//Put the removed cells back.
	for (int i = (numCellsRemoved - 1); i >= 0; --i) {
		TCell cRemovedCell = cRemovedCells[i];
		cCells_[iRemovedCellIndexes[i]] = cRemovedCell;
	}

	return pathScore;
}


int StepEvaluator::getBestMove() const {
	if (NULL == rootStep_) {
		return UP;
	}

	std::vector<bool> bestDirections = rootStep_->getBestDirections();
	
	//Pick the first move that doesn't lead into a wall.
	for (int direction = 0; direction < 4; ++direction) {
		if (bestDirections[direction]) {
			TCellIndex iNeighbour = GetNeighbour(iMe_, direction + 1);
			if (cCells_[iNeighbour]) {
				return (direction + 1);
			}
		}
	}

	// ==== Should not get here ====
	//If still haven't picked anything, go up.
	return UP;
}

void StepEvaluator::addStepToQue(Step* step) {
	//evaluationQue_.push_front(step);
	evaluationQue_.push_back(step);
	step->setPlacedInQue();
}

void StepEvaluator::freeStep(Step* step) {
	freeSteps_.push_back(step);
//	delete step;
}

Step* StepEvaluator::getStep() {
	Step* step = NULL;
	
	if (freeSteps_.empty()) {
		step = new Step(this);
	
	} else {
		step = freeSteps_.front();
		freeSteps_.pop_front();
	}

//	Step* step = new Step(this);

	return step;
}

bool StepEvaluator::preEvaluateStep(Step* step) {
	//Check whether the step is a dead end.
	//Don't do anything fancy like actually playing forward to the step; just
	//check for walls that are already there.
	const TCellIndex iMe = step->getMyPosition();
	const TCellIndex iOpponent = step->getOpponentPosition();

	int moveScore = 0;
	bool isDeadEnd = false;

	if (iMe == iOpponent || (!cCells_[iMe] && !cCells_[iOpponent])) {
		moveScore = 0;
		isDeadEnd = true;
	
	} 
	if (isDeadEnd) {
		//Don't need to perform a full evaluation.
		step->setScore(moveScore);
		step->setDeadEnd(true);
		return false;
	
	} else {
		//Need to perform a full evaluation.
		return true;
	}
}

/*****************************
    Class Step
*****************************/
Step::Step(StepEvaluator* stepEvaluator)
:idInParent_(0), iMe_(0), iOpponent_(0), parent_(NULL),
stepEvaluator_(stepEvaluator), score_(VERY_BAD), isDeadEnd_(false), 
isFarFromOpponent_(false), isSeparatedFromOpponent_(false),
isInStepTree_(true), isInEvaluationQue_(false), hasChildren_(false), 
children_(), childrenLeftToEvaluate_(0), childScores_(),
hasBranchedChildren_(false), myGoodMoves_(), opponentGoodMoves_() {
}

void Step::initialize(TCellIndex iMe, 
					  TCellIndex iOpponent, 
					  Step* parent, 
					  int idInParent) {
	idInParent_ = idInParent;
	iMe_ = iMe;
	iOpponent_ = iOpponent;
	parent_ = parent;
	score_ = 0;
	isDeadEnd_ = false;

	isInStepTree_ = true;
	isInEvaluationQue_ = false;
	
	hasChildren_ = false;
	
	children_.clear();
	if (children_.capacity() != 16) {
		children_.reserve(16);
	}
	
	childScores_.clear();
	if (childScores_.capacity() != 16) {
		children_.reserve(16);
	}

	childrenLeftToEvaluate_ = 0;
	hasBranchedChildren_ = false;
}

Step::~Step() {
	//Nothing left to clean up.
}

void Step::setScore(TMoveScore score) {
	score_ = score;
	parent_->updateChildStepScore(score, idInParent_);
}

void Step::removeFromEvaluationQue() {
	isInEvaluationQue_ = false;

	if (!isInStepTree_) {
		stepEvaluator_->freeStep(this);
	}
}

Step* Step::advance(const int myDirection, const int opponentDirection) {
	Step* newRootStep = NULL;
	
	if (this != NULL && this->hasChildren_) {
		const int newRootChildId = (myDirection + opponentDirection * 4);
		newRootStep = children_[newRootChildId];
		children_[newRootChildId] = NULL;
		
		this->unlink();
	
	} else {
		newRootStep = NULL;
	}
	
	if (NULL != newRootStep) {
		newRootStep->parent_ = NULL;
		
		//Make sure that the new root step has branched.
		if (!newRootStep->hasChildren_) {
			newRootStep->branch();
		}

	} else {
		//Need to make a new root step.
		newRootStep = stepEvaluator_->getStep();
		newRootStep->initialize(stepEvaluator_->getMyPosition(), stepEvaluator_->getOpponentPosition(),
			NULL /* parent */, 0 /* id in parent */);
		newRootStep->branch();
	}

	return newRootStep;
}

std::vector<bool> Step::getBestDirections() {
	//Initialize the possible moves that can be made.
	std::vector<bool> availableDirections;
	
	//ForceBreak();

	for (int direction = 0; direction < 4; ++direction) {
		availableDirections.push_back(true);
	}
	
	//Bail early if this step doesn't actually exist.
	if (NULL == this) {
		//The game's over, we can go anywhere we want.
		return availableDirections;
	}
	
	if (!isDeadEnd_) {
		//Pick the best moves for the opponent
		TMoveScore opponentMoveScores[4];
		TMoveScore bestOpponentScore = -VERY_BAD;

		for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
			TMoveScore worstScoreInThisDirection = -VERY_GOOD;

			for (int myDirection = 0; myDirection < 4; ++myDirection) {
				const int childStepId = myDirection + (opponentDirection * 4);

				if (worstScoreInThisDirection < childScores_[childStepId]) {
					worstScoreInThisDirection = childScores_[childStepId];
				}
			}
			
			opponentMoveScores[opponentDirection] = worstScoreInThisDirection;

			if (bestOpponentScore > worstScoreInThisDirection) {
				bestOpponentScore = worstScoreInThisDirection;
			}
		}
		
		//Pick the best move for my bot in response to opponent's
		//best moves.
		TMoveScore myMoveScores[4];
		TMoveScore myResponseScores[4];
		TMoveScore myBestResponseScore = VERY_BAD;

		for (int myDirection = 0; myDirection < 4; ++myDirection) {
			TMoveScore worstScoreInThisDirection = VERY_GOOD;
			TMoveScore worstResponseScoreInThisDirection = VERY_GOOD;

			for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
				const int childStepId = myDirection + (opponentDirection * 4);
				
				if (opponentMoveScores[opponentDirection] == bestOpponentScore) {
					if (worstResponseScoreInThisDirection > childScores_[childStepId]) {
						worstResponseScoreInThisDirection = childScores_[childStepId];
					}
				}

				if (worstScoreInThisDirection > childScores_[childStepId]) {
					worstScoreInThisDirection = childScores_[childStepId];
				}
			}
			
			myMoveScores[myDirection] = worstScoreInThisDirection;
			myResponseScores[myDirection] = worstResponseScoreInThisDirection;

			if (myBestResponseScore < worstResponseScoreInThisDirection) {
				myBestResponseScore = worstResponseScoreInThisDirection;
			}
		}
		
		//When winning, pick the best overall move to avoid collisions.
		if (true /*score_ > 0*/) {
			TMoveScore myBestScore = VERY_BAD;

			for (int direction = 0; direction < 4; ++direction) {
				if (availableDirections[direction] && (myMoveScores[direction] > myBestScore)) {
					myBestScore = myMoveScores[direction];
				}
			}

			for (int direction = 0; direction < 4; ++direction) {
				if (availableDirections[direction] && (myMoveScores[direction] < myBestScore)) {
					availableDirections[direction] = false;
				}
			}
		} else {
			//Otherwise, pick the best moves for my bot to make in response tot he opponent's move.
			int numMyBestDirections = 0;
			for (int direction = 0; direction < 4; ++direction) {
				bool isGoodDirection = (myResponseScores[direction] == myBestResponseScore);
				availableDirections[direction] = isGoodDirection;

				if (isGoodDirection) {
					numMyBestDirections++;
				}
			}

			//If there are more than one best direction to take, consider
			//other directions opponent can move into as well.
			if (numMyBestDirections > 1) {
				TMoveScore myBestScore = VERY_BAD;

				for (int direction = 0; direction < 4; ++direction) {
					if (availableDirections[direction] && (myMoveScores[direction] > myBestScore)) {
						myBestScore = myMoveScores[direction];
					}
				}

				for (int direction = 0; direction < 4; ++direction) {
					if (availableDirections[direction] && (myMoveScores[direction] < myBestScore)) {
						availableDirections[direction] = false;
					}
				}
			}
		}
	} else {
		//If we're at a dead end, pick only moves that lead into empty spaces.
		TCell* cCells = stepEvaluator_->getCells();

		for (int direction = 0; direction < 4; ++direction) {
			const TCellIndex iNeighbour = GetNeighbour(iMe_, direction + 1);

			if (!cCells[iNeighbour]) {
				availableDirections[direction] = false;
			}
		}
	}
	
	return availableDirections;
}

void Step::branch() {
	//Create and initialize child steps.  Add all child steps to the
	//evaluation que.
	hasChildren_ = true;

	//children_.reserve(16);
	//childScores_.reserve(16);

	for (int childId = 0; childId < 16; ++childId) {
		children_.push_back(NULL);
		childScores_.push_back(VERY_BAD);
	}

	childrenLeftToEvaluate_ = 0xffff;

	for (int myDirection = 0; myDirection < 4; ++myDirection) {
		const int iNewMe = GetNeighbour(iMe_, myDirection + 1);

		for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
			//Create the step.
			const int iNewOpponent = GetNeighbour(iOpponent_, opponentDirection + 1);
			const int childId = myDirection + opponentDirection * 4;
			
			Step* childStep = stepEvaluator_->getStep();
			childStep->initialize(iNewMe, iNewOpponent, this, childId);
			childStep->setDepth(this->depth_ + 1);
			childStep->setFarFromOpponent(isFarFromOpponent_);
			childStep->setSeparatedFromOpponent(isSeparatedFromOpponent_);

			if (iNewMe == 65 && iNewOpponent == 103) {
				int x = 3;
			}

			//See if the step can be easily scored; if not, 
			//add it to the evaluation que.
			const bool needsMoreWork = stepEvaluator_->preEvaluateStep(childStep);

			if (needsMoreWork) {
				children_[childId] = childStep;
				childScores_[childId] = VERY_BAD;
				stepEvaluator_->addStepToQue(childStep);

			} else {
				//Nothing else to evaluate, the child is a dead end.
				children_[childId] = NULL;
				childScores_[childId] = childStep->getScore();
				stepEvaluator_->freeStep(childStep);
			}
		}
	}
}

void Step::unlink() {
	//Unlink all children.
	if (hasChildren_) {
		for (int childId = 0; childId < 16; ++childId) {
			if (NULL != children_[childId]) {
				children_[childId]->unlink();
			}
		}
	}
	
	//If the step is ready to be deleted, delete it;
	//Otherwise, indicate that it's been unlinked.
	if (isInEvaluationQue_) {
		isInStepTree_ = false;

	} else {
		stepEvaluator_->freeStep(this);
	}
}

void Step::updateChildStepScore(const TMoveScore score, const int childId) {
	const TMoveScore oldChildScore = childScores_[childId];
	childScores_[childId] = score;
	
	//Update that this child no longer needs to be evaluated.
	if (childrenLeftToEvaluate_) {
		unsigned int bit = 1 << (childId);
		unsigned int mask = ~bit;
		childrenLeftToEvaluate_ &= mask;
	}

	//Prune the dead ends to save space.
	Step* reportingChild = children_[childId];

	if (NULL != reportingChild && reportingChild->isDeadEnd_) {
		reportingChild->unlink();
		children_[childId] = NULL;
	}

	//If all children have been scored, update the
	//overall score for this step.
	if (!childrenLeftToEvaluate_) {
		//If the child's score is different from the old score, 
		//will need to sort the moves from the most
		//interesting to the least iteresting.
		if (!hasBranchedChildren_ || oldChildScore != score) {
			this->sortChildrenByInterest();
		}

		//const TMoveScore bestScore = childScores_[interestingChildren_[0]];
		
		TMoveScore bestScore = VERY_BAD;
		
		for (int myDirection = 0; myDirection < 4; ++myDirection) {
			TMoveScore worstScoreInThisDirection = VERY_GOOD;

			for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
				const int childId = myDirection + opponentDirection * 4;

				if (worstScoreInThisDirection > childScores_[childId]) {
					worstScoreInThisDirection = childScores_[childId];
				}
			}

			if (bestScore < worstScoreInThisDirection) {
				bestScore = worstScoreInThisDirection;
			}
		}
		
		//If the new score is different from the old one,
		//perform some updates.
		if (bestScore != score_ && !isRootStep()) {
			parent_->updateChildStepScore(bestScore, idInParent_);
		}

		//Pick children to branch.
		if (!hasBranchedChildren_ /*|| (bestScore != score_)*/) {
			hasBranchedChildren_ = true;

			//We want to check out the paths that aren't
			//dead ends.
			//Also, branch only the most interesting children - those that
			//lead to the best moves.
			//const int numInterestingChildren = static_cast<int>(interestingChildren_.size());

			for (int childId = 0; childId < 16; ++childId) {
				const int myDirection = childId % 4;
				const int opponentDirection = childId / 4;
				
				if (!myGoodMoves_[myDirection] || !opponentGoodMoves_[opponentDirection]) {
					continue;
				}

				Step* childStep = children_[childId];
				
				if (NULL != childStep 
					&& !childStep->hasChildren_ 
					&& !childStep->isDeadEnd_ 
					/*&& (childScores_[childId] == bestScore)*/) {

					childStep->branch();
				}
			}
			
			//for (int childId = 0; childId < 16; ++childId) {
			//	Step* childStep = children_[childId];

			//	if (NULL != childStep 
			//		&& !childStep->hasChildren_ 
			//		&& !childStep->isDeadEnd_ 
			//		/*&& (childScores_[childId] == bestScore)*/) {

			//		childStep->branch();
			//	}
			//}
		}

		score_ = bestScore;
	}
}

void Step::sortChildrenByInterest() {
	//Order children from the most interesting ones to the
	//least interesting ones.
	//The most interesting children are the ones that
	//contain the steps most likely to be made by me and
	//opponent.
	if (childScores_.empty()) {
		return;
	}
	
	//Find the moves most likely to be made by the opponent.
	//For opponent, negative scores => good.
	TMoveScore opponentBestScore = -VERY_BAD;
	std::vector<TMoveScore> opponentMoveScores;
	
	for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
		TMoveScore worstScoreThisDirection = -VERY_GOOD;

		for (int myDirection = 0; myDirection < 4; ++myDirection) {
			const int childId = myDirection + opponentDirection * 4;
			const TMoveScore moveScore = childScores_[childId];

			if (worstScoreThisDirection < moveScore) {
				worstScoreThisDirection = moveScore;
			}
		}

		opponentMoveScores.push_back(worstScoreThisDirection);

		if (opponentBestScore > worstScoreThisDirection) {
			opponentBestScore = worstScoreThisDirection;
		}
	}

	//Find most interesting of opponent's directions. 
	const int numCellsRemaining = stepEvaluator_->getNumCellsRemaining();
	const float maxScoreDifference = static_cast<float>(StepEvaluator::kScoreChangeCutoff) / 100;

	for (char i = 0; i < 4; ++i) {
		if (numCellsRemaining > StepEvaluator::kMaxPathCalculationDepth) {
			const float scoreDifference = static_cast<float>(opponentMoveScores[i] - score_) 
				/ static_cast<float>(numCellsRemaining);
			
			if (scoreDifference < maxScoreDifference) {
				opponentGoodMoves_[i] = true;
			} else {
				opponentGoodMoves_[i] = false;
			}
		} else {
			if (opponentMoveScores[i] != VERY_BAD) {
				opponentGoodMoves_[i] = true;
			} else {
				opponentGoodMoves_[i] = false;
			}
		}
	}

	//Find my bot's best moves.
	TMoveScore myBestScore = VERY_BAD;
	std::vector<TMoveScore> myMoveScores;
	
	for (int myDirection = 0; myDirection < 4; ++myDirection) {
		TMoveScore worstScoreThisDirection = -VERY_GOOD;

		for (int opponentDirection = 0; opponentDirection < 4; ++opponentDirection) {
			const int childId = myDirection + opponentDirection * 4;
			const TMoveScore moveScore = childScores_[childId];

			if (worstScoreThisDirection < moveScore) {
				worstScoreThisDirection = moveScore;
			}
		}

		myMoveScores.push_back(worstScoreThisDirection);

		if (myBestScore > worstScoreThisDirection) {
			myBestScore = worstScoreThisDirection;
		}
	}
	
	const float minScoreDifference = -static_cast<float>(StepEvaluator::kScoreChangeCutoff) / 100;

	for (char i = 0; i < 4; ++i) {
		if (numCellsRemaining > StepEvaluator::kMaxPathCalculationDepth) {
			const float scoreDifference = static_cast<float>(myMoveScores[i] - score_) 
				/ static_cast<float>(numCellsRemaining);
			
			if (scoreDifference > minScoreDifference) {
				myGoodMoves_[i] = true;
			} else {
				myGoodMoves_[i] = false;
			}
		} else {
			if (myMoveScores[i] != VERY_BAD) {
				myGoodMoves_[i] = true;
			} else {
				myGoodMoves_[i] = false;
			}
		}
	}

}