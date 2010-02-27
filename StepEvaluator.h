
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
	void setScore(TMoveScore score);
	TMoveScore getScore() const			{ return score_; }
	void setPlacedInQue()				{ isInEvaluationQue_ = true;}
	void removeFromEvaluationQue();
	void setDeadEnd(bool deadEnd)		{ isDeadEnd_ = deadEnd;}

	Step* advance(int myDirection, int opponentDirection);

	std::vector<bool> getBestDirections();
	
	TCellIndex getMyPosition() const			{return iMe_;}
	TCellIndex getOpponentPosition() const		{return iOpponent_;}
	TCellIndex getPrevMyPosition() const		{return parent_->iMe_;}
	TCellIndex getPrevOpponentPosition() const	{return parent_->iOpponent_;}
	
	bool isFarFromOpponent() const				{return isFarFromOpponent_;}
	void setFarFromOpponent(bool far)			{isFarFromOpponent_ = far;}
	bool isSeparatedFromOpponent() const		{return isSeparatedFromOpponent_;}
	void setSeparatedFromOpponent(bool isSep)	{isSeparatedFromOpponent_ = isSep;}

	/**
	 * Evaluate this step's next steps.
	 */
	void branch();

	/**
	 * Indicates that this step is a root step.
	 */
	bool isRootStep() const						{return (parent_ == NULL);}

	bool hasChildren() const					{ return (children_.size() > 0);}
	int getDepth() const						{ return depth_;}
	void setDepth(int depth)						{ depth_ = depth;}
private:
	
	/**
	 * Unlink this step from the step tree.
	 */
	void unlink();
	
	/**
	 * Get updated step score from the child.
	 */
	void updateChildStepScore(TMoveScore score, int childId);
	
	/**
	 * Order the moves from the most interesting to the
	 * least interesting.
	 * 
	 * Exclude non-interesting moves.
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
	static const int kDepthLimit = 100;
	static const int kMaxPathCalculationDepth = 50;
	static const int kScoreChangeCutoff = 25; //in percent.
	static const int kMinCellsForScoreChangeCutoff = 10;
	static const int kMinFarDistance = 6;

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
	
	//Add a step to the list of steps to be evaluated.
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
	TCell* cCells_;
	TCellIndex iWidth_;
	TCellIndex iSize_;

	TCellIndex iMe_;
	TCellIndex iOpponent_;
	short numCellsRemaining_;

	Step* rootStep_;

	EvaluationQue evaluationQue_;
	std::deque<Step*> freeSteps_;

	//To be reused in calculations.
	std::vector<TCellIndex> removedCellIndexes_;
	std::vector<TCell> removedCells_;
	
	//Que of steps that haven't finished branching
	//because there was not enough memory.
	std::deque<Step*> branchingQue_;
	int numEvaluations_;

	//General interest.
	int maxDepth_;

	//Current depth
	int currentDepth_;			//Step number starting from the first step.

	//Temporary placeholders for path scoring
	//calculations.
	TCell* removedPathCells_;
	TCellIndex* removedPathCellIndexes_;
};

#endif /* STEP_EVALUATOR_H_ */