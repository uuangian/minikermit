If you type:<br>
- 'w' int the minikermit terminal, you can upload binary data at the microcontroller board at flash address 0 with an compatible loader .<br>
Minikermit asks for a file name and opens the file for upload.<br>
We use 2 protocols for upload binary programs or data currently, the protocol of bamo128 (own hastily cobbled together protocol, but allows upload more than 64K) and the protocol of arduino bootloader (stk500v1 compatible).<br>
#define ARDUINOBOOTLOADER<br>
If that is defined in minikermit.c, minikermit works with stk500V1 protocol.<br>
The arduino version of minikermit <a href='MiniKermitAndArduino.md'>MiniKermitAndArduino</a> use the arduino compatible protocol only.<br>
If your monitor or operating system at microcontroller board support the commands:<br>
- 'W' (upload data/code in flash starting at any address), <br>
- 'E' (upload data in eeprom starting at any address),<br>
- 'S' (upload data in ram starting at any address)<br>
you can successive load/reload any code/data.<br>
The monitors <a href='http://code.google.com/p/bamo128'>bamo128</a> or <a href='http://cs.ba-berlin.de'>bamo32</a>, and the Java Runtime Evironment for microcontrollers <a href='http://code.google.com/p/bajos'>Bajos</a> use this feature.