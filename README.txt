This is the my final submission to the Google AI Challenge contest.

CONTENTS
========
[0] Foreword
[1] File/module descriptions
[2] Overall strategy
[3] Tree of Chambers Calculation
[4] Additional Reading

[0] Foreword
============
This code is a product of about 2.5 weeks of work, and implements only
some of the features that I wanted to see in the final version.
Every once in a while the code I wrote would work fine on my computer,
but would mysteriously fail on the tournament computer, and I'd have
to spend hours trying to figure out why, in the end rewriting the
same thing differently.  

In the end, I'd say that the submission is close enough to what I'd
like it to be.

[1] File/module descriptions
============================
- MyTronBot.cc: main entry point.

- Timer.h/.cc: implements the timer for the program.  On Linux, timer
	was implemented using gettimeofday(); on Windows (my computer) this
	was done using clock().

- Map.h/.cc: part of the original starter package; minimal modifications.

- StepEvaluator.h/.cc: The logic for iterative depening, minimax.  Also has
	logic for some step evaulation functions (moved them from MoveScore.cc
	to fix the problems as per section [0]).

- MoveScore.h/.cc: Logic for the tree-of-chambers move scoring, the main
	move evaluation function.  
	Also, MoveScore.h has some typedefs defining cell, cell index (on a map),
	'branch' (a chamber for tree-of-chambers).
	
[2] Overall strategy
====================
- When near to or separated from opponent, use iterative deepening to search 
	through available moves. Score moves using tree-of-chambers approach 
	detailed below.  Pick best move using minimax.
- When far from opponent (> 6 steps), for each available move, replay the 
	game from that move forward using the above method (evaluating 1 step 
	deep) until the opponents are separated or one/both opponents die. 
	If the opponents are separated, the score for that path was the final 
	move score using the tree-of-chambers approach. This allowed to find 
	'strategically good' moves that would become useful only 20+ steps 
	later.

Most calculation results were stored between the MakeMove() function 
invocations to avoid recalculating the same thing. The side effect was 
that in open spaces on small maps the program would take up to 600MB memory.

[3] Tree of Chambers Calculation
================================
Given my position and opponent's position, we want to know who's doing
better.  Roughly speaking, whoever can reach more squares first is doing
better (this was called 'Voronoi method' on the tournament forums).

To find that, we split the cells into three categories: 
	(a) those that my bot can reach first
	(b) those that opponent can reach first
	(c) those that both would reach at the same time

A rough approximation of how well my bot is doing is (size(a) - size(b)).

This, however, does not account for the sections on the map where one can
enter, but cannot exit.  E.g. below:
	
#####################
#####               #
#X    ###############
#####              ##
#####################

The bot at position X can go down only one corridor.  We need a way to
calculate the bot's fillable area that would account for that.  

To do that, I split the area that my bot would reach first into
a tree of chambers, where if the bot goes into one of the branches
of this tree, it cannot exit that branch.  For example, the
map above after building the tree of chambers would look like this:
	
#####################
#####222222222222222#
#X1111###############
#####33333333333333##
#####################

Where chamber 1 would have two child chambers: 2 and 3.  Clearly,
the fillable area then is size(chamber1) + size(chamber2).

Here's the specific algorithm used to do that (copied from
a post to the tournament forum):

============
Algorithm goes as follows:
Part 1. Split the area into cells that your opponent can reach first, 
and the cells that your bot can reach first. If a cell can be 
reached at the same time by both bots, it's not included in either 
area. I did this originally by doing the following:
	- label the cell where my bot is as '1', label the cell 
		where the opponent is as '-1'. Add them to the queue of 
		cells to be examined.
	- While the queue is not empty, take a cell from the queue, 
		call it the 'current cell'. Look at all of its neighbours. 
		Label all its unlabeled neighbours as 
		({currentCellLabel} + sign(currentCellLabel)). This way, 
		all the cells immediately next to your position will be labeled 
		as '2', all cells one more step away will be labeled as '3', 
		etc. Likewise, for your opponent, all cells one step away from 
		it will be labeled as '-2', all cells two steps away will be 
		labeled '-3', etc. Add all newly labeled neighbours to the 
		queue of cells to be processed.
	- Obviously, it's very likely that the two areas will be 
		gradually spreading and will run into each other. Thus, 
		if the current cell has a neighbour that has the 
		opposite sign and a label with absolute value 
		abs(currentCellLabel) + 1), then the neighbour is a neutral 
		cell, and should be relabeled as '0' (or whatever you chose 
		the neutral label to be).

This does preparatory work of finding which area is whose. This 
step can be actually rolled into part 2, but the algorithm is easier 
to explain if this part is separate.

Part 2. Create the tree of chambers. For this, we will need:
	- a matrix the same dimensions as the map; this will 
		store which cell is in which chamber. Call it the 'chamber map'.
	- a 'chamber' class that will describe chamber objects; will 
		have member variables:
		* entrance (refers to a valid cell in the chamber map)
		* size 
		* whether a chamber a leaf chamber - true if this 
			chamber doesn't lead to any more chambers, false otherwise
		* note that a chamber does not directly contain 
			links to parent or children; you never really need 
			to access children, and the parent will be 
			chamberMap[thisChamber.entrance] (if you want, you 
			can wrap this in a function call).

The algorithm below will construct the tree of chambers only for the 
your bot; it's easy to generalize this to work for both bots. The algorithm:
	
	- on the chamber map, set all cells as belonging to no chamber.
	- Create the first chamber (chamber 1), set its entrance to be 
		the cell where you were before you stepped on the current 
		cell (note that chamberMap[chamber1.entrance] == no_chamber). 
		Set the chamber1 size to 1.
	- Set chamberMap[yourPosition] = chamber1. Add your position 
		to the queue of cells to be processed.
	- While the queue is not empty, take a cell from the queue. 
		Call it 'currentCell'; call the chamber it belongs to the 
		'currentChamber'. For every neighbour of currentCell:
		* if the neighbour is not in the area that is closer to your 
			bot (from Part 1), ignore it.
		* if the neighbour has not been assigned to a chamber and 
			is not a bottleneck (i.e. there's extra empty space 
			on the sides - specifics are left to the reader), set 
			chabmberMap[neighbour] = currentChamber; increment 
			currentChamber.size. Add the neighbour to the queue.
		* if the neighbour has not been assigned to a chamber and 
			is bottleneck, create a new chamber (call it newChamber), 
			set chamberMap[neighbour] = newChamber, 
			newChamber.entrance=currentCell, newChamber.size = 1. 
			Add the neighbour to the queue.
		* if the neighbour has been assigned to a chamber, and if 
			it is currentChamber or is the entrance to currentChamber, 
			ignore it.
		* if the neighbour has been assigned to a chamber which 
			is not currentChamber (and is not the entrance currentChamber), 
			merge the two chambers. This is actually relatively 
			complicated operation that requires you to determine the 
			lowest common parent chamber and merge them into this 
			common parent; chamberMap will need to be updated too. 
			The specifics are left to the reader. After this is done, 
			do not add the neigbour to the queue of cells to process.

After all this is done, just iterate over the leaf chambers, computing 
the total size of the path from the root chamber to the leaf chamber. 
Pick the best one.

Hope the general idea of the algorithm is pretty clear. There are 
a number of things that can be done to speed up the execution, and 
I've obviously omitted some of the details. That should be a decent 
starting point though.
========

[4] Additional Reading
======================

- Iterative Deepening: http://en.wikipedia.org/wiki/Iterative_deepening_depth-first_search
- Minimax: http://en.wikipedia.org/wiki/Minimax
- A paper on Articulation Verteces, didn't get to implementing it:
	http://www.eecs.wsu.edu/~holder/courses/CptS223/spr08/slides/graphapps.pdf
- The Chessboard Problem: http://en.wikipedia.org/wiki/Mutilated_chessboard_problem
	It's not directly related, but thinking in terms of red/black squares lets
	one figure out whether an area can be fully covered by a path.
	Didn't have time to implement it, unfortunately.