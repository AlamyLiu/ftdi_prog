/*
    Header of Options class

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


#ifndef _OPTIONS_HPP_
#define _OPTIONS_HPP_

#include <string>           /* string */
#include <getopt.h>		// getopt_long()


using namespace std;


#define IN_OUT_EEPROM_NAME          "EEPROM"


typedef struct OPT_FLAGS_S {
    int verbose;                    /* Verbose mode */

    int view_binary;                /* Binary dump    (eeprom_buffer) */
    int view_human;                 /* Human readable (struct ftdi_eeprom) */

    int open_bus;                   /* open usb with bus:dev */
    int open_id;                    /* open usb with vid:pid */

    int update;                     /* --update-xxx option */

    int in_ftdidev;                 /* Read from FTDI Device (EEPROM) */
    int out_ftdidev;                /* Write to FTDI Device (EEPROM) */
} OPT_FLAGS_T;

typedef struct OPT_UPDATE_S {
    unsigned int    vid;
    unsigned int    pid;
#if 0
    string  manufacturer;
    string  product;
    string  serial;
#else
    char    *manufacturer;
    char    *product;
    char    *serial;
#endif
} OPT_UPDATE_T;

typedef struct OPT_VALUE_S {
    OPT_FLAGS_T flags;

    /* bus:devnum */
    unsigned int    bus;
    unsigned int    dev;

    /* vid:pid */
    unsigned int    vid;
    unsigned int    pid;

    string          iFname;         /* EEPROM, or Input file name */
    string          oFname;         /* EEPROM, or Output file name */

    long            iFsize;         /* Input file size (compare with EEPROM size) */

    OPT_UPDATE_T    update;

} OPT_VALUE_T;


class EEPROM;

class Options {
//friend class EEPROM;
private:
    /* TODO: get rid of the size definition
     *
     * Options.hpp: error: too many initializers for ‘const option [0]’
     */
    const struct option long_opts[14] = {
        {"help",        no_argument,        NULL,   'h'},

        {"verbose",     no_argument,        &(optValue.flags.verbose), 1},

        {"show-binary", no_argument,        &(optValue.flags.view_binary), 1},
        {"show-human",  no_argument,        &(optValue.flags.view_human),  1},

        /* -s [bus:dev] : similar to libusb */
        {"bus",         required_argument,  NULL,       's'},
        /* -d [vid:pid] : similar to libusb */
        {"id",          required_argument,  NULL,       'd'},

        {"in",          required_argument,  NULL,       'i'},
        {"out",         required_argument,  NULL,       'o'},

#if 0
        {"manufacturer",required_argument,  NULL,                      0},
        {"product"     ,required_argument,  NULL,                      0},
        {"max-bus-power",required_argument, NULL,                      0},
#endif

        {"update-vid",          required_argument,  NULL,   'm'},
        {"update-pid",          required_argument,  NULL,   'n'},

        {"update-manufacturer", required_argument,  NULL,   'x'},
        {"update-product",      required_argument,  NULL,   'y'},
        {"update-serial",       required_argument,  NULL,   'z'},

        {NULL, 0, NULL, 0},
    };

protected:

    OPT_VALUE_T     optValue;           /* values from parameters */

    void setInFTDIDEV() {
        optValue.flags.in_ftdidev = 1;
        optValue.iFname.clear();
    }
    void setOutFTDIDEV() {
        optValue.flags.out_ftdidev = 1;
        optValue.oFname.clear();
    }
    void setOutFile( string fName ) {
        optValue.flags.out_ftdidev = 0;
        optValue.oFname = fName;
    }

public:
    /* Constructor / Destructor */
    Options(int argc, char* argv[]);
    ~Options()  {}

    void    applyHiddenRules( void );
    int     validateOptions( int eeprom_size );

    int     getBus()        { return optValue.bus; }
    int     getDev()        { return optValue.dev; }
    bool    isBusDefined() {
                return ((getBus() != 0) && (getDev() != 0));
    }
    int     getVid()        { return optValue.vid; }
    int     getPid()        { return optValue.pid; }
    bool    isIdDefined() {
                return ((getVid() != 0) && (getPid() != 0));
    }

    bool    verboseMode()   { return optValue.flags.verbose; }

    bool    viewBinary()    { return optValue.flags.view_binary; }
    bool    viewHuman()     { return optValue.flags.view_human; }

    bool    isInFTDIDEV()   { return optValue.flags.in_ftdidev; }
    bool    isOutFTDIDEV()  { return optValue.flags.out_ftdidev; }

    string  getInFname()    { return optValue.iFname; }
    string  getOutFname()   { return optValue.oFname; }
    bool    isInFile()      { return !getInFname().empty(); }
    bool    isOutFile()     { return !getOutFname().empty(); }
    long    getInFileSize() { return optValue.iFsize; }

    bool    isInputDefined() {
                return ( isInFTDIDEV() || !getInFname().empty() );
            }
    bool    isOutputDefined() {
                return ( isOutFTDIDEV() || !getOutFname().empty() );
            }
    void    setOutNULL() {
        optValue.flags.out_ftdidev = 0;
        optValue.oFname.clear();
    }


    bool    isUpdate()              { return optValue.flags.update; }
    bool    isUpdate_vid()          { return (getUpdate_vid() != 0x0); }
    bool    isUpdate_pid()          { return (getUpdate_pid() != 0x0); }

#if 0
    bool    isUpdate_manufacturer() { return !getUpdate_manufacturer().empty(); }
    bool    isUpdate_product()      { return !getUpdate_product().empty(); }
    bool    isUpdate_serial()       { return !getUpdate_serial().empty(); }
#else
    bool    isUpdate_manufacturer() { return (getUpdate_manufacturer() != NULL); }
    bool    isUpdate_product()      { return (getUpdate_product() != NULL); }
    bool    isUpdate_serial()       { return (getUpdate_serial() != NULL); }
#endif

    unsigned int getUpdate_vid()        { return optValue.update.vid; };
    unsigned int getUpdate_pid()        { return optValue.update.pid; };

#if 0
    string  getUpdate_manufacturer()    { return optValue.update.manufacturer; }
    string  getUpdate_product()         { return optValue.update.product; }
    string  getUpdate_serial()          { return optValue.update.serial; }
#else
    char    *getUpdate_manufacturer()   { return optValue.update.manufacturer; }
    char    *getUpdate_product()        { return optValue.update.product; }
    char    *getUpdate_serial()         { return optValue.update.serial; }

#endif

    void ShowHelp( void );
    void ShowOpts( void );


};  /* class Options */

#endif  /* _OPTIONS_HPP_ */
