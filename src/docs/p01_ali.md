Rehan Ali<br>
CSE 143<br>
Programming Assingment 1 Design <br>
April 18th, 2024<br>

# Design

## Shell

### Data Structures Created

I did not create any new data structures for this part of the assignment. 

### Algorithms Used

To create the algorithm to use the shell, I used a rather simple algorithm. First, I made a macro called COMMAND\_BUFFER\_SIZE defined as 256 for the maximum number of characters the command buffer could hold. This command buffer was defined as an array of characters that was then parsed for commands. Then, I used the input.h, string.h, and console.h headers for the following functions:
```
puts();
putchar();
strlen();
input_getc();
strcmp();
```  
puts() and putchar() are similar in functionality, where they take in a string and a character respectively and write it to the console. However, puts() also writes a newline to the console, so I had to use putchar() in order to write information to the console without newlines. Then, I used putchar() to write the CSE134> prompt by writing each character individually, using the strlen() function in a for loop conditional to ensure I wrote the correct amount of characters. Inside the infinite while loop, I made another while loop that terminates when the user enters a carriage return, or the enter key. While this condition is not true, it is constantly getting characters from the keyboard buffer and then using putchar() to display them to the console. Additionally, it is incrementing an idx variable to keep track of how long the command is and writing those characters to that idx within the command buffer. Once a carriage return is detected, a newline is printed and then the strcmp() function is used to compare whatever is in the command buffer to 2 strings: whoami and exit. If it is whoami, the string rali3, my cruzid, is printed to the console. If it is exit, it exits out of the shell and shuts down the operating system. Finally, if it does not match either of these strings, Invalid is printed to the console. Once it finishes this switch statement, it resets the command buffer, idx variable, and current key (which is used to check the current key being pressed) to start a new iteration.   

### Design Justification

As this part of the assignment was implementing a simple shell that had 2 commands and 3 outputs: whoami, and exit for the commands and rali3, exiting the kernel, and Invalid respectively, I have followed all of these requirements. When the user types whoami, my cruzid is printed, satisfying the first requirement. When the user types exit, the kernel is allowed to exit, satisfying the second requirement. Finally, when anything other than these two commands is typed in, Invalid is printed, satisfying the third requirement. Additionally, it is able to take in additional commands once the user has completed one, satisfying the fourth requirement. Thus, my design follows all of the specifications and is thus justified.  

## Alarm Clock 
### Data Structures Created

While I did not create any new data structures for part 1 of this assignment, I created 1 new data structure called 
```
thread_time_left
```
that had 4 attributes for part 2: elem, sleep\_ticks, my\_thread, and ticks\_at\_calltime. This was a new data structure I created in the timer.h file, as it is used in the timer.c file exclusively. The elem attribute is necessary for this data structure to be placed in the list object found within pintos, the sleep ticks attribute is a uint64\_t to store the amoutn of ticks that the thread is supposed to sleep for, the my\_thread attribute is used to reference the thread that is supposed to be sleeping, and the ticks\_at\_calltime is used to reference the amount of ticks that have passed since the thread started sleeping. 

### Algorithms Used

To create a more efficient thread\_sleep function, I developed my own algorithm that takes inspiration from the semaphores found within sync.h. Firstly, I create a global static list of the aforementioned thread\_time\_left objects to store all of the threads sleeping. I then initialize this list in the timer\_init() function to initialize the list at the start of the program. Now, there are two functions that need to be modified: timer\_sleep() and timer\_interrupt(). 

#### Timer\_Sleep()

The changes I made to this function were rather simple. First, I instantiated a thread\_time\_left struct and filled it out with the appropriate values, with the current ticks going into ticks\_at\_calltime, the argument of thread\_sleep() going into sleep\_ticks, the current thread going into the my\_thread attribute, and an initialized list\_elem pointer for the elem attribute. Then, I needed to disable interrupts to then block the current thread, and then reenable interrupts to preserve the functionality of the OS. As this is only meant to sleep a thread, the changes to this are rather simple. Moving on, I will now discuss the changes made in the timer interrupt handler.

#### Timer\_Interrupt()
The majority of the changes I made were here, as I needed a different way of keeping track of the time that did not involve busy-waiting. Much like its predecessor, this function first starts by incrementing the amount of ticks that have passed via 
```
tick++; 
thread_tick();
``` 
Then, my logic comes into play. Every tick, I iterate through the list I made earlier that stores all of the thread\_time\_left objects. Then I use the following calculation to check if a thread should be woken back up:
```
timer_ticks() - curr->ticks_at_calltime >= curr->sleep_ticks)
``` 
where timer\_ticks() is the current amount of ticks that have passed since the operating system booted, and curr is the current thread\_time\_left object that is being operated on. By taking the difference of timestamps of when the thread started sleeping and what the current time stamp is, we can find out the amount of time that the thread has been sleeping. Then, we can compare it to the amount of time it is supposed to be sleeping for to determine whether it should be woken up or not. I wake the thread up simply by using the thread\_unblock() function on the my\_thread object stored in curr. Additionally, I also remove it from the list of sleeping threads as it is no longer sleeping. 

### Synchronization 

In order to put the thread to sleep and wake it up, I used the thread\_block() and thread\_unblock() functions respectively. What these functions do is thread\_block() deschedules a thread until it is then rescheduled by its counterpart thread\_unblock(). This is different in comparison to the thread\_yield() function, which can be thought of as immediately descheduling and then rescheduling the target thread.

### Design Justification

One of the goals of this assignment was to implement a more optimized thread\_sleep function that did not waste CPU cycles by busy-waiting. I have achieved this goal as forcing a thread to sleep by blocking it does not count as busy waiting. The implications of this are that the CPU can now schedule other threads while this thread is sleeping, allowing for a more efficient operating system. While the algorithm I wrote to iterate through the sleeping threads list is O(N), where N is the amount of sleeping threads on the system, I figured there would never be a large enough amount of sleeping threads for this to be an issue. Additionally, the timer interrupt is already always being called to keep track of the amount of time that has passed since boot, thus less CPU cycles are wasted by not having threads that are constantly yielding to the CPU being scheduled, an issue with the original implementation. This satisfies the initial design requirement for this part of the project, and thus my design is justified. 
