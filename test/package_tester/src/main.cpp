#include "CCSDSPack.h"
#include <iostream>

int main() {
  CCSDS::Manager mgr; // Minimal smoke test: type is visible and linkable
  std::cout << "CCSDSPack hello: Manager constructed successfully.\n";
  return 0;
}
