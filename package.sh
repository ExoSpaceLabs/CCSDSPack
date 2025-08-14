cd build
cmake -S . -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -- -j

# cpack might require super user privileges.
cpack -G DEB -C Release -V

# check for consistency:
#dpkg-deb --info ccsdspack-v1.0.0-Linux-x86_64.deb
#dpkg-deb --contents ccsdspack-v1.0.0-Linux-x86_64.deb

mv ccsdspack-v*.deb ../packages/.
# to install on the system (requires sudo)
# dpkg -i ccsdspack-v1.0.0-Linux-x86_64.deb

# to remove:
# dpkg --remove ccsdspack