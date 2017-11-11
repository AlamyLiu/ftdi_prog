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

FTDIDEV::FTDIDEV()
{
    string err_string;

    /* Already created */
    if (ftdi != NULL) {
        return;
    }

    if ((ftdi = ftdi_new()) == NULL) {
        err_string = "Failed to new FTDI!";
        goto err_new;
    }

    return;

err_new:
    throw std::runtime_error( err_string );
/*
    cerr << err_string << endl;
    throw rc;
*/
}

FTDIDEV::FTDIDEV( int vid, int pid )
{
    int     rc;
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
    if ((rc = ftdi_usb_open(ftdi, vid, pid)) < 0) {
        goto err_open;
    }

    /* IMPORTANT: Perform a EEPROM read to get eeprom size */
    if ((rc = read_eeprom()) < 0) {
        goto err_open;
    }

    return;

err_open:
    err_string = ftdi_get_error_string(ftdi);

    ftdi_free( ftdi );
    ftdi = NULL;
err_new:
    throw std::runtime_error( err_string );
//    cerr << err_string << endl;
//    throw rc;
}

FTDIDEV::FTDIDEV( int vid, int pid, enum ftdi_chip_type type )
    : FTDIDEV( vid, pid )
{
    if (ftdi) {
        ftdi->type = type;
    }
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

int FTDIDEV::read_file(string path)
{
    int rc;
    unsigned int buf_size;
    ifstream ifs( path, ios::in | ios::binary);

    /* if output is EEPROM, eeprom_buf_size[I] would be EEPROM size
     * see iSize, oSize in main() function.
     */
    buf_size = eeprom_buf_size[I];

    /* TODO: handle ERROR */
    ifs.read( reinterpret_cast<char*>(file_buf), buf_size);
    ifs.close();

    /* Copy data to EEPROM buffer */
    // buffer size had been set by set_buffer_sizes()
    rc = ftdi_set_eeprom_buf(ftdi, file_buf, buf_size);
    if ( rc != 0 ) {
        cerr << "Fail to set EEPROM buffer" << endl;
    }

    /* CAUTION: Hacking libftdi to enable this feature */
    ftdi_set_eeprom_value(ftdi, CHIP_SIZE, buf_size);

    return 0;
}

int FTDIDEV::write_file(string path)
{
    int rc;
    unsigned int buf_size;
    ofstream ofs( path, ios::out | ios::trunc | ios::binary);

    /* Copy data from EEPROM buffer */
    buf_size = eeprom_buf_size[O];
    rc = ftdi_get_eeprom_buf(ftdi, file_buf, buf_size);
    if (rc < 0) {
        cerr << "Fail to get EEPROM buffer" << endl;
        return rc;
    }

    /* output size depends on input (EEPROM or file) */
    ofs.write( reinterpret_cast<const char*>(file_buf), buf_size);
    ofs.close();

    return 0;
}

int FTDIDEV::read_eeprom()
{
    int rc;

    if ( !ftdi )        return -ENODEV;

    if ((rc = ftdi_read_eeprom(ftdi)) < 0) {
        cerr << "Fail to Read EEPROM: " << rc
             << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
    }

    /* size will also be set to -1 in the case of Blank EEPROM */
    eeprom_blank = (get_eeprom_size() == -1);

    return rc;
}

int FTDIDEV::write_eeprom()
{
    int rc;

    if ( !ftdi )        return -ENODEV;

    if ((rc = ftdi_write_eeprom(ftdi)) == 0) {
        cout << "Replug device to see the result!" << endl;
    } else {
        cerr << "Fail to Write EEPROM: " << rc
             << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
    }

    return rc;
}

/* ------------------------------------------------------------------ */

int FTDIDEV::read(bool isInFTDIDEV, string fName, bool verboseMode)
{
    int rc;

    if ( isInFTDIDEV ) {
        rc = read_eeprom();
        /* data had been directly read into FTDI buffer */
    } else {
        rc = read_file(fName);
    }

    if ( verboseMode ) {
        dump( eeprom_buf_size[I] );
    }

    return rc;
}

int FTDIDEV::write(bool isOutFTDIDEV, string fName, bool verboseMode)
{
    int rc;

    if ( verboseMode ) {
        dump( eeprom_buf_size[O] );
    }

    if ( isOutFTDIDEV ) {
        rc = write_eeprom();
        /* data had been directly read into FTDI buffer */
    } else {
        rc = write_file(fName);
    }

    return rc;
}

/* ------------------------------------------------------------------ */

int FTDIDEV::decode(int verbose)
{
    int rc;

    if ( !ftdi )        return -ENODEV;

    if ((rc = ftdi_eeprom_decode(ftdi, verbose)) < 0) {
        cerr << "Fail to Decode: " << rc << endl;
        if ( ftdi ) {
            cerr << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
        }
    }

    return rc;
}

void FTDIDEV::show_info( void )
{
    char *ChipType[] = {
        (char *)"AM",
        (char *)"BM",
        (char *)"2232C",
        (char *)"R",
        (char *)"2232H",
        (char *)"4232H",
        (char *)"232H",
        (char *)"230X",
    };

    if ( !ftdi ) {
        cerr << __func__ << ": FTDI device not available!" << endl;
        return;
    }

    cout << "Chip type: "   << ChipType[ftdi->type] << endl;
    cout << "EEPROM size: " << get_eeprom_size() << endl;
}

void FTDIDEV::dump(unsigned int buf_size)
{
    unsigned int    offset, i;
    char            ch;
    unsigned char   *buf = file_buf;

    if (is_EEPROM_blank()) {
        buf_size = FTDI_MAX_EEPROM_SIZE;
        cout << "EEPROM is empty, use maximum size: " << buf_size << endl;
    }

    /* The data might already be in the *file_buf*
     * i.e.: Input from file_buf
     * Just copy it for other cases, don't bother to check
     */
    /* Copy data from EEPROM buffer */
    if (ftdi_get_eeprom_buf(ftdi, buf, buf_size) < 0) {
        cerr << "Fail to get EEPROM buffer" << endl;
        return;
    }

    cout << hex << setfill('0');

    for(offset = 0; offset < buf_size; offset += 16) {

        /* show the offset */
        cout << setw(8) << offset;

        /* show the hex codes */
        for (i = 0; i < 16; i++) {
            if (i % 8 == 0) cout << ' ';
            if (offset + i < buf_size)
                cout << ' ' << setw(2) << (unsigned)(buf[offset + i]);
            else
                cout << "   ";
        }

        /* show the ASCII */
        cout << "  ";
        for (i = 0; i < 16; i++) {
            if (offset + i < buf_size) {
                ch = buf[offset + i];
                ch = ( ((ch >= 0x20) && (ch <= 0x7E)) ? ch : '.' );
            } else {
                ch = '.';
            }
            cout << ch;
        }
        cout << endl;

    }

    cout.unsetf(ios::hex);
    cout << endl;
}
