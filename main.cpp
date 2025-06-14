#include <iostream>
#include "wublin.h"


//Map key:
/* 
33 = ! (void space)

*/
int main()
{
    //Spaces on the map are represented by chars
    //''0'' is empty space, //30 (30 items in a column) rows __ columns
    //Column is X, Rows is Y

    char wublinMap[30][30] = {0};
  /*   char wublinMap[30][]= {
  
        



    }; */
    //Wublin test;
    
    // std::cout << '!';'

    for(int r{0}; r < 30; r++)
    {
        for(int c{0}; c < 2; c++)
        {
            std::cout << wublinMap[r][c] << " ";
        }
        std::cout << '\n';
    }

    //std::cout << test;
}
