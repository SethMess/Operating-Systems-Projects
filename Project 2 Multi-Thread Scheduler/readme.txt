After invoking 'make', this project can be ran by passing an input text file as the second parameter, or by running the test cases in the tester file according to the README.md in that folder.
Example usage: ./mts input.txt

Description of how the design evolved:
My assignment implementation ended up being very similar to what I planned in the design document, the main difference was the 
addition of one more condition variable that each thread signals once it is ready so that the main thread can check if all the 
trains are loaded. Appart from that difference my assignment is very similar to the document, with three mutexes one for loading
synchronisation, one to guard the queue, and one to guard the main track. I also have the two priority queues that I planned to
use, one for each station (west and east) that are sorted based on the respective priority of the trains. I also used the same 
thread structure of one main thread and one worker thread  for each train in the input file. The main thread is has the same role 
as planned where it schedules the other threads and is idle while the trains are crossing or loading.

There are comments in the code explaining what is going on.

There is an included test case called input.txt that contains 9 trains testing a few of the conditions of the assignment.
