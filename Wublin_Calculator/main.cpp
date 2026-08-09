#include <vector>
#include "wublin.h"
#include <ilcplex/ilocplex.h>  
#include <iostream>
#include <map> // for representing wublin map with a coordinate pair system
#include <utility> // for pairs
#include <algorithm>// For find
using Coordinate = std::pair<int, int>; // type alias 

// All imports required for google's CP-SAT
#include "absl/base/log_severity.h"
#include "absl/log/globals.h"
#include "ortools/base/init_google.h"
#include "ortools/base/logging.h"
#include "ortools/sat/cp_model.h"
#include "ortools/sat/cp_model.pb.h"
#include "ortools/sat/cp_model_solver.h"
#include "ortools/util/sorted_interval_list.h"
using namespace operations_research; //this is fine so long as you don't make/use another library that uses objects + methods of the same name.
using namespace operations_research::sat;



// TODO: MAKE SURE TO ADD CONDITION OF: If hateradius = 0, then SKIP ASSIGNING THE CONSTRAINTS
// HUGE TODO: Need to research rare + epic hate relationship with the rarities below it (it appear to be MUCH more complicated than thought)
// Current theory is that we need to add to hate constraints all the rarities of the negative polarity monster in the veto list
// Possibly still allow my current layout settings but add a filter to show only solutions that have NO harm on polarity at ALL (in my release version)


// NOTE: Make sure to randomize CPLEX as CPLEX is deterministic, so otherwise, the same solution will be found each time
// OR look into increasing solution pool size
// NOTE: I want to add ability to separate various solutions to the problem by which stage the user has (as it can take a long time to achieve each stage)
/* 
This shows how range works:
    radius of 2 means:
    A A A A A A
    A A A A A A
    A A 0 0 A A
    A A 0 0 A A
    A A A A A A
    A A A A A A
    Where A means the affect radius and 0 designates the actual wublin (of size 2x2).
    The target only needs 1 block to be within this range to trigger the like/hate polarity

// Like and hate radius by polarity amplifier lvl: (the user's wublin island "level" effects polarity behavior)
1; Like radius = 2, Hate radius = 2 (includes diagonals)
2: Like radius = 3, Hate radius = 2 
3: Like radius = 3, Hate radius = 1 
4: Like radius = 3, Hate radius = 0
5: Unknown currently
 */
//Map key:
/* 
33 = ! (void space)

*/

int indexOfWublinName(std::string wublinName, const std::vector<Wublin>& wublins)
{
    for (int i{ 0 }; i < wublins.size(); i++)
        if (wublins[i].getName() == wublinName)
            return i;
    return -1;
}

//Must be done in terms of lowerRight for AddNoOverlap2D() to be supported (as size constraint for this method only supports objects defined UPWARD
bool canFit(const Coordinate& lowerLeft, const int size, const std::map<Coordinate, std::string>& wublinMapCoordPairs) // WHY DOES upperLeft REQUIRE CONST?
{ //check 4 corners, if all corners are part of wublinMapCoordPairs, then canFit returns true. 
    //NOTE: This works for the wublin map but might not work for all. A placement over a C shaped grid space could technically satisfy being on all 4 corners while
    // the inbetweens are outside of the grids
    int distFromOtherCorners = size - 1;
    Coordinate upperRight = Coordinate(lowerLeft.first + distFromOtherCorners, lowerLeft.second + distFromOtherCorners);
    Coordinate lowerRight = Coordinate(lowerLeft.first + distFromOtherCorners, lowerLeft.second);
    Coordinate upperLeft = Coordinate(lowerLeft.first, lowerLeft.second + distFromOtherCorners);
    return wublinMapCoordPairs.contains(upperLeft) &&
        wublinMapCoordPairs.contains(upperRight) &&
        wublinMapCoordPairs.contains(lowerLeft) &&
        wublinMapCoordPairs.contains(lowerRight);
}

bool isOccupiedSpace(const Coordinate& lowerLeft, const int size, const std::map<Coordinate, std::string>& wublinMapCoordPairs)
{
    int distFromOtherCorners = size - 1;
    Coordinate upperRight = Coordinate(lowerLeft.first + distFromOtherCorners, lowerLeft.second + distFromOtherCorners);
    Coordinate lowerRight = Coordinate(lowerLeft.first + distFromOtherCorners, lowerLeft.second);
    Coordinate upperLeft = Coordinate(lowerLeft.first, lowerLeft.second + distFromOtherCorners);
    return wublinMapCoordPairs.contains(upperLeft) &&
        wublinMapCoordPairs.contains(upperRight) &&
        wublinMapCoordPairs.contains(lowerLeft) &&
        wublinMapCoordPairs.contains(lowerRight);
}



std::vector<Coordinate> calcValidPlacements(const int size, const std::map<Coordinate, std::string>& wublinMapCoordPairs)
{
    std::vector<Coordinate> validPlacements;
    for (const auto& [upperLeftCoord, value] : wublinMapCoordPairs)
        if (canFit(upperLeftCoord, size, wublinMapCoordPairs))
            validPlacements.push_back(upperLeftCoord);
    return validPlacements;
}







void findLayouts(int likeRadius, int hateRadius, const std::map<Coordinate, std::string>& wublinMapCoordPairs, const std::vector<Wublin>& wublinPool, const std::vector<std::vector<char>>& wublinMapArr) //Uses IBM's CPLEX to find all the maximum polarity placements using like radius and hate radius
{
    CpModelBuilder wublinProblem; // model of the wublin PROBLEM
    const Domain xAxisRange(0, wublinMapArr[0].size());
    const Domain yAxisRange(0, wublinMapArr.size());
    std::vector<IntVar> wublinsXAxis; //ith index represents the ith wublin's x axis 
    std::vector<IntVar> wublinsYAxis;
    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        IntVar xAxis = wublinProblem.NewIntVar(xAxisRange).WithName(wublinPool[i].getName() + "'s x coordinate");
        IntVar yAxis = wublinProblem.NewIntVar(yAxisRange).WithName(wublinPool[i].getName() + "'s y coordinate");
        wublinsXAxis.push_back(xAxis);
        wublinsYAxis.push_back(yAxis);
    }
    
    std::vector<Coordinate> size2ValidPlacements = calcValidPlacements(2, wublinMapCoordPairs);
    std::vector<Coordinate> size3ValidPlacements = calcValidPlacements(3, wublinMapCoordPairs);
    std::vector<Coordinate> size4ValidPlacements = calcValidPlacements(4, wublinMapCoordPairs);



    // ALL OF THE BELOW CODE CAN BE COMVBINED INTO ONE FOR: for (int i{ 0 }; i < wublinPool.size(); i++)

    //std::vector<TableConstraint> t;
    for (int i{ 0 }; i < wublinPool.size(); i++) //allowd groups are x y coordinates
    {
        
        // 1. allowed groupings (avoids placing outside of map bounds)
        auto wublinAllowed = wublinProblem.AddAllowedAssignments({wublinsXAxis[i], wublinsYAxis[i]});
        if (wublinPool[i].getSize() == 2)
            for (int i{ 0 }; i < size2ValidPlacements.size(); i++)
            {
                Coordinate c = size2ValidPlacements[i];
                wublinAllowed.AddTuple({c.first, c.second});
            }
        else if (wublinPool[i].getSize() == 3)
            for (int i{ 0 }; i < size3ValidPlacements.size(); i++)
            {
                Coordinate c = size3ValidPlacements[i];
                wublinAllowed.AddTuple({ c.first, c.second });
            }
        else // This else is for size == 4, but only works for this very project (so would need to be changed if changing for a different my singing monster island
            for (int i{ 0 }; i < size4ValidPlacements.size(); i++)
            {
                Coordinate c = size4ValidPlacements[i];
                wublinAllowed.AddTuple({ c.first, c.second });
            }

        // 2. Overlap prevention constraints

        // 3. Negative polarity prevention constraints

        // 4. Positive polarity requirement constraints
    }

    // MUST BE BEFORE HATE CODE
    auto wublinsNoShapeOverlapConstraint = wublinProblem.AddNoOverlap2D(); 
    std::vector<IntervalVar> xOfShapes; // TODOOOOOOOOOOO LOOK INTO IF WE EVEN NEED THESE (DO WE NEED TO REUSE?
    std::vector<IntervalVar> yOfShapes;
    //defines that no wublin can share the same spaces (overlap prevention)
    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        xOfShapes.push_back( wublinProblem.NewIntervalVar( wublinsXAxis[i], wublinPool[i].getSize(), wublinsXAxis[i] + wublinPool[i].getSize() ) ); // X axis shape
        yOfShapes.push_back( wublinProblem.NewIntervalVar( wublinsYAxis[i], wublinPool[i].getSize(), wublinsYAxis[i] + wublinPool[i].getSize() ) ); // Y axis shape (when combined, makes the full shape)
        wublinsNoShapeOverlapConstraint.AddRectangle(xOfShapes[i], yOfShapes[i]); // Adds wublins spaces it takes up to the constraint of non overlap.
    }
    //

    // No negative polarity constraint: Defines that the wublin that applies negative polarity cannot be in the negative polarity radius of the wublin which would recieve the negative polarity 
    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        int hatedWublinIndex = indexOfWublinName( wublinPool[i].getHates(), wublinPool);
        if (hatedWublinIndex != -1) // If wublin has no negative polarity target, then we skip (i.e. wubbox or monoculus
        {
            int currWublinSize = wublinPool[i].getSize() + (2 * hateRadius);
            auto wublinNegativePolarityConstraint = wublinProblem.AddNoOverlap2D();
            IntervalVar currWublinHateRadiusX = wublinProblem.NewIntervalVar(wublinsXAxis[i] - hateRadius, currWublinSize, wublinsXAxis[i] - hateRadius + currWublinSize);
            IntervalVar currWublinHateRadiusY = wublinProblem.NewIntervalVar(wublinsYAxis[i] - hateRadius, currWublinSize, wublinsYAxis[i] - hateRadius + currWublinSize);

            wublinNegativePolarityConstraint.AddRectangle(xOfShapes[hatedWublinIndex],  yOfShapes[hatedWublinIndex]); //adds the hated/negative polarity source wublins physical space
            wublinNegativePolarityConstraint.AddRectangle(currWublinHateRadiusX, currWublinHateRadiusY); // Adds the radius that the hatedWublin must be in to be considered hated (applying negative polarity)

        }
    }

    // All positive polarity constraint: Defines that the wublin that applies positive polarity must be in the positive polarity radius of the wublin which would achieve positive polarity with it's presence.
    
    // impelemented via setting ranges that the liked wublin must be from the current wublin

    /*  General idea:
    * NOTE: This functionality is inspired/was learned of by this resource: https://www.geeksforgeeks.org/dsa/find-two-rectangles-overlap/
    * IF x of bottom right corner (highest x coordinate) of one square is < the others upper left x (the lowest x value of the square), then NO overlapping is occuring (also check vise versa)
    * IF the y of the top left of one square (the highest y of the square) is < the y of the bottom right of the other square (lowest point of square), then the squares are not overlapping
    * ANY other case entails overlap is occuring
    */
    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        int likedWublinindex = indexOfWublinName(wublinPool[i].getLikes(), wublinPool); //holds which wublin index references the liked wublin
        if (likedWublinindex == -1) // skip if wublin does not have positive polarity
            continue;
        
        LinearExpr upperLeftOfLikeRadiusX = wublinsXAxis[i] - likeRadius;
        LinearExpr upperLeftOfLikeRadiusY = wublinsYAxis[i] + (wublinPool[i].getSize() - 1) + likeRadius;
        LinearExpr lowerRightOfLikeRadiusX = wublinsXAxis[i] + (wublinPool[i].getSize() - 1) + likeRadius; 
        LinearExpr lowerRightOfLikeRadiusY = wublinsYAxis[i] - likeRadius;

        // Because the placement decisions are on the bottom left, traversal goes right and up in this nested for loop
        LinearExpr upperLeftOfDesiredWublinX = wublinsXAxis[likedWublinindex]; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr upperLeftOfDesiredWublinY = wublinsYAxis[likedWublinindex] + wublinPool[likedWublinindex].getSize() - 1;
        LinearExpr lowerRightOfDesiredWublinX = wublinsXAxis[likedWublinindex] + wublinPool[likedWublinindex].getSize() - 1; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr lowerRightOfDesiredWublinY = wublinsYAxis[likedWublinindex];
       
        // All conditions must be met for overlap to occur
        wublinProblem.AddGreaterOrEqual(lowerRightOfLikeRadiusX, upperLeftOfDesiredWublinX); 
        wublinProblem.AddGreaterOrEqual(lowerRightOfDesiredWublinX, upperLeftOfLikeRadiusX);
        wublinProblem.AddGreaterOrEqual(upperLeftOfLikeRadiusY, lowerRightOfDesiredWublinY);
        wublinProblem.AddGreaterOrEqual(upperLeftOfDesiredWublinY, lowerRightOfLikeRadiusY);   
    }


/* ( LOOK INTO DOING DOUBLE RANGE BASED SYSTEM
*         | This gives the min x, max y
*         V
        (0,2) (1,2) (2,2)
        (0,1) (1,1) (2,1)
        (0,0)  (1,0) (2,0) // this gets max x, min y


*  |
*  (0,0)
* 
* Square name = 2
*  (0,3) Upper left = UL2
*  (1,2) Lower right = LR2
* 
* Square name = 1
*  (1,5) upper left = UL1
*  (6,0) Lower right = LR1
* 
* 
        
* 
* 
* Does overlap = YES
* 
* 
*/


/*
       std::vector<LinearExpr> likeRadiusSquaresX; // contains all coordinates of like radius for the i-th wublin. The ith index of this vector matches with the ith index of the likeRadiusSquaresY for defining a single square coordiante
        std::vector<LinearExpr> likeRadiusSquaresY;

        int likedWublinindex = indexOfWublinName(wublinPool[i].getLikes(), wublinPool); //holds which wublin index references the liked wublin
        LinearExpr upperLeftOfLikeRadiusX = wublinsXAxis[i] - likeRadius;
        LinearExpr upperLeftOfLikeRadiusY = wublinsYAxis[i] + (wublinPool[i].getSize() - 1) + likeRadius;

        int squareNumber = wublinPool[i].getSize() + (2 * likeRadius);
        int rowI{ 0 }; // traverses rows
        int colI{ 0 }; //traverses col of like radius
        for (int t{ 0 }; t < squareNumber * squareNumber ; t++) // For loop for each square of the like radius
        {
            if(isOccupiedSpace()) // leaves out the physical squares the wublin takes up
            likeRadiusSquaresX.push_back(upperLeftOfLikeRadiusX + colI);
            likeRadiusSquaresY.push_back(upperLeftOfLikeRadiusY - rowI);


            if (t % (squareNumber - 1)) //case of reaching end of row
            {
                rowI++;
                colI = 0;
            }

        }
        
        */







    // REMINDER, PLACEMENT IS DONE IN TERMS OF THE BOTTOM LEFT CORNER

   
    

/* Go from bottom left to upper left radius via: 
lowerLeftYCoord + ( (size - 1) + likeRadius) 
lowerLeftXCoord - likeRadius
*  0 0 0 0 0 0
*  0 0 0 0 0 0
*  0 0 1 1 0 0
*  0 0 1 1 0 0
*  0 0 0 0 0 0
*  0 0 0 0 0 0
* 
*  0 0 0 0 0 0 0
*  0 0 0 0 0 0 0
*  0 0 1 1 1 0 0
*  0 0 1 1 1 0 0
*  0 0 1 1 1 0 0 
*  0 0 0 0 0 0 0
*  0 0 0 0 0 0 0
*  
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 9
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 8 
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 7
! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 6
! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 ! ! ! ! M M ! ! ! ! ! ! ! ! 5
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 M M 0 0 0 ! ! ! ! ! 4
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 W W W W ! ! ! ! ! 3
! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 ! ! ! 0 0 W W W W ! ! ! ! ! 2
! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! 0 0 W W W W 0 ! ! ! ! 1 
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! 0 W W W W 0 ! ! ! ! 0 
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! 0 0 0 0 ! ! ! ! ! 9
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 8
0 0 0 0 0 0 0 0 0 0 0 G G G 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 7 
0 0 0 0 0 0 0 0 D D D G G G 0 0 0 ! ! ! ! ! ! ! ! ! 0 0 0 0 6
0 0 0 C C C z z D D D G G G 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 5
0 0 0 C C C z z D D D 0 Z Z 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 4 
0 0 0 C C C 0 A A 0 0 0 Z Z 0 T T 0 0 0 0 0 0 ! ! ! 0 0 0 0 3
0 0 0 B B W W A A 0 0 0 D D D T T 0 0 0 0 0 0 ! ! ! ! 0 0 0 2 
S S 0 B B W W 0 0 0 0 0 D D D P P P 0 0 0 0 0 ! ! ! ! ! ! ! 1
S S 0 0 S S S P P P 0 0 D D D P P P 0 0 0 0 0 ! ! ! ! ! ! ! 0 
! 0 0 0 S S S P P P 0 b b 0 0 P P P 0 0 0 0 ! ! ! ! ! ! ! ! 9
! 0 0 0 S S S P P P 0 b b 0 0 T T 0 0 0 0 0 ! ! ! ! ! ! ! ! 8
! 0 0 0 0 P P 0 0 0 U U B B 0 T T 0 0 0 0 0 ! ! ! ! ! ! ! ! 7
! 0 0 0 0 P P 0 0 0 U U B B 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 6
! 0 0 0 0 0 0 0 0 0 0 M M M 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 5
! 0 0 0 0 0 0 0 F F F M M M 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 4
! ! ! 0 0 0 0 0 F F F M M M 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 3
! ! ! ! 0 0 0 0 F F F 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! 2
! ! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! 1
! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! 0
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
*/



/*
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 9
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 8
! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 7
! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 6
! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 ! ! ! ! 0 0 ! ! ! ! ! ! ! ! 5
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 ! ! ! ! ! 4
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 ! ! ! ! ! 3
! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 ! ! ! 0 0 0 0 0 0 ! ! ! ! ! 2 
! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! 0 0 0 0 0 0 0 ! ! ! ! 1
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! 0 0 0 0 0 0 ! ! ! ! 0
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! 0 0 0 0 ! ! ! ! ! 9
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 8 
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 7
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! 0 0 0 0 6
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 5
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 4
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! 0 0 0 0 3
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! 0 0 0 2
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 1
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 0
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 9
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 8
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 7
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 6
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 5
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 4
! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! 3
! ! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! 2
! ! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! 1
! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! 0
0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8
*/
  
    // Negative polarity is just another set of AddNoOverlap2D but done in sets of 2 instead
    // Positive polarity we have to figure out how to do the inverse of NoOverlap2d to say RequireOverlap2D



    // Solver params + running
    std::cout << "Started solving!";
    Model model; // model for the SOLVER
    
    model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& result) 
    { // This logs ALL the feasible solutions the solver comes across
            // TODO: Add here how I want to log/extract the solutions
            std::cout << "Wublin solution:" << std::endl;
            for (int i{ 0 }; i < wublinPool.size(); i++)       
                std::cout << wublinPool[i].getName() << " = (" << SolutionIntegerValue(result, wublinsXAxis[i]) << "," << SolutionIntegerValue(result, wublinsYAxis[i]) << ")" << std::endl;
            
    }));
    SatParameters params;
    //params.set_max_time_in_seconds(86400);
    params.set_max_time_in_seconds(300);
    model.Add(NewSatParameters(params));
    const CpSolverResponse result = SolveCpModel(wublinProblem.Build(), &model); // solves problem and stores result
    //
    


}

//In converting from array representation to a coordinate based map, the coordinates are made with the bottom left being (0,0). Going right increases the x and up increases the y.
std::map<Coordinate,std::string> wublinArrToCoordinatePairMap(std::vector<std::vector<char>>& wublinMapArrForm)
{
    std::map<Coordinate, std::string> wublinMap;
    int xVal{ 0 }, yVal{ 0 };

    for (int r{ 29 }; r >= 0; r--) // y axis
    {
        for (int c{ 0 }; c < (wublinMapArrForm[0].size()); c++) // x axis
        {
            if (wublinMapArrForm[r][c] == '0')
                wublinMap[Coordinate(xVal, yVal)] = "Empty";
            xVal++;
        }
        xVal = 0;
        yVal++;
    }
    
    return wublinMap;
}

//NOTE TO SELF: Wubbox does not have a polarity system, but we want to make sure to have some method to include them in the map (such as saving a spot for them)
int main()
{

    int likeRadius{ 2 };
    int hateRadius{ 2 };

    //Map is traversed via and X and Y axis systen. (shown in a image I made: https://imgur.com/a/SoHMJGK (made with help from an outline on the msm wiki))

   // wublinMap is used to assist in converting the array version of the map into a map data structure, so that the problem can be solved in terms of coordinate pairs
    std::vector<std::vector<char>> wublinMapArr= {
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '!', '!', '!', '!', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!'},
    {'!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!'},
    {'!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '0', '0', '0', '0', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '0', '0', '0', '0', '0'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '0', '0', '0', '0', '0'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '0', '0', '0', '0'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '0', '0', '0'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'}
    };

    std::vector<Wublin> wublinPool = { //All you have to do to add a Wublin to the algorithm is to add the Wublin and their characteristics here, if you want multiples of a wublin, change the count
//Common Wublins
//              Name:           size:  Likes:             Hates:         count:
        {Wublin("Brump",            2, "Fleechwurm",     "Blipsqueak",       1 )},
        {Wublin("Zynth",            2, "Gheegur",        "Astropod",         1 )},
        {Wublin("Zuuker",           2, "Maulch",         "Screemu",          1 )},
        {Wublin("Blipsqueak",       2, "Screemu",        "Tympa",            1 )},
        {Wublin("Bona-Petite",      3, "Zuuker",         "Creepuscule",      1 )},
        {Wublin("Poewk",            3,"Brump",           "Bona-Petite",      1 )},
        {Wublin("Screemu",          2,"Creepuscule",     "Pixolotl",         1 )},
        {Wublin("Tympa",            2,"Poewk",           "Thwok",            1 )},
        {Wublin("Creepuscule",      3,"Whajje",          "Gheegur",          1 )},
        {Wublin("Whajje",           2,"Dwumrohl",        "Zynth",            1 )},
        {Wublin("Astropod",         2,"Bona-Petite",     "Brump",            1 )},
        {Wublin("Pixolotl",         2,"Scargo",          "Whajje",           1 )},
        {Wublin("Monculus",         2,"None",            "None",             1 )}, //This is an edge case
        {Wublin("Thwok",            2,"Dermit",          "Zuuker",           1 )},
        {Wublin("Dwumrohl",         3,"Astropod",        "Fleechwurm",       1 )},
        {Wublin("Scargo",           3,"Blipsqueak",      "Maulch",           1 )},
        {Wublin("Fleechwurm",       3,"Pixolotl",        "Dwumrohl",         1 )},
        {Wublin("Maulch",           3,"Thwok",           "Poewk",            1 )},
        {Wublin("Dermit",           3,"Zynth",           "Scargo",           1 )},
        {Wublin("Gheegur",          3,"Tympa",           "Dermit",           1 )},
        {Wublin("Wubbox",           4,"None",            "None",             1 )},
//Rare Wublins
//              Name:           size:  Likes:             Hates:         count:
        {Wublin("Rare Brump",       2,"Rare Fleechwurm", "Rare Blipsqueak",  1 )},
        {Wublin("Rare Zynth",       2,"Rare Gheegur",    "Rare Astropod",    1 )},
        {Wublin("Rare Zuuker",      2,"Rare Maulch",     "Rare Screemu",     1 )},
        {Wublin("Rare Blipsqueak",  2,"Rare Screemu",    "Rare Tympa",       1 )},
        {Wublin("Rare Bona-Petite", 3,"Rare Zuuker",     "Rare Creepuscule", 1 )},
        {Wublin("Rare Poewk",       3,"Rare Brump",      "Rare Bona-Petite", 1 )},
        {Wublin("Rare Screemu",     2,"Rare Creepuscule","Rare Pixolotl",    1 )},
        {Wublin("Rare Tympa",       2,"Rare Poewk",      "Rare Thwok",       1 )},
        {Wublin("Rare Creepuscule", 3,"Rare Whajje",     "Rare Gheegur",     1 )},
        {Wublin("Rare Whajje",      2,"Rare Dwumrohl",   "Rare Zynth",       1 )},
        {Wublin("Rare Astropod",    2,"Rare Bona-Petite","Rare Brump",       1 )},
        {Wublin("Rare Pixolotl",    2,"Rare Scargo",     "Rare Whajje",      1 )},
        {Wublin("Rare Monculus",    2,"None",            "None",             1 )}, //This is an edge case
        {Wublin("Rare Thwok",       2,"Rare Dermit",     "Rare Zuuker",      1 )},
        {Wublin("Rare Dwumrohl",    3,"Rare Astropod",   "Rare Fleechwurm",  1 )},
        {Wublin("Rare Scargo",      3,"Rare Blipsqueak", "Rare Maulch",      1 )},
        {Wublin("Rare Fleechwurm",  3,"Rare Pixolotl",   "Rare Dwumrohl",    1 )},
        {Wublin("Rare Maulch",      3,"Rare Thwok",      "Rare Poewk",       1 )},
        {Wublin("Rare Dermit",      3,"Rare Zynth",      "Rare Scargo",      1 )},
        {Wublin("Rare Gheegur",     3,"Rare Tympa",      "Rare Dermit",      1 )},
        {Wublin("Rare Wubbox",      4,"None",            "None",             1 )},
//Epic Wublins (Not all of these are officially released yet).
//              Name:            size: Likes:            Hates:          count: ID:
        {Wublin("Epic Brump",       2,"Epic Fleechwurm", "Epic Blipsqueak",  1 )},
        {Wublin("Epic Zynth",       2,"Epic Gheegur",    "Epic Astropod",    1 )},
        {Wublin("Epic Zuuker",      2,"Epic Maulch",     "Epic Screemu",     1 )},
        {Wublin("Epic Blipsqueak",  2,"Epic Screemu",    "Epic Tympa",       1 )},
        {Wublin("Epic Bona-Petite", 3,"Epic Zuuker",     "Epic Creepuscule", 1 )},
        {Wublin("Epic Poewk",       3,"Epic Brump",      "Epic Bona-Petite", 1 )},
        {Wublin("Epic Screemu",     2,"Epic Creepuscule","Epic Pixolotl",    1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Tympa",       2,"Epic Poewk",      "Epic Thwok",       1 )},
        {Wublin("Epic Creepuscule", 3,"Epic Whajje",     "Epic Gheegur",     1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Whajje",      2,"Epic Dwumrohl",   "Epic Zynth",       1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Astropod",    2,"Epic Bona-Petite","Epic Brump",       1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Pixolotl",    2,"Epic Scargo",     "Epic Whajje",      1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Monculus",    2,"None",            "None",             1 )}, //This is an edge case (along with wubbox)
        {Wublin("Epic Thwok",       2,"Epic Dermit",     "Epic Zuuker",      1 )},
        {Wublin("Epic Dwumrohl",    3,"Epic Astropod",   "Epic Fleechwurm",  1 )},
        {Wublin("Epic Scargo",      3,"Epic Blipsqueak", "Epic Maulch",      1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Fleechwurm",  3,"Epic Pixolotl",   "Epic Dwumrohl",    1 )},
        {Wublin("Epic Maulch",      3,"Epic Thwok",      "Epic Poewk",       1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Dermit",      3,"Epic Zynth",      "Epic Scargo",      1 )},
        {Wublin("Epic Gheegur",     3,"Epic Tympa",      "Epic Dermit",      1 )},
        {Wublin("Epic Wubbox",      4,"None",            "None",             1 )},
    };
    
    for(int r{0}; r < 30; r++)
    {
        for(int c{0}; c < 30; c++)
        {
            std::cout << wublinMapArr[r][c] << " ";
        }
        std::cout << '\n';
    }

    std::map<Coordinate,std::string> wublinMapCoordPairs = wublinArrToCoordinatePairMap(wublinMapArr);


    findLayouts(likeRadius,hateRadius, wublinMapCoordPairs, wublinPool, wublinMapArr);
    std::cout << "DONE!";
}



// Answer should probably be stored as a vector of coordinates, with the ith index of the vector representing the ith wublin from the wublinPool vector

/*
Brump = (12,6) X
Zynth = (12,13) X
Zuuker = (10,6) X
Blipsqueak = (3,11) X
Bona-Petite = (7,8) X
Poewk = (15,9) X
Screemu = (0,10) X
Tympa = (15,12) X
Creepuscule = (3,13) X
Whajje = (5,11) X
Astropod = (7,12) X
Pixolotl = (5,6) X
Monculus = (20,24) X
Thwok = (15,7) X
Dwumrohl = (8,14) x
Scargo = (4,8) X
Fleechwurm = (8,2) X
Maulch = (11,3) x
Dermit = (12,10) X
Gheegur = (11,15) X
Wubbox = (21,20) x

Rare Brump = (11,8) x
Rare Zynth = (6,14)
Rare Zuuker = (2,19)
Rare Blipsqueak = (3,3)
Rare Bona-Petite = (2,16)
Rare Poewk = (14,4)
Rare Screemu = (1,4)
Rare Tympa = (13,8)
Rare Creepuscule = (1,6)
Rare Whajje = (2,9)
Rare Astropod = (0,15)
Rare Pixolotl = (5,4)
Rare Monculus = (19,21)
Rare Thwok = (8,17)
Rare Dwumrohl = (0,12)
Rare Scargo = (5,1)
Rare Fleechwurm = (7,5)
Rare Maulch = (4,19)
Rare Dermit = (5,16)
Rare Gheegur = (9,11)
Rare Wubbox = (13,26)
Epic Brump = (20,4)
Epic Zynth = (14,2)
Epic Zuuker = (20,13)
Epic Blipsqueak = (12,18)
Epic Bona-Petite = (17,13)
Epic Poewk = (16,1)
Epic Screemu = (10,20)
Epic Tympa = (14,0)
Epic Creepuscule = (7,19)
Epic Whajje = (10,18)
Epic Astropod = (14,17)
Epic Pixolotl = (18,11)
Epic Monculus = (19,2)
Epic Thwok = (17,7)
Epic Dwumrohl = (13,20)
Epic Scargo = (14,14)
Epic Fleechwurm = (19,7)
Epic Maulch = (20,10)
Epic Dermit = (17,4)
Epic Gheegur = (11,0)
Epic Wubbox = (26,13)

*/


