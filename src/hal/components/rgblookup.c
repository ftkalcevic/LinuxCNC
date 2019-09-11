
//

//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

/* A component to lookup an rgb value from the given index. */

#include "rtapi.h"
#include "rtapi_app.h"
#include "rtapi_string.h"
#include "hal.h"
#include "rtapi_math.h"

#if !defined(__KERNEL__)
#include <stdio.h>
#include <stdlib.h>
#endif

/* module information */
MODULE_AUTHOR("Frank Tkalcevic");
MODULE_DESCRIPTION("Find RGB value from input index. Mapping defined on command line");
MODULE_LICENSE("GPL");

char *mapping;
RTAPI_MP_STRING(mapping, "rgb list")

#define MAX_ENTRIES 20

typedef struct {
    hal_float_t r,g,b;
} rgb_t;

typedef struct {
    hal_float_t *r,*g,*b;
    hal_s32_t *ir,*ig,*ib;
    hal_u32_t *index;
} hal_data_t;

static hal_data_t *hal_data;
static rgb_t rgb[MAX_ENTRIES];
static hal_u32_t rgb_entries;
static int comp_id;


static int read_config(void) {
   
    char *ptr = mapping;
    hal_float_t data[3];
    int colour = 0;
    int decimal = 0;

    rtapi_print_msg(RTAPI_MSG_ERR, "rgblookup: mapping=%s\n", mapping);
    data[0] = data[1] = data[2] = 0;
    while ( *ptr != '\0' ) {
        switch ( *ptr )
        {
            case ',':
                colour++;
                if ( colour > 2 ) {
                    rtapi_print_msg(RTAPI_MSG_ERR, "rgblookup: more than 3 colours for entry %d\n", rgb_entries);
                    return 0;
                }
                break;

            case ';':
                if ( colour != 2 ) {
                    rtapi_print_msg(RTAPI_MSG_ERR, "rgblookup: less than 3 colours for entry %d\n", rgb_entries);
                    return 0;
                }
                rgb[rgb_entries].r = data[0];
                rgb[rgb_entries].g = data[1];
                rgb[rgb_entries].b = data[2];
                rgb_entries++;
                colour = 0;
                decimal = 0;
                data[0] = data[1] = data[2] = 0;
                break;
            case '.':
                decimal = -1;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
                if ( decimal ) {
                    data[colour] += (*ptr - '0') * pow(10,decimal);
                    decimal--;
                } else {
                    data[colour] = data[colour] * 10 + (*ptr - '0');
                }
                break;
        }
        ptr++;
    }

    return 1;
}

void write(void *arg, long period){
    hal_data_t *data = arg;

    if ( *(data->index) >= rgb_entries )
    {
        *(data->ir) = *(data->r) = 0;
        *(data->ig) = *(data->g) = 0;
        *(data->ib) = *(data->b) = 0;
    }
    else
    {
        *(data->ir) = *(data->r) = rgb[*(data->index)].r;
        *(data->ig) = *(data->g) = rgb[*(data->index)].g;
        *(data->ib) = *(data->b) = rgb[*(data->index)].b;
    }
}


int rtapi_app_main(void){
    int retval;
    
    comp_id = hal_init("rgblookup");
    if (comp_id < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "rgblookup: ERROR: hal_init() failed\n");
        return -1;
    }

    // Read mapping from command line
    if ( !read_config() )
        return -1;

    // allocate shared memory for data
    hal_data = hal_malloc(sizeof(hal_data_t));
    if (hal_data == 0) {
        rtapi_print_msg(RTAPI_MSG_ERR,
                        "rgblookup component: Out of Memory\n");
        hal_exit(comp_id);
        return -1;
    }
    
    retval = hal_pin_float_newf(HAL_OUT, &(hal_data->r), comp_id, "rgblookup.r");
    retval = hal_pin_float_newf(HAL_OUT, &(hal_data->g), comp_id, "rgblookup.g");
    retval = hal_pin_float_newf(HAL_OUT, &(hal_data->b), comp_id, "rgblookup.b");
    retval = hal_pin_s32_newf(HAL_OUT, &(hal_data->ir), comp_id, "rgblookup.ir");
    retval = hal_pin_s32_newf(HAL_OUT, &(hal_data->ig), comp_id, "rgblookup.ig");
    retval = hal_pin_s32_newf(HAL_OUT, &(hal_data->ib), comp_id, "rgblookup.ib");
    retval = hal_pin_u32_newf(HAL_IN, &(hal_data->index), comp_id, "rgblookup.index");
    if (retval != 0)
        return retval;

    retval = hal_export_funct("rgblookup", write, hal_data, 1, 0, comp_id); //needs fp?
    if (retval < 0) {
        rtapi_print_msg(RTAPI_MSG_ERR, "rgblookup: ERROR: function export failed\n");
        return -1;
    }
    
    hal_ready(comp_id);

    return 0;
}


void rtapi_app_exit(void)
{
    hal_exit(comp_id);
}
