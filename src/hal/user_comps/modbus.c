/* 
   Copyright (C) 2001-2005 Stéphane Raimbault <stephane.raimbault@free.fr>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
   
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.
   
   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA.

   The functions included here have been derived from the Modicon
   Modbus Protocol Reference Guide which can be obtained from
   Schneider at www.schneiderautomation.com.	
   
   Documentation:
   http://www.easysw.com/~mike/serial/serial.html
*/

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termio.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>

/* TCP */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include "modbus.h"

#define UNKNOWN_ERROR_MSG "Not defined in modbus specification"

static const int SIZE_TAB_ERROR_MSG = 12;
static const char *TAB_ERROR_MSG[] = {
	/* 0x00 */ UNKNOWN_ERROR_MSG,
	/* 0x01 */ "Illegal function code",
	/* 0x02 */ "Illegal data address",
	/* 0x03 */ "Illegal data value",
	/* 0x04 */ "Slave device or server failure",
	/* 0x05 */ "Acknowledge",
	/* 0x06 */ "Slave device or server busy",
	/* 0x07 */ "Negative acknowledge",
	/* 0x08 */ "Memory parity error",
	/* 0x09 */ UNKNOWN_ERROR_MSG,
	/* 0x0A */ "Gateway path unavailable",
	/* 0x0B */ "Target device failed to respond"
};

/* Table of CRC values for high-order byte */
static unsigned char table_crc_hi[] = {
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static unsigned char table_crc_lo[] = {
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 
  0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD, 
  0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A, 
  0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 
  0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3, 
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 
  0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4, 
  0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29, 
  0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 
  0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26, 
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 
  0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67, 
  0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68, 
  0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 
  0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5, 
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 
  0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92, 
  0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B, 
  0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 
  0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C, 
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 
  0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

/* Local declaration */
static int read_reg_response(modbus_param_t *mb_param, 
			     int *data_dest,
	unsigned char *query);

static void error_treat(int ret, const char *message, modbus_param_t *mb_param)
{
  if (!mb_param->print_errors)
    return;
  if (ret == -1)
    perror(message);
  g_print("\n\nERROR %s\n\n", message);

  if (mb_param->type_com == RTU) {
    tcflush(mb_param->fd, TCIOFLUSH);
  } else {
    modbus_close(mb_param);
    modbus_connect(mb_param);
  }
}

static unsigned int compute_response_size(modbus_param_t *mb_param, 
					  unsigned char *query)
{
  int response_size_computed;
  int offset;

  offset = mb_param->header_length;

  switch (query[offset + 1]) {
    case 0x01: {
      /* Header + nb values (code from force_multiple_coils) */
      int coil_count = (query[offset + 4] << 8) | query[offset + 5];
      response_size_computed = 3 +
	  (coil_count / 8) + ((coil_count % 8) ? 1 : 0);
    }	break;
    case 0x03:
    case 0x04:
      /* Header + 2 * nb values */
      response_size_computed = 3 + 
	  2 * (query[offset + 4] << 8 | query[offset + 5]);
      break;
    case 0x07:
      response_size_computed = 4;
      break;
    default:
      response_size_computed = 6;
  }

  response_size_computed += offset + mb_param->checksum_size;

  return response_size_computed;
}

/* The following functions construct the required query into
   a modbus query packet */
int build_request_packet_rtu(int slave, int function, int start_addr,
			     int count, unsigned char *packet)
{
  packet[0] = slave;
  packet[1] = function;
  packet[2] = start_addr >> 8;
  packet[3] = start_addr & 0x00ff;
  packet[4] = count >> 8;
  packet[5] = count & 0x00ff;

  return PRESET_QUERY_SIZE_RTU;
}

int build_request_packet_tcp(int slave, int function, int start_addr,
			     int count, unsigned char *packet)
{
  static unsigned short t_id = 0;

  /* Transaction ID */
  if (t_id < USHRT_MAX)
    t_id++;
  else
    t_id = 0;
  packet[0] = t_id >> 8;
  packet[1] = t_id & 0x00ff;

  /* Protocol Modbus */
  packet[2] = 0;
  packet[3] = 0;

  /* Length to fix later with set_packet_length_tcp (4 and 5) */

  packet[6] = 0xFF;
  packet[7] = function;
  packet[8] = start_addr >> 8;
  packet[9] = start_addr & 0x00ff;
  packet[10] = count >> 8;
  packet[11] = count & 0x00ff;

  return PRESET_QUERY_SIZE_TCP;
}

void set_packet_length_tcp(unsigned char *packet, size_t packet_size)
{
  unsigned short mbap_length;

  /* Substract MBAP header length */
  mbap_length = packet_size - 6;

  packet[4] = mbap_length >> 8;
  packet[5] = mbap_length & 0x00FF;
}

int build_request_packet(modbus_param_t *mb_param, int slave, 
			 int function, int start_addr,
    int count, unsigned char *packet)
{
  if (mb_param->type_com == RTU)
    return build_request_packet_rtu(slave, function, start_addr,
				    count, packet);
  else
    return build_request_packet_tcp(slave, function, start_addr,
				    count, packet);
}

/* Fast CRC */
static unsigned short crc16(unsigned char *buffer,
			    unsigned short buffer_length)
{
  unsigned char crc_hi = 0xFF; /* high CRC byte initialized */
  unsigned char crc_lo = 0xFF; /* low CRC byte initialized */
  unsigned int i; /* will index into CRC lookup */

  /* pass through message buffer */
  while (buffer_length--) {
    i = crc_hi ^ *buffer++; /* calculate the CRC  */
    crc_hi = crc_lo ^ table_crc_hi[i];
    crc_lo = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}

/* Function to send a query out to a modbus slave */
static int modbus_query(modbus_param_t *mb_param, unsigned char *query,
			size_t query_size)
{
  int write_ret;
  unsigned short s_crc;
  int i;
	
  if (mb_param->type_com == RTU) {
    s_crc = crc16(query, query_size);
    query[query_size++] = s_crc >> 8;
    query[query_size++] = s_crc & 0x00FF;
  } else {
    set_packet_length_tcp(query, query_size);
  }

  if (mb_param->debug) {
    g_print("\n");
    for (i = 0; i < query_size; i++)
      g_print("[%.2X]", query[i]);

    g_print("\n");
  }
	
  if (mb_param->type_com == RTU)
    write_ret = write(mb_param->fd, query, query_size);
  else
    write_ret = send(mb_param->fd, query, query_size, 0);

	/* Return the number of bytes written (0 to n)
  or PORT_SOCKET_FAILURE on error */
  if ((write_ret == -1) || (write_ret != query_size)) {
    error_treat(write_ret, "Write port/socket failure", mb_param);
    write_ret = PORT_SOCKET_FAILURE;
  }
	
  return write_ret;
}

#define WAIT_DATA()									\
{											\
        while ((select_ret = select(mb_param->fd+1, &rfds, NULL, NULL, &tv)) == -1) {	\
	        if (errno == EINTR) {							\
	        	if (mb_param->print_errors)					\
		        	g_print("A non blocked signal was caught\n");		\
		        /* Necessary after an error */					\
		        FD_ZERO(&rfds);							\
			FD_SET(mb_param->fd, &rfds);					\
} else {								\
			error_treat(select_ret, "Select failure", mb_param);		\
			return SELECT_FAILURE;						\
}									\
}										\
											\
        if (select_ret == 0) {								\
                /* Call to error_treat is done later to manage exceptions */		\
		return COMM_TIME_OUT;							\
}										\
}

/* Function to monitor for the reply from the modbus slave.
   This function blocks for timeout seconds if there is no reply.

   Returns:
   - error_code 0 == OK, < 0 == error
   - (arg) total number of characters received.
*/
int receive_response(modbus_param_t *mb_param,
		     int response_size_computed,
       unsigned char *response,
       int *response_size)
{
  int select_ret;
  int read_ret;
  fd_set rfds;
  struct timeval tv;
  int size_to_read;
  unsigned char *p_response;

  if (mb_param->debug)
    g_print("Waiting for response (%d)...\n", response_size_computed);

  /* Add a file descriptor to the set */
  FD_ZERO(&rfds);
  FD_SET(mb_param->fd, &rfds);
	
  /* Wait for a response */
  tv.tv_sec = 0;
  tv.tv_usec = TIME_OUT_BEGIN_OF_TRAME;

  WAIT_DATA();

  /* Read the trame */
  (*response_size) = 0;
  size_to_read = response_size_computed;
  p_response = response;

  while (select_ret) {
    if (mb_param->type_com == RTU)
      read_ret = read(mb_param->fd, p_response, size_to_read);
    else
      read_ret = recv(mb_param->fd, p_response, size_to_read, 0);

    if (read_ret == -1) {
      error_treat(read_ret, "Read port/socket failure", mb_param);
      return PORT_SOCKET_FAILURE;
    } else {
      /* Sums bytes received */ 
      (*response_size) += read_ret;
			
			/* Display the hex code of each
			* character received */
      if (mb_param->debug) {
	int i;
	for (i=0; i < read_ret; i++)
	  g_print("<%.2X>", p_response[i]);
      }
      /* Moves the pointer to receive other datas */
      p_response = &(p_response[read_ret]);
      size_to_read = response_size_computed - (*response_size); 

      if ((*response_size) > MAX_PACKET_SIZE) {
	error_treat(0, "Too many datas", mb_param);
	return TOO_MANY_DATAS;
      }
    }

    if (size_to_read > 0) {
			/* If no character at the buffer wait
      TIME_OUT_END_OF_TRAME before to generate an error.
			*/
      tv.tv_sec = 0;
      tv.tv_usec = TIME_OUT_END_OF_TRAME;
			
      WAIT_DATA();
    } else {
      /* All chars are received */
      select_ret = FALSE;
    }
  }
	
  if (mb_param->debug)
    g_print("\n");

  /* OK */
  return 0;
}

static int check_crc16(modbus_param_t *mb_param,
		       unsigned char *response,
	 int response_size)
{
  int ret;
	
  if (mb_param->type_com == RTU) {
    unsigned short crc_calc;
    unsigned short crc_received;
		
    crc_calc = crc16(response, response_size - 2);
		
    crc_received = response[response_size - 2];
    crc_received = (unsigned) crc_received << 8;
    crc_received = crc_received | 
	(unsigned) response[response_size - 1];
		
    /* Check CRC of response */
    if (crc_calc == crc_received) {
      ret = TRUE;
    } else {
      char *message;
      message = g_strdup_printf(
				"invalid crc received %0X - crc_calc %0X", 
    crc_received, crc_calc);
      error_treat(0, message, mb_param);
      g_free(message);
      ret = INVALID_CRC;
    }
  } else {
		/* In TCP, CRC doesn't exist but it doesn't check
    length because it's not really useful */ 
    ret = TRUE;
  }

  return ret;
}

/* Function to the correct response is returned and that the checksum
   is correct.

   Returns:
   - the numbers of values (bits or word) if success
   - less than 0 for exception errors

   Note: All functions used for sending or receiving data via modbus
   return these values.
*/

static int modbus_response(modbus_param_t *mb_param, 
			   unsigned char *query,
      unsigned char *response)
{
  int response_size;
  int response_size_computed;
  int offset = mb_param->header_length;
  int error_code;

  response_size_computed = compute_response_size(mb_param, query);
  error_code = receive_response(mb_param, response_size_computed,
				response, &response_size);
  if (error_code == 0) {
    /* No error */
    int ret;

    ret = check_crc16(mb_param, response, response_size);
    if (ret != TRUE)
      return ret;

    /* Good response */
    switch (response[offset + 1]) {
      case 0x01:
      case 0x02:
	/* Read functions 1 value = 1 byte */
	response_size = response[offset + 2];
	break;
      case 0x03:
      case 0x04:
	/* Read functions 1 value = 2 bytes */
	response_size = response[offset + 2] / 2;
	break;
      case 0x0F:
      case 0x10:
	/* N Write functions */
	response_size = response[offset + 4] << 8 |
	    response[offset + 5];
	break;
      case 0x11:
	/* Report slave ID (bytes received) */
	break;
      default:
	/* 1 Write functions & others */
	response_size = 1;
    }

  } else if (error_code == COMM_TIME_OUT &&
	     response_size == offset + 3 + mb_param->checksum_size) {
		/* Optimisation allowed because exception response is
	       the smallest trame in modbus protocol (3) so always
	       raise an timeout error */
	       int ret;
		
	       /* CRC */
	       ret = check_crc16(mb_param, response, response_size);
	       if (ret != TRUE)
		 return ret;

		/* Check for exception response
	       0x80 + function */
	       if (0x80 + query[offset + 1] == response[offset + 1]) {

		 if (response[offset + 2] < SIZE_TAB_ERROR_MSG) {
		   error_treat(0,
			       TAB_ERROR_MSG[response[offset + 2]],
	  mb_param);
		   /* Modbus error code (negative) */
		   return -response[offset + 2];
		 } else {
				/* The chances are low to hit this
		   case but can avoid a vicious
		   segfault */
		   char *message;
		   message = g_strdup_printf(
					     "Invalid exception code %d",
	  response[offset + 2]);
		   error_treat(0, message, mb_param);
		   g_free(message);
		   return INVALID_EXCEPTION_CODE;
		 }
	       }
	     } else if (error_code == COMM_TIME_OUT) {
	       error_treat(0, "Communication time out", mb_param);
	       return COMM_TIME_OUT;
	     } else {
	       return error_code;
	     }

	     return response_size;
}

/* Read IO status */
static int read_io_status(modbus_param_t *mb_param, int slave, int function,
			  int start_addr, int count, int *data_dest)
{
  int query_size;
  int query_ret;
  int response_ret;

  unsigned char query[MIN_QUERY_SIZE];
  unsigned char response[MAX_PACKET_SIZE];

  query_size = build_request_packet(mb_param, slave, function, 
				    start_addr, count, query);

  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0) {
    int i, temp, bit;
    int pos = 0;
    int processed = 0;
    int offset;
    int offset_length;

    response_ret = modbus_response(mb_param, query, response);
    offset = mb_param->header_length;

    offset_length = offset + response_ret;		
    for (i = offset; i < offset_length; i++) {
      /* Shift reg hi_byte to temp */
      temp = response[3 + i];
			
      for (bit = 0x01;
	   (bit & 0xff) && (processed < count);) {
	     data_dest[pos++] = 
		 (temp & bit) ? TRUE : FALSE;
	     processed++;
	     bit = bit << 1;
	   }
			
    }
  } else {
    response_ret = query_ret;
  }

  return response_ret;
}

/* Reads the boolean status of coils and sets the array elements
   in the destination to TRUE or FALSE */
int read_coil_status(modbus_param_t *mb_param, int slave, int start_addr,
		     int count, int *data_dest)
{
  int function = 0x01;
  int status;

  status = read_io_status(mb_param, slave, function, start_addr,
			  count, data_dest);

  if (status > 0)
    status = count;
	
  return status;
}

/* Same as read_coil_status but reads the slaves input table */
int read_input_status(modbus_param_t *mb_param, int slave, int start_addr,
		      int count, int *data_dest)
{
  int function = 0x02;
  int status;

  status = read_io_status(mb_param, slave, function, start_addr,
			  count, data_dest);

  if (status > 0)
    status = count;

  return status;
}

/* Read the data from a modbus slave and put that data into an array */
static int read_registers(modbus_param_t *mb_param, int slave, int function,
			  int start_addr, int count, int *data_dest)
{
  int query_size;
  int status;
  int query_ret;
  unsigned char query[MIN_QUERY_SIZE];

  query_size = build_request_packet(mb_param, slave, function, 
				    start_addr, count, query);

  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0)
    status = read_reg_response(mb_param, data_dest, query);
  else
    status = query_ret;
	
  return status;
}

/* Read the holding registers in a slave and put the data into an
   array */
int read_holding_registers(modbus_param_t *mb_param, int slave,
			   int start_addr, int count, int *data_dest)
{
  int function = 0x03;
  int status;

  if (count > MAX_READ_HOLD_REGS) {
    if (mb_param->print_errors)
	    g_print("WARNING Too many holding registers requested\n");
    count = MAX_READ_HOLD_REGS;
  }

  status = read_registers(mb_param, slave, function,
			  start_addr, count, data_dest);
  return status;
}

/* Read the input registers in a slave and put the data into
   an array */
int read_input_registers(modbus_param_t *mb_param, int slave,
			 int start_addr, int count, int *data_dest)
{
  int function = 0x04;
  int status;

  if (count > MAX_READ_INPUT_REGS) {
    if (mb_param->print_errors)
	    g_print("WARNING Too many input registers requested\n");
    count = MAX_READ_INPUT_REGS;
  }

  status = read_registers(mb_param, slave, function,
			  start_addr, count, data_dest);

  return status;
}

/* Reads the response data from a slave and puts the data into an
   array */
static int read_reg_response(modbus_param_t *mb_param, int *data_dest,
			     unsigned char *query)
{
  unsigned char response[MAX_PACKET_SIZE];
  int response_ret;
  int offset;
  int i;

  response_ret = modbus_response(mb_param, query, response);
	
  offset = mb_param->header_length;

  /* If response_ret is negative, the loop is jumped ! */
  for (i = 0; i < response_ret; i++) {
    /* shift reg hi_byte to temp OR with lo_byte */
    data_dest[i] = response[offset + 3 + (i << 1)] << 8 | 
	response[offset + 4 + (i << 1)];    
  }
	
  return response_ret;
}

/* Gets the raw data from the input stream */
static int preset_response(modbus_param_t *mb_param, unsigned char *query) 
{
  int ret;
  unsigned char response[MAX_PACKET_SIZE];

  ret = modbus_response(mb_param, query, response);

  return ret;
}

/* Sends a value to a register in a slave */
static int set_single(modbus_param_t *mb_param, int slave, int function,
		      int addr, int value)
{
  int status;
  int query_size;
  int query_ret;
  unsigned char query[MAX_PACKET_SIZE];

  query_size = build_request_packet(mb_param, slave, function, 
				    addr, value, query);

  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0)
    status = preset_response(mb_param, query);
  else
    status = query_ret;

  return status;
}


/* Turn on or off a single coil on the slave device */
int force_single_coil(modbus_param_t *mb_param, int slave,
		      int coil_addr, int state)
{
  int function = 0x05;
  int status;

  if (state)
    state = 0xFF00;

  status = set_single(mb_param, slave, function, coil_addr, state);

  return status;
}

/* Sets a value in one holding register in the slave device */
int preset_single_register(modbus_param_t *mb_param, int slave,
			   int reg_addr, int value)
{
  int function = 0x06;
  int status;

  status = set_single(mb_param, slave, function, reg_addr, value);

  return status;
}

/* Takes an array of ints and sets or resets the coils on a slave
   appropriatly */
int force_multiple_coils(modbus_param_t *mb_param, int slave,
			 int start_addr, int coil_count,
    int *data_src)
{
  int function = 0x0F;
  int i;
  int byte_count;
  int query_size;
  int coil_check = 0;
  int status;
  int query_ret;

  unsigned char query[MAX_PACKET_SIZE];

  if (coil_count > MAX_WRITE_COILS) {
    if (mb_param->print_errors)
	    g_print("WARNING Writing to too many coils\n");
    coil_count = MAX_WRITE_COILS;
  }

  query_size = build_request_packet(mb_param, slave, function, 
				    start_addr, coil_count, query);
  byte_count = (coil_count / 8) + ((coil_count % 8) ? 1 : 0);
  query[query_size++] = byte_count;

  for (i = 0; i < byte_count; i++) {
    int bit;
    int pos = 0;

    bit = 0x01;
    query[query_size] = 0;

    while ((bit & 0xFF) && (coil_check++ < coil_count)) {
      if (data_src[pos++])
	query[query_size] |= bit;
      else
	query[query_size] &=~ bit;
			
      bit = bit << 1;
    }
    query_size++;
  }

  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0)
    status = preset_response(mb_param, query);
  else
    status = query_ret;

  return status;
}

/* Copy the values in an array to an array on the slave */
int preset_multiple_registers(modbus_param_t *mb_param, int slave,
			      int start_addr, int reg_count, int *data_src)
{
  int function = 0x10;
  int i;
  int query_size;
  int byte_count;
  int status;
  int query_ret;

  unsigned char query[MAX_PACKET_SIZE];

  if (reg_count > MAX_WRITE_REGS) {
    if (mb_param->print_errors)
	    g_print("WARNING Trying to write to too many registers\n");
    reg_count = MAX_WRITE_REGS;
  }

  query_size = build_request_packet(mb_param, slave, function, 
				    start_addr, reg_count, query);
  byte_count = reg_count * 2;
  query[query_size++] = byte_count;

  for (i = 0; i < reg_count; i++) {
    query[query_size++] = data_src[i] >> 8;
    query[query_size++] = data_src[i] & 0x00FF;
  }

  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0)
    status = preset_response(mb_param, query);
  else
    status = query_ret;

  return status;
}

/* Returns the slave id ! */
int report_slave_id(modbus_param_t *mb_param, int slave, 
		    unsigned char *data_dest)
{
  int function = 0x11;
  int query_size;
  int query_ret;
  int response_ret;

  unsigned char query[MIN_QUERY_SIZE];
  unsigned char response[MAX_PACKET_SIZE];
	
  query_size = build_request_packet(mb_param, slave, function, 
				    0, 0, query);
	
  /* start_addr and count are not used */
  query_size -= 4;
	
  query_ret = modbus_query(mb_param, query, query_size);
  if (query_ret > 0) {
    int i;
    int offset;
    int offset_length;

		/* Byte count, slave id, run indicator status,
    additional data */
    response_ret = modbus_response(mb_param, query, response);
		
    offset = mb_param->header_length;
    offset_length = offset + response_ret;

    for (i = offset; i < offset_length; i++)
      data_dest[i] = response[i];
  } else {
    response_ret = query_ret;
  }

  return response_ret;
}

/* Initialises the modbus_param_t structure for RTU */
void modbus_init_rtu(modbus_param_t *mb_param, char *device,
		     int baud_i, char *parity, int data_bit,
       int stop_bit, int print_errors)
{
  memset(mb_param, 0, sizeof(modbus_param_t));
  strcpy(mb_param->device, device);
  mb_param->baud_i = baud_i;
  strcpy(mb_param->parity, parity);
  mb_param->debug = FALSE;
  mb_param->data_bit = data_bit;
  mb_param->stop_bit = stop_bit;
  mb_param->type_com = RTU;
  mb_param->header_length = HEADER_LENGTH_RTU;
  mb_param->checksum_size = CHECKSUM_SIZE_RTU;
  mb_param->print_errors = print_errors;
}

/* Initialises the modbus_param_t structure for TCP */
void modbus_init_tcp(modbus_param_t *mb_param, char *ip, int print_errors)
{
  memset(mb_param, 0, sizeof(modbus_param_t));
  strncpy(mb_param->ip, ip, sizeof(char)*16);
  mb_param->type_com = TCP;
  mb_param->header_length = HEADER_LENGTH_TCP;
  mb_param->checksum_size = CHECKSUM_SIZE_TCP;
  mb_param->print_errors = print_errors;
}


/* This function sets up a serial port for RTU communications to
   modbus */
static int modbus_connect_rtu(modbus_param_t *mb_param)
{
  struct termios tios;
  speed_t baud_rate;

  if (mb_param->debug) {
    g_print("Opening %s at %d bauds (%s)\n",
	    mb_param->device, mb_param->baud_i, mb_param->parity);
  }

	/* The O_NOCTTY flag tells UNIX that this program doesn't want
  to be the "controlling terminal" for that port. If you
  don't specify this then any input (such as keyboard abort
  signals and so forth) will affect your process

  Timeouts are ignored in canonical input mode or when the
  NDELAY option is set on the file via open or fcntl */
  mb_param->fd = open(mb_param->device, O_RDWR | O_NOCTTY | O_NDELAY);
  if (mb_param->fd < 0) {
    perror("open");
    g_print("ERROR Opening device %s (no : %d)\n",
	    mb_param->device, errno);
    return -1;
  }

  /* Save */
  tcgetattr(mb_param->fd, &(mb_param->old_tios));

  memset(&tios, 0, sizeof(struct termios));
	
	/* C_ISPEED     Input baud (new interface)
  C_OSPEED	Output baud (new interface)
	*/
  switch (mb_param->baud_i) {
    case 110:
      baud_rate = B110;
      break;
    case 300:
      baud_rate = B300;
      break;
    case 600:
      baud_rate = B600;
      break;
    case 1200:
      baud_rate = B1200;
      break;
    case 2400:
      baud_rate = B2400;
      break;
    case 4800:
      baud_rate = B4800;
      break;
    case 9600: 
      baud_rate = B9600;
      break;
    case 19200:
      baud_rate = B19200;
      break;
    case 38400:
      baud_rate = B38400;
      break;
    case 57600:
      baud_rate = B57600;
      break;
    case 115200:
      baud_rate = B115200;
      break;
    default:
      baud_rate = B9600;
    if (mb_param->print_errors)
	      g_print("WARNING Unknown baud rate %d for %s (B9600 used)\n",
	      mb_param->baud_i, mb_param->device);
  }

  /* Set the baud rate */
  if ((cfsetispeed(&tios, baud_rate) < 0) ||
       (cfsetospeed(&tios, baud_rate) < 0)) {
    perror("cfsetispeed/cfsetospeed\n");
    return -1;
       }
	
	/* C_CFLAG	Control options
       CLOCAL	Local line - do not change "owner" of port
       CREAD	Enable receiver
	*/
       tios.c_cflag |= (CREAD | CLOCAL);
       /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

	/* Set data bits (5, 6, 7, 8 bits)
       CSIZE	Bit mask for data bits
	*/
       tios.c_cflag &= ~CSIZE;
       switch (mb_param->data_bit) {
	 case 5:
	   tios.c_cflag |= CS5;
	   break;
	 case 6:
	   tios.c_cflag |= CS6;
	   break;
	 case 7:
	   tios.c_cflag |= CS7;
	   break;
	 case 8:
	 default:
	   tios.c_cflag |= CS8;
	   break;
       }

       /* Stop bit (1 or 2) */
       if (mb_param->stop_bit == 1)
	 tios.c_cflag &=~ CSTOPB;
       else /* 2 */
	 tios.c_cflag |= CSTOPB;

	/* PARENB	Enable parity bit
       PARODD	Use odd parity instead of even */
       if (strncmp(mb_param->parity, "none", 4) == 0) {
	 tios.c_cflag &=~ PARENB;
       } else if (strncmp(mb_param->parity, "even", 4) == 0) {
	 tios.c_cflag |= PARENB;
	 tios.c_cflag &=~ PARODD;
       } else {
	 /* odd */
	 tios.c_cflag |= PARENB;
	 tios.c_cflag |= PARODD;
       }
	
	/* Read your man page for the meaning of all this (man
       termios). Its a bit to involved to comment here :) */
       tios.c_line = 0; 

	/* C_LFLAG	Line options 

       ISIG	Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON	Enable canonical input (else raw)
       XCASE	Map uppercase \lowercase (obsolete)
       ECHO	Enable echoing of input characters
       ECHOE	Echo erase character as BS-SP-BS
       ECHOK	Echo NL after kill character
       ECHONL	Echo NL
       NOFLSH	Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN	Enable extended functions
       ECHOCTL	Echo control characters as ^char and delete as ~?
       ECHOPRT	Echo erased character as character erased
       ECHOKE	BS-SP-BS entire line on line kill
       FLUSHO	Output being flushed
       PENDIN	Retype pending input at next read or input char
       TOSTOP	Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.  

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
	*/

       /* Raw input */
       tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

	/* C_IFLAG	Input options 

       Constant	Description
       INPCK	Enable parity check
       IGNPAR	Ignore parity errors
       PARMRK	Mark parity errors
       ISTRIP	Strip parity bits
       IXON	Enable software flow control (outgoing)
       IXOFF	Enable software flow control (incoming)
       IXANY	Allow any character to start flow again
       IGNBRK	Ignore break condition
       BRKINT	Send a SIGINT when a break condition is detected
       INLCR	Map NL to CR
       IGNCR	Ignore CR
       ICRNL	Map CR to NL
       IUCLC	Map uppercase to lowercase
       IMAXBEL	Echo BEL on input line too long
	*/
       if (strncmp(mb_param->parity, "none", 4) == 0) {
	 tios.c_iflag &= ~INPCK;
       } else {
	 tios.c_iflag |= INPCK;
       }

       /* Software flow control is disabled */
       tios.c_iflag &= ~(IXON | IXOFF | IXANY);
	
	/* C_OFLAG	Output options
       OPOST	Postprocess output (not set = raw output)
       ONLCR	Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
	*/	   

       /* Raw ouput */
       tios.c_oflag &=~ OPOST;

	/* C_CC         Control characters 
       VMIN	        Minimum number of characters to read
       VTIME	Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
	*/
       /* Unused because we use open with the NDELAY option */
       tios.c_cc[VMIN] = 0;
       tios.c_cc[VTIME] = 0;

       if (tcsetattr(mb_param->fd, TCSANOW, &tios) < 0) {
	 perror("tcsetattr\n");
	 return -1;
       }

       return 0;
}

static int modbus_connect_tcp(modbus_param_t *mb_param)
{
  int ret;
  int option;
  struct sockaddr_in addr;

  addr.sin_family = AF_INET;
  addr.sin_port = htons(MODBUS_TCP_PORT);
  addr.sin_addr.s_addr = inet_addr(mb_param->ip);

  mb_param->fd = socket(AF_INET, SOCK_STREAM, 0);
  if (mb_param->fd < 0) {
    return mb_param->fd;
  }

  /* Set the TCP no delay flag */
  /* SOL_TCP = IPPROTO_TCP */
  option = 1;
  ret = setsockopt(mb_param->fd, SOL_TCP, TCP_NODELAY,
		   (const void *)&option, sizeof(int));
  if (ret < 0) {
    perror("setsockopt");
    close(mb_param->fd);
    return ret;
  }

  /* Set the IP low delay option */
  option = IPTOS_LOWDELAY;
  ret = setsockopt(mb_param->fd, SOL_TCP, IP_TOS,
		   (const void *)&option, sizeof(int));
  if (ret < 0) {
    perror("setsockopt");
    close(mb_param->fd);
    return ret;
  }

  if (mb_param->debug) {
    g_print("Connecting to %s\n", mb_param->ip);
  }
	
  ret = connect(mb_param->fd, (struct sockaddr *)&addr,
		sizeof(struct sockaddr_in));
  if (ret < 0) {
    perror("connect");
    close(mb_param->fd);
    return ret;
  }

  return 0;
}

int modbus_connect(modbus_param_t *mb_param)
{
  int ret;

  if (mb_param->type_com == RTU)
    ret = modbus_connect_rtu(mb_param);
  else
    ret = modbus_connect_tcp(mb_param);

  return ret;
}

void modbus_listen_tcp(modbus_param_t *mb_param)
{
  int ret;
  int new_socket;
  struct sockaddr_in addr;
  socklen_t addrlen;

  addr.sin_family = AF_INET;
	/* The modbus port is < 1024
  This program must be made setuid root. */
  addr.sin_port = htons(MODBUS_TCP_PORT);
  addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(addr.sin_zero), '\0', 8);

  new_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (new_socket < 0) {
    perror("socket");
    exit(1);
  } else {
    if (mb_param->print_errors)
	    g_print("Socket OK\n");
  }

  ret = bind(new_socket, (struct sockaddr *)&addr,
	     sizeof(struct sockaddr_in));
  if (ret < 0) {
    perror("bind");
    close(new_socket);
    exit(1);
  } else {
    if (mb_param->print_errors)
	    g_print("Bind OK\n");
  }

  ret = listen(new_socket, 1);
  if (ret != 0) {
    perror("listen");
    close(new_socket);
    exit(1);
  } else {
    if (mb_param->print_errors)
	    g_print("Listen OK\n");
  }

  addrlen = sizeof(struct sockaddr_in);
  mb_param->fd = accept(new_socket, (struct sockaddr *)&addr, &addrlen);
  if (ret < 0) {
    perror("accept");
    close(new_socket);
    exit(1);
  } else {
    unsigned char response[MAX_PACKET_SIZE];
    int response_size;

    if (mb_param->print_errors)
	    g_print("The client %s is connected\n", 
		    inet_ntoa(addr.sin_addr));

    receive_response(mb_param, MAX_PACKET_SIZE,
		     response, &response_size);
  }

  close(new_socket);
}

/* Close the file descriptor */
static void modbus_close_rtu(modbus_param_t *mb_param)
{
  if (tcsetattr(mb_param->fd, TCSANOW, &(mb_param->old_tios)) < 0)
    perror("tcsetattr");
	
  close(mb_param->fd);
}

static void modbus_close_tcp(modbus_param_t *mb_param)
{
  shutdown(mb_param->fd, SHUT_RDWR);
  close(mb_param->fd);
}

void modbus_close(modbus_param_t *mb_param)
{
  if (mb_param->type_com == RTU)
    modbus_close_rtu(mb_param);
  else
    modbus_close_tcp(mb_param);
}

void modbus_set_debug(modbus_param_t *mb_param, int boolean)
{
  mb_param->debug = boolean;
}
