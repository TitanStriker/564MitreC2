# Beach head

This file describes how to build and run the beach head program for the CS564 Final Project, as well as extra notes on cases where it will run. The goal of this program is given a url of an unencrypted binary, download that binary and run it. In the future, this beach head program could set up persistence for the program or even select which binary to deploy based on the target running this code.

## Build and Run
To build and obtain the beachhead string:
```bash
cd Beachhead
Make
cat beachhead
```

## Setting the URL
To set the URL, edit the `Makefile` URL value. Do not remove the double quotes.

## Notes, Assumptions, and Dependencies
After compiling, the compiled program is around 34 KiB. This program depends on the target system having wget available. This method was chosen to keep the beach head binary small and not depend on system wide dependencies. An alternative method would be to use a library such as c++-https or libcurl, however these approaches would bloat the binary size, add stricter dependencies, or both. Also, this program will link against glibc. Glibc is designed to be backwards compatible. It seems that the best way to get greater glibc compatibility is to use a Docker image with an older version of glibc when compiling.
