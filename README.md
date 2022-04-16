# ftdi_prog
The Linux alternative to the FTDI Utility FT_PROG

#### Windows based FT Prog applicaion
http://www.ftdichip.com/Support/Documents/AppNotes/AN_124_User_Guide_For_FT_PROG.pdf

#### Required package
- libftdi 1.4
  ~~installed under /usr/local~~
~~```$ cmake -DCMAKE_INSTALL_PREFIX="/usr/local" ../```~~
  Modified to use pkg-config since libftdi1 is available on a lot of distribution.

#### Compile & Run
If the local built libftdi is not installed in one of the system directories
```$ export LD_LIBRARY_PATH=/home/alamy/lib```
Where it could find the libftdi1.so.2.4.0

#### The code is based on LIBFTDI 1.4
- Refer to /usr/local/include/libftdi1/ftdi.h
```export LD_LIBRARY_PATH=/usr/local/lib```

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

#### Procedure
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

#### Data flow
```{.cpp}
----------------------------------------
libftdi 1.4 structure
----------------------------------------

/* Even on 93xx66 at max 256 bytes are used (AN_121)*/
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
