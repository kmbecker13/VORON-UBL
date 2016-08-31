# VORON-UBL
VORON Unified Bed Leveling

Hello!  Welcome to the VORON Unified Bed Leveling firmware repo.  

The files you find here are my attempt at merging VORON firmware with the new Marlin UBL Dev branch.

<h3>There are currently two main folders:</h3>

1) VORON_12inch_Volume <-- This is meant for my personal VORON with a 300 x 300 x 230mm build volume and has been tested with success.

2) VORON_Stock_Volume <-- ***UNTESTED*** This is meant for those using a stock VORON build, but hasn't been tested fully (only difference is bed size)

The ONLY changes between these two folders is the Configuration.h file, which has been changed for the different build areas.

I have chosen to place all files in each folder to reduce confusion on how it should work.


<h3>THINGS YOU SHOULD DO:</h3>

1) Grab a copy of the entire Marlin folder for your printer

2) Ensure your printer is NOT smaller than 228mm in XY for stock version and 300mm in XY for 12inch version.

3) Load firmware, check functionality of G29 commands (see UBL documentation on Marlin's Github)

4) Re-tune your extruder e-steps/mm, the default is 600 in the code i supply
