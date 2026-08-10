## Intro:
This project is an algorithm that finds the best case placements for Wublins on Wublin island in the game "My Singing Monsters".
If you don't know how Wublins work, here's a quick rundown: Wublins are creatures that have a like/dislike system called "polarity" based on who is nearby them.
Wublins are more productive with someone they like and less productive near someone they hate. This algorithm searches for the best layouts on the Wublin island map
to maximize 'like' relationships and minimize dislikes. The current plan is to store all of these layouts on a website and let the user filter through all these layouts 
to find the design they like most to use in their own game. This has many applications if modified, so feel free to make your own forks.

## Requirement:
This program requires Google's OR tools API for c++ and was made in Visual Studio 2022. The solution files are specifically set up for OR tools version 9.11.421. I utilized the binary installation as it's generally an easier setup process:
1. Download binary (version 9.11 or less, as ortools_full.lib is defunct in modern versions)
2. Open in file explorer 
3. In vs project settings do:
### Project > Properties > C/C++ > Additional Include Directories 
Enter C:\or-tools_x64_VisualStudio2022_cpp_v9.11.4210\include 
### Linker > General > Additional library directories 
Enter C:\or-tools_x64_VisualStudio2022_cpp_v9.11.4210\lib 
### Linker > Input > Additional dependencies 
Enter C:\or-tools_x64_VisualStudio2022_cpp_v9.11.4210\lib\ortools_full.lib and also C:\or-tools_x64_VisualStudio2022_cpp_v9.11.4210\lib\utf8_validity.lib

The above steps will get you set up on running the release version of this project.

## Request:
If you end up using this project yourself (like in a fork or as inspiration), please give credit.
