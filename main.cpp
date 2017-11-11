/*
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

/* Based on LIBFTDI 0.20.4 */

/*
    1. Read binary (buffer) from EEPROM or FILE
    2. (a) Decode: convert binary[IN] to ftdi_eeprom structure[IN]
       (b) copy ftdi_eeprom structure IN --> OUT
    3. update ftdi_eeprom[OUT]
    4. Build: convert ftdi_eeprom[OUT] to binary[OUT]
    5. Write binary[OUT] to EEPROM or FILE
 */

#include <cerrno>
#include <iostream>
//#include <string>
#include <cstdlib>      // malloc, free
#include <fstream>		// ifstream, ofstream
#include <iomanip>      // setw, setfill, ...
#include <stdlib.h>		// atoi
#include <unistd.h>		// getopt()
//#include <ftdi.h>
#include "Options.hpp"
#include "ftdi_dev.hpp"
//#include "DebugW.hpp"		// Debug

using namespace std;

// Classes
Options *opt;
FTDIDEV *ftdi_dev;
FILE    *file;

//Debug		*dbg;		/* Warning: should be created before List */


static void atexit_free_options(void)
{
//    cout << __func__ << ":" << __LINE__ << endl;
    delete opt;
}
static void atexit_delete_ftdidev(void)
{
//    cout << __func__ << ":" << __LINE__ << endl;
    delete ftdi_dev;
}




int main(int argc, char* argv[])
{
    int rc = EXIT_SUCCESS;

    try {
        opt = new Options(argc, argv);
    } catch (int e) {
        if (e != -ECANCELED) {
            cerr << "Unknown error: " << e << endl;
        }
        /* 'help' option exit */
        exit( EXIT_SUCCESS );
    }
    atexit( &atexit_free_options );
    opt->applyHiddenRules();
    opt->ShowOpts();

    /* open FTDI device */
    /* Allow fails: so that FTDIDEV methods could still be used.
     * i.e.: just browsing file content
     */
    try {
        ftdi_dev = (opt->isInFTDIDEV() || opt->isOutFTDIDEV())
            ? new FTDIDEV( opt->getVid(), opt->getPid() )
            : new FTDIDEV();
    } catch (std::runtime_error &e) {
        cerr << e.what() << endl;
        exit( EXIT_FAILURE );
    }
/*
    } catch (int e) {
        cerr << "Error: " << e << endl;
        exit( EXIT_FAILURE );
    }
*/
    atexit( &atexit_delete_ftdidev );

    if (opt->isInFTDIDEV() || opt->isOutFTDIDEV()) {
        ftdi_dev->show_info();  /* debugging */
    }

    /* At this point, we know
     *  EEPROM size: from FTDIDEV
     *  Input file size: from Options
     */

    /* Options conflict/error detection (after we know EEPROM size) */
    if (opt->validateOptions( ftdi_dev->get_eeprom_size() ) != 0)
            return EXIT_FAILURE;


    /* ---------- EEPROM ---------- */

    /*
     *      IN      OUT     size ?
     *      FTDI    file    oSize = iSize = EEPROM size
     *      FTDI    FTDI    oSize = iSize = EEPROM size
     *      file    file    oSize = iSize
     *      file    FTDI    just read EEPROM size (pad 0 or truncate)
     */
    unsigned int iSize = 0, oSize = 0;
    if ( opt->isInFTDIDEV() || opt->isOutFTDIDEV() ) {
        iSize = oSize = ftdi_dev->get_eeprom_size();
    } else {
        /* eeprom size type is INT, prevent overflow */
        long fSize = min(opt->getInFileSize(), (long)FTDI_MAX_EEPROM_SIZE);
        oSize = iSize = static_cast<unsigned int>(fSize);   /* Safe: value in INT scope */
    }
    cout << "Size (I,O) = " << iSize << ", " << oSize << endl;
    ftdi_dev->set_buffer_sizes(iSize, oSize);


    /*
     * 1. INPUT: Read from EEPROM or File
     */
    if ( ftdi_dev->read(
        opt->isInFTDIDEV(),
        opt->getInFname(),
        opt->verboseMode()) < 0 )
    {
        cerr << "Failed to Read!" << endl;
        return EXIT_FAILURE;
    }


    /*
     * 2. DECODE (binary -> structure)
     */
    ftdi_dev->decode( opt->verboseMode() );


    /*
     * 3. UPDATE
     */
    /* Input Only or No Update: Skip UPDATE & ENCODE steps.
     * Note: still output data (if required)
     */
    if ( !opt->isOutputDefined() || !opt->isUpdate() )
        goto skip_update;

    if ( opt->isUpdate_vid() )  ftdi_dev->update_vid( opt->getUpdate_vid() );
    if ( opt->isUpdate_pid() )  ftdi_dev->update_pid( opt->getUpdate_pid() );
#if 0
    if ( opt->isUpdate_manufacturer() )
        eeprom->update_manufacturer( opt->getUpdate_manufacturer() );
    if ( opt->isUpdate_product() )
        eeprom->update_product( opt->getUpdate_product() );
    if ( opt->isUpdate_serial() )
        eeprom->update_serial( opt->getUpdate_serial() );
#else
    ftdi_dev->update_strings(
        opt->getUpdate_manufacturer(),
        opt->getUpdate_product(),
        opt->getUpdate_serial()
    );
#endif


    /*
     * 4. ENCODE (structure -> binary)
     */
    /* if things go wrong, don't write out. But still like to show information */
    try {
        if ( ftdi_dev->encode( opt->verboseMode() ) < 0 ) {
            cerr << "Something is wrong in ENCODING. No output!" << endl;
            opt->setOutNULL();
            rc = EXIT_FAILURE;
        }
    } catch (int e) {
        cout << "Something is wrong (" << e << "). No output!" << endl;
        opt->setOutNULL();
        rc = EXIT_FAILURE;
    }

skip_update:
    /* show output information */
    if ( opt->isOutputDefined() || opt->isUpdate() ) {
        if ( opt->viewBinary() )    ftdi_dev->dump( oSize );  /* Binary dump */
//        if ( opt->viewHuman() )     ftdi_dev->showInOut();  /* Human readable */
    }


    /*
     * 5. OUTPUT: Write to EEPROM or File
     */
    /* Need to check isOutputDefined() here, as the _input only_ case
     * also comes here.
     */
    if ( opt->isOutputDefined() ) {
        if ( ftdi_dev->write(
            opt->isOutFTDIDEV(),
            opt->getOutFname(),
            opt->verboseMode()) < 0 )
        {
            cerr << "Failed to Write!" << endl;
            return EXIT_FAILURE;
        }
    }


//	delete dbg;
    return rc;
}
