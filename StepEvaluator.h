/*
 * The two classes defined here are responsible for traversal of
 * the tree of possible moves through iterative deepening.  
 * 
 * Class Step implements the actual tree of the moves and contains
 * logic for updating and manipulating that tree.  In particular,
 * it contains logic that decides which moves should be explored
 * further, and how to remove the branches that are no longer
 * useful.
 *
 * Class StepEvaluator contains a queue of Step objects to be evaluated,
 * and a heap of recycled Step objects.  It also provides easier 
 * outside access to the results computed by the tree of Steps.
 *
 * The tree of Steps is kept between MakeMove() invocations
 * in MyTronBot.cc to avoid recomputing the same thing over and over.
 * To avoiding memory bloat, every time me and opponent make a move,
 * branches of the Step tree that are no longer useful are removed
 * in StepEvaluator::updateMoves() and Step::advance().
 *
 * Individual steps were evaluated in one of two ways:
 * (1) When separated from the opponent, or close to the opponent,
 *		this was done using a 'tree of chambers' method to figure
 *		out who controlled most territory.  Higer scores were
 *		better for me, lower were better for opponent.
 * (2) When far from the opponent, steps were evaluated by
 *		playing the game forward using minimax at depth of 1
 *		with the above evaluation function.  The game was played
 *		forward until one of the bots died or the bots were
 *		separated.  The final score was +inf/-inf/0 if one/both
 *		of the bots died; otherwise it was the territory score
 *		as mentioned above.  This method was named the 'Path'
 *		method of evaluation.
 */

#include "MoveScore.h"
#include <list>
#include <deque>
#include <bitset>

#ifndef STEP_EVALUATOR_H_
#define STEP_EVALUATOR_H_

class Step;
class StepEvaluator;
class Map;

//typedef std::list<Step*> EvaluationQue;
typedef std::deque<Step*> EvaluationQue;

/**
 * A class that keeps track of the possible steps to be taken.
 */
class Step {
public:
	Step(StepEvaluator* stepEvaluator);
	~Step();
	void initialize(TCellIndex iMe, TCellIndex iOpponent, Step* parent, int idInParent);
	
	Step* getParent()				{ return parent_;}
	
	bool isInStepTree() const			{ return isInStepTree_;}
	bool isInEvaluationQue() const		{ return isInEvaluationQue_;}
	
	/**
	 * Set the result of evaluating the step.  Is used in StepEvaluator::performEvaluations().
	 */
	void setScore(TMoveScore score);
	
	TMoveScore getScore() const			{ return score_; }
	
	void setPlacedInQue()				{ isInEvaluationQue_ = true;}
	void removeFromEvaluationQue();
	
	/**
	 * Indicates whether no further exploration of this branch of steps is
	 * possible.  Occurs when one/both of the players run into a wall.
	 */
	void setDeadEnd(bool deadEnd)		{ isDeadEnd_ = deadEnd;}
	
	/** 
	 * Me and opponent have made new moves.  Set the new
	 * root of the tree of Step objects, and remove
	 * all branches that are no longer under consideration.
	 */
	Step* advance(int myDirection, int opponentDirection);
	
	/**
	 * Decides wich moves from the current position are acceptable.
	 */
	std::vector<bool> getBestDirections();
	
	TCellIndex getMyPosition() const			{return iMe_;}
	TCellIndex getOpponentPosition() const		{return iOpponent_;}
	TCellIndex getPrevMyPosition() const		{return parent_->iMe_;}
	TCellIndex getPrevOpponentPosition() const	{return parent_->iOpponent_;}
	
	//Switches for near/far/separated strategies.
	bool isFarFromOpponent() const				{return isFarFromOpponent_;}
	void setFarFromOpponent(bool far)			{isFarFromOpponent_ = far;}
	bool isSeparatedFromOpponent() const		{return isSeparatedFromOpponent_;}
	void setSeparatedFromOpponent(bool isSep)	{isSeparatedFromOpponent_ = isSep;}

	/**
	 * Make this Step object create child step objects, effectively
	 * exploring this branch of Steps further.
	 */
	void branch();

	/**
	 * Indicates that this step is a root step.
	 */
	bool isRootStep() const						{return (parent_ == NULL);}

	bool hasChildren() const					{ return (children_.size() > 0);}

	//Depth of this step, from the very first position (not from the current root step).
	int getDepth() const						{ return depth_;}
	void setDepth(int depth)					{ depth_ = depth;}
private:
	
	/**
	 * Unlink this step from the step tree.
	 */
	void unlink();
	
	/**
	 * A way for child to notify the parent that it has a new move score.
	 */
	void updateChildStepScore(TMoveScore score, int childId);
	
	/**
	 * Order the moves from the most interesting to the
	 * least interesting.
	 * 
	 * Exclude non-interesting moves.
	 * NOTE: NO LONGER USED.
	 */
	void sortChildrenByInterest();
	
	//General info.
	char idInParent_;
	TCellIndex iMe_;
	TCellIndex iOpponent_;
	Step* parent_;
	StepEvaluator* stepEvaluator_;
	TMoveScore score_;
	bool isDeadEnd_;
	int depth_;			//Step number starting from the first step.
	
	//Position of me relative to the opponent.
	bool isFarFromOpponent_;
	bool isSeparatedFromOpponent_;
	
	//Current state of the step.
	bool isInStepTree_;
	bool isInEvaluationQue_;
	
	//Step's child steps.
	bool hasChildren_;
	std::vector<Step*> children_;
	std::vector<TMoveScore> childScores_;
	unsigned short childrenLeftToEvaluate_;
	bool hasBranchedChildren_;
	std::bitset<4> myGoodMoves_;
	std::bitset<4> opponentGoodMoves_;
};

/**
 * A class for calculating the next best step.
 */
class StepEvaluator {
public:
	/*
	 * General settings.
	 */
	//May not explore farther than this many steps from the root step.
	//Effectively no longer used.
	static const int kDepthLimit = 100;

	//Maximum path length for the path evaluation method.
	static const int kMaxPathCalculationDepth = 50;

	//Do not explore moves that cause this much drop/increace in
	//relative move score.  We assume that neither me nor opponent
	//will make moves that will cause sudden drop of the bot's fortune.
	static const int kScoreChangeCutoff = 25; //in percent.

	//Notwithstanding what's written immediately above, explore
	//the possible moves to the fullest when there are at most
	//this many cells left.
	static const int kMinCellsForScoreChangeCutoff = 10;

	//Switch to 'near' strategy when closer than this to opponent.
	static const int kMinFarDistance = 6;

	//Constructor/destructor.
	StepEvaluator ();
	~StepEvaluator();

	void initialize(const Map& map);
	
	/**
	 * Update my and opponent's positions after moves have
	 * been made.
	 */
	void updateMoves(const Map& map);
	
	/**
	 * Perform calculations, refining the calculation for the best next
	 * step to make.
	 *
	 * @return indicator whether there's any more work to be done.
	 */
	bool performEvaluations();

	/**
	 * Calculate the path score of a step.
	 */
	TMoveScore calculatePathScore(Step* step);

	int getBestMove() const;
	
	/**
	 * Add step to the queue of steps to be evaluated.
	 */
	void addStepToQue(Step* step);

	TCell* getCells()		{return cCells_;}

	//Managing free step objects.
	void freeStep(Step* step);
	
	/**
	 * Get a new step.  Returns NULL if there are no more steps
	 * available.
	 */
	Step* getStep();

	/**
	 * Pre-evaluate a step to keep down the size of the eval que.
	 * @return whether the step needs further evaluation.
	 */
	bool preEvaluateStep(Step* step);
	
	/**
	 * Add a step to the que of steps that need to finish
	 * branching when there are free steps available.
	 */
//	bool addStepToBranchingQue(Step* step);
	
	int getMaxDepth() const		{ return maxDepth_;}
	int getNumEvaluations() const	{ return numEvaluations_;}

	TCellIndex getMyPosition() const		{return iMe_;}
	TCellIndex getOpponentPosition() const	{return iOpponent_;}
	short getNumCellsRemaining() const		{return numCellsRemaining_;}

private:
	//Internal copy of the map
	//TCellm TCellIndex, etc are defined in MoveScore.h
	TCell* cCells_;
	TCellIndex iWidth_;
	TCellIndex iSize_;
	
	TCellIndex iMe_;
	TCellIndex iOpponent_;
	short numCellsRemaining_;
	
	//The root step in the step tree.
	Step* rootStep_;

	EvaluationQue evaluationQue_;
	std::deque<Step*> freeSteps_;

	//To be reused in calculations to avoid reallocating large arrays.
	std::vector<TCellIndex> removedCellIndexes_;
	std::vector<TCell> removedCells_;
	
	//Que of steps that haven't finished branching
	//because there was not enough memory.
	//NO LONGER USED.
	std::deque<Step*> branchingQue_;

	//General interest.
	int numEvaluations_;
	int maxDepth_;

	//Current depth
	int currentDepth_;			//Step number starting from the first step.

	//Temporary placeholders for path scoring
	//calculations.  Created once to avoid reallocating large arrays.
	TCell* removedPathCells_;
	TCellIndex* removedPathCellIndexes_;
};

#endif /* STEP_EVALUATOR_H_ */