#include "wublin.h"

Wublin::Wublin(std::string n, int s, std::string l, std::string h, int c, char id) : name{n}, size{s}, likes{l}, hates{h}, count{c}, uniqueID{id}{}
int Wublin::getCount() const {return count;}
void Wublin::setCount(int c) {count = c;}
int Wublin::getSize() const {return size;} 
void Wublin::setSize(int s){size = s;} // size is in terms of radius, so 1 means 1x1 square, 2 means a 2x2, 3 means a 3x3
std::string Wublin::getName() const {return name;}
void Wublin::setName(std::string n){name = n;}
std::string Wublin::getLikes() const {return likes;}
void Wublin::setLikes(std::string l){likes = l;}
std::string Wublin::getHates() const {return hates;}
void Wublin::setHates(std::string h){hates = h;}
char Wublin::getID(){return uniqueID;}
void Wublin::setID(char id){uniqueID = id;}

std::ostream& operator<<(std::ostream& os, const Wublin& passedWublin) //This is the smarter practice I've heard about using the output stream operator overload
{
    os << "Name:  " << passedWublin.getName() << "\nSize:  " << passedWublin.getSize() << "\nLikes: " << passedWublin.getLikes() << "\nHates: " << passedWublin.getHates() << "\nCount: " << passedWublin.getCount() << "\nID: " << passedWublin.getCount() << '\n';
    return os;
}

