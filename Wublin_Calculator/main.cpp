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

std::vector<int> indexesOfAllWublinsRarities(std::string wublinName, const std::vector<Wublin>& wublins)
{
    std::vector<int> wublinIndexes;
    std::string targetName = wublinName;
    int spaceIndex = wublinName.find(' ');
    if (spaceIndex != -1) // entails name is 2 words (For a rare/epic wublin, 1st word is always rarity and 2nd is the actual name of the wublin)
        targetName = wublinName.substr(spaceIndex + 1);

    for (int i{ 0 }; i < wublins.size(); i++)
    {
        std::string ithWublinName = wublins[i].getName();
        spaceIndex = ithWublinName.find(' ');
        if(spaceIndex != -1)
            ithWublinName = ithWublinName.substr(spaceIndex + 1);
        if (ithWublinName == targetName)
            wublinIndexes.push_back(i);
    }
    return wublinIndexes;
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




// Used for only allowing placements that do not go outside of the map
void addValidPlacements(int i, CpModelBuilder& wublinProblem, const std::vector<IntVar>& wublinsXAxis, const std::vector<IntVar>& wublinsYAxis, 
    const std::vector<Wublin>& wublinPool, const std::vector<Coordinate>& size2ValidPlacements, const std::vector<Coordinate>& size3ValidPlacements, 
    const std::vector<Coordinate>& size4ValidPlacements)
{
    auto wublinAllowed = wublinProblem.AddAllowedAssignments({ wublinsXAxis[i], wublinsYAxis[i] });
    if (wublinPool[i].getSize() == 2)
        for (int p{ 0 }; p < size2ValidPlacements.size(); p++)
        {
            Coordinate c = size2ValidPlacements[p];
            wublinAllowed.AddTuple({ c.first, c.second });
        }
    else if (wublinPool[i].getSize() == 3)
        for (int p{ 0 }; p < size3ValidPlacements.size(); p++)
        {
            Coordinate c = size3ValidPlacements[p];
            wublinAllowed.AddTuple({ c.first, c.second });
        }
    else // This else is for size == 4
        for (int p{ 0 }; p < size4ValidPlacements.size(); p++)
        {
            Coordinate c = size4ValidPlacements[p];
            wublinAllowed.AddTuple({ c.first, c.second });
        }
}

void addToNoOverlapPool(int i, NoOverlap2DConstraint& wublinsNoShapeOverlapConstraint, CpModelBuilder& wublinProblem, std::vector<IntervalVar>& xOfShapes, 
    std::vector<IntervalVar>& yOfShapes, const std::vector<IntVar>& wublinsXAxis, const std::vector<IntVar>& wublinsYAxis, const std::vector<Wublin>& wublinPool)
{
    xOfShapes.push_back(wublinProblem.NewIntervalVar(wublinsXAxis[i], wublinPool[i].getSize(), wublinsXAxis[i] + wublinPool[i].getSize())); // X axis shape
    yOfShapes.push_back(wublinProblem.NewIntervalVar(wublinsYAxis[i], wublinPool[i].getSize(), wublinsYAxis[i] + wublinPool[i].getSize())); // Y axis shape (when combined, makes the full shape)
    wublinsNoShapeOverlapConstraint.AddRectangle(xOfShapes[i], yOfShapes[i]); // Adds wublins spaces it takes up to the constraint of non overlap.
}


// TODO: Add to negative pol prevention the ALL rarities of the negative polarity wublin
//Adds negative polarity prevention for the ith wublin
void addNegativePolPrevention(int i, int hateRadius, CpModelBuilder& wublinProblem, const std::vector<Wublin>& wublinPool, 
    const std::vector<IntVar>& wublinsXAxis, const std::vector<IntVar>& wublinsYAxis, std::vector<IntervalVar>& xOfShapes, std::vector<IntervalVar>& yOfShapes)
{
    

    // Find all rarities of the hatedWublin
    std::vector<int> hatedWublinIndexes = indexesOfAllWublinsRarities(wublinPool[i].getHates(), wublinPool);
    
    if (hatedWublinIndexes.size() == 0) // If wublin has no negative polarity target, then we skip (i.e. wubbox or monoculus)
        return;

    auto wublinNegativePolarityConstraint = wublinProblem.AddNoOverlap2D();
    int currWublinSize = wublinPool[i].getSize() + (2 * hateRadius);
    IntervalVar currWublinHateRadiusX = wublinProblem.NewIntervalVar(wublinsXAxis[i] - hateRadius, currWublinSize, wublinsXAxis[i] - hateRadius + currWublinSize);
    IntervalVar currWublinHateRadiusY = wublinProblem.NewIntervalVar(wublinsYAxis[i] - hateRadius, currWublinSize, wublinsYAxis[i] - hateRadius + currWublinSize);
    wublinNegativePolarityConstraint.AddRectangle(currWublinHateRadiusX, currWublinHateRadiusY); // Adds the radius that the hatedWublin must be in to be considered hated (applying negative polarity)
    
    for (int h{ 0 }; h < hatedWublinIndexes.size(); h++)
        wublinNegativePolarityConstraint.AddRectangle(xOfShapes[hatedWublinIndexes[h]], yOfShapes[hatedWublinIndexes[h]]); //adds the hated/negative polarity source wublins physical space
       
}





void addPositivePolRequirement(int i, int likeRadius, CpModelBuilder& wublinProblem, const std::vector<Wublin>& wublinPool, 
    const std::vector<IntVar>& wublinsXAxis, const std::vector<IntVar>& wublinsYAxis)
{
    int likedWublinindex = indexOfWublinName(wublinPool[i].getLikes(), wublinPool); //holds which wublin index references the liked wublin
    if (likedWublinindex == -1) // skip if wublin does not have positive polarity
        return;

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


// REMINDER, PLACEMENT IS DONE IN TERMS OF THE BOTTOM LEFT CORNER
void findLayouts(int likeRadius, int hateRadius, const std::map<Coordinate, std::string>& wublinMapCoordPairs, const std::vector<Wublin>& wublinPool, const std::vector<std::vector<char>>& wublinMapArr) //Uses IBM's CPLEX to find all the maximum polarity placements using like radius and hate radius
{
    // 1. allowed groupings initializations + general problem setup
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

    // 2 + 3. Overlap prevention + negative polarity initializations
    NoOverlap2DConstraint wublinsNoShapeOverlapConstraint = wublinProblem.AddNoOverlap2D();
    std::vector<IntervalVar> xOfShapes;
    std::vector<IntervalVar> yOfShapes;

    // placement constraints
    for (int i{ 0 }; i < wublinPool.size(); i++) 
    {
        // 1. allowed groupings (avoids placing outside of map bounds)
        addValidPlacements(i, wublinProblem, wublinsXAxis, wublinsYAxis, wublinPool, size2ValidPlacements, size3ValidPlacements, size4ValidPlacements);
        // 2. Overlap prevention constraints
        addToNoOverlapPool(i, wublinsNoShapeOverlapConstraint, wublinProblem, xOfShapes, yOfShapes, wublinsXAxis, wublinsYAxis, wublinPool);
    }

    // wublin relationship constraints
    for (int i{ 0 }; i < wublinPool.size(); i++) 
    {
        // 3. Negative polarity prevention constraints
        addNegativePolPrevention(i, hateRadius, wublinProblem, wublinPool, wublinsXAxis, wublinsYAxis, xOfShapes, yOfShapes);
        // 4. Positive polarity requirement constraints
        addPositivePolRequirement(i, likeRadius, wublinProblem, wublinPool, wublinsXAxis, wublinsYAxis);
    }
    

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
* 
* 
* 
* 
* 
* 
* Proper layout test:
! ! ! ! ! ! ! ! ! ! ! ! 0 w w w w ! ! ! ! ! ! ! ! ! ! ! ! ! 29
! ! ! ! ! ! ! ! ! ! ! ! 0 w w w w ! ! ! ! ! ! ! ! ! ! ! ! ! 28
! ! ! ! ! ! ! ! ! ! ! ! 0 w w w w ! ! ! ! ! ! ! ! ! ! ! ! ! 27
! ! ! ! ! ! ! ! ! ! ! ! ! w w w w ! ! ! ! ! ! ! ! ! ! ! ! ! 26
! ! ! ! ! ! ! ! ! ! ! ! ! ! 0 0 ! ! ! ! 0 0 ! ! ! ! ! ! ! ! 25
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! W W W W M M ! ! ! ! ! 24
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! W W W W M M ! ! ! ! ! 23
! ! ! ! 0 ! ! ! ! ! 0 t t p p p ! ! ! W W W W m m ! ! ! ! ! 22
! ! ! p p 0 0 0 f f f t t p p p ! ! ! W W W W m m 0 ! ! ! ! 21
! ! 0 p p S S S f f f b b p p p ! ! ! ! 0 0 0 0 0 0 ! ! ! ! 20
! ! F F F S S S f f f b b g g g ! ! ! ! ! 0 0 0 0 ! ! ! ! ! 19
! 0 F F F S S S p p 0 0 0 g g g ! ! ! ! ! ! ! ! ! ! ! ! ! ! 18
0 0 F F F c c c p p s s s g g g ! ! ! ! ! ! ! ! ! ! ! ! ! ! 17
0 b b w w c c c B B s s s 0 b b 0 ! ! ! ! ! ! ! ! ! w w w w 16
0 b b w w c c c B B s s s 0 b b 0 z z d d d 0 ! ! 0 w w w w 15
P P P D D D s s f f f 0 0 0 p p p z z d d d 0 ! ! 0 w w w w 14
P P P D D D s s f f f c c c p p p s s d d d 0 ! ! ! w w w w 13
P P P D D D b b f f f c c c p p p s s t t 0 0 ! ! ! ! 0 0 0 12
t t s s s 0 b b 0 s s c c c 0 0 0 0 0 t t 0 0 ! ! ! ! ! ! ! 11
t t s s s p p a a s s 0 b b t t 0 0 0 c c c 0 ! ! ! ! ! ! ! 10
! 0 s s s p p a a 0 w w b b t t m m m c c c ! ! ! ! ! ! ! ! 9
! G G G b b b d d d w w 0 0 0 0 m m m c c c ! ! ! ! ! ! ! ! 8
! G G G b b b d d d B B B 0 0 0 m m m w w 0 ! ! ! ! ! ! ! ! 7
! G G G b b b d d d B B B 0 0 0 z z 0 w w 0 ! ! ! ! ! ! ! ! 6
! z z 0 z z a a t t B B B 0 0 0 z z 0 b b b ! ! ! ! ! ! ! ! 5
! z z 0 z z a a t t t t z z Z Z d d d b b b 0 ! ! ! ! ! ! ! 4
! ! ! 0 D D D 0 0 0 t t z z Z Z d d d b b b ! ! ! ! ! ! ! ! 3
! ! ! ! D D D 0 0 0 M M M 0 0 0 d d d 0 0 ! ! ! ! ! ! ! ! ! 2
! ! ! ! D D D 0 0 0 M M M 0 0 0 a a 0 ! ! ! ! ! ! ! ! ! ! ! 1
! ! ! ! ! ! ! ! ! ! M M M 0 0 0 a a ! ! ! ! ! ! ! ! ! ! ! ! 0
0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2
                    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
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
    std::cout << "Started solving!" << std::endl;
    Model model; // model for the SOLVER
    
    model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& result) 
    { // This logs ALL the feasible solutions the solver comes across
            // TODO: Add here how I want to log/extract the solutions
            std::cout << "Wublin solution:" << std::endl << "Finished in " << result.wall_time() << " seconds" << std::endl;
            for (int i{ 0 }; i < wublinPool.size(); i++)       
                std::cout << wublinPool[i].getName() << " = (" << SolutionIntegerValue(result, wublinsXAxis[i]) << "," << SolutionIntegerValue(result, wublinsYAxis[i]) << ")" << std::endl;
            
    }));
    SatParameters params;
    //params.set_max_time_in_seconds(86400);
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



// Answer should probably be stored as a vector of ints (each square is given a number), with the ith index of the vector representing the ith wublin from the wublinPool vector

/* 562 seconds to solve
Brump = (1,15) X
Zynth = (1,4) x
Zuuker = (14,3) X
Blipsqueak = (8,15) X
Bona-Petite = (10,5) x
Poewk = (0,12) x
Screemu = (6,13) x
Tympa = (0,10) x
Creepuscule = (5,15) x
Whajje = (3,15) x
Astropod = (7,9) x
Pixolotl = (3,20) x
Monculus = (23,23) x
Thwok = (8,4) x
Dwumrohl = (3,12) X
Scargo = (5,18) X
Fleechwurm = (2,17) x
Maulch = (10,0) x
Dermit = (4,1) X
Gheegur = (1,6) X
Wubbox = (19,21) x
Rare Brump = (11,19) x
Rare Zynth = (17,14) x
Rare Zuuker = (16,5) x
Rare Blipsqueak = (14,15) x
Rare Bona-Petite = (19,3) x
Rare Poewk = (13,20) x
Rare Screemu = (17,12) x
Rare Tympa = (11,21) x
Rare Creepuscule = (19,8) x
Rare Whajje = (19,6) x
Rare Astropod = (16,0) x
Rare Pixolotl = (8,17) x
Rare Monculus = (23,21) x
Rare Thwok = (19,11) x
Rare Dwumrohl = (16,2) x
Rare Scargo = (10,15) x
Rare Fleechwurm = (8,19) x
Rare Maulch = (16,7) x
Rare Dermit = (19,13) x
Rare Gheegur = (13,17) x
Rare Wubbox = (13,26) x

Epic Brump = (12,9) x
Epic Zynth = (12,3) x
Epic Zuuker = (4,4) x
Epic Blipsqueak = (6,11) x
Epic Bona-Petite = (4,6) x
Epic Poewk = (14,12) x
Epic Screemu = (9,10) x
Epic Tympa = (14,9) x
Epic Creepuscule = (11,11) x
Epic Whajje = (10,8) x
Epic Astropod = (6,4) x
Epic Pixolotl = (5,9) x
Epic Monculus = (16,10) x
Epic Thwok = (10,3) x
Epic Dwumrohl = (7,6) x
Epic Scargo = (2,9) x
Epic Fleechwurm = (8,12) x
Epic Maulch = (7,1) x
Epic Dermit = (13,0) x
Epic Gheegur = (13,6)
Epic Wubbox = (26,13)
*/


