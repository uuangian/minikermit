**Compiling**<br>
Download and unpack to arduinokermit.c and open a shell.<br>
Arduinokermit is presetted for:<br>
#define BAUDRATE  B57600<br>
#define MODEMDEVICE 	"/dev/ttyUSB0"<br>
#define SCANSERIALDEVICES	// test for /dev/ttyUSB0,1,2,..9 or ACM0,1,2,..9<br>
For USB/ACM ports sometimes you have awhile "skeletons in the closet" if you reset your microcontroller board or temporary disconnect the board. Therefore arduinokermit tests successive for USB0, USB1,.... USB9 for a "living" connection port. Remove this code (comment prpeprocessor directive above), if you not need this behavior.<br>
<br>
Arduinokermit use the stk500V1 protocol only for uploading like the arduinoIDE.<br>

Now compile it:<br>
#> g++ arduinokermit.c -lncurses -oarduinokermit<br>
compiles arduinokermit.c and produces the executable file arduinokermit. Be sure, you have installed the GNU c-development tools (g++) and the ncurses-library before.<br>
Connect your pc with the serial cable with your development board and start ardinokermit from the command line:<br>
#>./arduinokermit<br>
You can communicate now with your development board according to the monitor program.<br>
If arduinokermit is connected with the original bootloader of arduino boards, you can use the minimal monitor inside the bootloader (Reset the board and type 3 time '!').<br>
If you the define the preprocesser macro:<br>
#define ARDUINOSWRESET<br>
in arduinokermit.c, you can upload any binary software at arduino board with the <a href='http://code.google.com/p/minikermit/wiki/Uploading'>'w' - command</a>. Arduinokermit then carried out a software reset of board and initiate a bootloading.<br>
A better solution is to burn the monitor <a href='http://code.google.com/p/bamo128'>bamo128</a> in the arduino und upload any data/code with the monitor-commands and arduinokermit as terminal.