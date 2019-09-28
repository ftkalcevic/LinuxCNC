/*
    sj200_vfd.c
    Copyright (C) 2011 Frank Tkalcevic, www.franksworkshop.com.au
    Based on...

    gs2_vfd.c
    Copyright (C) 2007, 2008 Stephen Wille Padnos, Thoth Systems, Inc.

    Based on a work (test-modbus program, part of libmodbus) which is
    Copyright (C) 2001-2005 Stéphane Raimbault <stephane.raimbault@free.fr>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation, version 2.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.


    This is a userspace HAL program, which may be loaded using the halcmd "loadusr" command:
        loadusr sj200_vfd
    There are several command-line options.  Options that have a set list of possible values may
        be set by using any number of characters that are unique.  For example, --rate 5 will use
        a baud rate of 57600, since no other available baud rates start with "5"
    -b or --bits <n> (default 8)
        Set number of data bits to <n>, where n must be from 5 to 8 inclusive
    -d or --device <path> (default /dev/ttyS0)
        Set the name of the serial device node to use
    -g or --debug
        Turn on debugging messages.  This will also set the verbose flag.  Debug mode will cause
        all modbus messages to be printed in hex on the terminal.
    -n or --name <string> (default sj200_vfd)
        Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin
        and parameter names will begin with <string>.
    -p or --parity {even,odd,none} (defalt odd)
        Set serial parity to even, odd, or none.
    -r or --rate <n> (default 38400)
        Set baud rate to <n>.  It is an error if the rate is not one of the following:
        110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200
    -s or --stopbits {1,2} (default 1)
        Set serial stop bits to 1 or 2
    -t or --target <n> (default 1)
        Set MODBUS target (slave) number.  This must match the device number you set on the GS2.
    -v or --verbose
        Turn on debug messages.  Note that if there are serial errors, this may become annoying.
        At the moment, it doesn't make much difference most of the time.
    
    Add is-stopped pin John Thornton
*/

#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <getopt.h>
#include "rtapi.h"
#include "hal.h"
#include "modbus.h"

#define countof(x)	((sizeof(x))/sizeof((x)[0]))

union hal_data_t
{
    hal_bit_t bit;
    hal_u32_t u32;
    hal_s32_t s32;
    hal_float_t f;
};

typedef union 
{
    hal_bit_t **bit;
    hal_u32_t **u32;
    hal_s32_t **s32;
    hal_float_t **f;
} hal_data_ptr_ptr_t;

struct CoilData
{
    int enable;
    int bit;
    const char *name;
    int write;
    hal_bit_t **value;
    hal_bit_t old;
};

struct RegisterData
{
    int enable;
    int reg;
    const char *name;
    int write;
    hal_type_t type;
    double scale;
    hal_data_ptr_ptr_t value;
    hal_s32_t old;
};

/* Coils */
static struct CoilData coils[] =
{
    { 1, 0x01, "RunCommandR", 0  },
    { 1, 0x01, "RunCommandW", 1  },
    { 1, 0x02, "FwRevCommandR", 0 },
    { 1, 0x02, "FwRevCommandW", 1 },
    { 1, 0x03, "ExternalTripR", 0 },
    { 1, 0x03, "ExternalTripW", 1 },
    { 1, 0x04, "TripResetR", 0 },
    { 1, 0x04, "TripResetW", 1 },
    { 1, 0x07, "Input1", 0 },
    { 1, 0x08, "Input2", 0 },
    { 1, 0x09, "Input3", 0 },
    { 1, 0x0A, "Input4", 0 },
    { 1, 0x0B, "Input5", 0 },
    { 1, 0x0C, "Input6", 0 },
    { 1, 0x0E, "RunStopStatus", 0 },
    { 1, 0x0F, "FwRevStatus", 0 },
    { 1, 0x10, "InvertReady", 0 },
    { 1, 0x14, "Alarm", 0 },
    { 1, 0x15, "PIDDeviation", 0 },
    { 1, 0x16, "Overload", 0 },
    { 1, 0x17, "FrequencyArrival", 0 },
    { 1, 0x18, "FrequencyArrivalConstant", 0 },
    { 1, 0x19, "RunMode", 0 },
    { 1, 0x1A, "DataWriting", 0 },
    { 1, 0x1B, "CRCError", 0 },
    { 1, 0x1C, "OverrunError", 0 },
    { 1, 0x1D, "FramingError", 0 },
    { 1, 0x1E, "ParityError", 0 },
    { 1, 0x1F, "ChecksumError", 0 },
};
/* SJ200-2 */
static struct RegisterData registers[] =
{
    { 1, 0x0002, "OutputFrequencyCommandW", 1, HAL_FLOAT, 6.0  },
    { 1, 0x0002, "OutputFrequencyCommandR", 0, HAL_FLOAT, 6.0  },
    { 0, 0x0003, "InverterStatusW", 1, HAL_S32, 1.0  },
    { 1, 0x0003, "InverterStatusR", 0, HAL_S32, 1.0  },
    { 0, 0x0005, "ProcessVariable", 0, HAL_S32, 1.0  },
    { -1 },
    { 1, 0x0011, "TripCounter", 0, HAL_S32, 1.0  },
    { 1, 0x0012, "TripMonitor1", 0, HAL_S32, 1.0  },
    { 1, 0x001C, "TripMonitor2", 0, HAL_S32, 1.0  },
    { 1, 0x0026, "TripMonitor3", 0, HAL_S32, 1.0  },
    { -1 },
    { 1, 0x1002, "OutputFrequencyMonitor", 0, HAL_FLOAT, 6  },
    { 1, 0x1003, "OutputCurrentMonitor", 0, HAL_FLOAT, 8.0/1000.0  },
    { 1, 0x1004, "RotationDirectionMonitor", 0, HAL_S32, 1.0  },
    { 0, 0x1005, "PIDFeedbackHigh", 0, HAL_S32, 1.0  },
    { 0, 0x1006, "PIDFeedbackLow", 0, HAL_S32, 1.0  },
    { 0, 0x1007, "InputStatus", 0, HAL_S32, 1.0  },
    { 0, 0x1008, "OutputStatus", 0, HAL_S32, 1.0  },
    { 0, 0x1009, "ScaledOutputFreqHigh", 0, HAL_S32, 1.0  },
    { 0, 0x100A, "ScaledOutputFreqLow", 0, HAL_S32, 1.0  },
    { 1, 0x100C, "OutputVoltageMonitor", 0, HAL_FLOAT, 220.0/10000.0  },
    { 1, 0x100E, "CumulativeOperationRunHigh", 0, HAL_S32, 1.0  },
    { 1, 0x100F, "CumulativeOperationRunLow", 0, HAL_S32, 1.0  },
    { 1, 0x1010, "CumulativePowerOnHigh", 0, HAL_S32, 1.0  },
    { 1, 0x1011, "CumulativePowerOnLow", 0, HAL_S32, 1.0  },
};


/*
static struct RegisterData registers[] =
{
    { 1, 0x0001, "OutputFrequencyCommandW", 1  },
    { 1, 0x0001, "OutputFrequencyCommandR", 0  },
    { 0, 0x0002, "InverterStatusW", 1  },
    { 1, 0x0002, "InverterStatusR", 0  },
    { 0, 0x0003, "ProcessVariable", 0  },
    { -1 },
    { 1, 0x000A, "OutputFrequencyMonitor", 0  },
    { 1, 0x000B, "OutputCurrentMonitor", 0  },
    { 1, 0x000C, "RotationDirectionMonitor", 0  },
    { 0, 0x000D, "PIDFeedbackHigh", 0  },
    { 0, 0x000E, "PIDFeedbackLow", 0  },
    { 0, 0x000F, "InputStatus", 0  },
    { 0, 0x0010, "OutputStatus", 0  },
    { 0, 0x0011, "ScaledOutputFreqHigh", 0  },
    { 0, 0x0012, "ScaledOutputFreqLow", 0  },
    { 1, 0x0013, "OutputVoltageMonitor", 0  },
    { 1, 0x0014, "CumulativeOperationRunHigh", 0  },
    { 1, 0x0015, "CumulativeOperationRunLow", 0  },
    { 1, 0x0016, "CumulativePowerOnHigh", 0  },
    { 1, 0x0017, "CumulativePowerOnLow", 0  },
    { 1, 0x0018, "TripCounter", 0  },
    { 1, 0x0019, "TripMonitor1", 0  },
    { 1, 0x001A, "TripMonitor2", 0  },
    { 1, 0x001B, "TripMonitor3", 0  },
};
*/

static int hal_comp_id;
static int read_index = 0;
static int max_read_registers = 60;
static int debug, verbose;

/* Read Registers:
	0x2100 = status word 1
	0x2101 = status word 2
	0x2102 = frequency command
	0x2103 = actual frequency
	0x2104 = output current
	0x2105 = DC bus voltage
	0x2106 = actual output voltage
	0x2107 = actual RPM
	0x2108 + 0x2109 = scale freq (not sure what this actually is - it's the same as 0x2103)
	0x210A = power factor.  Not sure of the units (1/10 or 1/100)
	0x210B = load percentage
	0x210C = Firmware revision (never saw anything other than 0 here)
	total of 13 registers		*/
#define START_REGISTER_R	0x2100
#define NUM_REGISTERS_R		13
/* write registers:
	0x91A = Speed reference, in 1/10Hz increments
	0x91B = RUN command, 0=stop, 1=run
	0x91C = direction, 0=forward, 1=reverse
	0x91D = serial fault, 0=no fault, 1=fault (maybe can stop with this?)
	0x91E = serial fault reset, 0=no reset, 1 = reset fault
	total of 5 registers */
#define START_REGISTER_W	0x091A
#define NUM_REGISTERS_W		5

#undef DEBUG
//#define DEBUG

/* modbus slave data struct */
typedef struct {
	int slave;		/* slave address */
	int read_reg_start;	/* starting read register number */
	int read_reg_count;	/* number of registers to read */
	int write_reg_start;	/* starting write register number */
	int write_reg_count;	/* number of registers to write */
} slavedata_t;

/* HAL data struct */
typedef struct {
  hal_s32_t	*error_count;
  hal_s32_t	*message_count;;
} haldata_t;

static int done;
char *modname = "sj200_vfd";

static struct option long_options[] = {
    {"bits", 1, 0, 'b'},
    {"device", 1, 0, 'd'},
    {"debug", 0, 0, 'g'},
    {"help", 0, 0, 'h'},
    {"name", 1, 0, 'n'},
    {"parity", 1, 0, 'p'},
    {"rate", 1, 0, 'r'},
    {"stopbits", 1, 0, 's'},
    {"target", 1, 0, 't'},
    {"verbose", 0, 0, 'v'},
    {0,0,0,0}
};

static char *option_string = "b:d:hn:p:r:s:t:v";

static char *bitstrings[] = {"5", "6", "7", "8", NULL};
static char *paritystrings[] = {"even", "odd", "none", NULL};
static char *ratestrings[] = {"110", "300", "600", "1200", "2400", "4800", "9600",
    "19200", "38400", "57600", "115200", NULL};
static char *stopstrings[] = {"1", "2", NULL};

static void quit(int sig) {
    done = 1;
}


int match_string(char *string, char **matches) {
    int len, which, match;
    which=0;
    match=-1;
    if ((matches==NULL) || (string==NULL)) return -1;
    len = strlen(string);
    while (matches[which] != NULL) {
        if ((!strncmp(string, matches[which], len)) && (len <= strlen(matches[which]))) {
            if (match>=0) return -1;        // multiple matches
            match=which;
        }
        ++which;
    }
    return match;
}

void usage(int argc, char **argv) {
    printf("Usage:  %s [options]\n", argv[0]);
    printf(
    "This is a userspace HAL program, typically loaded using the halcmd \"loadusr\" command:\n"
    "    loadusr sj200_vfd\n"
    "There are several command-line options.  Options that have a set list of possible values may\n"
    "    be set by using any number of characters that are unique.  For example, --rate 5 will use\n"
    "    a baud rate of 57600, since no other available baud rates start with \"5\"\n"
    "-b or --bits <n> (default 8)\n"
    "    Set number of data bits to <n>, where n must be from 5 to 8 inclusive\n"
    "-d or --device <path> (default /dev/ttyS0)\n"
    "    Set the name of the serial device node to use\n"
    "-g or --debug\n"
    "    Turn on debugging messages.  This will also set the verbose flag.  Debug mode will cause\n"
    "    all modbus messages to be printed in hex on the terminal.\n"
    "-n or --name <string> (default sj200_vfd)\n"
    "    Set the name of the HAL module.  The HAL comp name will be set to <string>, and all pin\n"
    "    and parameter names will begin with <string>.\n"
    "-p or --parity {even,odd,none} (defalt odd)\n"
    "    Set serial parity to even, odd, or none.\n"
    "-r or --rate <n> (default 38400)\n"
    "    Set baud rate to <n>.  It is an error if the rate is not one of the following:\n"
    "    110, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200\n"
    "-s or --stopbits {1,2} (default 1)\n"
    "    Set serial stop bits to 1 or 2\n"
    "-t or --target <n> (default 1)\n"
    "    Set MODBUS target (slave) number.  This must match the device number you set on the GS2.\n"
    "-v or --verbose\n"
    "    Turn on debug messages.  Note that if there are serial errors, this may become annoying.\n"
    "    At the moment, it doesn't make much difference most of the time.\n");
}

int read_coils(modbus_param_t *param, slavedata_t *slavedata, haldata_t *hal_data_block) {
#define COIL_START	1
#define COIL_COUNT	30
    int receive_data[COIL_COUNT];
    int retval;

    retval = read_coil_status(param, slavedata->slave, COIL_START-1,
                                COIL_COUNT, receive_data);

    if ( retval == COIL_COUNT )
    {
	int i;
        (*hal_data_block->message_count)++;
	for ( i = 0; i < countof(coils); i++ )
	{
	    int bit = coils[i].bit - COIL_START;
	    int state = receive_data[bit];
	    **(coils[i].value) = state;
	}
    }
    else
    {
        (*hal_data_block->error_count)++;
    }
    return retval;
}

int write_coils(modbus_param_t *param, slavedata_t *slavedata, haldata_t *hal_data_block) 
{
    int i,retval=-1;

    for ( i = 0; i < countof(coils); i++ )
    {
        if ( coils[i].enable == 1 && coils[i].write )
        {
	    int reg = coils[i].bit;
	    int value = **(coils[i].value);
            if ( debug ) printf( "%d %d=%d\n", reg, value, coils[i].old );
	    if ( value != coils[i].old )
	    {
    		retval=force_single_coil(param, slavedata->slave, reg, value  );

		if ( retval == 1 )
		{
		    coils[i].old = **(coils[i].value);
                    (*hal_data_block->message_count)++;
		}
                else
                {
                    (*hal_data_block->error_count)++;
                }
	    }
        }
    }

    return retval;
}



int MakeCoilPins()
{
    int i;
    hal_bit_t **hal_data;

    /* grab some shmem to store the coil data in */
    hal_data = (hal_bit_t **)hal_malloc(countof(coils)*sizeof(hal_bit_t *));
    if ((hal_data == 0) || done) 
    {
        printf("%s: ERROR: unable to allocate shared memory\n", modname);
        return -1;
    }

    for ( i = 0; i < countof(coils); i++ )
    {   
        int retval;
	int direction;
	if ( coils[i].write )
	    direction = HAL_IN;
	else
	    direction = HAL_OUT;

        retval = hal_pin_bit_newf(direction, hal_data + i, hal_comp_id, "%s.%s", modname, coils[i].name); 

        if (retval!=0) 
	{
	    return -1;
	}
	coils[i].value = hal_data + i;
        **(coils[i].value) = 0;
        coils[i].old = 0;
    }
    return 0;
}

int read_registers(modbus_param_t *param, slavedata_t *slavedata, haldata_t *hal_data_block) {
#define SLAVE		1

    // Prepare the next block of registers to read
    int index = read_index;
    int reg_start = index;
    int reg_last = reg_start;
    int reg_count = 1;
    
    for(;;)
    {
        int new_count;

        index++;
        if ( index >= countof(registers) )
        {
            // looped around.
            index = 0;
            break;
        }
        if ( registers[index].enable == -1 )    
        {
            // enable == -1 is end of block
            index++;
            break;
        }
        if ( registers[index].write ||
             registers[index].enable != 1 )
        {
            // not enabled, or write only
            continue;
        }
        // Include this register if there is room
        new_count = registers[index].reg - registers[reg_start].reg + 1;
        if ( new_count > max_read_registers )
        {
            // won't fit.
            break;
        }
        reg_count = new_count;
        reg_last = index;
    };

    // scope for alloca
    {
        int *receive_data = (int *)alloca( sizeof(int) * reg_count );
        int retval;
        if ( debug ) printf( "start=%04X, count=%04X\n", registers[reg_start].reg, reg_count );

        retval = read_holding_registers(param, SLAVE, registers[reg_start].reg-1,
                                        reg_count, receive_data);

        if ( retval == reg_count )
        {
            int i;
            (*hal_data_block->message_count)++;
            for ( i = reg_start; i <= reg_last; i++ )
            {
                if ( debug ) printf("i=%d\n",i);
                if ( registers[i].enable == 1 &&
                    !registers[i].write )
                {
                    int data_index, state;
                    data_index = registers[i].reg - 
                                 registers[reg_start].reg;
                    if ( debug ) printf("data_index=%d\n",data_index);
                    state = receive_data[data_index];
                    if ( debug ) printf("state=%d\n",state);
                    switch ( registers[i].type )
                    {
                        case HAL_BIT:
                            **(registers[i].value.bit) = state * registers[i].scale;
                            break;
                        case HAL_FLOAT:
                            **(registers[i].value.f) = state * registers[i].scale;
                            break;
                        case HAL_S32:
                            **(registers[i].value.s32) = state * registers[i].scale;
                            break;
                        case HAL_U32:
                            **(registers[i].value.u32) = state * registers[i].scale;
                            break;
                    }

                }
            }
            // only move on to the next block of registers if this one was 
            // successful
            read_index = index;
        }
        else
        {
            (*hal_data_block->error_count)++;
        }
        return retval;
    }
}

int write_registers(modbus_param_t *param, slavedata_t *slavedata, haldata_t *hal_data_block) 
{
    int i,retval=-1;

    for ( i = 0; i < countof(registers); i++ )
    {
        if ( registers[i].enable == 1 && registers[i].write )
        {
	    int reg = registers[i].reg;
	    int value = 0;
            int old = registers[i].old;

            switch ( registers[i].type )
            {
                case HAL_BIT:
                    value = **(registers[i].value.bit) / registers[i].scale;
                    break;
                case HAL_FLOAT:
                    value = **(registers[i].value.f) / registers[i].scale;
                    if ( debug ) printf( "%f %f %d\n", **(registers[i].value.f), registers[i].scale,  value  );
                    break;
                case HAL_S32:
                    value = **(registers[i].value.s32) / registers[i].scale;
                    break;
                case HAL_U32:
                    value = **(registers[i].value.u32) / registers[i].scale;
                    break;
            }

	    if ( value != old )
	    {
    		retval=preset_single_register(param, slavedata->slave, reg-1, value  );

		if ( retval == 1 )
		{
                    (*hal_data_block->message_count)++;
                    registers[i].old = value;
		}
                else
                {
                    (*hal_data_block->error_count)++;
                }
	    }
        }
    }

    return retval;
}


int MakeRegisterPins()
{
    int i;
    hal_s32_t **hal_data;

    /* grab some shmem to store the coil data in */
    hal_data = (hal_s32_t **)hal_malloc(countof(registers)*sizeof(hal_s32_t *));
    if ((hal_data == 0) || done) 
    {
        printf("%s: ERROR: unable to allocate shared memory\n", modname);
        return -1;
    }

    for ( i = 0; i < countof(registers); i++ )
    {
        if ( registers[i].enable == 1 )
        {   
            int retval;
            int direction;
            if ( registers[i].write )
                direction = HAL_IN;
            else
                direction = HAL_OUT;

            switch ( registers[i].type )
            {
                case HAL_BIT:
                    retval = hal_pin_bit_newf(direction, (hal_bit_t**)(hal_data + i), hal_comp_id, "%s.%s", modname, registers[i].name); 
                    break;
                case HAL_FLOAT:
                    retval = hal_pin_float_newf(direction, (hal_float_t **)(hal_data + i), hal_comp_id, "%s.%s", modname, registers[i].name); 
                    break;
                case HAL_S32:
                    retval = hal_pin_s32_newf(direction, (hal_s32_t **)(hal_data + i), hal_comp_id, "%s.%s", modname, registers[i].name); 
                    break;
                case HAL_U32:
                    retval = hal_pin_u32_newf(direction, (hal_u32_t **)(hal_data + i), hal_comp_id, "%s.%s", modname, registers[i].name); 
                    break;
                default:
                    printf( "unknown data type - %d\n", registers[i].type );
                    return -1;
                    break;
            }

            if (retval!=0) 
            {
                return -1;
            }

            registers[i].old = 0;
            switch ( registers[i].type )
            {
                case HAL_BIT:
                    registers[i].value.bit = (hal_bit_t **)(hal_data + i);
                    **(registers[i].value.bit) = 0;
                    break;
                case HAL_FLOAT:
                    registers[i].value.f = (hal_float_t **)(hal_data + i);
                    **(registers[i].value.f) = 0;
                    break;
                case HAL_S32:
                    registers[i].value.s32 = (hal_s32_t **)(hal_data + i);
                    **(registers[i].value.s32) = 0;
                    break;
                case HAL_U32:
                    registers[i].value.u32 = (hal_u32_t **)(hal_data + i);
                    **(registers[i].value.u32) = 0;
                    break;
            }

        }
    }
    return 0;
}
 
int main(int argc, char **argv)
{
    int retval;
    modbus_param_t mb_param;
    haldata_t *haldata;
    slavedata_t slavedata;
    int baud, bits, stopbits;
    char *device, *parity, *endarg;
    int opt;
    int argindex, argvalue;
    done = 0;

    // assume that nothing is specified on the command line
    baud = 38400;
    bits = 8;
    stopbits = 1;
    debug = 0;
    verbose = 0;
    device = "/dev/ttyS0";
    parity = "odd";

    /* slave / register info */
    slavedata.slave = 1;
    slavedata.read_reg_start = START_REGISTER_R;
    slavedata.read_reg_count = NUM_REGISTERS_R;
    slavedata.write_reg_start = START_REGISTER_W;
    slavedata.write_reg_count = NUM_REGISTERS_R;

    // process command line options
    while ((opt=getopt_long(argc, argv, option_string, long_options, NULL)) != -1) {
        switch(opt) {
            case 'b':   // serial data bits, probably should be 8 (and defaults to 8)
                argindex=match_string(optarg, bitstrings);
                if (argindex<0) {
                    printf("sj200_vfd: ERROR: invalid number of bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                bits = atoi(bitstrings[argindex]);
                break;
            case 'd':   // device name, default /dev/ttyS0
                // could check the device name here, but we'll leave it to the library open
                if (strlen(optarg) > FILENAME_MAX) {
                    printf("sj200_vfd: ERROR: device node name is too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                device = strdup(optarg);
                break;
            case 'g':
                debug = 1;
                verbose = 1;
                break;
            case 'n':   // module base name
                if (strlen(optarg) > HAL_NAME_LEN-20) {
                    printf("sj200_vfd: ERROR: HAL module name too long: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                modname = strdup(optarg);
                break;
            case 'p':   // parity, should be a string like "even", "odd", or "none"
                argindex=match_string(optarg, paritystrings);
                if (argindex<0) {
                    printf("sj200_vfd: ERROR: invalid parity: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                parity = paritystrings[argindex];
                break;
            case 'r':   // Baud rate, 38400 default
                argindex=match_string(optarg, ratestrings);
                if (argindex<0) {
                    printf("sj200_vfd: ERROR: invalid baud rate: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                baud = atoi(ratestrings[argindex]);
                break;
            case 's':   // stop bits, defaults to 1
                argindex=match_string(optarg, stopstrings);
                if (argindex<0) {
                    printf("sj200_vfd: ERROR: invalid number of stop bits: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                stopbits = atoi(stopstrings[argindex]);
                break;
            case 't':   // target number (MODBUS ID), default 1
                argvalue = strtol(optarg, &endarg, 10);
                if ((*endarg != '\0') || (argvalue < 1) || (argvalue > 254)) {
                    printf("sj200_vfd: ERROR: invalid slave number: %s\n", optarg);
                    retval = -1;
                    goto out_noclose;
                }
                slavedata.slave = argvalue;
                break;
            case 'v':   // verbose mode (print modbus errors and other information), default 0
                verbose = 1;
                break;
            case 'h':
            default:
                usage(argc, argv);
                exit(0);
                break;
        }
    }

    printf("%s: device='%s', baud=%d, bits=%d, parity='%s', stopbits=%d, address=%d, verbose=%d\n",
           modname, device, baud, bits, parity, stopbits, slavedata.slave, debug);
    /* point TERM and INT signals at our quit function */
    /* if a signal is received between here and the main loop, it should prevent
            some initialization from happening */
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    /* Assume 38.4k O-8-1 serial settings, device 1 */
    modbus_init_rtu(&mb_param, device, baud, parity, bits, stopbits, verbose);
    mb_param.debug = debug;
    /* the open has got to work, or we're out of business */
    if (((retval = modbus_connect(&mb_param))!=0) || done) {
        printf("%s: ERROR: couldn't open serial device\n", modname);
        goto out_noclose;
    }

    /* create HAL component */
    hal_comp_id = hal_init(modname);
    if ((hal_comp_id < 0) || done) {
        printf("%s: ERROR: hal_init failed\n", modname);
        retval = hal_comp_id;
        goto out_close;
    }

    if ( MakeCoilPins() < 0 )
    {
        retval = -1;
        goto out_close;
    }

    if ( MakeRegisterPins() < 0 )
    {
        retval = -1;
        goto out_close;
    }

    /* grab some shmem to store the HAL data in */
    haldata = (haldata_t *)hal_malloc(sizeof(haldata_t));
    if ((haldata == 0) || done) {
        printf("%s: ERROR: unable to allocate shared memory\n", modname);
        retval = -1;
        goto out_close;
    }

    retval = hal_pin_s32_newf(HAL_IO, &(haldata->message_count), hal_comp_id, "%s.message-count", modname);
    if (retval!=0) goto out_closeHAL;
    retval = hal_pin_s32_newf(HAL_IO, &(haldata->error_count), hal_comp_id, "%s.error-count", modname);
    if (retval!=0) goto out_closeHAL;

    /* make default data match what we expect to use */
/*
    *(haldata->stat1) = 0;
    *(haldata->stat2) = 0;
    *(haldata->freq_cmd) = 0;
    *(haldata->freq_out) = 0;
    *(haldata->curr_out) = 0;
    *(haldata->DCBusV) = 0;
    *(haldata->outV) = 0;
    *(haldata->RPM) = 0;
    *(haldata->scale_freq) = 0;
    *(haldata->power_factor) = 0;
    *(haldata->load_pct) = 0;
    *(haldata->FW_Rev) = 0;
    haldata->errorcount = 0;
    haldata->looptime = 0.1;
    haldata->motor_RPM = 1730;
    haldata->motor_hz = 60;
    haldata->speed_tolerance = 0.01;
    haldata->ack_delay = 2;
    *(haldata->err_reset) = 0;
    *(haldata->spindle_on) = 0;
    *(haldata->spindle_fwd) = 1;
    *(haldata->spindle_rev) = 0;
    haldata->old_run = -1;		// make sure the initial value gets output
    haldata->old_dir = -1;
    haldata->old_err_reset = -1;
*/

    hal_ready(hal_comp_id);
    
    /* here's the meat of the program.  loop until done (which may be never) */
    while (done==0) {
        read_coils(&mb_param, &slavedata, haldata);
        read_registers(&mb_param, &slavedata, haldata);
        write_registers(&mb_param, &slavedata, haldata);
        write_coils(&mb_param, &slavedata, haldata);

        /* don't want to scan too fast, and shouldn't delay more than a few seconds */
        //if (haldata->looptime < 0.001) haldata->looptime = 0.001;
        //if (haldata->looptime > 2.0) haldata->looptime = 2.0;
        //loop_timespec.tv_sec = (time_t)(haldata->looptime);
        //loop_timespec.tv_nsec = (long)((haldata->looptime - loop_timespec.tv_sec) * 1000000000l);
        //nanosleep(&loop_timespec, &remaining);
    }
    
    retval = 0;	/* if we get here, then everything is fine, so just clean up and exit */
out_closeHAL:
    hal_exit(hal_comp_id);
out_close:
    modbus_close(&mb_param);
out_noclose:
    return retval;
}
