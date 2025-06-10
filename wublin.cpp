#include "wublin.h"

Wublin::Wublin(int s, std::string n, std::string l, std::string h) : size{s}, name{n}, likes{l}, hates{h}{}
int Wublin::getSize() const {return size;} 
void Wublin::setSize(int s){size = s;}
std::string Wublin::getName() const {return name;}
void Wublin::setName(std::string n){name = n;}
std::string Wublin::getLikes() const {return likes;}
void Wublin::setLikes(std::string l){likes = l;}
std::string Wublin::getHates() const {return hates;}
void Wublin::setHates(std::string h){hates = h;}

std::ostream& operator<<(std::ostream& os, const Wublin& passedWublin) //This is the smarter practice I've heard about using the output stream operator overload
{
    os << "Name: " << passedWublin.getName() << "\nSize: " << passedWublin.getSize() << "\nLikes: " << passedWublin.getLikes() << "\nHates: " << passedWublin.getHates() << '\n';
    return os;
}

