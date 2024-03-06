#Author: ZuperZeus
#!/bin/bash
#Error checking, do this if the user did not give any arguments
if [[ $# -eq 0 ]]
then
	echo "This program will test whether your program is correct or not"
	echo "You must be inside the directory containing make"
	echo "The input file will be used to run ./mts <input file>"
	echo "The output file should contain the expected output"
	echo "For help, run with argument -h"
	exit
fi
#Error checking, do this if the user runs with arg -h or has wrong number of args
if [[ $# -ne 2 ]]
then
	echo "You must provide 2 arguments"
	echo "The 1st argument being: <input file>"
	echo "The 2nd argument being: <output file>"
	exit
fi
#Error checking, do this if either of the args are not a valid file
if ! [[ -f $1 ]] || ! [[ -f $2 ]]
then
	echo "Both arguments must be a file in the current directory"
	echo " input file: $1"
	echo "output file: $2"
	exit
fi
#Self-explanatory, >/dev/null is to prevent make from writing to the terminal
echo "running make..."
make > /dev/null
#Error checking, do this if make failed
if ! [[ $? -eq 0 ]]
then
	echo "make failed, exiting..."
	exit
fi
if ! [[ -f ./mts ]]
then
	echo "The make file must output the output with the name 'mts' as per section 5.1.4"
	echo "The tester couldn't find this file, so it is now exiting..."
	exit
fi
echo "make done, running your program..."
#run the program with the output file, 
#and check for any differences against the output file
./mts $1 | diff --width=140 --side-by-side <(nl -) <(nl $2)
if [[ $? -eq 0 ]]
then
	echo "The program has passed this test :)"
else
	echo "The program might have failed this test"
	echo "Please check that the errors marked are correct"
	echo "Some false errors may be because of threads outputting simultaneously "
fi
