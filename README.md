Project 3

Created by Patrick Kong

    A license manager.

Usage: %s [-h] [-t <seconds. DEFAULT:100>] [<n processes(MAX 20)>] < <testing.data>

-h: will bring up the help and the program will terminate.

-t: brings up the allowed timer for the program. Default is at 100.
If the user attempts to input 0 on seconds, t value will default to 100.

n: allows the number of concurrent processes to be created.

testing.data is the file used to test out program

TESTING.DATA CONTAINS:

testsim <seconds of sleep> <number of iterations>

example:
testsim 1 2

this means testsim will contain 1 second of sleep and 2 iterations of this.

TROUBLE:
Had some trouble making the semaphores initially. I think I got it worked out, now. Then last minute for some reason it kept spawning 2 grandchildren process each time. I think I've resolved the issue. NOTE: the file that is being streamed should not contain an extra line at the end of file or at the last line. I'm going to resubmit this one since this one is way better now that we don't have processes creating 2 of the same exec arguments.

TODO:
Definitely clean up. A lot of functions are used and could definitely be put into organized files for future use.