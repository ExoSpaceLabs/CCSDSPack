#include "CCSDSPack.h"


// constructor
CCSDSPack::CCSDSPack() {}

// methods
std::string CCSDSPack::getMessage() {
   
    setHello("Hello, from CCSDSPack class!");
    return getHello();
}

