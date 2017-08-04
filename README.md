# Eclipse2017
Programs to assist with photographing the August 21, 2017 total solar eclipse.

The two programs work out when totality, when the moon completely obstructs the sun, will occur 
for a location. This is done using data published by NASA that projects the irregular shape of 
the moon's umbra onto the irregular shape of the Earth at one second intervals. The programs 
should be able to tell when totality starts and ends to within one second. The NASA data is 
available at:

https://svs.gsfc.nasa.gov/4518

The umbra17_1s files are the important ones. The programs by default expect to find the files in 
a subdirectory of this project called "nasashapes".

The code here was written in a bit of a rush, so it isn't my best. I don't have a lot of time to 
test. It does seem to be working and useful, though.

# Programs

The umbra_test program takes longitude and latitude values either from command-line arguments or 
from stdin. It evaluates if the location will see totality, and if it does, when it will start, 
when it will end, and the total duration. On a Raspberry Pi Zero for a location near the 
longest totality, it takes about 15 seconds to produce the answer, and under 8 seconds for 
additional nearby locations.

The umbra_lcd program is intended for use on a portable computer system built around a Raspberry 
Pi or something similar. It uses GPSD to find its current location, and a HD44780 or compatible 
LCD to show the results. It presently uses a 16x2 display, but I may change it to a 20x4 display 
or maybe two 16x2 displays. I also wrote it with the expectation that ntpd will be running and 
synchronizing the clock with GPS.

What the umbra_lcd program does not yet do that I'd like it to:
 - Estimate the very beginning and ending of the eclipse
 - Show sun azimuth and elevation for the very beginning, ending, and mid-totality

# Dependencies

Both programs need the following libraries:
 - GDAL
 - GEOS

The umbra_lcd program additionally needs these libraries:
 - [Boost](http://www.boost.org/)
 - [DUDS](https://github.com/jjackowski/duds)
 - GPSD's C++ library
 - evdev

If those last three libraries are not found, the umbra_lcd program will not build.

The default location for the DUDS library is at the same directory level as wherever this code 
finds itself.
