#include <vector>
#include "wublin.h"
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

std::string extractWublinNameWithoutRarity(const std::string& wublinName)
{
    std::string ans = wublinName;
    int spaceIndex = wublinName.find(' ');
    if (spaceIndex != -1) // entails name is 2 words (For a rare/epic wublin, 1st word is always rarity and 2nd is the actual name of the wublin)
        ans = wublinName.substr(spaceIndex + 1);
    return ans;
}

/*std::vector<int> indexesOfAllWublinsRarities(std::string wublinName, const std::vector<Wublin>& wublinPool)
{
    std::vector<int> wublinIndexes;
    std::string targetName = extractWublinNameWithoutRarity(wublinName);

    for (int i{ 0 }; i < wublinPool.size(); i++)
    {
        std::string ithWublinName = extractWublinNameWithoutRarity(wublinPool[i].getName());
        if (ithWublinName == targetName)
            wublinIndexes.push_back(i);
    }
    return wublinIndexes;
}*/

//gets the index of a given wublin's higher rarity variants
std::vector<int> indexesOfAllWublinsRarityAboveOrEqual(const std::string& wublinName, const std::vector<Wublin>& wublinPool)
{
    std::vector<int> wublinIndexes; 
    std::string targetName = extractWublinNameWithoutRarity(wublinName);

    int startI{ 0 };
    for (int i{ 0 }; i < wublinPool.size(); i++)
        if (wublinPool[i].getName() == wublinName)
        {
            startI = i;
            break;
        }

    if (wublinPool[startI].getName() != wublinName && wublinName != "None")// Checks for error case of not found
        std::cerr << "The passed wublin " << wublinName << " was not found in the wublinPool.";

    for (int i{ startI }; i < wublinPool.size(); i++)
    {
        std::string ithWublinName = extractWublinNameWithoutRarity(wublinPool[i].getName());
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
    //std::vector<int> hatedWublinIndexes = indexesOfAllWublinsRarities(wublinPool[i].getHates(), wublinPool);
    std::vector<int> hatedWublinIndexes =  indexesOfAllWublinsRarityAboveOrEqual(extractWublinNameWithoutRarity(wublinPool[i].getHates()), wublinPool);


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






void addPositivePolRequirement(int targetI, int likeRadius, CpModelBuilder& wublinProblem, const std::vector<Wublin>& wublinPool, 
    const std::vector<IntVar>& wublinsXAxis, const std::vector<IntVar>& wublinsYAxis)
{
    std::string currWublinPolarityPartner = wublinPool[targetI].getLikes();

    std::vector<int> likedWublinIndexes = indexesOfAllWublinsRarityAboveOrEqual(currWublinPolarityPartner, wublinPool); //holds which wublin index references the liked wublin
    if (likedWublinIndexes.size() == 0) // skip if wublin does not have positive polarity (i.e. wubbox or monculus)
        return;

    LinearExpr upperLeftOfLikeRadiusX = wublinsXAxis[targetI] - likeRadius;
    LinearExpr upperLeftOfLikeRadiusY = wublinsYAxis[targetI] + (wublinPool[targetI].getSize() - 1) + likeRadius;
    LinearExpr lowerRightOfLikeRadiusX = wublinsXAxis[targetI] + (wublinPool[targetI].getSize() - 1) + likeRadius;
    LinearExpr lowerRightOfLikeRadiusY = wublinsYAxis[targetI] - likeRadius;


    std::vector<BoolVar> areWublinInPositivePolarityRadius;
    for (int likedWublinIndex : likedWublinIndexes)
    {
        // Because the placement decisions are on the bottom left, traversal goes right and up in this nested for loop
        LinearExpr upperLeftOfDesiredWublinX = wublinsXAxis[likedWublinIndex]; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr upperLeftOfDesiredWublinY = wublinsYAxis[likedWublinIndex] + wublinPool[likedWublinIndex].getSize() - 1;
        LinearExpr lowerRightOfDesiredWublinX = wublinsXAxis[likedWublinIndex] + wublinPool[likedWublinIndex].getSize() - 1; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr lowerRightOfDesiredWublinY = wublinsYAxis[likedWublinIndex];

        // At least 1 of the rarities above or = the wublins current rarity must be in postive polarity radius to satisfy demand for positive polarity
        BoolVar isInRadius = wublinProblem.NewBoolVar().WithName("is " + wublinPool[likedWublinIndex].getName() + " in positive polarity radius of " + wublinPool[targetI].getName());
        wublinProblem.AddGreaterOrEqual(lowerRightOfLikeRadiusX, upperLeftOfDesiredWublinX).OnlyEnforceIf(isInRadius); // If need be, can implement that negatiion with .AddLess().OnlyEnforceIf(~isInRadius))
        wublinProblem.AddGreaterOrEqual(lowerRightOfDesiredWublinX, upperLeftOfLikeRadiusX).OnlyEnforceIf(isInRadius);
        wublinProblem.AddGreaterOrEqual(upperLeftOfLikeRadiusY, lowerRightOfDesiredWublinY).OnlyEnforceIf(isInRadius);
        wublinProblem.AddGreaterOrEqual(upperLeftOfDesiredWublinY, lowerRightOfLikeRadiusY).OnlyEnforceIf(isInRadius);
        areWublinInPositivePolarityRadius.push_back(isInRadius);
    }
    wublinProblem.AddBoolOr(areWublinInPositivePolarityRadius);

    //  I could use this to generate a layout where all rarities are near each other

    /* Version to guarantee all wublins of same name are to be near each other (done via each wublins likes
        for (int likedWublinIndex : likedWublinIndexes)
    {
        // Because the placement decisions are on the bottom left, traversal goes right and up in this nested for loop
        LinearExpr upperLeftOfDesiredWublinX = wublinsXAxis[likedWublinIndex]; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr upperLeftOfDesiredWublinY = wublinsYAxis[likedWublinIndex] + wublinPool[likedWublinIndex].getSize() - 1;
        LinearExpr lowerRightOfDesiredWublinX = wublinsXAxis[likedWublinIndex] + wublinPool[likedWublinIndex].getSize() - 1; // desired wublin is the one that the ith wublin requires to be in proximity in order to achieve positive polarity
        LinearExpr lowerRightOfDesiredWublinY = wublinsYAxis[likedWublinIndex];

        // All conditions must be met for overlap to occur
        wublinProblem.AddGreaterOrEqual(lowerRightOfLikeRadiusX, upperLeftOfDesiredWublinX);
        wublinProblem.AddGreaterOrEqual(lowerRightOfDesiredWublinX, upperLeftOfLikeRadiusX);
        wublinProblem.AddGreaterOrEqual(upperLeftOfLikeRadiusY, lowerRightOfDesiredWublinY);
        wublinProblem.AddGreaterOrEqual(upperLeftOfDesiredWublinY, lowerRightOfLikeRadiusY);
    }  //TODO: NEED TO MAKE CONDITIONAL, ANY OF THESE LIKEDWUBLININDEXES CAN OVERLAP THE LIKE RADIUS TO SATISFY THIS DEMAND, currently it forces all to be true
    // BUT, I could use this to generate a layout where all rarities are next to each other
    */
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
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 29
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 28
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 27
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 26
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 25
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 24
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 23
! ! ! ! ! ! ! ! ! ! 0 0 0 d d d ! ! ! ! ! ! ! ! ! ! ! ! ! ! 22
! ! ! 0 0 p p p t t P P P d d d ! ! ! ! ! ! ! ! ! ! ! ! ! ! 21
! ! C C C p p p t t P P P d d d ! ! ! ! ! ! ! ! ! ! ! ! ! ! 20
! ! C C C p p p t t P P P g g g ! ! ! ! ! ! ! ! ! ! ! ! ! ! 19
! 0 C C C 0 P P t t 0 A A g g g ! ! ! ! ! ! ! ! ! ! ! ! ! ! 18
s s t t t t P P F F F A A g g g ! ! ! ! ! ! ! ! ! ! ! ! ! ! 17
s s t t t t B B F F F M M M B B B ! ! ! ! ! ! ! ! ! ! ! ! ! 16
W W S S S 0 B B F F F M M M B B B t t t t b b ! ! ! ! ! ! ! 15
W W S S S c c c d d d M M M B B B t t t t b b ! ! ! ! ! ! ! 14
B B S S S c c c d d d D D D t t Z Z t t f f f ! ! ! ! ! ! ! 13
B B D D D c c c d d d D D D t t Z Z t t f f f ! ! ! ! ! ! ! 12
w w D D D S S T T B B D D D 0 t t P P P f f f ! ! ! ! ! ! ! 11
w w D D D S S T T B B Z Z T T t t P P P t t 0 ! ! ! ! ! ! ! 10
! C C C S S W W M M M Z Z T T 0 0 P P P t t ! ! ! ! ! ! ! ! 9
! C C C S S W W M M M z z S S S p p T T B B ! ! ! ! ! ! ! ! 8
! C C C A A b b M M M z z S S S p p T T B B ! ! ! ! ! ! ! ! 7
! D D D A A b b G G G Z Z S S S F F F G G G ! ! ! ! ! ! ! ! 6
! D D D a a Z Z G G G Z Z b b b F F F G G G ! ! ! ! ! ! ! ! 5
! D D D a a Z Z G G G T T b b b F F F G G G ! ! ! ! ! ! ! ! 4
! ! ! 0 s s s B B B 0 T T b b b t t P P 0 0 ! ! ! ! ! ! ! ! 3
! ! ! ! s s s B B B m m m D D D t t P P 0 ! ! ! ! ! ! ! ! ! 2
! ! ! ! s s s B B B m m m D D D z z 0 ! ! ! ! ! ! ! ! ! ! ! 1
! ! ! ! ! ! ! ! ! ! m m m D D D z z ! ! ! ! ! ! ! ! ! ! ! ! 0
0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2
                    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
                      */

    // 19 unused spaces



/*
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 29
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 28
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 27
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 26
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 25
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 24
! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! ! 23
! ! ! ! ! ! ! ! ! ! 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 22
! ! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 21
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 20
! ! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 19
! 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 18
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! ! 17
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! ! ! ! ! ! ! 16
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 15
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 14
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 13
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 12
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 11
0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 ! ! ! ! ! ! ! 10
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
0 1 2 3 4 5 6 7 8 9 1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2
                    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9
*/
  
    // Negative polarity is just another set of AddNoOverlap2D but done in sets of 2 instead
    // Positive polarity we have to figure out how to do the inverse of NoOverlap2d to say RequireOverlap2D
std::cout << "Var count: " << wublinProblem.Proto().variables_size() << std::endl << "constraint count: " << wublinProblem.Proto().constraints_size() << std::endl;

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

    int likeRadius{ 3 }; //lvl 1 radius
    int hateRadius{ 2 };

    //Map is traversed via and X and Y axis systen. (shown in a image I made: https://imgur.com/a/SoHMJGK (made with help from an outline on the msm wiki))

   // wublinMap is used to assist in converting the array version of the map into a map data structure, so that the problem can be solved in terms of coordinate pairs
    std::vector<std::vector<char>> wublinMapArr= { // version excluding the upper platform (as its been proven that wubbox takes up most of the space of the upper platforms
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'!', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
    {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '!', '!', '!', '!', '!', '!', '!'},
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


/*
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
*/

    std::vector<Wublin> wublinPool = { //All you have to do to add a Wublin to the algorithm is to add the Wublin and their characteristics here, if you want multiples of a wublin, change the count
//Common Wublins
//              Name:           size:  Likes:             Hates:         count: (NOTE THAT COUNT IS "vestigial" and that the actual way to add duplicates is to just copy + paste the wublin in this vector
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
        {Wublin("Thwok",            2,"Dermit",          "Zuuker",           1 )},
        {Wublin("Dwumrohl",         3,"Astropod",        "Fleechwurm",       1 )},
        {Wublin("Scargo",           3,"Blipsqueak",      "Maulch",           1 )},
        {Wublin("Fleechwurm",       3,"Pixolotl",        "Dwumrohl",         1 )},
        {Wublin("Maulch",           3,"Thwok",           "Poewk",            1 )},
        {Wublin("Dermit",           3,"Zynth",           "Scargo",           1 )},
        {Wublin("Gheegur",          3,"Tympa",           "Dermit",           1 )},
        //{Wublin("Monculus",         2,"None",            "None",             1 )}, //This is an edge case
        //{Wublin("Wubbox",           4,"None",            "None",             1 )},
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
        {Wublin("Rare Thwok",       2,"Rare Dermit",     "Rare Zuuker",      1 )},
        {Wublin("Rare Dwumrohl",    3,"Rare Astropod",   "Rare Fleechwurm",  1 )},
        {Wublin("Rare Scargo",      3,"Rare Blipsqueak", "Rare Maulch",      1 )},
        {Wublin("Rare Fleechwurm",  3,"Rare Pixolotl",   "Rare Dwumrohl",    1 )},
        {Wublin("Rare Maulch",      3,"Rare Thwok",      "Rare Poewk",       1 )},
        {Wublin("Rare Dermit",      3,"Rare Zynth",      "Rare Scargo",      1 )},
        {Wublin("Rare Gheegur",     3,"Rare Tympa",      "Rare Dermit",      1 )},
        //{Wublin("Rare Monculus",    2,"None",            "None",             1 )}, //This is an edge case
        //{Wublin("Rare Wubbox",      4,"None",            "None",             1 )},
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
        {Wublin("Epic Thwok",       2,"Epic Dermit",     "Epic Zuuker",      1 )},
        {Wublin("Epic Dwumrohl",    3,"Epic Astropod",   "Epic Fleechwurm",  1 )},
        {Wublin("Epic Scargo",      3,"Epic Blipsqueak", "Epic Maulch",      1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Fleechwurm",  3,"Epic Pixolotl",   "Epic Dwumrohl",    1 )},
        {Wublin("Epic Maulch",      3,"Epic Thwok",      "Epic Poewk",       1 )}, // Unreleased as of 7/25/26
        {Wublin("Epic Dermit",      3,"Epic Zynth",      "Epic Scargo",      1 )},
        {Wublin("Epic Gheegur",     3,"Epic Tympa",      "Epic Dermit",      1 )},
        //{Wublin("Epic Wubbox",      4,"None",            "None",             1 )},
        //{Wublin("Epic Monculus",    2,"None",            "None",             1 )}, //This is an edge case (along with wubbox)
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

// SOLUTIONS REPRESENT THE BOTTOM LEFT PLACEMENT
/*
Var count: 265
constraint count: 994
Started solving!
Wublin solution:
Finished in 637.506 seconds
Brump = (8,3)
Zynth = (13,13)
Zuuker = (19,2)
Blipsqueak = (7,17)
Bona-Petite = (0,13)
Poewk = (10,4)
Screemu = (8,20)
Tympa = (3,14)
Creepuscule = (10,20)
Whajje = (13,2)
Astropod = (3,16)
Pixolotl = (18,10)
Thwok = (17,8)
Dwumrohl = (10,1)
Scargo = (0,10)
Fleechwurm = (20,13)
Maulch = (19,4)
Dermit = (12,15)
Gheegur = (5,1)
Tympa = (13,0)
Tympa = (11,11)
Tympa = (9,9)
Tympa = (13,9)
Tympa = (3,12)
Tympa = (8,1)
Rare Brump = (11,13)
Rare Zynth = (18,14)
Rare Zuuker = (17,3)
Rare Blipsqueak = (1,6)
Rare Bona-Petite = (16,5)
Rare Poewk = (6,8)
Rare Screemu = (1,8)
Rare Tympa = (11,9)
Rare Creepuscule = (13,20)
Rare Whajje = (14,18)
Rare Astropod = (15,3)
Rare Pixolotl = (1,4)
Rare Thwok = (18,12)
Rare Dwumrohl = (15,0)
Rare Scargo = (3,4)
Rare Fleechwurm = (3,7)
Rare Maulch = (19,7)
Rare Dermit = (20,10)
Rare Gheegur = (15,10)
Epic Brump = (12,18)
Epic Zynth = (11,7)
Epic Zuuker = (1,16)
Epic Blipsqueak = (13,4)
Epic Bona-Petite = (2,18)
Epic Poewk = (7,14)
Epic Screemu = (9,7)
Epic Tympa = (3,10)
Epic Creepuscule = (5,11)
Epic Whajje = (5,15)
Epic Astropod = (5,17)
Epic Pixolotl = (13,11)
Epic Thwok = (10,15)
Epic Dwumrohl = (9,17)
Epic Scargo = (13,6)
Epic Fleechwurm = (15,13)
Epic Maulch = (5,19)
Epic Dermit = (8,11)
Epic Gheegur = (6,5)
DONE!


// Bring a plan
// Kanban board
// USe case diagram
//List requirements
// Nice to haves
// Gant charts (GITHUB feature)

*/


/* 
Var count: 271
constraint count: 1017
Started solving!
Wublin solution:
Finished in 1500.86 seconds
Brump = (4,4)
Zynth = (12,18)
Zuuker = (8,18)
Blipsqueak = (16,6)
Bona-Petite = (0,12)
Poewk = (7,1)
Screemu = (12,11)
Tympa = (6,18)
Creepuscule = (1,7)
Whajje = (3,10)
Astropod = (3,12)
Pixolotl = (20,6)
Thwok = (17,14)
Dwumrohl = (5,9)
Scargo = (19,3)
Fleechwurm = (3,19)
Maulch = (19,13)
Dermit = (20,10)
Gheegur = (13,20)
Tympa = (10,6)
Tympa = (8,20)
Tympa = (8,6)
Tympa = (12,6)
Tympa = (12,4)
Tympa = (10,18)
Tympa = (1,10)
Rare Brump = (8,11)
Rare Zynth = (18,6)
Rare Zuuker = (3,14)
Rare Blipsqueak = (13,0)
Rare Bona-Petite = (0,15)
Rare Poewk = (9,15)
Rare Screemu = (13,2)
Rare Tympa = (8,4)
Rare Creepuscule = (16,3)
Rare Whajje = (14,6)
Rare Astropod = (14,4)
Rare Pixolotl = (6,4)
Rare Thwok = (8,13)
Rare Dwumrohl = (15,0)
Rare Scargo = (10,1)
Rare Fleechwurm = (1,4)
Rare Maulch = (3,16)
Rare Dermit = (5,12)
Rare Gheegur = (4,1)
Epic Brump = (10,11)
Epic Zynth = (10,13)
Epic Zuuker = (20,8)
Epic Blipsqueak = (14,18)
Epic Bona-Petite = (17,8)
Epic Poewk = (5,6)
Epic Screemu = (15,15)
Epic Tympa = (10,4)
Epic Creepuscule = (12,15)
Epic Whajje = (14,11)
Epic Astropod = (14,13)
Epic Pixolotl = (6,20)
Epic Thwok = (12,13)
Epic Dwumrohl = (11,8)
Epic Scargo = (10,20)
Epic Fleechwurm = (6,15)
Epic Maulch = (16,11)
Epic Dermit = (8,8)
Epic Gheegur = (14,8)
Wublin solution:
Finished in 1500.92 seconds
Brump = (4,4)
Zynth = (12,18)
Zuuker = (8,18)
Blipsqueak = (16,6)
Bona-Petite = (0,12)
Poewk = (7,1)
Screemu = (12,11)
Tympa = (6,18)
Creepuscule = (1,7)
Whajje = (3,10)
Astropod = (3,12)
Pixolotl = (20,6)
Thwok = (17,14)
Dwumrohl = (5,9)
Scargo = (19,3)
Fleechwurm = (3,19)
Maulch = (19,13)
Dermit = (20,10)
Gheegur = (13,20)
Tympa = (10,6)
Tympa = (8,20)
Tympa = (8,6)
Tympa = (12,6)
Tympa = (12,4)
Tympa = (10,18)
Tympa = (1,10)
Rare Brump = (8,11)
Rare Zynth = (18,6)
Rare Zuuker = (3,14)
Rare Blipsqueak = (13,0)
Rare Bona-Petite = (0,15)
Rare Poewk = (9,15)
Rare Screemu = (13,2)
Rare Tympa = (8,4)
Rare Creepuscule = (16,3)
Rare Whajje = (14,6)
Rare Astropod = (14,4)
Rare Pixolotl = (6,4)
Rare Thwok = (8,13)
Rare Dwumrohl = (15,0)
Rare Scargo = (10,0)
Rare Fleechwurm = (1,4)
Rare Maulch = (3,16)
Rare Dermit = (5,12)
Rare Gheegur = (4,1)
Epic Brump = (10,11)
Epic Zynth = (10,13)
Epic Zuuker = (20,8)
Epic Blipsqueak = (14,18)
Epic Bona-Petite = (17,8)
Epic Poewk = (5,6)
Epic Screemu = (15,15)
Epic Tympa = (10,4)
Epic Creepuscule = (12,15)
Epic Whajje = (14,11)
Epic Astropod = (14,13)
Epic Pixolotl = (6,20)
Epic Thwok = (12,13)
Epic Dwumrohl = (11,8)
Epic Scargo = (10,20)
Epic Fleechwurm = (6,15)
Epic Maulch = (16,11)
Epic Dermit = (8,8)
Epic Gheegur = (14,8)


*/


/* BEST OF THE BEST (9 Tympas!)
Finished in 8802.62 seconds
Brump = (14,14)
Zynth = (6,4)
Zuuker = (18,7)
Blipsqueak = (12,6)
Bona-Petite = (20,13)
Poewk = (10,18)
Screemu = (10,4)
Tympa = (7,6)
Creepuscule = (9,6)
Whajje = (2,19)
Astropod = (19,2)
Pixolotl = (20,8)
Thwok = (18,14)
Dwumrohl = (14,0)
Scargo = (15,3)
Fleechwurm = (17,11)
Maulch = (12,8)
Dermit = (13,18)
Gheegur = (9,1)
Tympa = (10,21)
Tympa = (1,4)
Tympa = (1,6)
Tympa = (12,21)
Tympa = (14,21)
Tympa = (17,1)
Tympa = (3,11)
Tympa = (12,1)
Rare Brump = (4,1)
Rare Zynth = (12,13)
Rare Zuuker = (14,16)
Rare Blipsqueak = (6,15)
Rare Bona-Petite = (9,12)
Rare Poewk = (3,8)
Rare Screemu = (4,17)
Rare Tympa = (1,8)
Rare Creepuscule = (7,19)
Rare Whajje = (4,15)
Rare Astropod = (6,13)
Rare Pixolotl = (3,6)
Rare Thwok = (6,17)
Rare Dwumrohl = (1,16)
Rare Scargo = (1,13)
Rare Fleechwurm = (6,1)
Rare Maulch = (4,19)
Rare Dermit = (8,15)
Rare Gheegur = (15,8)
Epic Brump = (8,4)
Epic Zynth = (16,14)
Epic Zuuker = (18,9)
Epic Blipsqueak = (7,11)
Epic Bona-Petite = (18,4)
Epic Poewk = (12,3)
Epic Screemu = (4,13)
Epic Tympa = (16,6)
Epic Creepuscule = (0,10)
Epic Whajje = (5,11)
Epic Astropod = (14,6)
Epic Pixolotl = (5,6)
Epic Thwok = (12,11)
Epic Dwumrohl = (9,9)
Epic Scargo = (6,8)
Epic Fleechwurm = (3,3)
Epic Maulch = (14,11)
Epic Dermit = (11,15)
Epic Gheegur = (20,10)
DONE!

*/




/* NEW BEST, 10 common TYMPAS (12 total)
Var count: 283
constraint count: 1063
Started solving!
Wublin solution:
Finished in 15981.9 seconds
Brump = (21,14) x
Zynth = (16,0) x
Zuuker = (11,7) x
Blipsqueak = (6,6) x
Bona-Petite = (13,3) x
Poewk = (5,19) x
Screemu = (0,16) x
Tympa = (17,14) x
Creepuscule = (5,12) x
Whajje = (0,10) x
Astropod = (4,4) x
Pixolotl = (16,7) x
Thwok = (16,2) x
Dwumrohl = (13,20) x
Scargo = (4,1) x
Fleechwurm = (20,11) x
Maulch = (10,0) x
Dermit = (8,12) x
Gheegur = (13,17) x
Tympa = (2,16) x
Tympa = (18,12) x
Tympa = (19,14) x
Tympa = (20,9) x
Tympa = (8,20) x
Tympa = (4,16) x
Tympa = (14,12) x 
Tympa = (15,10) x
Tympa = (8,18) x
Rare Brump = (6,15) X
Rare Zynth = (11,5) X
Rare Zuuker = (16,12) X
Rare Blipsqueak = (0,12) X
Rare Bona-Petite = (14,14) X
Rare Poewk = (10,19) X
Rare Screemu = (4,8) X
Rare Tympa = (18,7) X
Rare Creepuscule = (2,18) X
Rare Whajje = (0,14) X
Rare Astropod = (11,17) X
Rare Pixolotl = (6,17) X
Rare Thwok = (11,3) X
Rare Dwumrohl = (1,4) X
Rare Scargo = (2,13) X
Rare Fleechwurm = (8,15) X
Rare Maulch = (11,14) X
Rare Dermit = (13,0) X
Rare Gheegur = (19,4) X
Epic Brump = (20,7) X
Epic Zynth = (11,9) X
Epic Zuuker = (6,4) X
Epic Blipsqueak = (9,10) X
Epic Bona-Petite = (7,1) X
Epic Poewk = (17,9) X
Epic Screemu = (5,10) X
Epic Tympa = (13,9) X
Epic Creepuscule = (1,7) X
Epic Whajje = (6,8) X
Epic Astropod = (4,6) X
Epic Pixolotl = (18,2) X
Epic Thwok = (7,10) X
Epic Dwumrohl = (2,10) X
Epic Scargo = (13,6) X
Epic Fleechwurm = (16,4) X
Epic Maulch = (8,7) X
Epic Dermit = (11,11) X
Epic Gheegur = (8,4) X
DONE!



// 11 TYMPAS:
*/