Minikermit is a linux terminal program especially for communication with microcontroller development boards with monitors (minimal operating systems) which supports serial communication and upload software/data.<br><br>
<b>How it works</b> <br>
Connect the development board over a serial communication line with your pc and start minikermit in your shell. Minikermit then waits for key pressed at pc keypad and sends the character over serial line to the development board. Characters, which the development board sends over serial line, are displayed in the minikermit terminal window.<br>
Minikermit terminates, if you press 'q' or 'Q' at keypad. If you press 'w', minikermit<br>
alters to upload-state (upload binary data in microcontroller board).<br>
This is the NONECHOMODE of minikermit (default after startup). The microcontroller can toogle to ECHOMODE by serial sending the TOGGLEECHOMODE-char (0x03).minikermit in ECHOMODE terminates only with CTRL-C sequence from shell.<br>
When the microcontroller sends SUPRESSSERINPUT	(0x04), the pc blocks the output of chars from serial in the terminal window, if it sends ACCEPTSERINPUT (0x05), the chars from serial are written in the pc terminal window (default at startup).<br>
<br><br>
<b>Compiling</b><br>
Download and unpack to minikermit.c and open a shell. Edit minikermit.c for your environment with an editor.<br>
Minikermit works with different serial communication ports (ttyi, USBi, ACMi), baudrates and you can it adapt to different monitor programs at the development boards. Set the values for the preprocessor directives in minikermit.c for your environment.<br>
// select your serial device<br>
//#define MODEMDEVICE "/dev/ttyACM0"	/<code>*</code> 11-th position is 0<code>*</code>/<br>
//#define MODEMDEVICE 	"/dev/ttyS0"	/<code>*</code>  first RS232 port<code>*</code>/<br>
#define MODEMDEVICE 	"/dev/ttyUSB0"	/<code>*</code>  USB port<code>*</code>/<br>
//#define MODEMDEVICE 	"/dev/ttyUSB1"	/<code>*</code>  USB port<code>*</code>/<br>
//#define MODEMDEVICE 	"/dev/ttyS1"	/<code>*</code>  second RS232 port<code>*</code>/<br>

#define SCANSERIALDEVICES	// test for /dev/ttyUSB0,1,2,..9 or ACM0,1,2,..9<br>
<br>
For USB/ACM ports sometimes you have awhile "skeletons in the closet" if you reset your microcontroller board or temporary disconnect the board. Therefore minikermit tests successive for USB0, USB1,.... USB9 for a "living" connection port. Remove this code, if you not need this behavior.<br>
<br>
// select your baudrate<br>
//#define BAUDRATE B1200<br>
//#define BAUDRATE B9600<br>
//#define BAUDRATE B19200<br>
//#define BAUDRATE B38400<br>
#define BAUDRATE  B57600 // its the arduino baudrate (mega and duemilanove)<br>
//#define BAUDRATE B115200<br>
<br><br>
Minikermit works with many development boards and monitor programs. It is prepared for cooperation with:<br>
- diverse avr8 and avr32 controllers, which are equipped with the monitor program <a href='http://code.google.com/p/bamo128'>bamo128</a> or <a href='http://cs.ba-berlin.de'>bamo32</a>,<br>
- diverse avr8 and avr32 controllers, which the Java Runtime Evironment <a href='http://code.google.com/p/bajos'>Bajos</a>,<br>
- arduino boards with arduino bootloader (shipped with board)<br>
- Motorola EVB board with 68HC11 and monitor<br>
- Z80mini with monitor<br>

// select your target processor or better your board (select only one !!)<br>
#define ARDUINOBOOTLOADER	// any (avr) arduino board with arduino compatible bootloader<br>
If this macro is not defined, minikermit support an "own brewed" protocol, which allows to upload more than 64K.<br>
// #define CPU68HC11		// 68HC11 development board with monitor<br>
// #define CPUZ80		// the Z80 development board z80mini with monitor<br>

#define SCANSERIALDEVICES	// test for /dev/ttyUSB0,1,2,..9 or ACM0,1,2,..9<br>

// #define ARDUINORESET   // normally not uncommented<br>
with the 'w'-command the arduino board ist resetted per software und the bootloader upload the bin-file in flash (with arduino/stk500v1-protocol)<br><br>
<b>Now compile it !!</b> <br>
From the shell prompt type:<br>
#> g++ minikermit.c -lncurses -ominikermit<br>
compiles minikermit.c and produces the executable file minikermit. Be sure, you have installed the GNU c-development tools (g++) and the ncurses-library before.<br>
Connect your pc with the serial cable with your development board and start minikermit from the command line:<br>
#>./minikermit<br>
You can communicate now with your development board according to the monitor program.<br>

You can download the working version from the repository or the latest stable version from the download area.