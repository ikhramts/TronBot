
#include <vector>
#include <deque>
#include "MoveScore.h"


const int CELL_QUE_CAPACITY = 2500;
const int MAX_PATH_LENGTH = 1;

class CellIndexQue_ {
public: 
	CellIndexQue_();
	~CellIndexQue_();

	unsigned int size() const;
	TCellIndex front();
	void pop_front();
	void push_back(TCellIndex iCell);
	TCellIndex back();
	void pop_back();
	bool empty() const;
	void clear();

private:
	unsigned int front_;
	unsigned int back_;
	TCellIndex* storage_;
};

CellIndexQue_::CellIndexQue_() 
:front_(0), back_(0), storage_(NULL) {
	storage_ = new TCellIndex[CELL_QUE_CAPACITY];
}

CellIndexQue_::~CellIndexQue_() {
	delete[] storage_;
}

unsigned int CellIndexQue_::size() const {
	return ((front_ <= back_) ? (back_ - front_) : (CELL_QUE_CAPACITY - back_ + front_));
}

TCellIndex CellIndexQue_::front() {
	return storage_[front_];
}

void CellIndexQue_::pop_front() {
	front_++;
	front_ %= CELL_QUE_CAPACITY;
}

void CellIndexQue_::push_back(TCellIndex iCell) {
	storage_[back_] = iCell;
	back_++;
	back_ %= CELL_QUE_CAPACITY;
}

bool CellIndexQue_::empty() const {
	return (front_ == back_);
}

void CellIndexQue_::clear() {
	front_ = 0;
	back_ = 0;
}

TCellIndex CellIndexQue_::back() {
	const unsigned int lastIndex = (back_ == 0 )? (CELL_QUE_CAPACITY - 1) : (back_ - 1);
	return storage_[lastIndex];
}

void CellIndexQue_::pop_back() {
	back_ = (back_ == 0 )? (CELL_QUE_CAPACITY - 1) : (back_ - 1);
}

//Matrixes used in temp calculations.
typedef CellIndexQue_ CellIndexQue;
typedef std::vector<TCell> CellStack;
typedef std::vector<TCellIndex> CellIndexStack;

bool* gTempGrid = NULL;
TColor* gTempColorMatrix = NULL;
short* gTempIntMatrix1 = NULL;
short* gTempIntMatrix2 = NULL;

CellIndexQue* g_TempCellIndexQue = NULL;
CellIndexStack* g_TempCellIndexStack = NULL;
CellStack* g_TempCellStack = NULL;

TCellIndex g_iSize_ = 0;
TCellIndex g_iWidth_ = 0;

//TBranchCount g_bcBranchCount = 0;
TCellIndex* g_iBranchRoots = NULL;
TBranchSize* g_bsBranchSizes = NULL;
TBranch* g_bCellBranches = NULL;
bool* g_leafBranches = NULL;
bool* g_myBranches = NULL;
bool g_areBotsSeparated = false;
int g_distanceFromOpponent = 40;

TBranch g_bFirstBranch = 0;

void InitMoveScoreCalculator(const TCellIndex size, const TCellIndex width) {
	g_iSize_ = size;
	g_iWidth_ = width;

	gTempGrid = new bool[size];
	gTempIntMatrix1 = new short[size];
	gTempIntMatrix2 = new short[size];
	gTempColorMatrix = new TColor[size];

	g_iBranchRoots = new TCellIndex[MAX_BRANCHES];
	g_bsBranchSizes = new TBranchSize[MAX_BRANCHES];
	g_bCellBranches = new TBranch[MAX_BRANCHES];
	g_leafBranches = new bool[MAX_BRANCHES];
	g_myBranches = new bool[MAX_BRANCHES];

	g_TempCellIndexQue = new CellIndexQue();
	g_TempCellIndexStack = new CellIndexStack();
	g_TempCellStack = new CellStack();

	g_distanceFromOpponent = 40;
}

/**
 * Get a neighbouring cell in a specific direction. 
 * @param cell: the cell for which to get the neighbour.
 * @param direction: the direction from which to get the cell.
 * @return index of the cell in the specified direction, or -1 
 *	if it's out of bounds.
 */
TCellIndex GetNeighbour(const TCellIndex iPosition, const int direction) {
	switch (direction) {
		case UP:
			return iPosition - g_iWidth_;

		case DOWN:
			return iPosition + g_iWidth_;
		
		case LEFT:
			return iPosition - 1;

		case RIGHT:
			return iPosition + 1;

		default:
			return NO_INDEX;
	}
}

/**
 * Replace a cell on a map with a wall.  Return the removed cell.
 * @param edges: the matrix of cells on the field.
 */
TCell RemoveCell(TCell* cells, const TCellIndex iPosition, const TCellIndex iWidth) {
	TCell cCell = cells[iPosition];
	cells[iPosition] = WALL;
	
	//Decrease the edge counts for all neighbouring cells.
	if (cCell & EDGE_TOP) {
		const int iNeighbour = iPosition - iWidth;
		
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = RemoveEdge(cells[iNeighbour], EDGE_BOTTOM);
	}

	if (cCell & EDGE_RIGHT) {
		const int iNeighbour = iPosition + 1;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = RemoveEdge(cells[iNeighbour], EDGE_LEFT);
	}
	if (cCell & EDGE_BOTTOM) {
		const int iNeighbour = iPosition + iWidth;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = RemoveEdge(cells[iNeighbour], EDGE_TOP);
	}
	if (cCell & EDGE_LEFT) {
		const int iNeighbour = iPosition - 1;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = RemoveEdge(cells[iNeighbour], EDGE_RIGHT);
	}
	
	return cCell;
}

/**
 * Place a cell onto the map, removing the wall.
 * Asserts that the wall is at the spot where a cell is
 * to be placed.
 */
void AddCell(TCell* cells, const TCell cCell, const TCellIndex iPosition, const TCellIndex iWidth) {
	//Place the cell.
	cells[iPosition] = cCell;
	
	//Increase the edge counts for all neighbouring cells.
	if (cCell & EDGE_TOP) {
		const int iNeighbour = iPosition - iWidth;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = AddEdge(cells[iNeighbour], EDGE_BOTTOM);
	}

	if (cCell & EDGE_RIGHT) {
		const int iNeighbour = iPosition + 1;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = AddEdge(cells[iNeighbour], EDGE_LEFT);
	}

	if (cCell & EDGE_BOTTOM) {
		const int iNeighbour = iPosition + iWidth;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = AddEdge(cells[iNeighbour], EDGE_TOP);
	}

	if (cCell & EDGE_LEFT) {
		const int iNeighbour = iPosition - 1;
		if (iNeighbour == 338) {
			int x = 2;
		}
		cells[iNeighbour] = AddEdge(cells[iNeighbour], EDGE_RIGHT);
	}
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
				   const TCellIndex iBranchEnd1,
				   const TCellIndex iBranchEnd2,
				   const TBranch bNeutralBranch) {
	TBranch bBranch1 = bCellBranches[iBranchEnd1];
	TBranch bBranch2 = bCellBranches[iBranchEnd2];

	//Bail early if there's nothing to merge.
	if (bBranch1 == bBranch2) {
		return bBranch1;
	}
	
	//Find the lowest common parent of the two branches.
	//While doing that, build a list of branches to merge.
	bool* hasVisited = new bool[bNumBranches];
	std::vector<TBranch> bPath1;
	std::vector<TBranch> bPath2;

	bPath1.reserve(bNumBranches);
	bPath2.reserve(bNumBranches);

	for (TBranch bBranch = bNeutralBranch + 1; bBranch < bNumBranches; ++bBranch) {
		hasVisited[bBranch] = false;
	}
	
	TBranch bCommonParent = NO_BRANCH;
	
	bool continuePath1 = true;
	bool continuePath2 = true;

	hasVisited[bBranch1] = true;
	hasVisited[bBranch2] = true;
	bPath1.push_back(bBranch1);
	bPath2.push_back(bBranch2);

	while(NO_BRANCH == bCommonParent) {
		if (continuePath1) {
			TBranch bParent1 = GetParentBranch(bBranch1, bCellBranches, iBranchRoots);
			
			if (NO_BRANCH == bParent1) {
				continuePath1 = false;
			
			} else if (hasVisited[bParent1]) {
				bCommonParent = bParent1;
				break;
			} else {
				hasVisited[bParent1] = true;
				bPath1.push_back(bParent1);
				bBranch1 = bParent1;
			}
		}

		if (continuePath2) {
			TBranch bParent2 = GetParentBranch(bBranch2, bCellBranches, iBranchRoots);
			
			if (NO_BRANCH == bParent2) {
				continuePath2 = false;
			
			} else if (hasVisited[bParent2]) {
				bCommonParent = bParent2;
				break;
			} else {
				hasVisited[bParent2] = true;
				bPath2.push_back(bParent2);
				bBranch2 = bParent2;
			}
		}
	}

	//Found the common parent.  Compile a list of branches to relabel as common parent.
	std::vector<TBranch> bBranchesToMerge;
	bBranchesToMerge.reserve(bNumBranches);
	const int path1Size = static_cast<int>(bPath1.size());
	const int path2Size = static_cast<int>(bPath2.size());
	
	for (int i = 0; i < path1Size; ++i) {
		if (bPath1[i] != bCommonParent) {
			bBranchesToMerge.push_back(bPath1[i]);

		} else {
			break;
		}
	}

	for (int i = 0; i < path2Size; ++i) {
		if (bPath2[i] != bCommonParent) {
			bBranchesToMerge.push_back(bPath2[i]);

		} else {
			break;
		}
	}

	//Merge the branches.
	//First, merge the branch sizes and make sure the target branches are
	//no longer leaf branches.

	const int numBranchesToMerge = static_cast<int>(bBranchesToMerge.size());
	TBranchSize bsAddToParentSize = 0;
	
	for (int i = 0; i < numBranchesToMerge; ++i) {
		const TBranch bBranchToMerge = bBranchesToMerge[i];
		bsAddToParentSize += bsBranchSizes[bBranchToMerge];
		leafBranches[bBranchToMerge] = false;
	}

	const TBranch bLowestCommonParent = bCommonParent;
	bsBranchSizes[bLowestCommonParent] += bsAddToParentSize;
	leafBranches[bLowestCommonParent] = true;

	
	//Relabel the cells belonging to the branches that need 
	//to be merged as bLowestCommonParent.
	std::deque<TCellIndex> iCellsToCheck;

	if (bLowestCommonParent != bCellBranches[iBranchEnd1]) {
		bCellBranches[iBranchEnd1] = bLowestCommonParent;
		iCellsToCheck.push_back(iBranchEnd1);
	}

	if (bLowestCommonParent != bCellBranches[iBranchEnd2]) {
		bCellBranches[iBranchEnd2] = bLowestCommonParent;
		iCellsToCheck.push_back(iBranchEnd2);
	}

	while (!iCellsToCheck.empty()) {
		const TCellIndex iCellToCheck = iCellsToCheck.front();
		iCellsToCheck.pop_front();
		
		//Label of this cell's neighbours that don't already belong to
		//the lowest common parent as belonging to the lowest
		//common parent.
		const TCell cCellToCheck = cCells[iCellToCheck];

		for (int direction = 1; direction <= 4; ++direction) {
			//Check whether there's a wall in this direction.
			const TCellIndex iNeighbour = GetNeighbour(iCellToCheck, direction);
			
			if (!cCells[iNeighbour]) {
				continue;
			}
			
			//Check whether the neighbour already belongs to the
			//lowest common parent branch.
			const TBranch bNeighbourBranch = bCellBranches[iNeighbour];
			const TBranch bNeighbourClaim = bNeighbourBranch & CELL_CLAIM;
			const TBranch bNeighbourBranchId = bNeighbourBranch & CELL_BRANCH;

			if (bLowestCommonParent != bNeighbourBranch 
				&& bNeighbourBranch != NO_BRANCH) {
				//Check whether this branch should be merged.
				const int numBranchesToMerge = static_cast<int>(bBranchesToMerge.size());

				for (int i = 0; i < numBranchesToMerge; ++i) {
					if (bNeighbourBranchId == bBranchesToMerge[i]) {
						bCellBranches[iNeighbour] = (bLowestCommonParent | bNeighbourClaim);
						iCellsToCheck.push_back(iNeighbour);
						break;
					}
				}
			}
		}
	}
	
	delete[] hasVisited;

	return bLowestCommonParent;
}

/* Calculating move scores. */
/**
 * Calculate distance to the opponent.
 */
short GetOpponentDistance(TCell* cCells, const TCellIndex iMe, const TCellIndex iOpponent) {
	TColor* cellColors = gTempColorMatrix;
	
	//Reset the colors;
	for (int i = 0; i < g_iSize_; ++i) {
		cellColors[i] = NO_COLOR;
	}

	cellColors[iMe] = BLACK_COLOR;
	TColor currentColor = BLACK_COLOR;
	TColor nextColor = RED_COLOR;

	//Search the cells breadth-first.
	CellIndexQue* cellsToCheck = g_TempCellIndexQue;
	
	cellsToCheck->push_back(iMe);
	short distance = 0;
	bool hasFoundOpponent = false;

	while (!cellsToCheck->empty() && !hasFoundOpponent) {
		const TCellIndex iCurrentCell = cellsToCheck->front();
		cellsToCheck->pop_front();

		if (cellColors[iCurrentCell] != currentColor) {
			distance++;
			currentColor = cellColors[iCurrentCell];
			nextColor = (currentColor == BLACK_COLOR ? RED_COLOR : BLACK_COLOR);
		}

		for (int direction = 0; direction < 4; ++direction) {
			const TCellIndex iNeighbour = GetNeighbour(iCurrentCell, direction + 1);
			
			if (iNeighbour == iOpponent) {
				hasFoundOpponent = true;
				distance++;
				break;
			}
			
			if (WALL != cCells[iNeighbour] && NO_COLOR == cellColors[iNeighbour]) {
				cellColors[iNeighbour] = nextColor;
				cellsToCheck->push_back(iNeighbour);
			}
		}
	}

	cellsToCheck->clear();
	
//	return 10;
	
	if (hasFoundOpponent) {
		return distance;
	} else {
		return OPPONENT_SEPARATED;
	}
}

const short NOT_VISITED = -3000;  //Indicates that the cell yet has not been visited.

/**
 * Calculate the score for a move.
 * 
 * The score for the move is
 * (#cells fillable by me) - (# cells fillable by opponent).
 */
TMoveScore GetCellBalance(TCell* cCells, 
				   const TCellIndex iMe, 
				   const TCellIndex iOpponent, 
				   const TCellIndex iPrevMe,
				   const TCellIndex iPrevOpponent,
				   const bool useTreeBalance) {
	TBranch* bCellBranches = g_bCellBranches;
	
	//if (g_bFirstBranch + g_iSize_ >= MAX_BRANCHES) {
	//	g_bFirstBranch = 0;
		//Reset the connected squares grid, and the branch tracking grid.
		for (int iCell = 0; iCell < g_iSize_; ++iCell) {
			bCellBranches[iCell] = NO_BRANCH;
		}
	//}

	g_areBotsSeparated = true;
	g_distanceFromOpponent = 0;

	//Find a better estimate of the areas under conrtol by each bot
	//by examining the dead-end branches.
	CellIndexQue* cellsToCheck = g_TempCellIndexQue;
	TBranchSize* bsBranchSizes = g_bsBranchSizes;
	TCellIndex* iBranchRoots = g_iBranchRoots;
	bool* leafBranches = g_leafBranches;
	bool* myBranches = g_myBranches;

	//Start out by crating base branches for my bot and the opponent's bot.
	TBranch bNextNewBranch = g_bFirstBranch + 3;
	const TBranch bNeutralBranch = g_bFirstBranch;
	const TBranch myBaseBranch = bNeutralBranch + 1;
	const TBranch opponentBaseBranch = bNeutralBranch + 2;
	leafBranches[bNeutralBranch] = false;
	leafBranches[myBaseBranch] = true;
	leafBranches[opponentBaseBranch] = true;
	myBranches[myBaseBranch] = true;
	myBranches[opponentBaseBranch] = false;
	iBranchRoots[myBaseBranch] = iPrevMe;
	iBranchRoots[opponentBaseBranch] = iPrevOpponent;
	bsBranchSizes[myBaseBranch] = 0;
	bsBranchSizes[opponentBaseBranch] = 0;
	bCellBranches[iMe] = myBaseBranch | ODD_CLAIM | MY_CLAIM;
	bCellBranches[iOpponent] = opponentBaseBranch | ODD_CLAIM | OPPONENT_CLAIM;

	bCellBranches[iPrevMe] = NO_BRANCH;
	bCellBranches[iPrevOpponent] = NO_BRANCH;

	cellsToCheck->push_back(iMe);
	cellsToCheck->push_back(iOpponent);

	bool wasOddClaim = true;

	if (iMe == 35 && iOpponent == 189) {
		int x = 3;
	}

	//Build the branch data.
	while (!cellsToCheck->empty()) {
		//Load the next cell to process.
		const TCellIndex iCurrentCell = cellsToCheck->front();
		cellsToCheck->pop_front();

		TBranch bCurrentBranch = bCellBranches[iCurrentCell];
		const bool isOddClaim = ((bCurrentBranch & ODD_CLAIM) != 0);
		
		//Check whether this cell has been relabeled as NO_BRANCH
		if (bNeutralBranch == bCurrentBranch) {
			//This is a neutral cell now, nothing to do here.
			continue;
		
		} else {
			//Other bot didn't try to claim this cell.
			//Remove the claim flag.
			bCurrentBranch &= CELL_BRANCH;
			bCellBranches[iCurrentCell] = bCurrentBranch;
			bsBranchSizes[bCurrentBranch]++;

			if ((wasOddClaim ^ isOddClaim) && g_areBotsSeparated) {
				g_distanceFromOpponent += 2;
			}
		}
		
		const bool myBranch = myBranches[bCurrentBranch];

		//Attempt to lay claim to all nearby cells.
		for (int direction = 1; direction <= 4; ++direction) {
			//Check whether a wall lies in that direction.
			const int iNeighbour = GetNeighbour(iCurrentCell, direction);
			const TCell cNeighbour = cCells[iNeighbour];

			if (!cNeighbour) {
				continue;
			}
			
			//Check whether the neighbour has already been claimed by either
			//of the bots.
			const TBranch bNeighbourBranch = bCellBranches[iNeighbour];
			const TBranch bCurrentBotClaim = (myBranch ? MY_CLAIM : OPPONENT_CLAIM);
			const TBranch bOtherBotClaim = (myBranch ? OPPONENT_CLAIM : MY_CLAIM);
			const TBranch bNextClaimFlavour = (isOddClaim ? EVEN_CLAIM : ODD_CLAIM);
			
			//Check whether anyone attempted to claim the neighbour.  
			if (bNeighbourBranch == NO_BRANCH || (bNeighbourBranch < bNeutralBranch)) {
				//Noone claimed it yet.  Do so.  
				//Check whether a new branch will need to be created.
				//Check whether a bottleneck was encountered.
				const TCellIndex left = GetNeighbour(0, (direction + 2)%4 + 1);
				const TCellIndex right = -left;
				
				const TCell cLeftOfThis = cCells[iCurrentCell + left];
				const TCell cLeftOfNeighbour = cCells[iNeighbour + left];
				const TCell cRightOfThis = cCells[iCurrentCell + right];
				const TCell cRightOfNeighbour = cCells[iNeighbour + right];
				const bool isABottleNeck = !((cLeftOfThis && cLeftOfNeighbour) || (cRightOfThis && cRightOfNeighbour));
				bool isACorridor = false;

				if (isABottleNeck) {
					//Check whether we're inside a corridor.
					//Count the number of neighbour's neighbours.
					int numNeighbourNeighbours = 1;
					if (cLeftOfNeighbour) numNeighbourNeighbours++;
					if (cRightOfNeighbour) numNeighbourNeighbours++;
					const TCellIndex iFrontOfNeighbour = GetNeighbour(iNeighbour, direction);
					if (cCells[iFrontOfNeighbour]) numNeighbourNeighbours++;
	
					//Count the number of current cell's neighbours
					int numThisNeighbours = 1;
					if (cLeftOfThis) numThisNeighbours++;
					if (cRightOfThis) numThisNeighbours++;
					const TCellIndex iBackOfThis = iCurrentCell - (iFrontOfNeighbour - iNeighbour);
					if (cCells[iBackOfThis]) numThisNeighbours++;

					if (numThisNeighbours == 2 && numNeighbourNeighbours <= 2) {
						isACorridor = true;
					}

				}

				if (isABottleNeck && (!isACorridor || (iCurrentCell == (myBranch ? iMe : iOpponent)))) {
					const TBranch bNewBranch = bNextNewBranch;
					bNextNewBranch++;
					leafBranches[bNewBranch] = true;
					leafBranches[bCurrentBranch] = false;
					myBranches[bNewBranch] = myBranch;
					iBranchRoots[bNewBranch] = iCurrentCell;
					bsBranchSizes[bNewBranch] = 0;
					bCellBranches[iNeighbour] = (bNewBranch | bCurrentBotClaim | bNextClaimFlavour);
				
				} else {
					bCellBranches[iNeighbour] = (bCurrentBranch | bCurrentBotClaim | bNextClaimFlavour);
				}

				cellsToCheck->push_back(iNeighbour);
				continue;
			
			} else if (bNeighbourBranch & bCurrentBotClaim) {
				//Current bot has claimed the cell; nothing to do.
				continue;
			
			} else if (bNeighbourBranch & bOtherBotClaim) {
				//Other bot has claimed this cell.  
				g_areBotsSeparated = false;

				//Check whether the other bot's claim is odd or even.  
				//Odd cells cannot neutralize even claims, and vice versa.
				const bool isNeighbourClaimOdd = ((bNeighbourBranch & ODD_CLAIM) != 0);
				if (!(isOddClaim ^ isNeighbourClaimOdd)) {
					continue;
				}

				//Both claims are odd or both claims are even.  Since this bot would reach
				//this cell at the same time as the opponent, the cell should
				//be nobody's.
				bCellBranches[iNeighbour] = bNeutralBranch;

				//If the claiming branch was empty, remove it; if can't, take it out of
				//further consideration.
				const TBranch bNeighbourBranchId = bNeighbourBranch & CELL_BRANCH;
				if (0 == bsBranchSizes[bNeighbourBranchId]) {
					leafBranches[bCellBranches[iBranchRoots[bNeighbourBranchId]]] = true;
					
					if (bNeighbourBranchId == (bNextNewBranch - 1)) {
						bNextNewBranch--;
					
					} else {
						leafBranches[bNeighbourBranchId] = false;
					}
				}

				//Decrease # of branches.
				continue;
			
			} else if (bNeighbourBranch == bNeutralBranch) {
				//The neighbour has been declared neutral; nothing to do here.
				continue;

			} else {
				//Check whether the branch belongs to the current bot.
				//If so, check if it is in a different branch; if it is, we
				//may need to merge branches.
				if (bNeighbourBranch != bCurrentBranch && myBranches[bNeighbourBranch] == myBranch) {
					//If the neighbour is not the root of the
					//current branch, then the branches need to be
					//merged.
					if (iNeighbour != iBranchRoots[bCurrentBranch]) {
						bCurrentBranch = MergeBranches(iBranchRoots, bCellBranches, 
							bsBranchSizes, cCells, leafBranches, bNextNewBranch, iCurrentCell, iNeighbour, bNeutralBranch);
					}
				}
			}
		}
	}
	
	//Calculate max fillable space by each of the bots.
	//Calculate fillable area for each branch by starting with the leaf
	//branches and going up to the base branch, adding up the branch
	//sizes.
	TBranchSize myMaxPath = 0;
	TBranchSize opponentMaxPath = 0;

	for (TBranch bLeafBranch = (bNeutralBranch + 1); bLeafBranch < bNextNewBranch; ++bLeafBranch) {
		if (!leafBranches[bLeafBranch]) {
			continue;
		}
		
		TBranchSize bsPathSize = 0;
		TBranch bBranch = bLeafBranch;

		while(NO_BRANCH != bBranch) {
			bsPathSize += bsBranchSizes[bBranch];
			bBranch = GetParentBranch(bBranch, bCellBranches, iBranchRoots);
		}

		if (myBranches[bLeafBranch]) {
			if (myMaxPath < bsPathSize) {
				myMaxPath = bsPathSize;
			}

		} else {
			if (opponentMaxPath < bsPathSize) {
				opponentMaxPath = bsPathSize;
			}
		}
	}

	//g_bFirstBranch = bNextNewBranch;

	const TMoveScore cellBalance = static_cast<TMoveScore>(myMaxPath - opponentMaxPath);
	return cellBalance;
}

/**
 * Check whether the bots were separated during the previous 
 * evaluation, i.e. whether one bot could not possibly have entered
 * territory of another bot.
 */
bool WereBotsSeparated() {
	return g_areBotsSeparated;
}

int LastDistanceToOpponent() {
	return g_distanceFromOpponent;
}

void ForceBreak() {
	int* bad;
	bad = (int*) 10;
	(*bad) = 20;

	int x = 2;
}

/** 
 * Sort a vector based on values in another vector.
 */
void SortIndexes(std::vector<char>& indexes, const std::vector<TMoveScore>& values, bool ascending) {
	const int rangeEnd = static_cast<int>(indexes.size());
	SortIndexRange(indexes, values, ascending, 0, rangeEnd);
}

/** 
 * Sort a subset of vector based on values in another vector.
 */
void SortIndexRange(std::vector<char>& indexes, 
					const std::vector<TMoveScore>& values,
					bool ascending,
					const int rangeStart, 
					const int rangeEnd) {
	//Do a bubble sort.
	const int lastIndex = static_cast<int>(rangeEnd - 1);

	if (lastIndex <= rangeStart) {
		return;
	}

	bool done = false;

	if (ascending) {
		while (!done) {
			done = true;

			for (int i = rangeStart; i < lastIndex; ++i) {
				if (values[indexes[i]] > values[indexes[i+1]]) {
					char temp = indexes[i];
					indexes[i] = indexes[i + 1];
					indexes[i + 1] = temp;

					done = false;
				}
			}
		}

	} else {
		while (!done) {
			done = true;

			for (int i = rangeStart; i < lastIndex; ++i) {
				if (values[indexes[i]] < values[indexes[i+1]]) {
					char temp = indexes[i];
					indexes[i] = indexes[i + 1];
					indexes[i + 1] = temp;

					done = false;
				}
			}
		}
	}
}
