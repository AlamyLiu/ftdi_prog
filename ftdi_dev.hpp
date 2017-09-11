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
#include <fstream>		// ifstream, ofstream
#include <string.h>     // memcpy
#include <ftdi.h>
#include "eeprom.hpp"


using namespace std;


#define VID_INDEX               (0x02)
#define PID_INDEX               (0x04)

#define MANUFACTURER_INDEX      (0x0E)
#define PRODUCT_INDEX           (0x10)
#define SERIAL_INDEX            (0x12)
#define VENDOR_INDEX            MANUFACTURER_INDEX  /* alias */


class FTDIDEV {

private:
    /* FTDI */
    struct ftdi_context *ftdi;

protected:

public:
    /* Constructor / Destructor */
    FTDIDEV( int vid, int pid );
    ~FTDIDEV();

    int     get_eeprom_size( void ) {
        return  (ftdi ? ftdi->eeprom_size : 0);
    }

    int     read_eeprom( unsigned char *buf, int buf_size );
    int     write_eeprom( unsigned char *buf, int buf_size );

    int     eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *buf, int size);

    void    show_info( void );

};  /* class Options */

#endif  /* _FTDIDEV_HPP_ */
