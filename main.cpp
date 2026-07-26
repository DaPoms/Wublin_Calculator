
#include <vector>
#include "wublin.h"
#include <ilcplex/ilocplex.h>                    
#include <iostream>


int likeRadius{0};
int hateRadius{0};
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
    Where A means the affect radius and 0 designates the actual wublin.
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

void findLayout(int likeRadius, int hateRadius) //Uses IBM's CPLEX to find all the maximum polarity placements using like radius and hate radius
{}

//NOTE TO SELF: Wubbox does not have a polarity system, but we want to make sure to have some method to include them in the map (such as saving a spot for them)
int main()
{
    //Spaces on the map are represented by chars
    //''0'' is empty space, //30 (30 items in a column) rows __ columns
    //Columns are traversed via Y, Rows are via X (shown in a image I made https://imgur.com/a/HIay8dw (made with help from an outline on the msm wiki))

   
    char wublinMap[30][30] = {
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
//              Name:        size: Likes:        Hates:    count: ID:
        {Wublin("Brump",       2, "Fleechwurm", "Blipsqueak",  1, '1')},
        {Wublin("Zynth",       2, "Gheegur",    "Astropod",    1, '2')},
        {Wublin("Zuuker",      2, "Maulch",     "Screemu",     1, '3')},
        {Wublin("Blipsqueak",  2, "Screemu",    "Tympa",       1, '4')},
        {Wublin("Bona-Petite", 3, "Zuuker",     "Creepuscule", 1, '5')},
        {Wublin("Poewk",       3, "Brump",      "Bona-Petite", 1, '6')},
        {Wublin("Screemu",     2, "Creepuscule","Pixolotl",    1, '7')},
        {Wublin("Tympa",       2, "Poewk",      "Thwok",       1, '8')},
        {Wublin("Creepuscule", 3, "Whajje",     "Gheegur",     1, '9')},
        {Wublin("Whajje",      2, "Dwumrohl",   "Zynth",       1, 'a')},
        {Wublin("Astropod",    2, "Bona-Petite","Brump",       1, 'b')},
        {Wublin("Pixolotl",    2, "Scargo",     "Whajje",      1, 'c')},
        {Wublin("Monculus",    2, "None",       "None",        1, 'd')}, //This is an edge case
        {Wublin("Thwok",       2, "Dermit",     "Zuuker",      1, 'e')},
        {Wublin("Dwumrohl",    3, "Astropod",   "Fleechwurm",  1, 'f')},
        {Wublin("Scargo",      3, "Blipsqueak", "Maulch",      1, 'g')},
        {Wublin("Fleechwurm",  3, "Pixolotl",   "Dwumrohl",    1, 'h')},
        {Wublin("Maulch",      3, "Thwok",      "Poewk",       1, 'i')},
        {Wublin("Dermit",      3, "Zynth",      "Scargo",      1, 'j')},
        {Wublin("Gheegur",     3, "Tympa",      "Dermit",      1, 'k' )},
        {Wublin("Wubbox",      4, "None",       "None",        1, 'l' )},
//Rare Wublins
//              Name:            size: Likes:            Hates:          count: ID:
        {Wublin("Rare Brump",       2,"Rare Fleechwurm", "Rare Blipsqueak",  1, 'm')},
        {Wublin("Rare Zynth",       2,"Rare Gheegur",    "Rare Astropod",    1, 'n')},
        {Wublin("Rare Zuuker",      2,"Rare Maulch",     "Rare Screemu",     1, 'o')},
        {Wublin("Rare Blipsqueak",  2,"Rare Screemu",    "Rare Tympa",       1, 'p')},
        {Wublin("Rare Bona-Petite", 3,"Rare Zuuker",     "Rare Creepuscule", 1, 'q')},
        {Wublin("Rare Poewk",       3,"Rare Brump",      "Rare Bona-Petite", 1, 'r')},
        {Wublin("Rare Screemu",     2,"Rare Creepuscule","Rare Pixolotl",    1, 's')},
        {Wublin("Rare Tympa",       2,"Rare Poewk",      "Rare Thwok",       1, 't')},
        {Wublin("Rare Creepuscule", 3,"Rare Whajje",     "Rare Gheegur",     1, 'u')},
        {Wublin("Rare Whajje",      2,"Rare Dwumrohl",   "Rare Zynth",       1, 'v')},
        {Wublin("Rare Astropod",    2,"Rare Bona-Petite","Rare Brump",       1, 'w')},
        {Wublin("Rare Pixolotl",    2,"Rare Scargo",     "Rare Whajje",      1, 'x')},
        {Wublin("Rare Monculus",    2,"None",            "None",             1, 'y')}, //This is an edge case
        {Wublin("Rare Thwok",       2,"Rare Dermit",     "Rare Zuuker",      1, 'z')},
        {Wublin("Rare Dwumrohl",    3,"Rare Astropod",   "Rare Fleechwurm",  1, 'A')},
        {Wublin("Rare Scargo",      3,"Rare Blipsqueak", "Rare Maulch",      1, 'B')},
        {Wublin("Rare Fleechwurm",  3,"Rare Pixolotl",   "Rare Dwumrohl",    1, 'C')},
        {Wublin("Rare Maulch",      3,"Rare Thwok",      "Rare Poewk",       1, 'D')},
        {Wublin("Rare Dermit",      3,"Rare Zynth",      "Rare Scargo",      1, 'E')},
        {Wublin("Rare Gheegur",     3,"Rare Tympa",      "Rare Dermit",      1, 'F')},
        {Wublin("Rare Wubbox",      4,"None",            "None",             1, 'l' )},
//Epic Wublins (Not all of these are officially released yet).
//              Name:            size: Likes:            Hates:          count: ID:
        {Wublin("Epic Brump",       2,"Epic Fleechwurm", "Epic Blipsqueak",  1, 'G')},
        {Wublin("Epic Zynth",       2,"Epic Gheegur",    "Epic Astropod",    1, 'H')},
        {Wublin("Epic Zuuker",      2,"Epic Maulch",     "Epic Screemu",     1, 'I')},
        {Wublin("Epic Blipsqueak",  2,"Epic Screemu",    "Epic Tympa",       1, 'J')},
        {Wublin("Epic Bona-Petite", 3,"Epic Zuuker",     "Epic Creepuscule", 1, 'K')},
        {Wublin("Epic Poewk",       3,"Epic Brump",      "Epic Bona-Petite", 1, 'L')},
        {Wublin("Epic Screemu",     2,"Epic Creepuscule","Epic Pixolotl",    1, 'M')}, // Unreleased as of 7/25/26
        {Wublin("Epic Tympa",       2,"Epic Poewk",      "Epic Thwok",       1, 'N')},
        {Wublin("Epic Creepuscule", 3,"Epic Whajje",     "Epic Gheegur",     1, 'O')}, // Unreleased as of 7/25/26
        {Wublin("Epic Whajje",      2,"Epic Dwumrohl",   "Epic Zynth",       1, 'P')}, // Unreleased as of 7/25/26
        {Wublin("Epic Astropod",    2,"Epic Bona-Petite","Epic Brump",       1, 'Q')}, // Unreleased as of 7/25/26
        {Wublin("Epic Pixolotl",    2,"Epic Scargo",     "Epic Whajje",      1, 'R')}, // Unreleased as of 7/25/26
        {Wublin("Epic Monculus",    2,"None",            "None",             1, 'S')}, //This is an edge case (along with wubbox)
        {Wublin("Epic Thwok",       2,"Epic Dermit",     "Epic Zuuker",      1, 'T')},
        {Wublin("Epic Dwumrohl",    3,"Epic Astropod",   "Epic Fleechwurm",  1, 'U')},
        {Wublin("Epic Scargo",      3,"Epic Blipsqueak", "Epic Maulch",      1, 'V')}, // Unreleased as of 7/25/26
        {Wublin("Epic Fleechwurm",  3,"Epic Pixolotl",   "Epic Dwumrohl",    1, 'W')},
        {Wublin("Epic Maulch",      3,"Epic Thwok",      "Epic Poewk",       1, 'X')}, // Unreleased as of 7/25/26
        {Wublin("Epic Dermit",      3,"Epic Zynth",      "Epic Scargo",      1, 'Y')},
        {Wublin("Epic Gheegur",     3,"Epic Tympa",      "Epic Dermit",      1, 'Z')},
        // possible epic wubbox in the future?
    };
    
    for(int r{0}; r < 30; r++)
    {
        for(int c{0}; c < 30; c++)
        {
            std::cout << wublinMap[r][c] << " ";
        }
        std::cout << '\n';
    }

    // findLayout();
}
