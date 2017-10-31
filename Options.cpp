/*
    Implementation of Options class

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

#include <getopt.h>         /* getopt_long */
#include <iostream>         /* cout */
#include <iomanip>          /* setw, setfill, ... */
#include <fstream>          /* ifstream, ofstream */
#include <string.h>         /* strcmp */
#include "Options.hpp"


Options::Options(
    int argc,
    char* argv[]
) {
	// Retrieve options
	int opt_index = 0;
	int opt;

    int rc;

	while ( (opt = getopt_long(argc, argv, "hv:p:i:o:m:n:x:y:z:",
		long_opts, &opt_index)) != -1) {

		switch (opt) {
		case 'h':   ShowHelp();
                    throw -ECANCELED;
                    break;

        /* VID / PID */
        case 'v':   optValue.vid = stoi( optarg, nullptr, 0 );  break;
        case 'p':   optValue.pid = stoi( optarg, nullptr, 0 );  break;

        /* In / Out */
        case 'i':   if (strcmp(optarg, IN_OUT_EEPROM_NAME) == 0)
                        optValue.flags.in_ftdidev = 1;
                    else
                        optValue.iFname = string( optarg );
                    break;
        case 'o':   if (strcmp(optarg, IN_OUT_EEPROM_NAME) == 0)
                        optValue.flags.out_ftdidev = 1;
                    else
                        optValue.oFname = string( optarg );
                    break;

#if 0   /* enabled by default */
        /* verbose mode */
        case 'v':   optValue.flags.verbose = 1;     break;
        case 'd':   optValue.flags.view_binary = 1;        break;
        case 'd':   optValue.flags.view_human = 1;        break;
#endif

        /* ----- UPDATE ----- */
        case 'm':   optValue.flags.update = 1;
                    optValue.update.vid = stoi( optarg, nullptr, 0 );   break;
        case 'n':   optValue.flags.update = 1;
                    optValue.update.pid = stoi( optarg, nullptr, 0 );   break;

        /* Manufacturer, Product, Serial */
#if 0
        case 'x':   optValue.flags.update = 1;
                    optValue.update.manufacturer    = string( optarg ); break;
        case 'y':   optValue.flags.update = 1;
                    optValue.update.product         = string( optarg ); break;
        case 'z':   optValue.flags.update = 1;
                    optValue.update.serial          = string( optarg ); break;
#else
        case 'x':   optValue.flags.update = 1;
                    optValue.update.manufacturer    = optarg;   break;
        case 'y':   optValue.flags.update = 1;
                    optValue.update.product         = optarg;   break;
        case 'z':   optValue.flags.update = 1;
                    optValue.update.serial          = optarg;   break;
#endif

		case '?': /* Unknown option (ignore) */
		default : /* Do nothing */		break;
		} // End of switch(opt)
	} // End of while(opt)


    /* ---------- Get extra information ---------- */

    // Get input file size
    if ( isInFile() ) {
        ifstream in( getInFname(), ios::binary | ios::ate );
        if ( in.good() )
            optValue.iFsize = in.tellg();
        in.close();
    }

}

void Options::applyHiddenRules()
{
    /* if IN/OUT were not defined, but VID/PID are provided
     * {
     *      set IN to FTDIDEV
     * }
     */
    if ( !isInputDefined() && isVidPidDefined() ) {
        setInFTDIDEV();
    }


    /* if OUT was not defined, but (any update-xxx is provided)
     * {
     *      set OUT to IN
     * }
     */
    if ( !isOutputDefined() && isUpdate() ) {
        if ( isInFTDIDEV() )    setOutFTDIDEV();
        if ( isInFile() )       setOutFile( getInFname() );
    }

}

int Options::validateOptions( int eeprom_size )
{
    /* Need Input, eithre from FTDIDEV or File */
    if ( !isInputDefined() ) {
        cerr << "No Input (EEPROM or File) specified!" << endl;
        return EXIT_FAILURE;

        return -EINVAL;
    }

    /* FTDIDEV depends on VID/PID */
    if ( isInFTDIDEV() || isOutFTDIDEV() ) {
        if ( !isVidPidDefined() ) {
            cerr << "VID/PID is not defined!" << endl;
            return -EINVAL;
        }
    }

    /* Input file existence */
    if ( isInFile() ) {
        /* check optValue.iFsize instead of opening file to check f.good()
         * as it had been done in Constructor
         */
        if ( optValue.iFsize == 0 ) {
            cerr << "Input file, " << getInFname()
                 << ", does not exist or size is zero!" << endl;
            return -EINVAL;
        }

        /* Input file size > EEPROM size */
        if ( isOutFTDIDEV() && ( optValue.iFsize > eeprom_size ) ) {
            cerr << "Input file size ("
                 << hex << optValue.iFsize << ") > EEPROM size ("
                 << eeprom_size << ")!" << dec << endl;
            return -EINVAL;
        }
    }

    /* Output file overwrite: use ifstream to test, not typo */
    if ( isOutFile() ) {
        ifstream out( getOutFname(), ios::binary | ios::ate );
        if ( out.good() ) {
            cerr << "Output file, " << getOutFname()
                  << ", already exists. Overwrite!" << endl;
        }

        /* Output file size depends on input device (EEPROM or File).
         * No need to verify
         */

        out.close();
    }

    return 0;
}

void Options::ShowHelp( void )
{
    cout << endl << "----- Help menu -----" << endl;
    cout << "help           Help menu" << endl
         << "verbose        Verbose mode" << endl
         << "show-binary    Dump EEPROM binary data" << endl
         << "show-human     Human readable (decode from binary)" << endl
         << "vid            Vendor ID" << endl
         << "pid            Product ID" << endl
         << "in             Input (EEPROM or filename)" << endl
         << "out            Output (EEPROM or filename)" << endl
         << endl
         << "update-xxx     update output field, where 'xxx' could be:" << endl
         << "       vid     VID field (update-vid)" << endl
         << "       pid     PID field (update-pid)" << endl
         << "  manufacturer Manufacturer field" << endl
         << "   product     Product field" << endl
         << "    serial     Serial field" << endl
         << endl;
}

void Options::ShowOpts( void )
{
    cout.setf(ios::hex, ios::basefield);
    cout << setfill('0');
    cout << "(vid, pid) = "
         << setw(4) << optValue.vid << ", "
         << setw(4) << optValue.pid << endl;
    cout.unsetf(ios::hex);

    cout << "flag: verbose = "
         << (optValue.flags.verbose ? "Yes" : "No") << endl;
    cout << "flag: view_binary = "
         << (optValue.flags.view_binary ? "Yes" : "No") << endl;
    cout << "flag: view_human = "
         << (optValue.flags.view_human ? "Yes" : "No") << endl;
    cout << "flag: in_ftdidev = "
         << (optValue.flags.in_ftdidev ? "Yes" : "No") << endl;
    cout << "flag: out_ftdidev = "
         << (optValue.flags.out_ftdidev ? "Yes" : "No") << endl;

    cout << "In Fname = "
         << (isInFile()  ? getInFname()  : "(null)") << endl;
    cout << "Out Fname = "
         << (isOutFile() ? getOutFname() : "(null)") << endl;

    cout << endl;
}
