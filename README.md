# ftdi_prog
The Linux alternative to the FTDI Utility FT_PROG

### Windows based FT Prog applicaion
http://www.ftdichip.com/Support/Documents/AppNotes/AN_124_User_Guide_For_FT_PROG.pdf

### Required package
- libftdi 1.4 (dev)
- libusb-1.0.0-dev

#### Install from package
```
    $ sudo apt install libusb-1.0.0-dev
    $ sudo apt install libftdi1-dev
```
#### Compile libftdi1 from source
Note: just an idea. You know what you are doing, tweak paths yourself.
```
$ cd <libftdi>
$ mkdir BUILD; cd BUILD
$ cmake -DCMAKE_INSTALL_PREFIX=~/local ../
$ make; make install

~/local
|-- bin
|   `-- libftdi1-config
|-- include
|   `-- libftdi1
|       `-- ftdi.h         # <-- header
`-- lib
    |-- cmake
    |   `-- libftdi1
    |       |-- LibFTDI1Config.cmake
    |       |-- LibFTDI1ConfigVersion.cmake
    |       `-- UseLibFTDI1.cmake
    |-- pkgconfig
    |   |-- libftdi1.pc
    |   `-- libftdipp1.pc
    |-- libftdi1.a
    |-- libftdi1.so -> libftdi1.so.2
    |-- libftdi1.so.2 -> libftdi1.so.2.4.0
    `-- libftdi1.so.2.4.0  # <-- library
```

### Compile & Run
If the local built libftdi1 is not installed in one of the system directories
```
    $ export LD_LIBRARY_PATH=~/local/lib
```
Where it could find the **libftdi1.so.2.4.0**

```
$ cd <ftdi_prog>
$ make

<ftdi_prog>
`-- ftdi_prog              # <-- target binary
```

### The code is based on LIBFTDI 1.4
- Refer to /usr/local/include/libftdi1/ftdi.h

```
  parameters
       |
       V
 +----------+      +----------+      +----------+
 | Options  | ---> |   MAIN   | <--> |  FTDIDEV |
 +----------+      +----------+      +----------+
                                /--> |  libftdi |
  FTDI device <----------------/     +----------+

```

### Procedure
1. Input  (read: EEPROM or FILE: binary)
2. Decode (binary -> structure)
3. Update (modify fields in structure)
4. Encode (structure -> binary)
5. Output (write: EEPROM or FILE: binary)

```
              ___________            ___________
             /          /           /          /
INPUT       /  EEPROM  /           /   FILE   /
           /          /           /          /
          ------------           ------------
                     \       (set_eeprom_buffer)
                      \         /
                       V       V
                      +----------+
DECODE                |  Decode  |
                      +----------+
                            |
                            v
                      +----------+
UPDATE                |  Update  |
                      +----------+
                            |
                            v
                      +----------+
ENCODE                |  Encode  |
                      +----------+
                       /         \
                      /      (get_eeprom_buffer)
              _______v___        __v________
             /          /       /          /
OUTPUT      /  EEPROM  /       /   FILE   /
           /          /       /          /
          ------------       ------------
```

### Data flow
```{.cpp}
----------------------------------------
libftdi 1.4 structure
----------------------------------------

/* Even on 93xx66 at max 256 bytes are used (AN_121) */
#define FTDI_MAX_EEPROM_SIZE 256

struct ftdi_context                                 # ftdi_new()
{
    struct ftdi_eeprom *eeprom                      # eeprom structure
    {                                                  ^        |
        int vendor_id;                                 |     (ENCODE)
        int product_id;                                |        |
        char *manufacturer;                            |        |
        char *product;                              (DECODE)    |
        ...                                            |        V
        unsigned char buf[FTDI_MAX_EEPROM_SIZE];    # eeprom binary
    }                                                  |        ^
    ...                                                |        |
}                                        (ftdi_get_eeprom_buf)  |
                                                       |        |
----------------------------------------               |        |
This application                                       |        |
----------------------------------------               |  (ftdi_set_eeprom_buf)
class FTDIDEV                                          |        |
{                                                      V        |
    unsigned char file_buf[FTDI_MAX_EEPROM_SIZE];   # file_buf[]
}
```


- References
ftx-prog: https://github.com/richardeoin/ftx-prog
