/*
    Header of EEPROM class

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


#ifndef _EEPROM_HPP_
#define _EEPROM_HPP_

#include <cstdlib>      // malloc, free
#include <fstream>		// ifstream, ofstream
#include <string.h>     // memcpy
#include <ftdi.h>
#include "Options.hpp"
#include "ftdi_dev.hpp"


using namespace std;


#define MAX_EEPROM_SIZE         (512)               /* MUST fit in INT */

#define VID_INDEX               (0x02)
#define PID_INDEX               (0x04)

#define MANUFACTURER_INDEX      (0x0E)
#define PRODUCT_INDEX           (0x10)
#define SERIAL_INDEX            (0x12)
#define VENDOR_INDEX            MANUFACTURER_INDEX  /* alias */


enum EEPROM_INDEX {
    IN,
    OUT,
    EEPROM_INDEX_MAX
};

class FTDIDEV;

class EEPROM {

private:
    unsigned int    eeprom_size;                /* EEPROM size from FTDIDEV */
    unsigned int    buf_size[EEPROM_INDEX_MAX]; /* might be File size or EEPROM size */

    unsigned char       *(eeprom_buffer[EEPROM_INDEX_MAX]);
    struct ftdi_eeprom  *(ftdi_eeprom[EEPROM_INDEX_MAX]);

protected:
    int     read_from_ftdidev( FTDIDEV *dev );
    int     write_to_ftdidev( FTDIDEV *dev );

    int     read_from_file( string path );
    int     write_to_file( string path );


    int build_string( unsigned char *buf, int buf_size, int &pos, int ofst, char *s );

    int update_manufacturer_product_serial( char *m, char *p, char *s );

    uint16_t check_sum(const unsigned char *buf, size_t len)
    {
        int i;
        uint16_t value;
        uint16_t checksum;

        checksum = 0xAAAA;
        for (i = 0; i < len / 2 - 1; i++) {
            value  = buf[i * 2];
            value += buf[(i * 2) + 1] << 8;
            checksum = value ^ checksum;
            checksum = (checksum << 1) | (checksum >> 15);
        }
        return checksum;
    }
    int build_checksum( EEPROM_INDEX index );

#if 0
    /* CRC-32C (iSCSI) polynomial in reversed bit order. */
    #define POLY 0x82f63b78

    /* CRC-32 (Ethernet, ZIP, etc.) polynomial in reversed bit order. */
    /* #define POLY 0xedb88320 */
    uint32_t crc32c(uint32_t crc, const unsigned char *buf, size_t len)
    {
        int k;

        crc = ~crc;
        while (len--) {
            crc ^= *buf++;
            for (k = 0; k < 8; k++)
                crc = crc & 1 ? (crc >> 1) & POLY : crc >> 1;
        }

        return ~crc;
    }
#endif


public:
    /* Constructor / Destructor */
    EEPROM( unsigned int in_size, unsigned int out_size, unsigned int eeprom_size )
    {
        int i;

        /* Save to internal variables */
        buf_size[IN]    = in_size;
        buf_size[OUT]   = out_size;
//        eeprom_size     = eeprom_size;    /* not used, but keep it */

        for (i = 0; i < EEPROM_INDEX_MAX; i++) {
            eeprom_buffer[i]    = new unsigned char[ buf_size[i] ];
            ftdi_eeprom[i]      = new (struct ftdi_eeprom);

//          ftdi_eeprom_initdefaults( ftdi_eeprom[i] );
        }

    }
    ~EEPROM()
    {
        int i;

        for (i = 0; i < EEPROM_INDEX_MAX; i++) {
//            ftdi_eeprom_free( ftdi_eeprom[i] );

            delete ftdi_eeprom[i];
            delete eeprom_buffer[i];
        }
    }

    int     read( Options *opt, FTDIDEV *dev )
    {
        int rc;

        if ( opt->isInFTDIDEV() ) {
            rc = read_from_ftdidev( dev );
        } else if ( opt->isInFile() ) {
            rc = read_from_file( opt->getInFname() );
        } else {
            /* Should never happen */
            cerr << "No Input (EEPROM or File) specified!" << endl;
        }

        if ( opt->verboseMode() ) {
            dump( IN );
        }

        /* Decode binary to structure */
        if (rc == 0) {
            rc = decode( dev, IN );
            if (rc < 0) {
                cerr << "Fail to DECODE input binary!" << endl;
            }
        }

        return rc;
    }

    int     write( Options *opt, FTDIDEV *dev ) {
        int rc;

        if ( opt->isOutFTDIDEV() ) {
            rc = write_to_ftdidev( dev );
        } else if ( opt->isOutFile() ) {
            rc = write_to_file( opt->getOutFname() );
        } else {
            /* Should never happen */
            cerr << "No Output (EEPROM or File) specified!" << endl;
        }

        return rc;
    }

    void copy_eeprom_buffer( EEPROM_INDEX to, EEPROM_INDEX from )
    {
        int size = min( buf_size[to], buf_size[from] );
        memcpy( eeprom_buffer[to], eeprom_buffer[from], size );
    }

    void copy_ftdi_eeprom( EEPROM_INDEX to, EEPROM_INDEX from )
    {
        memcpy( ftdi_eeprom[to], ftdi_eeprom[from], sizeof(struct ftdi_eeprom) );
    }

#if 0
    /* Does not work on BCM chips */
    uint32_t    calculateCRC( EEPROM_INDEX index )
            {
                uint32_t crc = 0;

                return crc32c( crc, reinterpret_cast<const unsigned char*>(eeprom_buffer[index]), ftdi->eeprom_size );
            }
#endif

    uint16_t    calculateChecksum( EEPROM_INDEX index )
            {
                return check_sum( reinterpret_cast<const unsigned char*>(eeprom_buffer[index]),
                    buf_size[index] );
            }

    int     update_vid( unsigned int vid )
            { ftdi_eeprom[OUT]->vendor_id = vid; }
    int     update_pid( unsigned int pid )
            { ftdi_eeprom[OUT]->product_id = pid; }


    /* string interface not completed ... yet */
    int     update_manufacturer( string s )
            { ftdi_eeprom[OUT]->manufacturer = const_cast<char *>(s.c_str()); }
    int     update_product( string s )
            { ftdi_eeprom[OUT]->product = const_cast<char *>(s.c_str()); }
    int     update_serial( string s )
            { ftdi_eeprom[OUT]->serial = const_cast<char *>(s.c_str()); }

#if 0
    int     update_manufacturer( char *s )
            { ftdi_eeprom[IN]->manufacturer = s; }
    int     update_product( char *s )
            { ftdi_eeprom[IN]->product = s; }
    int     update_serial( char *s )
            { ftdi_eeprom[IN]->serial = s; }
#endif


    int     update_manufacturer( char *s )
            { ftdi_eeprom[OUT]->manufacturer = s; }
    int     update_product( char *s )
            { ftdi_eeprom[OUT]->product = s; }
    int     update_serial( char *s )
            { ftdi_eeprom[OUT]->serial = s; }

#if 0
    int     update_buffer_manufacturer( char *s );
    int     update_buffer_product( char *s );
    int     update_buffer_serial( char *s );
#endif


    void    dump( EEPROM_INDEX index );         /* Binary dump */
    void    show( EEPROM_INDEX index );         /* Human readable display */
    void    dumpInOut( void );                  /* up/down binary dump */
    void    showInOut( void );                  /* side-by-side display */

    /* Binary (eeprom_buffer) <--> Structure (ftdi_eeprom) */
    int     decode( FTDIDEV *dev, EEPROM_INDEX index ); /* binary -> structure */
    int     build( EEPROM_INDEX index );                /* structure -> binary */

};  /* class Options */

#endif  /* _EEPROM_HPP_ */
