



clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/grow-file-size -a grow-file-size -p tests/filesys/extended/tar -a tar -- -q  -f run grow-file-size


clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/grow-root-lg -a grow-root-lg -p tests/filesys/extended/tar -a tar -- -q  -f run grow-root-lg

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/read-normal -a read-normal -p ../../tests/userprog/sample.txt -a sample.txt -- -q  -f run read-normal

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/args-none -a args-none -- -q  -f run args-none
