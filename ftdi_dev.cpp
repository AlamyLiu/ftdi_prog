/*
    Implementation of FTDIDEV class

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

#include <iostream>         /* cout */
#include <iomanip>          /* setw, setfill, ... */
#include <assert.h>         /* assert */
#include "ftdi_dev.hpp"


/* -------------------- Constructor / Destructor -------------------- */

FTDIDEV::FTDIDEV( int vid, int pid )
{
    string  err_string;

    /* Already created */
    if (ftdi != NULL) {
        return;
    }

    if ((ftdi = ftdi_new()) == NULL) {
        err_string = "Failed to new FTDI!";
        goto err_new;
    }

    /* Use ftdi_usb_open_desc() to specify _description_ & _serial_ of
     * the device
     */
    if (ftdi_usb_open(ftdi, vid, pid) < 0) {
        err_string = ftdi_get_error_string(ftdi);
        goto err_open;
    }

    return;

err_open:
    ftdi_free( ftdi );
    ftdi = NULL;
err_new:
//    throw std::runtime_error( err_string );
    cerr << err_string << endl;
}

FTDIDEV::~FTDIDEV()
{
    if (ftdi) {
        ftdi_usb_close( ftdi );
        ftdi_free( ftdi );
        ftdi = NULL;
    }
}

/* ------------------------------------------------------------------ */

int FTDIDEV::read_eeprom( unsigned char *buf, int buf_size )
{
    int rc;

    if ( !ftdi )    return -ENODEV;
    if ( !buf )     return -EINVAL;

    if ((rc = ftdi_read_eeprom(ftdi, buf)) < 0) {
        cerr << "Fail to Read EEPROM: " << rc
             << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
    }

    return rc;
}

int FTDIDEV::write_eeprom( unsigned char *buf, int buf_size )
{
    int rc;

    if ( !ftdi )    return -ENODEV;
    if ( !buf )     return -EINVAL;

    if ((rc = ftdi_write_eeprom(ftdi, buf)) < 0) {
        cerr << "Fail to Write EEPROM: " << rc
             << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
    }

    return rc;
}

int FTDIDEV::eeprom_decode(struct ftdi_eeprom *eeprom, unsigned char *buf, int size)
{
    int rc;

    /* Input might be File, we don't check ftdi here */
    if ( !eeprom )  return -EINVAL;
    if ( !buf )     return -EINVAL;

    if ((rc = ftdi_eeprom_decode(eeprom, buf, size )) < 0)
    {
        cerr << "Fail to Decode EEPROM: " << rc << endl;
        if ( ftdi ) {
            cerr << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
        }
    }

    return rc;
}

void FTDIDEV::show_info( void )
{
    if ( !ftdi ) {
        cerr << __func__ << ": FTDI device not available!" << endl;
        return;
    }

    cout << "Chip type: "   << ftdi->type << endl;
    cout << "EEPROM size: " << ftdi->eeprom_size << endl;
}
