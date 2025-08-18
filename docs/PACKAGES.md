# Packages

[../](README.md) - CCSDSPack Documentation

## Linux 
### Deb:
A sample bash script has been prepared to generate .deb package using cpack.

The script can be launched using the following command, note for generation
super/user privileges might be required. In that case use `sudo` command 
before running `bash`.

```bash

bash package.sh
```
If successful, the generated package will be placed under the `packages` directory.

the generated package can be installed using the following command:
```bash

dpkg -i ccsdspack-v{version}-{system}-{architecture}.deb
```

and uninstalled using `dpkg`
```bash

dpkg --remove ccsdspack
```
Once installed, cmake `find_package()` can be used to find the library and link it as follows:
```cmake

find_package(CCSDSPack CONFIG REQUIRED)
target_link_libraries(my_app PRIVATE ccsdspack::CCSDSPack)
```




## Windows
