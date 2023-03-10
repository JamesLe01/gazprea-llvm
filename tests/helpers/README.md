this directory contains a few utility programs
1. testsplit.py, splits every test-source
2. cleansplit.py, cleans every test cases in input/ inStream/ and output/ directories but leaves the folders there
3. memchk.py, runs test cases in ./tests/ similar to tester. It can check the memory leak of the program.
4. testerr.py, runs test cases end with ".test" in error-reporting folder

To run testsplit.py:
- make sure all .test files are in test-source folder or its subfolders
- run 'python3 testsplit.py'
- the split test files (.in .ins .out) overwrite old test flies if they have not been cleaned

To run cleansplit.py:
- run 'python3 cleansplit.py'

To run memchk.py
- run it with no arguments will work like tester. It will run every single test in tests/input folder and print results into stderr
- run it with no argument: 'python3 memchk.py', make sure your terminal is inside the helper folder when running the script
- run it with output redirection 'python3 memchk.py 2&>../memchk.out' redicts stderr to tests/memchk.out; I don't put the output in helpers folder because I don't know how to exclude them in the gitignore if they are nested inside a ignore->include->ignore directory
- run it with one argument for the specific test case to run 'python3 memchk.py 2_Branch0_IfStat.test 2&>../memchk.ou' this will only run the given test
- run it with argument "-gazc" will use valgrind on gazc compiler to check for mem leak in the C++ side

To run testerr.py
- run 'python3 testerr.py 2&>../testerr.out' should generate test results in the parent folder
- like memchk.py, this can take one argument to specify running a single test case instead of running all test cases
- unlike memchk.py, testerr.py does not need to split test cases, it just runs ".test" files directly