#ifndef WUBLIN_H
#define WUBLIN_H
#include <string>
#include <iostream>
class Wublin
{
    private:
        int count{};
        int size{};
        std::string name{};
        std::string likes{};
        std::string hates{};
        char uniqueID{};
    public:
        Wublin(std::string n = "NULL", int s = -1, std::string l = "NONE", std::string h = "NONE", int c = 0, char id = '!');
        
        int getSize() const;
        void setSize(int s);
        std::string getName() const;
        void setName(std::string n);
        std::string getLikes() const;
        void setLikes(std::string l);
        std::string getHates() const;
        void setHates(std::string h);
        int getCount() const;
        void setCount(int c);
        char getID();
        void setID(char id);
        

};
std::ostream& operator<<(std::ostream& os, const Wublin& passedWublin);        

#endif
