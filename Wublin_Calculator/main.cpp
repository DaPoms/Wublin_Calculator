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
    

    
  /*
  Does this sucessfully implement the hate/negative polarity constraints?
    // No negative polarity constraint: Defines that the wublin that applies negative polarity cannot be in the negative polarity radius of the wublin which would recieve the negative polarity 
    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        int hatedWublinIndex = indexOfWublinName( wublinPool[i].getHates(), wublinPool);
        if (hatedWublinIndex != -1) // If wublin has no negative polarity target, then we skip (i.e. wubbox or monoculus
        {
            int currWublinSize = wublinPool[i].getSize() + (2 * hateRadius);
            auto wublinNegativePolarityConstraint = wublinProblem.AddNoOverlap2D();
            IntervalVar currWublinHateRadiusX = wublinProblem.NewIntervalVar(wublinsXAxis[i] - hateRadius, currWublinSize, wublinsXAxis[i] - hateRadius + currWublinSize);
            IntervalVar currWublinHateRadiusY = wublinProblem.NewIntervalVar(wublinsYAxis[i] - hateRadius, currWublinSize, wublinsXAxis[i] - hateRadius + currWublinSize);

            wublinNegativePolarityConstraint.AddRectangle(xOfShapes[hatedWublinIndex],  yOfShapes[hatedWublinIndex]); //adds the hated/negative polarity source wublins physical space
            wublinNegativePolarityConstraint.AddRectangle(currWublinHateRadiusX, currWublinHateRadiusY); // Adds the radius that the hatedWublin must be in to be considered hated (applying negative polarity)

        }
    }
    
Every wublin has a hate radius with a unique wublin that when siad wublin is in their hate radius, the original wublin with the hate radius loses productivity. Hate radius is the distance from the actual space the wublin takes up, so for example,
  0 0 0 0 0 0
  0 0 0 0 0 0
  0 0 1 1 0 0
  0 0 1 1 0 0
  0 0 0 0 0 0
  0 0 0 0 0 0
this is a 2x2 wublin with a hate radius of 2, with 1 denoting its physical space it takes and 0 being its hate radius. The constraint should ensure the hated wublin never PHSICALLY is in the hate radius
  
  
  
  */



    // REMINDER, PLACEMENT IS DONE IN TERMS OF THE BOTTOM LEFT CORNER

   
    

/*
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
0 0 0 0 0 0 0 0 0 0 D D D 0 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 5
0 0 0 0 0 0 0 0 0 0 D D D 0 0 0 0 0 0 0 0 0 0 ! ! 0 0 0 0 0 4 
0 0 0 0 0 0 0 0 0 0 D D D 0 0 0 0 0 0 0 0 0 0 ! ! ! 0 0 0 0 3
0 0 0 0 0 0 0 0 0 0 B B 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! 0 0 0 2 
0 0 0 0 0 0 0 0 0 0 B B 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 1
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
    Model model; // model for the SOLVER
    
    model.Add(NewFeasibleSolutionObserver([&](const CpSolverResponse& result) 
    { // This logs ALL the feasible solutions the solver comes across
            // TODO: Add here how I want to log/extract the solutions
            LOG(INFO) << "Wublin solution:";
            for (int i{ 0 }; i < wublinPool.size(); i++)       
                LOG(INFO) << wublinPool[i].getName() << " = (" << SolutionIntegerValue(result, wublinsXAxis[i]) << "," << SolutionIntegerValue(result, wublinsYAxis[i]) << ")";
            
    }));
    SatParameters params;
    //params.set_max_time_in_seconds(86400);
    params.set_max_time_in_seconds(60);
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
}



// Answer should probably be stored as a vector of coordinates, with the ith index of the vector representing the ith wublin from the wublinPool vector




/*
I0000 00:00:1786117005.898851   19396 main.cpp:249] Wublin solution:
I0000 00:00:1786117005.906586   19396 main.cpp:251] Brump = (12,28)
I0000 00:00:1786117005.909867   19396 main.cpp:251] Zynth = (20,7)
I0000 00:00:1786117005.910832   19396 main.cpp:251] Zuuker = (3,14)
I0000 00:00:1786117005.911065   19396 main.cpp:251] Blipsqueak = (10,11) THIS
I0000 00:00:1786117005.911290   19396 main.cpp:251] Bona-Petite = (4,6)
I0000 00:00:1786117005.911481   19396 main.cpp:251] Poewk = (8,1)
I0000 00:00:1786117005.911686   19396 main.cpp:251] Screemu = (1,12)
I0000 00:00:1786117005.911896   19396 main.cpp:251] Tympa = (4,16)
I0000 00:00:1786117005.912197   19396 main.cpp:251] Creepuscule = (10,4)
I0000 00:00:1786117005.912417   19396 main.cpp:251] Whajje = (19,23)
I0000 00:00:1786117005.912580   19396 main.cpp:251] Astropod = (13,26)
I0000 00:00:1786117005.912782   19396 main.cpp:251] Pixolotl = (9,7)
I0000 00:00:1786117005.912997   19396 main.cpp:251] Monculus = (15,26)
I0000 00:00:1786117005.913198   19396 main.cpp:251] Thwok = (10,9)
I0000 00:00:1786117005.913384   19396 main.cpp:251] Dwumrohl = (10,13) THIS
I0000 00:00:1786117005.913554   19396 main.cpp:251] Scargo = (17,12)
I0000 00:00:1786117005.914319   19396 main.cpp:251] Fleechwurm = (13,4)
I0000 00:00:1786117005.914522   19396 main.cpp:251] Maulch = (26,14)
I0000 00:00:1786117005.914734   19396 main.cpp:251] Dermit = (9,16)
I0000 00:00:1786117005.914930   19396 main.cpp:251] Gheegur = (4,3)
I0000 00:00:1786117005.915139   19396 main.cpp:251] Wubbox = (11,0)
I0000 00:00:1786117005.916512   19396 main.cpp:251] Rare Brump = (5,14)
I0000 00:00:1786117005.917403   19396 main.cpp:251] Rare Zynth = (14,17)
I0000 00:00:1786117005.917897   19396 main.cpp:251] Rare Zuuker = (6,1)
I0000 00:00:1786117005.918160   19396 main.cpp:251] Rare Blipsqueak = (1,10)
I0000 00:00:1786117005.918369   19396 main.cpp:251] Rare Bona-Petite = (20,13)
I0000 00:00:1786117005.918571   19396 main.cpp:251] Rare Poewk = (0,14)
I0000 00:00:1786117005.918766   19396 main.cpp:251] Rare Screemu = (1,17)
I0000 00:00:1786117005.918966   19396 main.cpp:251] Rare Tympa = (11,7)
I0000 00:00:1786117005.919748   19396 main.cpp:251] Rare Creepuscule = (16,4)
I0000 00:00:1786117005.920119   19396 main.cpp:251] Rare Whajje = (19,21)
I0000 00:00:1786117005.920375   19396 main.cpp:251] Rare Astropod = (4,1)
I0000 00:00:1786117005.920520   19396 main.cpp:251] Rare Pixolotl = (27,12)
I0000 00:00:1786117005.920672   19396 main.cpp:251] Rare Monculus = (12,16)
I0000 00:00:1786117005.920825   19396 main.cpp:251] Rare Thwok = (1,4)
I0000 00:00:1786117005.920975   19396 main.cpp:251] Rare Dwumrohl = (16,7)
I0000 00:00:1786117005.921391   19396 main.cpp:251] Rare Scargo = (12,10)
I0000 00:00:1786117005.921612   19396 main.cpp:251] Rare Fleechwurm = (13,7)
I0000 00:00:1786117005.921938   19396 main.cpp:251] Rare Maulch = (13,20)
I0000 00:00:1786117005.922162   19396 main.cpp:251] Rare Dermit = (3,11)
I0000 00:00:1786117005.923790   19396 main.cpp:251] Rare Gheegur = (7,4)
I0000 00:00:1786117005.924254   19396 main.cpp:251] Rare Wubbox = (21,19)
I0000 00:00:1786117005.924759   19396 main.cpp:251] Epic Brump = (17,10)
I0000 00:00:1786117005.925292   19396 main.cpp:251] Epic Zynth = (17,1)
I0000 00:00:1786117005.926064   19396 main.cpp:251] Epic Zuuker = (7,7)
I0000 00:00:1786117005.926469   19396 main.cpp:251] Epic Blipsqueak = (4,9)
I0000 00:00:1786117005.927071   19396 main.cpp:251] Epic Bona-Petite = (7,13)
I0000 00:00:1786117005.927742   19396 main.cpp:251] Epic Poewk = (13,13)
I0000 00:00:1786117005.927956   19396 main.cpp:251] Epic Screemu = (19,2)
I0000 00:00:1786117005.928173   19396 main.cpp:251] Epic Tympa = (14,28)
I0000 00:00:1786117005.928423   19396 main.cpp:251] Epic Creepuscule = (19,9)
I0000 00:00:1786117005.928651   19396 main.cpp:251] Epic Whajje = (21,23)
I0000 00:00:1786117005.929770   19396 main.cpp:251] Epic Astropod = (15,10)
I0000 00:00:1786117005.930049   19396 main.cpp:251] Epic Pixolotl = (23,23)
I0000 00:00:1786117005.930253   19396 main.cpp:251] Epic Monculus = (15,0)
I0000 00:00:1786117005.930902   19396 main.cpp:251] Epic Thwok = (15,2)
I0000 00:00:1786117005.931103   19396 main.cpp:251] Epic Dwumrohl = (10,19)
I0000 00:00:1786117005.932229   19396 main.cpp:251] Epic Scargo = (19,4)
I0000 00:00:1786117005.936046   19396 main.cpp:251] Epic Fleechwurm = (6,16)
I0000 00:00:1786117005.940839   19396 main.cpp:251] Epic Maulch = (6,19)
I0000 00:00:1786117005.941088   19396 main.cpp:251] Epic Dermit = (1,6)
I0000 00:00:1786117005.941474   19396 main.cpp:251] Epic Gheegur = (3,18)
I0000 00:00:1786117005.942298   19396 main.cpp:251] Epic Wubbox = (6,9)




*/
