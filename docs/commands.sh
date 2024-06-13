



clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/grow-file-size -a grow-file-size -p tests/filesys/extended/tar -a tar -- -q  -f run grow-file-size


clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/grow-root-lg -a grow-root-lg -p tests/filesys/extended/tar -a tar -- -q  -f run grow-root-lg

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/read-normal -a read-normal -p ../../tests/userprog/sample.txt -a sample.txt -- -q  -f run read-normal

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/args-none -a args-none -- -q  -f run args-none

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/grow-sparse -a grow-sparse -p tests/filesys/extended/tar -a tar -- -q  -f run grow-sparse 

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-mk-tree -a dir-mk-tree -p tests/filesys/extended/tar -a tar -- -q  -f run dir-mk-tree

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-empty-name -a dir-empty-name -p tests/filesys/extended/tar -a tar -- -q  -f run dir-empty-name

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-mkdir -a dir-mkdir -p tests/filesys/extended/tar -a tar -- -q  -f run dir-mkdir

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/userprog/open-normal -a open-normal -p ../../tests/userprog/sample.txt -a sample.txt -- -q  -f run open-normal

clear && make all && pintos -v -k -T 60 --qemu --gdb --filesys-size=2 -p tests/filesys/base/syn-write -a syn-write -p tests/filesys/base/child-syn-wrt -a child-syn-wrt -- -q  -f run syn-write

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-over-file -a dir-over-file -p tests/filesys/extended/tar -a tar -- -q  -f run dir-over-file

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-open -a dir-open -p tests/filesys/extended/tar -a tar -- -q  -f run dir-open

clear && make all && rm -f tmp.dsk && pintos-mkdisk tmp.dsk --filesys-size=2 && pintos -v -k -T 60 --qemu --gdb --disk=tmp.dsk -p tests/filesys/extended/dir-mk-tree -a dir-mk-tree -p tests/filesys/extended/tar -a tar -- -q  -f run dir-mk-tree

