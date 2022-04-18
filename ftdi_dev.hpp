/*
    Header of FTDIDEV class

    Copyright (C) 2017  Alamy Liu <alamy.liu@gmail.com>


    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License
    as published by the Free Software Foundation; either version 2
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
    MA  02110-1301, USA.
*/


#ifndef _FTDIDEV_HPP_
#define _FTDIDEV_HPP_

#include <cstdlib>      // malloc, free
#include <fstream>      // ifstream, ofstream
#include <string.h>     // memcpy
#include <ftdi.h>
#include "Options.hpp"


/* copied from libftdi::ftdi_i.h */
#define FTDI_MAX_EEPROM_SIZE    (256)               /* MUST fit in INT */


using namespace std;


enum EEPROM_BUFFER_INDEX {
    I,      /* In */
    O,      /* Out */
    EEPROM_BUFFER_INDEX_MAX
};


class FTDIDEV {

private:
    /* FTDI */
    struct ftdi_context *ftdi;
    bool    eeprom_blank;

    unsigned char file_buf[FTDI_MAX_EEPROM_SIZE];
    unsigned int  eeprom_buf_size[EEPROM_BUFFER_INDEX_MAX]; /* might be File size or EEPROM size */

protected:
    int      read_file(string path);
    int     write_file(string path);

    int      read_eeprom();
    int     write_eeprom();

    int     update_string( enum ftdi_eeprom_value value_name, string s );


public:
    /* Constructor / Destructor */
    FTDIDEV( Options *opt );    /* set opt to NULL for file only operation */
    /* TODO: able to set chip type
    FTDIDEV( int vid, int pid, enum ftdi_chip_type type );  // set chip type
    */
    ~FTDIDEV();

    bool    is_EEPROM_blank()   { return eeprom_blank; }

    int     get_eeprom_size(void) {
        int size = 0;

        if (ftdi) {
            ftdi_get_eeprom_value(ftdi, CHIP_SIZE, &size);
        }
        return size;
    }

    void    set_buffer_sizes(unsigned int iSize, unsigned oSize) {
        eeprom_buf_size[I] = iSize;
        eeprom_buf_size[O] = oSize;
    }


    int     read(bool isInFTDIDEV, string fName, bool verboseMode);
    int     write(bool isOutFTDIDEV, string fName, bool verboseMode);

    int     decode(int verbose);
    int     encode(int verbose __attribute__((unused)))
            { return ftdi_eeprom_build(ftdi); }

    void    show_info(void);
    void    dump(unsigned int buf_size);

    int     update_vid( unsigned int vid )
            { return ftdi_set_eeprom_value(ftdi, VENDOR_ID, vid); }
    int     update_pid( unsigned int pid )
            { return ftdi_set_eeprom_value(ftdi, PRODUCT_ID, pid); }

    int     update_strings(
                char *m,    /* manufacturer */
                char *p,    /* product */
                char *s)    /* serial */
            { return ftdi_eeprom_set_strings(ftdi, m, p, s); }

    /* string interface not completed ... yet */
    int     update_manufacturer( string s )
            { return update_strings(const_cast<char *>(s.c_str()), NULL, NULL); }
    int     update_product( string s )
            { return update_strings(NULL, const_cast<char *>(s.c_str()), NULL); }
    int     update_serial( string s )
            { return update_strings(NULL, NULL, const_cast<char *>(s.c_str())); }


};  /* class Options */

#endif  /* _FTDIDEV_HPP_ */
