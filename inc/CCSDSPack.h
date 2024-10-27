#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include <string>

class CCSDSPack {
public:
    CCSDSPack();
    std::string getMessage();
    
    std::string getHello() {return m_hello;}
    void setHello(std::string value){m_hello = value;}
    
private:
    std::string m_hello;
};

#endif // CCSDSPACK_H

