/*
    Implementation of EEPROM class

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
#include "eeprom.hpp"


int EEPROM::read_from_ftdidev( FTDIDEV *dev )
{
    return dev->read_eeprom(eeprom_buffer[IN], buf_size[IN]);
}

int EEPROM::write_to_ftdidev( FTDIDEV *dev )
{
    return dev->write_eeprom(eeprom_buffer[OUT], buf_size[OUT]);
}

int EEPROM::read_from_file( string path )
{
    ifstream ifs( path, ios::in | ios::binary);

    /* if output is EEPROM, buf_size[IN] would be EEPROM size
     * see iSize, oSize before Constructor
     */
    /* TODO: handle ERROR */
    ifs.read( reinterpret_cast<char*>(eeprom_buffer[IN]), buf_size[IN] );
    ifs.close();

    return 0;
}

int EEPROM::write_to_file( string path )
{
    ofstream ofs( path, ios::out | ios::trunc | ios::binary);

    /* output size depends on input (EEPROM or file) */
    ofs.write( reinterpret_cast<const char*>(eeprom_buffer[OUT]), buf_size[OUT] );
    ofs.close();

    return 0;
}

/* buffer --> ftdi_eeprom */
int EEPROM::decode( FTDIDEV *dev, EEPROM_INDEX index )
{
    return dev->eeprom_decode(
                ftdi_eeprom[index],
                eeprom_buffer[index],
                buf_size[index]);
}

/* ftdi_eeprom --> buffer */
#if 0
int EEPROM::build( EEPROM_INDEX index )
{
    int rc;

    if ((rc = ftdi_eeprom_build( ftdi_eeprom[index], eeprom_buffer[index] )) < 0) {
        cerr << "Fail to Build EEPROM: " << rc
             << "(" << ftdi_get_error_string(ftdi) << ")" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
#else


/*
    0x0E: manufacturer string index
    0x10: product string index
    0x12: serial string index
 */

    /*
        [string index + 0]: string offset + 0x80
        [string index + 1]: length of string

        i.e:
        00000000  02 08 5c 0a fa 43 00 07  80 fa 08 00 00 00 9a 12  ..\..C..........
        00000010  ac 22 ce 10 00 00 00 00  46 00 12 03 42 00 72 00  ."......F...B.r.
        00000020  6f 00 61 00 64 00 63 00  6f 00 6d 00 22 03 42 00  o.a.d.c.o.m.".B.
        00000030  43 00 4d 00 39 00 34 00  33 00 39 00 30 00 37 00  C.M.9.4.3.9.0.7.
        00000040  41 00 45 00 56 00 41 00  4c 00 32 00 46 00 10 03  A.E.V.A.L.2.F...
        00000050  32 00 30 00 36 00 38 00  38 00 36 00 37 00 00 00  2.0.6.8.8.6.7...

        Serial : "2068867"
        [0x12] : 0xce : serial string offset + 0x80  : 0x4e
        [0x13] : 0x10 : (string length + 1) * 2      : length = 7

        [0x4e] : 0x10 : (string length + 1) * 2
        [0x4f] : 0x03 : what does it mean ?

        [0x50] : string
     */
int EEPROM::build_string( unsigned char *buf, int buf_size, int &pos, int str_index, char *s )
{
    int str_size = 0;
    int i;

    /*  str_index
        0x0E: manufacturer string index
        0x10: product string index
        0x12: serial string index
     */

    /*
        [string index + 0]: string offset + 0x80
        [string index + 1]: length of string

        i.e:
        00000000  02 08 5c 0a fa 43 00 07  80 fa 08 00 00 00 9a 12  ..\..C..........
        00000010  ac 22 ce 10 00 00 00 00  46 00 12 03 42 00 72 00  ."......F...B.r.
        00000020  6f 00 61 00 64 00 63 00  6f 00 6d 00 22 03 42 00  o.a.d.c.o.m.".B.
        00000030  43 00 4d 00 39 00 34 00  33 00 39 00 30 00 37 00  C.M.9.4.3.9.0.7.
        00000040  41 00 45 00 56 00 41 00  4c 00 32 00 46 00 10 03  A.E.V.A.L.2.F...
        00000050  32 00 30 00 36 00 38 00  38 00 36 00 37 00 00 00  2.0.6.8.8.6.7...

        Serial : "2068867"
        [0x12] : 0xce : serial string offset + 0x80  : 0x4e
        [0x13] : 0x10 : (string length + 1) * 2      : length = 7

        [0x4e] : 0x10 : (string length + 1) * 2
        [0x4f] : 0x03 : what does it mean ?

        [0x50] : string
     */

    str_size = strlen( s );

    /* last 2 bytes are checksum */
    if ( (pos + (str_size * 2 + 2)) > (buf_size - 2) ) {
        cerr << hex
             << "string too long (pos, len)=(" << pos << ", " << str_size << "), "
             << "buffer size = " << buf_size << endl;
        throw (-ENOMEM);
    }

    buf[str_index+0] = pos | buf_size;
    buf[str_index+1] = str_size * 2 + 2;

    /* leading: 2 bytes */
    buf[pos++] = str_size * 2 + 2;
    buf[pos++] = 0x03;

    /* string: size * 2 bytes */
    for (i = 0; i < str_size; i++) {
        buf[pos++] = *s++;
        buf[pos++] = 0x00;
    }

    return 0;
}

int EEPROM::build_checksum( EEPROM_INDEX index )
{
    uint16_t checksum;

    checksum = calculateChecksum( index );

    eeprom_buffer[index][ buf_size[index] - 2] = checksum & 0xFF;
    eeprom_buffer[index][ buf_size[index] - 1] = (checksum >> 8) & 0xFF;
}

int EEPROM::build( EEPROM_INDEX index )
{
    unsigned char *buf;                 /* Shorten coding statements */
    int m_pos;                          /* manufacturer string position */
    int p_pos;                          /* product string position */
    int s_pos;                          /* serial string position */
    int start_pos;                      /* where all the strings begin */

    buf = eeprom_buffer[index];

    /* rebuild vid/pid */
    buf[VID_INDEX + 0] = ftdi_eeprom[index]->vendor_id & 0xFF;
    buf[VID_INDEX + 1] = (ftdi_eeprom[index]->vendor_id >> 8) & 0xFF;
    buf[PID_INDEX + 0] = ftdi_eeprom[index]->product_id & 0xFF;
    buf[PID_INDEX + 1] = (ftdi_eeprom[index]->product_id >> 8) & 0xFF;


    /* rebuild all strings */
    m_pos = buf[MANUFACTURER_INDEX]  & (buf_size[index] - 1);
    p_pos = buf[PRODUCT_INDEX]       & (buf_size[index] - 1);
    s_pos = buf[SERIAL_INDEX]        & (buf_size[index] - 1);
#if 0   /* TODO: debugging */
    cout << "(m_pos, p_pos, s_pos) = (" << hex
         << m_pos << ", " << p_pos << ", " << s_pos << ")" << endl;
#endif

    start_pos = min( m_pos, p_pos );
    start_pos = min( s_pos, start_pos );

    /* assume strings are stored at 0x1A onwards */
    if ( start_pos != 0x1A ) {
        cerr << __func__ << ": Strings starts at " << hex << start_pos
             << " != 0x1A." << endl;
        throw (-EFAULT);
    }

    build_string( buf, buf_size[index], start_pos, MANUFACTURER_INDEX, ftdi_eeprom[index]->manufacturer );
    build_string( buf, buf_size[index], start_pos, PRODUCT_INDEX,      ftdi_eeprom[index]->product );
    build_string( buf, buf_size[index], start_pos, SERIAL_INDEX,       ftdi_eeprom[index]->serial );


    /* last step, re-calculate checksum */
    build_checksum( index );

    return 0;
}

#endif

#if 0
    /* Update in the ftdi_eeprom structure */
int EEPROM::update_manufacturer( char *s )
{
    ftdi_eeprom[OUT]->manufacturer = s;
}

int EEPROM::update_product( char *s )
{
    ftdi_eeprom[OUT]->product = s;
}

int EEPROM::update_serial( char *s )
{
    ftdi_eeprom[OUT]->serial = s;
}

#else
    /* Update in the eeprom binary buffer */

#if 0
int EEPROM::update_manufacturer_product_serial( char *m, char *p, char *s )
{


}

int EEPROM::update_manufacturer( char *s )
{
    update_manufacturer_product_serial( s, NULL, NULL );
}

int EEPROM::update_product( char *s )
{
    update_manufacturer_product_serial( NULL, s, NULL );
}

int EEPROM::update_serial( char *s )
{
    update_manufacturer_product_serial( NULL, NULL, s );
}
#endif

#endif


void EEPROM::dump( EEPROM_INDEX index )
{
    unsigned int    offset, i;
    char            ch;

    cout << hex << setfill('0');

    for(offset = 0; offset < buf_size[index]; offset += 16) {

        /* show the offset */
        cout << setw(8) << offset;

        /* show the hex codes */
        for (i = 0; i < 16; i++) {
            if (i % 8 == 0) cout << ' ';
            if (offset + i < buf_size[index])
                cout << ' ' << setw(2) << (unsigned)((eeprom_buffer[index])[offset + i]);
            else
                cout << "   ";
        }

        /* show the ASCII */
        cout << "  ";
        for (i = 0; i < 16; i++) {
            if (offset + i < buf_size[index]) {
                ch = (eeprom_buffer[index])[offset + i];
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

void EEPROM::show( EEPROM_INDEX index )
{
    struct ftdi_eeprom *ee;             /* Shorten coding statements */

    /* Banner */
    cout << "---------- < "
         << ((index == IN) ? "IN" : (index == OUT ? "OUT" : "Unknown" ))
         << " > ----------" << endl;

    ee = ftdi_eeprom[index];

    /* Content */
    cout << hex << setfill('0');
    cout << " vendor ID: 0x" << setw(4) << ee->vendor_id << endl;
    cout << "product ID: 0x" << setw(4) << ee->product_id << endl;
    cout.unsetf(ios::hex);
    cout << "Manufacturer: " << ee->manufacturer << endl;
    cout << "Product:      " << ee->product << endl;
    cout << "Serial:       " << ee->serial << endl;

}

void EEPROM::dumpInOut( void )
{
    cout << "----- <  IN  > -----" << endl;
    dump( IN );

    cout << "----- <  OUT > -----" << endl;
    dump( OUT );

    cout << endl;
}


#define FIELD_WIDTH     (22)
#define GAP_WIDTH       (2)

void EEPROM::showInOut( void )
{
    struct ftdi_eeprom *in, *out;

    in  = ftdi_eeprom[IN];
    out = ftdi_eeprom[OUT];

    cout << "               -----===   IN ===-----  -----===  OUT ===-----" << endl;
    cout << hex;
    cout << " vendor ID:    "
         << setfill(' ') << setw(FIELD_WIDTH-4) << ' '
         << setfill('0') << setw(4) << in->vendor_id
         << setfill(' ') << setw(GAP_WIDTH) << ' '
         << setfill(' ') << setw(FIELD_WIDTH-4) << ' '
         << setfill('0') << setw(4) << out->vendor_id
         << endl;
    cout << "product ID:    "
         << setfill(' ') << setw(FIELD_WIDTH-4) << ' '
         << setfill('0') << setw(4) << in->product_id
         << setfill(' ') << setw(GAP_WIDTH) << ' '
         << setfill(' ') << setw(FIELD_WIDTH-4) << ' '
         << setfill('0') << setw(4) << out->product_id
         << endl;
    cout << setfill(' ') << dec;

    cout << setfill(' ');
    cout << "High current:  "
         << setw(FIELD_WIDTH) << in->high_current << setw(GAP_WIDTH) << ' '
         << setw(FIELD_WIDTH) << out->high_current << endl;
    cout << "Max power:     "
         << setw(FIELD_WIDTH) << in->max_power << setw(GAP_WIDTH) << ' '
         << setw(FIELD_WIDTH) << out->max_power << endl;

    cout << "Manufacturer:  "
         << setw(FIELD_WIDTH) << in->manufacturer << setw(GAP_WIDTH) << ' '
         << setw(FIELD_WIDTH) << out->manufacturer << endl;
    cout << "Product:       "
         << setw(FIELD_WIDTH) << in->product << setw(GAP_WIDTH) << ' '
         << setw(FIELD_WIDTH) << out->product << endl;
    cout << "Serial:        "
         << setw(FIELD_WIDTH) << in->serial << setw(GAP_WIDTH) << ' '
         << setw(FIELD_WIDTH) << out->serial << endl;

    cout << endl;
}
