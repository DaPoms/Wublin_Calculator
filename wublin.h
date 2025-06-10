#ifndef WUBLIN_H
#define WUBLIN_H
#include<string>
#include<iostream>
class Wublin
{
    private:
        int size{};
        std::string name{};
        std::string likes{};
        std::string hates{};
    public:
        Wublin(int s = -1, std::string n = "NULL", std::string l = "NONE", std::string h = "NONE");
        int getSize() const;
        void setSize(int s);
        std::string getName() const;
        void setName(std::string n);
        std::string getLikes() const;
        void setLikes(std::string l);
        std::string getHates() const;
        void setHates(std::string h);

};
std::ostream& operator<<(std::ostream& os, const Wublin& passedWublin);        

#endif
