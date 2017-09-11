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
#include <stdlib.h>		// atoi
#include <unistd.h>		// getopt()
//#include <ftdi.h>
#include "Options.hpp"
#include "ftdi_dev.hpp"
#include "eeprom.hpp"
//#include "DebugW.hpp"		// Debug

using namespace std;

// Classes
Options *opt;
FTDIDEV *ftdi_dev;
EEPROM  *eeprom;

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
static void atexit_delete_eeprom(void)
{
//    cout << __func__ << ":" << __LINE__ << endl;
    delete eeprom;
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
    opt->ShowOpts();
    opt->applyHiddenRules();

    /* open FTDI device */
    /* Allow fails: so that FTDIDEV methods could still be use */
    ftdi_dev = new FTDIDEV( opt->getVid(), opt->getPid() );
    atexit( &atexit_delete_ftdidev );

    ftdi_dev->show_info();  /* debugging */

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
    long iSize = 0, oSize = 0;
    if ( opt->isInFTDIDEV() || opt->isOutFTDIDEV() ) {
        iSize = oSize = ftdi_dev->get_eeprom_size();
    } else {
        oSize = iSize = opt->getInFileSize();
    }

    try {
        eeprom = new EEPROM( iSize, oSize, ftdi_dev->get_eeprom_size() );
    } catch (std::bad_alloc& ba) {
        cerr << ba.what() << endl;
        cerr << "Fail to create EEPROM object!" << endl;
        return EXIT_FAILURE;
    }
    atexit( &atexit_delete_eeprom );


    /* Input from EEPROM or File */
    if ( eeprom->read( opt, ftdi_dev ) < 0 ) {
        cerr << "Failed to Read!" << endl;
        return EXIT_FAILURE;
    }

    /* Input Only: Skip bunch of things if there is no output
     * But still has to output data (if required)
     */
    if ( !opt->isOutputDefined() )      goto no_output;


    /* make a copy of binary data to output: for 'update' later */
    if ( opt->isOutputDefined() ) {
        eeprom->copy_eeprom_buffer( OUT, IN );  /* Binary */
        eeprom->decode( ftdi_dev, OUT );        /* Binary -> structure */
        // copy_ftdi_eeprom( OUT, IN );
    }

    /* No update: Skip update & build steps */
    if ( !opt->isUpdate() )     goto skip_update;

    /* Update (working in OUTPUT ftdi_eeprom structure) */
    if ( opt->isUpdate_vid() )  eeprom->update_vid( opt->getUpdate_vid() );
    if ( opt->isUpdate_pid() )  eeprom->update_pid( opt->getUpdate_pid() );

    if ( opt->isUpdate_manufacturer() )
        eeprom->update_manufacturer( opt->getUpdate_manufacturer() );
    if ( opt->isUpdate_product() )
        eeprom->update_product( opt->getUpdate_product() );
    if ( opt->isUpdate_serial() )
        eeprom->update_serial( opt->getUpdate_serial() );

    /* Build */
    /* if things go wrong, don't write out. But still like to show information */
    try {
        if ( eeprom->build( OUT ) < 0 ) {
            cerr << "Something is wrong. No output!" << endl;
            opt->setOutNULL();
            rc = EXIT_FAILURE;
        }
    } catch (int e) {
        cout << "Something is wrong (" << e << "). No output!" << endl;
        opt->setOutNULL();
        rc = EXIT_FAILURE;
    }

skip_update:
no_output:
    /* show IN & OUT side-by-side if there is '--out' */
    if ( opt->isOutputDefined() || opt->isUpdate() ) {
        if ( opt->viewBinary() )    eeprom->dumpInOut();    /* Binary dump */
        if ( opt->viewHuman() )     eeprom->showInOut();    /* Human readable */
    } else {
        /* Only IN */
        if ( opt->viewBinary() )    eeprom->dump( IN );
        if ( opt->viewHuman() )     eeprom->show( IN );
        if ( opt->verboseMode() ) {
            cout << hex
                 << "checksum: " << eeprom->calculateChecksum( IN ) << endl
                 << dec;
        }
    }

    /* Output to EEPROM or File */
    /* Need to check isOutputDefined() here, as the input only case jumps
     * to no_output label to display information
     */
    if ( opt->isOutputDefined() ) {
        if (eeprom->write( opt, ftdi_dev ) < 0) {
            cerr << "Fail to write EEPROM data!" << endl;
        }
    }


//	delete dbg;

    return rc;

}
