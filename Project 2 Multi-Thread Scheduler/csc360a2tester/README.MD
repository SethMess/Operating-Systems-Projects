# Assignment 2 Tester

## This is a tester program for csc360 assignment 2.

Please note that the tester failing does *not* necessarily mean that your program generated the wrong output,
if 2 threads output simultaneously, the order may not be the same every time.

To download this, first run `git clone https://github.com/ZuperZeus/csc360a2tester`

Once you have downloaded the files, move `Makefile` and `p2.c` inside the `csc360a2tester` folder

Then `cd` into `csc360a2tester`

Once inside the folder, to be able to run the tester, make the script executable by running `chmod u+x ./tester.sh`

Then, if you have your own files, run:
```
./tester.sh <input file> <output file>
```
or, if you are using the provided files, run
```
./tester.sh <num>.trains <num>.results
```

If using the provided files, note that each file number tests something different, the specifics are explained here:

### 1
This is the default test file provided in the assignment pdf
### 2
This test file has 4 `W` trains and 1 `e` train, to test the starvation rule
### 3
This test file has 1 high priority and 1 low priority trains, to test the priority rules
### 4
This test file has 3 high priority trains, `E` goes first, and then `W` should go first by 4b
### 5
This test file has 2 high priority trains, `E` and `W`. `W` should go first as Westbound has priority by 4b
### 6
This test file has 3 trains, 2 have everything identical except for load. lower load should go first
### 7
This test file has 2 identical trains, the only difference is in where they appear in the input file. train 0 goes first
### 8
integrates all of the previous examples, and should test all rules, a more detailed explanation follows below:
```
W 0 20 //first train to ensure there will always be an opposite train, no trains finish loading before second 2.0
//High priority:
	//Different from last, i.e. alternating directions
		//Load first
			E 1 2
			W 1 2
		//Load last
			E 2 2
			W 2 2
	//Same as last, i.e. same direction for all
		W 3 2
		W 4 2 //At this point, Eastbound trains are starved, so an e must cross
		e 0 2 //included to test the starvation rule, 0 to avoid other low priority trains from crossing here
		W 5 2
		W 6 2
//Low priority:
	//Different from last, i.e. alternating directions
		//Load first
			e 1 2
			w 1 2
		//Load last
			e 2 2
			w 2 2
	//Same as last, i.e. same direction for all
	//starvation, but there are no Westbound trains
	//This should not cause an error
		e 3 2
		e 4 2
		e 5 2
		e 6 2
```
## Examples
The lines provided by your program will always be on the left, and the output file will be on the right (same order as `./tester <input file> <output file>`). Differences are marked in the center.
### Correct output
This is an example of the correct output for case 1.
```
running make...
make done, running your program...
     1  00:00:00.3 Train  2 is ready to go East                              1  00:00:00.3 Train  2 is ready to go East
     2  00:00:00.3 Train  2 is ON the main track going East                  2  00:00:00.3 Train  2 is ON the main track going East
     3  00:00:00.6 Train  1 is ready to go West                              3  00:00:00.6 Train  1 is ready to go West
     4  00:00:01.0 Train  0 is ready to go East                              4  00:00:01.0 Train  0 is ready to go East
     5  00:00:01.3 Train  2 is OFF the main track after going East           5  00:00:01.3 Train  2 is OFF the main track after going East
     6  00:00:01.3 Train  1 is ON the main track going West                  6  00:00:01.3 Train  1 is ON the main track going West
     7  00:00:02.0 Train  1 is OFF the main track after going West           7  00:00:02.0 Train  1 is OFF the main track after going West
     8  00:00:02.0 Train  0 is ON the main track going East                  8  00:00:02.0 Train  0 is ON the main track going East
     9  00:00:02.6 Train  0 is OFF the main track after going East           9  00:00:02.6 Train  0 is OFF the main track after going East
The program has passed this test :)
```
### Incorrect output
This is an example of incorrect output, caused by running `./tester 1.trains 3.results`, which are files that don't match.
```
running make...
make done, running your program...
     1  00:00:00.3 Train  2 is ready to go East                      |       1  00:00:00.3 Train  0 is ready to go West
     2  00:00:00.3 Train  2 is ON the main track going East          |       2  00:00:00.3 Train  1 is ready to go West
     3  00:00:00.6 Train  1 is ready to go West                      |       3  00:00:00.3 Train  0 is ON the main track going West
     4  00:00:01.0 Train  0 is ready to go East                      |       4  00:00:01.0 Train  0 is OFF the main track after going West
     5  00:00:01.3 Train  2 is OFF the main track after going East   |       5  00:00:01.0 Train  1 is ON the main track going West
     6  00:00:01.3 Train  1 is ON the main track going West          |       6  00:00:01.4 Train  1 is OFF the main track after going West
     7  00:00:02.0 Train  1 is OFF the main track after going West   <
     8  00:00:02.0 Train  0 is ON the main track going East          <
     9  00:00:02.6 Train  0 is OFF the main track after going East   <
The program might have failed this test
Please check that the errors marked are correct
Some false errors may be because of threads outputting simultaneously
```
As you may note, `|` denotes different lines (i.e. errors) and `>`,`<` denotes lines that are present on one side only.

### False negative
This is an example of an output that is correct, but was marked as incorrect due to simultaneous output.
```
running make...
make done, running your program...
     1  00:00:00.3 Train  1 is ready to go West                      |       1  00:00:00.3 Train  0 is ready to go West
     2  00:00:00.3 Train  0 is ready to go West                      |       2  00:00:00.3 Train  1 is ready to go West
     3  00:00:00.3 Train  0 is ON the main track going West                  3  00:00:00.3 Train  0 is ON the main track going West
     4  00:00:01.0 Train  0 is OFF the main track after going West           4  00:00:01.0 Train  0 is OFF the main track after going West
     5  00:00:01.0 Train  1 is ON the main track going West                  5  00:00:01.0 Train  1 is ON the main track going West
     6  00:00:01.4 Train  1 is OFF the main track after going West           6  00:00:01.4 Train  1 is OFF the main track after going West
The program might have failed this test
Please check that the errors marked are correct
Some false errors may be because of threads outputting simultaneously
```
As you can see, `tester` marks some errors, but this is only because the order of the outbut changed, not because the output is incorrect.
If you get an error, you must carefully check the lines marked as `|`, to check whether it is a false negative.
This functionality is intended, as it is better to have false negatives than false positives.
