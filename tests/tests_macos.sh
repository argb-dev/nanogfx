let res=0
make -f makefile.osx metal
./test.elf
let res=res+$?
exit ${res}

