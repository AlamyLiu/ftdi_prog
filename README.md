# ftdi_prog
Linux alternative to the FTDI Utility FT_PROG

- Windows based FT Prog applicaion
http://www.ftdichip.com/Support/Documents/AppNotes/AN_124_User_Guide_For_FT_PROG.pdf

- Linux package
libftdi-dev/xenial,xenial,now 0.20-4build1 amd64 [installed]
libftdi1/xenial,xenial,now 0.20-4build1 amd64 [installed,automatic]

That says, the code is based on LIBFTDI 0.20.4
Refer to /usr/include/ftdi.h for coding



     parameters
          |
          V
    +----------+      +----------+       +----------+
    |          |      |          |       |          |
    | Options  | ---> |  EEPROM  | <---> |  FTDIDEV |
    |          |      |          |       |          |
    +----------+      +----------+       +----------+
                                         |  libftdi | <--> FTDI device
                                         +----------+


       __________       +----------+           __________
      /         /       |          |          /         /
     /  Input  / -----> |  EEPROM  | ----->  /  Output /
    /         /         |          |        /         /  
   -----------          +----------+       -----------  
   FTDI EEPROM            Binary          FTDI EEPROM
    or File               |    ^            or File
    (binary)       decode |    | build      (binary)
                          V    |
                     struct ftdi_eeprom


- References
ftx-prog: https://github.com/richardeoin/ftx-prog
