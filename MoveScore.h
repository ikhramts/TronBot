/*
 * This file defines basic types for that represent individual
 * cells on the map, branches in a tree of chambers, etc.
 * 
 * Also it defines functions for updating the map and
 * evaluating the tree-of-chambers score.
 * 
 * The internal representation of the map was kept in an
 * array of TCells with length=width*height).  Originally
 * I kept track of the number of open cells surrounding
 * each cell (the "edges"), but this was later discontinued.
 * Pieces of code dealing with this still exist.
 */
#ifndef MOVE_SCORE_H_
#define MOVE_SCORE_H_

#ifndef NULL
#define NULL 0
#endif

/* Movement directions. */
/* Note: often in code I actually use 0-based directions, 
   where 0=up, 1=right, etc.  It should be evident from
   the context where this happens.
*/
const int NO_DIRECTION = 0;		//Used when a strategy cannot decide which direction to move into.
const int UP = 1;
const int RIGHT = 2;
const int DOWN = 3;
const int LEFT = 4;

/* Indicates that the cell cannot be reached. */
//NO LONGER USED.
const int TOO_FAR = 10000;
const short OPPONENT_SEPARATED = -1;

/** Scoring the moves. */
typedef short TMoveScore;

const TMoveScore VERY_BAD = -10000;
const TMoveScore VERY_GOOD = 10000;
const TMoveScore CUTOFF_BONUS = 0;	//No longer used.

/* Indexing cells */
typedef int TCellIndex;
const TCellIndex NO_INDEX = 10000; //Indicates absence of a cell.

/**
 * Get a neighbouring cell in a specific direction. 
 * @param cell: the cell for which to get the neighbour.
 * @param direction: the direction from which to get the cell.
 * @return index of the cell in the specified direction, or -1 
 *	if it's out of bounds.
 */
TCellIndex GetNeighbour(TCellIndex iPosition, int direction);

/** Keeping track of cells */
typedef unsigned char TCell;
typedef unsigned int TEdgeCount;

const TCell WALL				= 0;
const TCell CELL_EDGE_COUNT		= 7;
const TCell CELL_EDGES			= 120; 
const TCell EDGE_FIRST			= 8;
const TCell EDGE_BEFORE_FIRST	= 4;
const TCell EDGE_TOP			= 8;
const TCell EDGE_RIGHT			= 16;
const TCell EDGE_BOTTOM			= 32;
const TCell EDGE_LEFT			= 64;
const TCell NOT_WALL			= 128;

inline TCell GetEdgeCount(TCell cCell) {
	return (CELL_EDGE_COUNT & cCell);
}

inline TCell AddEdge(const TCell cCell, const TCell cEdge) {
	TCell newCell = (cCell | cEdge) + 1;
	return newCell;
}

inline TCell RemoveEdge(const TCell cCell, const TCell cEdge) {
	TCell newCell = (cCell & (~cEdge)) - 1;
	return newCell;
}

/**
 * Replace a cell on a map with a wall.  Return the removed cell.
 * @param edges: the matrix of cells on the field.
 */
TCell RemoveCell(TCell* cells, TCellIndex iPosition, TCellIndex iWidth);

/**
 * Place a cell onto the map, removing the wall.
 * Asserts that the wall is at the spot where a cell is
 * to be placed.
 */
void AddCell(TCell* cells, TCell cCell, TCellIndex iPosition, TCellIndex iWidth);


/* Keeping track of dead-end branches. */
typedef unsigned short TBranch;
typedef unsigned short TBranchSize;
typedef unsigned short TBranchCount;

const TBranchCount MAX_BRANCHES = 4095;
const TBranch CELL_BRANCH = 4095;
const TBranch ODD_CLAIM = 4096;
const TBranch EVEN_CLAIM = 0;
const TBranch NO_BRANCH = 32768;
const TBranch MY_CLAIM = 8192;
const TBranch OPPONENT_CLAIM = 16384;
const TBranch CELL_CLAIM = 24576;

typedef unsigned char TColor;
const TColor NO_COLOR = 0;
const TColor CELL_COLOR = 3;
const TColor BLACK_COLOR = 1;
const TColor RED_COLOR = 2;
const TColor MY_COLOR_CLAIM = 4;
const TColor OP_COLOR_CLAIM = 8;
const TColor NEUTRAL_COLOR = 16;

inline TBranch GetParentBranch(const TBranch bBranch, TBranch* bCellBranches, TCellIndex* iBranchRoots) {
	//Should return NO_BRANCH for the top branch, because the root
	//of the top branch should be at an already occupied location.
	return bCellBranches[iBranchRoots[bBranch]];
}

/**
 * Merge two branches; return lowest common parent.
 */
TBranch MergeBranches(TCellIndex* iBranchRoots,
				   TBranch* bCellBranches,
				   TBranchSize* bsBranchSizes,
				   TCell* cCells,
				   bool* leafBranches,
				   TBranch bNumBranches,
				   TCellIndex iBranchEnd1,
				   TCellIndex iBranchEnd2);

/* Calculating the move scores. */

/**
 * Initialize the environment for calculating the move score.
 */
void InitMoveScoreCalculator(TCellIndex size, TCellIndex width);

/**
 * Calculate distance to the opponent.
 */
//short GetOpponentDistance(TCell* cCells, TCellIndex iMe, TCellIndex iOpponent);

/**
 * Calculate the score for a move using the tree-of-chambers method.
 * 
 * The score for the move is
 * (#cells fillable by me) - (# cells fillable by opponent).
 */
TMoveScore GetCellBalance(TCell* cCells, 
				   TCellIndex iMe, 
				   TCellIndex iOpponent, 
				   TCellIndex iPrevMe,
				   TCellIndex iPrevOpponent,
				   bool useTreeBalance);

/**
 * Check whether the bots were separated during the previous 
 * evaluation of GetCellBalance, i.e. whether one bot could 
 * not possibly have entered territory of another bot.
 */
bool WereBotsSeparated();

/**
 * Get the approximate distance (+/- 2 steps) to the opponent
 * during the last invocation of GetCellBalance().  Is valid
 * only if the bots aren't separated.
 */
int LastDistanceToOpponent();

/**
 * Attempt to calculate the longest possible path each of the 
 * players can create.
 *
 * NOTE: WAS MOVED TO StepEvaluator.
 */
TMoveScore GetPathScore(TCell* cCells, 
				   TCellIndex iMe, 
				   TCellIndex iOpponent, 
				   TCellIndex iPrevMe,
				   TCellIndex iPrevOpponent);

/* A general purpose function */
void ForceBreak();

/** 
 * Sort a vector based on values in another vector.
 * NO LONGER USED.
 */
void SortIndexes(std::vector<char>& indexes, const std::vector<TMoveScore>& values, bool ascending);

/** 
 * Sort a subset of vector based on values in another vector.
 * NO LONGER USED.
 */
void SortIndexRange(std::vector<char>& indexes, 
					const std::vector<TMoveScore>& values,
					bool ascending,
					int rangeStart, 
					int rangeEnd);


#endif /* MOVE_SCORE_H_ */