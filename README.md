# spool-helpter
Arduino code for tension sensor and pre-feeder for 3d printers.
Used for TPU to prevent stretching and for 5K or larger spools to reduce strain on extruder.

Setup
See included arduino program and images for connections.
After printing, you will need to feed two wires through the bottom and connect to an arduino for sensing.  The code uses the build in pull-up resistor and a ground wire.
Make sure to use a metal washer so it will make contact with both wires.
Included are stls with single and no-hole options for adjusting yourself.

Why?
This was made originally to remove any tension from TPU because the resistance was changing the extrusion amount.  When I got a 5k spool I modified it to work with rigid filament as well.  5K spool is set on a lazy susan with a bolt holding it centered and a spacer with washers preventing it from sliding. I added a spring inline with the ptfe tube after the sensor to take up any overflow from the sensor/feeder setup.  The feeder is set to speed up fast and slow down gently, and it has been so gentle it has stopped the spool from springing at all.  Adjust the arduino code as needed if your feedrate is higher than mine.  

Adjust:
int minDelay = 200;  //minimum delay (fastest feedrate)
to a smaller number to get a higher max speed.  Note this is a delay between steps, so the smaller you go the speed increases non-linearly.

New program includes serial monitoring and control.  Baud 115200:
Send "help" or "?" to the serial line for a list of commands.
