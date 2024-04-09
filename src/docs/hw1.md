Rehan Ali<br>
CSE 134<br>
Homework 1<br>
April 8th, 2024<br>
# Homework 1

## Question 1
When I first boot up pintos,

## Question 2

The bootloader reads disk sectors in packets, using the 0x13 BIOS interrupt in case it catches an error, and 0x42 to perform the actual read.

## Question 3

The bootloader knows it has found a kernel when the value in the $es register is equal to 0x80.

## Question 4

If the bootloader is unable to find the kernel, it calls the 0x18 software interrupt.

## Question 5

The bootloader transfers control to the operating system when it calls the load\_kernel branch. What this branch does is it loads the kernel and its various sectors until it is done loading. Then, it jump s to the starting of the sector, which seems to be 0x2000.

## Question 6
### Part A
The callstack is as follows:

\#0  palloc\_get\_page (flags=(PAL\_ASSERT | PAL\_ZERO))
    at ../../threads/palloc.c:112
\#1  0xc00203aa in paging\_init () at ../../threads/init.c:168
\#2  0xc002031b in pintos\_init () at ../../threads/init.c:100
\#3  0xc002013d in start () at ../../threads/start.S:180

### Part B
The return value of the function on its first invocation is 0xc0101000
## Question 7

### Part A

The callstack is as follows:

\#0  palloc\_get\_page (flags=PAL\_ZERO) at ../../threads/palloc.c:112
#1  0xc0020a81 in thread\_create (name=0xc002c4d5 "idle", priority=0,
    function=0xc0020eb0 <idle>, aux=0xc000efbc) at ../../threads/thread.c:178
#2  0xc0020976 in thread\_start () at ../../threads/thread.c:111
#3  0xc0020334 in pintos\_init () at ../../threads/init.c:119
#4  0xc002013d in start () at ../../threads/start.S:180

### Part B

The return value of palloc\_get\_page on its third invocation is 0xc0104000.
