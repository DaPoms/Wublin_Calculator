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

/*bool isUnclaimed(Coordinate upperLeft, Wublin )
{

}*/

//Returns indexes for all wublins of the given size
std::vector<int> calcWublinsWithGivenSize(int size, const std::vector<Wublin>& wublinPool)
{
    std::vector<int> wublins;
    for (int i{0}; i < wublinPool.size(); i++)
        if (wublinPool[i].getSize() == size)
            wublins.push_back(i);
    return wublins;
}

/*bool contains(IloNumVar target, std::vector<int>& vect)
{
    for (int num : vect)
        if (target == num)
            return true;
    return false;    
}*/

void findLayouts(int likeRadius, int hateRadius, const std::map<Coordinate, std::string>& wublinMapCoordPairs, const std::vector<Wublin>& wublinPool) //Uses IBM's CPLEX to find all the maximum polarity placements using like radius and hate radius
{
    std::vector<int> size2Wublins = calcWublinsWithGivenSize(2, wublinPool);
    std::vector<int> size3Wublins = calcWublinsWithGivenSize(3, wublinPool);
    std::vector<int> size4Wublins = calcWublinsWithGivenSize(4, wublinPool);


    //const Domain booleanType(0, 1); // CP-SAT requires all int types, so this is used to represent the int type in an integer domain
    const Domain wublinIndexes(0, wublinPool.size() - 1); // Based off of wublinPool indexes, references the associated wublin
    CpModelBuilder wublinProblem;

    //Decision variable setup
    std::map<Coordinate, IntVar> mapPlacementDecisions; //variable containing placement decisions for all (populatable) squares of the wublin map
    std::vector<IntVar> wublinDecisions;
    for (const auto& [coordinate, state] : wublinMapCoordPairs)
    {
        mapPlacementDecisions[coordinate] = wublinProblem.NewIntVar(wublinIndexes).WithName("(" + std::to_string(coordinate.first) + "," + std::to_string(coordinate.second) + ")"); // Wublins are referenced by their index in the wublinPool vector. EX: At index 0 is Brump, so the state/decision variable of 1 represents brump is taking up that square
        wublinDecisions.push_back(mapPlacementDecisions[coordinate]);
    }
    wublinProblem.AddAllDifferent(wublinDecisions); // Defines there must only be 1 of each decision variable (wublin)
    // If you want to run with > 1 of each wublin, then here are the constraints for that below (comment out the above constraint if doing this)
    /*  
    for (int i{0}; i < wublinPool.size(); i++) //checks how much of each wublin there is in the answer. Each wublin must have their count amount of references in the solution
    {
        int requiredCount = wublinPool[i].getCount();
        if (!requiredCount) // if == 0
            continue;

        std::vector<BoolVar> isThisWublin;

        for (int d{ 0 }; d < wublinDecisions.size(); d++)
        {
            BoolVar isEqual = wublinProblem.NewBoolVar();
            wublinProblem.AddEquality(wublinDecisions[d], i).OnlyEnforceIf(isEqual); // here we effectively define how the bools will be used to count each wublin
            wublinProblem.AddNotEqual(wublinDecisions[d], i).OnlyEnforceIf(isEqual.Not()); // defines behavior if not equal
            isThisWublin.push_back(isEqual);
        }
        
        wublinProblem.AddEquality(LinearExpr::Sum(isThisWublin), requiredCount); // Note that Sum() is from google OR tools library (and is required to define sum withih a constraint)
        
    }
    */
    //





    // Defines how placements work (placement is done via upper left corner, so we need to ensure we can place all square up to bottom right corner too)

/*    IloNumVar overlapConstraint; // Constraint that no wublins are allowed to overlap, so number must be kept = 0.
    for (const auto& [coordinate, decision] : mapDecisions)
    {
    
        
    }*/

/*    for (int i{ 0 }; i < wublinPool.size(); i++) // defines size 
    {

    }*/

    //MAKE SURE TO ACCOUNT FOR COORDINATES THAT DOING X - 1 

    // Need to define size constraints for each decision variable (if = 1, then size = 2x2

}

/*
//Ideas of all constraints:
    0-32 decision value (each decision NUMBER represents a different wublin (Each correlates to wublinPool[i])
    Decision value designates the top left block of ownership
        When placing, you must check that all occupied spaces do not overlap (for the entire space the wublin takes up
            This is done via making sure the to be placed box (which means its entire size, for example, 2x2) is NOT <= the highest ordered pair
            that can be made between the upper left and lower right (EX: (0,2) and (2,0) makes (2,2)) AND NOT >= the lowest coordinate that can be made from the min x and y values of the upper left and lower right (in this example, 0,0). If both are true, then it means the coordinate pair is within an already taken space.
    Each decision value must be in a size x size pattern (in terms of claiming coordinate pairs)
    Each decision vlaue must not overlap/overwrite with any other decision value
    Every decision value must have their positive polarity counterpart within radius (counnt = wubinPool.size())
    Every decision value must have their negative polarity counterpart NOT within radius (count = 0)
 */


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

    int likeRadius{ 0 };
    int hateRadius{ 0 };

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

    std::map<Coordinate,std::string> wublinMap = wublinArrToCoordinatePairMap(wublinMapArr);
  
    // findLayouts();
}
