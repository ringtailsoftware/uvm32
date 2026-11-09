/*
 * Copyright (c) 2016-2022 Matt Borgerson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef GDBSTUB_H
#define GDBSTUB_H

/* Enable debug statements (printf) */
#ifndef DEBUG
#define DEBUG 0
#endif

/*****************************************************************************
 *
 *  Mock
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_MOCK

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef unsigned int address;
typedef unsigned int reg;

enum GDB_REGISTER {
    GDB_CPU_NUM_REGISTERS = 4
};

struct gdb_state {
    int signum;
    reg registers[GDB_CPU_NUM_REGISTERS];
};

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

struct gdb_buffer;
extern struct gdb_buffer gdb_input, gdb_output;

void gdb_buf_write(struct gdb_buffer *buf, int ch);
int gdb_buf_read(struct gdb_buffer *buf);

#endif /* GDBSTUB_ARCH_MOCK */

/*****************************************************************************
 *
 *  x86
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_X86

/*****************************************************************************
 * Types
 ****************************************************************************/

#ifndef GDBSTUB_DONT_DEFINE_STDINT_TYPES
typedef unsigned char  uint8_t;
typedef unsigned short uint16_t;
typedef unsigned long  uint32_t;
#endif

typedef unsigned int address;
typedef unsigned int reg;

enum GDB_REGISTER {
    GDB_CPU_I386_REG_EAX  = 0,
    GDB_CPU_I386_REG_ECX  = 1,
    GDB_CPU_I386_REG_EDX  = 2,
    GDB_CPU_I386_REG_EBX  = 3,
    GDB_CPU_I386_REG_ESP  = 4,
    GDB_CPU_I386_REG_EBP  = 5,
    GDB_CPU_I386_REG_ESI  = 6,
    GDB_CPU_I386_REG_EDI  = 7,
    GDB_CPU_I386_REG_PC   = 8,
    GDB_CPU_I386_REG_PS   = 9,
    GDB_CPU_I386_REG_CS   = 10,
    GDB_CPU_I386_REG_SS   = 11,
    GDB_CPU_I386_REG_DS   = 12,
    GDB_CPU_I386_REG_ES   = 13,
    GDB_CPU_I386_REG_FS   = 14,
    GDB_CPU_I386_REG_GS   = 15,
    GDB_CPU_NUM_REGISTERS = 16
};

struct gdb_state {
    int signum;
    reg registers[GDB_CPU_NUM_REGISTERS];
};

#endif /* GDBSTUB_ARCH_X86 */

/*****************************************************************************
 *
 *  UVM32
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_UVM32

#include "uvm32.h"

typedef uint32_t address;
typedef uint32_t reg;
typedef int32_t (*uvm32_event_callback_t)(uvm32_state_t* vmst, uvm32_evt_t *evt);

typedef enum {
    SIGNONE = 0,
    SIGILL  = 4,
    SIGTRAP = 5,
    SIGSEGV = 11,
} gdbstub_signal_t;

enum GDB_REGISTER {
  /* 32 registers and PC */
  GDB_CPU_NUM_REGISTERS = (32 + 1),
};

struct gdb_state {
  /* Not used, only for compatibility */
  uint8_t signum;
  /* UVM32 state */
  uvm32_state_t* vmst;
  /* TCP file descriptor */
  int socket;
  /* User defined callback to handle events */
  uvm32_event_callback_t event_cb;
   /* Will be upated with `vmst->_core' registers */
  reg registers[GDB_CPU_NUM_REGISTERS];
};

int32_t gdb_sys_init_state(struct gdb_state* state,
                           uvm32_state_t* vmst,
                           int socket,
                           uvm32_event_callback_t event_cb);

#endif /* GDBSTUB_ARCH_UVM32 */

/*****************************************************************************
 *
 *  GDB Remote Serial Protocol
 *
 ****************************************************************************/

/*****************************************************************************
 * Macros
 ****************************************************************************/

#if DEBUG
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define GDB_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
#define GDB_PRINT(...)
#endif

#define GDB_EOF (-1)

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifndef GDB_ASSERT
#if DEBUG
#define GDB_ASSERT(x) { \
	if (!(x)) { \
		fprintf(stderr, "ASSERTION FAILED\n"); \
		fprintf(stderr, "  Assertion: %s\n", #x); \
		fprintf(stderr, "  Location:  %s @ %s:%d\n", __func__, \
		                                             __FILE__, __LINE__); \
		exit(1); \
	} \
}
#else
#define GDB_ASSERT(x) \
	do {} while (0)
#endif
#endif

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

int gdb_main(struct gdb_state *state);

/* System functions, supported by all stubs */
void gdb_sys_init(void);
int gdb_sys_getc(struct gdb_state *state);
int gdb_sys_putchar(struct gdb_state *state, int ch);
int gdb_sys_mem_readb(struct gdb_state *state, address addr, char *val);
int gdb_sys_mem_writeb(struct gdb_state *state, address addr, char val);
int gdb_sys_continue(struct gdb_state *state);
int gdb_sys_step(struct gdb_state *state);

#ifdef GDBSTUB_IMPLEMENTATION

/*****************************************************************************
 * Types
 ****************************************************************************/

typedef int (*gdb_enc_func)(char *buf, unsigned int buf_len, const char *data,
                            unsigned int data_len);
typedef int (*gdb_dec_func)(const char *buf, unsigned int buf_len, char *data,
                            unsigned int data_len);

/*****************************************************************************
 * Const Data
 ****************************************************************************/

static const char digits[] = "0123456789abcdef";

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

/* Communication functions */
static int gdb_write(struct gdb_state *state, const char *buf,
                     unsigned int len);
static int gdb_read(struct gdb_state *state, char *buf, unsigned int buf_len,
                    unsigned int len);

/* String processing helper functions */
static int gdb_strlen(const char *ch);
#if DEBUG
static int gdb_is_printable_char(char ch);
#endif
static char gdb_get_digit(int val);
static int gdb_get_val(char digit, int base);
static int gdb_strtol(const char *str, unsigned int len, int base,
                      const char **endptr);

/* Packet functions */
static int gdb_send_packet(struct gdb_state *state, const char *pkt,
                           unsigned int pkt_len);
static int gdb_recv_packet(struct gdb_state *state, char *pkt_buf,
                           unsigned int pkt_buf_len, unsigned int *pkt_len);
static int gdb_checksum(const char *buf, unsigned int len);
static int gdb_recv_ack(struct gdb_state *state);

/* Data encoding/decoding */
static int gdb_enc_hex(char *buf, unsigned int buf_len, const char *data,
                       unsigned int data_len);
static int gdb_dec_hex(const char *buf, unsigned int buf_len, char *data,
                       unsigned int data_len);
static int gdb_enc_bin(char *buf, unsigned int buf_len, const char *data,
                       unsigned int data_len);
static int gdb_dec_bin(const char *buf, unsigned int buf_len, char *data,
                       unsigned int data_len);

/* Packet creation helpers */
static int gdb_send_ok_packet(struct gdb_state *state, char *buf,
                              unsigned int buf_len);
static int gdb_send_conmsg_packet(struct gdb_state *state, char *buf,
                                  unsigned int buf_len, const char *msg);
static int gdb_send_signal_packet(struct gdb_state *state, char *buf,
                                  unsigned int buf_len, char signal);
static int gdb_send_error_packet(struct gdb_state *state, char *buf,
                                 unsigned int buf_len, char error);

/* Command functions */
static int gdb_mem_read(struct gdb_state *state, char *buf,
                        unsigned int buf_len, address addr, unsigned int len,
                        gdb_enc_func enc);
static int gdb_mem_write(struct gdb_state *state, const char *buf,
                         unsigned int buf_len, address addr, unsigned int len,
                         gdb_dec_func dec);
static int gdb_continue(struct gdb_state *state);
static int gdb_step(struct gdb_state *state);

/*****************************************************************************
 * String Processing Helper Functions
 ****************************************************************************/

/*
 * Get null-terminated string length.
 */
static int gdb_strlen(const char *ch)
{
    int len;

    len = 0;
    while (*ch++) {
        len += 1;
    }

    return len;
}

/*
 * Get integer value for a string representation.
 *
 * If the string starts with + or -, it will be signed accordingly.
 *
 * If base == 0, the base will be determined:
 *   base 16 if the string starts with 0x or 0X,
 *   base 10 otherwise
 *
 * If endptr is specified, it will point to the last non-digit in the
 * string. If there are no digits in the string, it will be set to NULL.
 */
static int gdb_strtol(const char *str, unsigned int len, int base,
                      const char **endptr)
{
    unsigned int pos;
    int sign, tmp, value, valid;

    value = 0;
    pos   = 0;
    sign  = 1;
    valid = 0;

    if (endptr) {
        *endptr = NULL;
    }

    if (len < 1) {
        return 0;
    }

    /* Detect negative numbers */
    if (str[pos] == '-') {
        sign = -1;
        pos += 1;
    } else if (str[pos] == '+') {
        sign = 1;
        pos += 1;
    }

    /* Detect '0x' hex prefix */
    if ((pos + 2 < len) && (str[pos] == '0') &&
        ((str[pos+1] == 'x') || (str[pos+1] == 'X'))) {
        base = 16;
        pos += 2;
    }

    if (base == 0) {
        base = 10;
    }

    for (; (pos < len) && (str[pos] != '\x00'); pos++) {
        tmp = gdb_get_val(str[pos], base);
        if (tmp == GDB_EOF) {
            break;
        }

        value = value*base + tmp;
        valid = 1; /* At least one digit is valid */
    }

    if (!valid) {
        return 0;
    }

    if (endptr) {
        *endptr = str+pos;
    }

    value *= sign;

    return value;
}

/*
 * Get the corresponding ASCII hex digit character for a value.
 */
static char gdb_get_digit(int val)
{
    if ((val >= 0) && (val <= 0xf)) {
        return digits[val];
    } else {
        return GDB_EOF;
    }
}

/*
 * Get the corresponding value for a ASCII digit character.
 *
 * Supports bases 2-16.
 */
static int gdb_get_val(char digit, int base)
{
    int value;

    if ((digit >= '0') && (digit <= '9')) {
        value = digit-'0';
    } else if ((digit >= 'a') && (digit <= 'f')) {
        value = digit-'a'+0xa;
    } else if ((digit >= 'A') && (digit <= 'F')) {
        value = digit-'A'+0xa;
    } else {
        return GDB_EOF;
    }

    return (value < base) ? value : GDB_EOF;
}

#if DEBUG
/*
 * Determine if this is a printable ASCII character.
 */
static int gdb_is_printable_char(char ch)
{
    return (ch >= 0x20 && ch <= 0x7e);
}
#endif

/*****************************************************************************
 * Packet Functions
 ****************************************************************************/

/*
 * Receive a packet acknowledgment
 *
 * Returns:
 *    0   if an ACK (+) was received
 *    1   if a NACK (-) was received
 *    GDB_EOF otherwise
 */
static int gdb_recv_ack(struct gdb_state *state)
{
    int response;

    /* Wait for packet ack */
    switch (response = gdb_sys_getc(state)) {
    case '+':
        /* Packet acknowledged */
        return 0;
    case '-':
        /* Packet negative acknowledged */
        return 1;
    default:
        /* Bad response! */
        GDB_PRINT("received bad packet response: 0x%2x\n", response);
        return GDB_EOF;
    }
}

/*
 * Calculate 8-bit checksum of a buffer.
 *
 * Returns:
 *    8-bit checksum.
 */
static int gdb_checksum(const char *buf, unsigned int len)
{
    unsigned char csum;

    csum = 0;

    while (len--) {
        csum += *buf++;
    }

    return csum;
}

/*
 * Transmits a packet of data.
 * Packets are of the form: $<packet-data>#<checksum>
 *
 * Returns:
 *    0   if the packet was transmitted and acknowledged
 *    1   if the packet was transmitted but not acknowledged
 *    GDB_EOF otherwise
 */
static int gdb_send_packet(struct gdb_state *state, const char *pkt_data,
                           unsigned int pkt_len)
{
    char buf[3];
    char csum;

    /* Send packet start */
    if (gdb_sys_putchar(state, '$') == GDB_EOF) {
        return GDB_EOF;
    }

#if DEBUG
    {
        unsigned int p;
        GDB_PRINT("-> ");
        for (p = 0; p < pkt_len; p++) {
            if (gdb_is_printable_char(pkt_data[p])) {
                GDB_PRINT("%c", pkt_data[p]);
            } else {
                GDB_PRINT("\\x%02x", pkt_data[p]&0xff);
            }
        }
        GDB_PRINT("\n");
    }
#endif

    /* Send packet data */
    if (gdb_write(state, pkt_data, pkt_len) == GDB_EOF) {
        return GDB_EOF;
    }

    /* Send the checksum */
    buf[0] = '#';
    csum = gdb_checksum(pkt_data, pkt_len);
    if ((gdb_enc_hex(buf+1, sizeof(buf)-1, &csum, 1) == GDB_EOF) ||
        (gdb_write(state, buf, sizeof(buf)) == GDB_EOF)) {
        return GDB_EOF;
    }

    return gdb_recv_ack(state);
}

/*
 * Receives a packet of data, assuming a 7-bit clean connection.
 *
 * Returns:
 *    0   if the packet was received
 *    GDB_EOF otherwise
 */
static int gdb_recv_packet(struct gdb_state *state, char *pkt_buf,
                           unsigned int pkt_buf_len, unsigned int *pkt_len)
{
    int data;
    char expected_csum, actual_csum;
    char buf[2];

    /* Wait for packet start */
    actual_csum = 0;

    while (1) {
        data = gdb_sys_getc(state);
        if (data == GDB_EOF) {
            return GDB_EOF;
        } else if (data == '$') {
            /* Detected start of packet. */
            break;
        }
    }

    /* Read until checksum */
    *pkt_len = 0;
    while (1) {
        data = gdb_sys_getc(state);

        if (data == GDB_EOF) {
            /* Error receiving character */
            return GDB_EOF;
        } else if (data == '#') {
            /* End of packet */
            break;
        } else {
            /* Check for space */
            if (*pkt_len >= pkt_buf_len) {
                GDB_PRINT("packet buffer overflow\n");
                return GDB_EOF;
            }

            /* Store character and update checksum */
            pkt_buf[(*pkt_len)++] = (char) data;
        }
    }

#if DEBUG
    {
        unsigned int p;
        GDB_PRINT("<- ");
        for (p = 0; p < *pkt_len; p++) {
            if (gdb_is_printable_char(pkt_buf[p])) {
                GDB_PRINT("%c", pkt_buf[p]);
            } else {
                GDB_PRINT("\\x%02x", pkt_buf[p] & 0xff);
            }
        }
        GDB_PRINT("\n");
    }
#endif

    /* Receive the checksum */
    if ((gdb_read(state, buf, sizeof(buf), 2) == GDB_EOF) ||
        (gdb_dec_hex(buf, 2, &expected_csum, 1) == GDB_EOF)) {
        return GDB_EOF;
    }

    /* Verify checksum */
    actual_csum = gdb_checksum(pkt_buf, *pkt_len);
    if (actual_csum != expected_csum) {
        /* Send packet nack */
        GDB_PRINT("received packet with bad checksum\n");
        gdb_sys_putchar(state, '-');
        return GDB_EOF;
    }

    /* Send packet ack */
    gdb_sys_putchar(state, '+');
    return 0;
}

/*****************************************************************************
 * Data Encoding/Decoding
 ****************************************************************************/

/*
 * Encode data to its hex-value representation in a buffer.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    GDB_EOF if the buffer is too small
 */
static int gdb_enc_hex(char *buf, unsigned int buf_len, const char *data,
                       unsigned int data_len)
{
    unsigned int pos;

    if (buf_len < data_len*2) {
        /* Buffer too small */
        return GDB_EOF;
    }

    for (pos = 0; pos < data_len; pos++) {
        *buf++ = gdb_get_digit((data[pos] >> 4) & 0xf);
        *buf++ = gdb_get_digit((data[pos]     ) & 0xf);
    }

    return data_len*2;
}

/*
 * Decode data from its hex-value representation to a buffer.
 *
 * Returns:
 *    0   if successful
 *    GDB_EOF if the buffer is too small
 */
static int gdb_dec_hex(const char *buf, unsigned int buf_len, char *data,
                       unsigned int data_len)
{
    unsigned int pos;
    int tmp;

    if (buf_len != data_len*2) {
        /* Buffer too small */
        return GDB_EOF;
    }

    for (pos = 0; pos < data_len; pos++) {
        /* Decode high nibble */
        tmp = gdb_get_val(*buf++, 16);
        if (tmp == GDB_EOF) {
            /* Buffer contained junk. */
            GDB_ASSERT(0);
            return GDB_EOF;
        }

        data[pos] = tmp << 4;

        /* Decode low nibble */
        tmp = gdb_get_val(*buf++, 16);
        if (tmp == GDB_EOF) {
            /* Buffer contained junk. */
            GDB_ASSERT(0);
            return GDB_EOF;
        }
        data[pos] |= tmp;
    }

    return 0;
}

/*
 * Encode data to its binary representation in a buffer.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    GDB_EOF if the buffer is too small
 */
static int gdb_enc_bin(char *buf, unsigned int buf_len, const char *data,
                       unsigned int data_len)
{
    unsigned int buf_pos, data_pos;

    for (buf_pos = 0, data_pos = 0; data_pos < data_len; data_pos++) {
        if (data[data_pos] == '$' ||
            data[data_pos] == '#' ||
            data[data_pos] == '}' ||
            data[data_pos] == '*') {
            if (buf_pos+1 >= buf_len) {
                GDB_ASSERT(0);
                return GDB_EOF;
            }
            buf[buf_pos++] = '}';
            buf[buf_pos++] = data[data_pos] ^ 0x20;
        } else {
            if (buf_pos >= buf_len) {
                GDB_ASSERT(0);
                return GDB_EOF;
            }
            buf[buf_pos++] = data[data_pos];
        }
    }

    return buf_pos;
}

/*
 * Decode data from its bin-value representation to a buffer.
 *
 * Returns:
 *    0+  if successful, number of bytes decoded
 *    GDB_EOF if the buffer is too small
 */
static int gdb_dec_bin(const char *buf, unsigned int buf_len, char *data,
                       unsigned int data_len)
{
    unsigned int buf_pos, data_pos;

    for (buf_pos = 0, data_pos = 0; buf_pos < buf_len; buf_pos++) {
        if (data_pos >= data_len) {
            /* Output buffer overflow */
            GDB_ASSERT(0);
            return GDB_EOF;
        }
        if (buf[buf_pos] == '}') {
            /* The next byte is escaped! */
            if (buf_pos+1 >= buf_len) {
                /* There's an escape character, but no escaped character
                 * following the escape character. */
                GDB_ASSERT(0);
                return GDB_EOF;
            }
            buf_pos += 1;
            data[data_pos++] = buf[buf_pos] ^ 0x20;
        } else {
            data[data_pos++] = buf[buf_pos];
        }
    }

    return data_pos;
}

/*****************************************************************************
 * Command Functions
 ****************************************************************************/

/*
 * Read from memory and encode into buf.
 *
 * Returns:
 *    0+  number of bytes written to buf
 *    GDB_EOF if the buffer is too small
 */
static int gdb_mem_read(struct gdb_state *state, char *buf,
                        unsigned int buf_len, address addr, unsigned int len,
                        gdb_enc_func enc)
{
    char data[64];
    unsigned int pos;

    if (len > sizeof(data)) {
        return GDB_EOF;
    }

    /* Read from system memory */
    for (pos = 0; pos < len; pos++) {
        if (gdb_sys_mem_readb(state, addr+pos, &data[pos])) {
            /* Failed to read */
            return GDB_EOF;
        }
    }

    /* Encode data */
    return enc(buf, buf_len, data, len);
}

/*
 * Write to memory from encoded buf.
 */
static int gdb_mem_write(struct gdb_state *state, const char *buf,
                         unsigned int buf_len, address addr, unsigned int len,
                         gdb_dec_func dec)
{
    char data[64];
    unsigned int pos;

    if (len > sizeof(data)) {
        return GDB_EOF;
    }

    /* Decode data */
    if (dec(buf, buf_len, data, len) == GDB_EOF) {
        return GDB_EOF;
    }

    /* Write to system memory */
    for (pos = 0; pos < len; pos++) {
        if (gdb_sys_mem_writeb(state, addr+pos, data[pos])) {
            /* Failed to write */
            return GDB_EOF;
        }
    }

    return 0;
}

/*
 * Continue program execution at PC.
 */
int gdb_continue(struct gdb_state *state)
{
    gdb_sys_continue(state);
    return 0;
}

/*
 * Step one instruction.
 */
int gdb_step(struct gdb_state *state)
{
    gdb_sys_step(state);
    return 0;
}

/*****************************************************************************
 * Packet Creation Helpers
 ****************************************************************************/

/*
 * Send OK packet
 */
static int gdb_send_ok_packet(struct gdb_state *state, char *buf,
                              unsigned int buf_len)
{
    return gdb_send_packet(state, "OK", 2);
}

/*
 * Send a message to the debugging console (via O XX... packet)
 */
static int gdb_send_conmsg_packet(struct gdb_state *state, char *buf,
                                  unsigned int buf_len, const char *msg)
{
    unsigned int size;
    int status;

    if (buf_len < 2) {
        /* Buffer too small */
        return GDB_EOF;
    }

    buf[0] = 'O';
    status = gdb_enc_hex(&buf[1], buf_len-1, msg, gdb_strlen(msg));
    if (status == GDB_EOF) {
        return GDB_EOF;
    }
    size = 1 + status;
    return gdb_send_packet(state, buf, size);
}

/*
 * Send a signal packet (S AA).
 */
static int gdb_send_signal_packet(struct gdb_state *state, char *buf,
                                  unsigned int buf_len, char signal)
{
    unsigned int size;
    int status;

    if (buf_len < 4) {
        /* Buffer too small */
        return GDB_EOF;
    }

    buf[0] = 'S';
    status = gdb_enc_hex(&buf[1], buf_len-1, &signal, 1);
    if (status == GDB_EOF) {
        return GDB_EOF;
    }
    size = 1 + status;
    return gdb_send_packet(state, buf, size);
}

/*
 * Send a error packet (E AA).
 */
static int gdb_send_error_packet(struct gdb_state *state, char *buf,
                                 unsigned int buf_len, char error)
{
    unsigned int size;
    int status;

    if (buf_len < 4) {
        /* Buffer too small */
        return GDB_EOF;
    }

    buf[0] = 'E';
    status = gdb_enc_hex(&buf[1], buf_len-1, &error, 1);
    if (status == GDB_EOF) {
        return GDB_EOF;
    }
    size = 1 + status;
    return gdb_send_packet(state, buf, size);
}

/*****************************************************************************
 * Communication Functions
 ****************************************************************************/

/*
 * Write a sequence of bytes.
 *
 * Returns:
 *    0   if successful
 *    GDB_EOF if failed to write all bytes
 */
static int gdb_write(struct gdb_state *state, const char *buf, unsigned int len)
{
    while (len--) {
        if (gdb_sys_putchar(state, *buf++) == GDB_EOF) {
            return GDB_EOF;
        }
    }

    return 0;
}

/*
 * Read a sequence of bytes.
 *
 * Returns:
 *    0   if successfully read len bytes
 *    GDB_EOF if failed to read all bytes
 */
static int gdb_read(struct gdb_state *state, char *buf, unsigned int buf_len,
                    unsigned int len)
{
    char c;

    if (buf_len < len) {
        /* Buffer too small */
        return GDB_EOF;
    }

    while (len--) {
        if ((c = gdb_sys_getc(state)) == GDB_EOF) {
            return GDB_EOF;
        }
        *buf++ = c;
    }

    return 0;
}

/*****************************************************************************
 * Main Loop
 ****************************************************************************/

/*
 * Main debug loop. Handles commands.
 */
int gdb_main(struct gdb_state *state)
{
    address addr;
    char pkt_buf[512];
    int status;
    unsigned int length;
    unsigned int pkt_len;
    const char *ptr_next;

    gdb_send_signal_packet(state, pkt_buf, sizeof(pkt_buf), state->signum);

    while (1) {
        /* Receive the next packet */
        status = gdb_recv_packet(state, pkt_buf, sizeof(pkt_buf), &pkt_len);
        if (status == GDB_EOF) {
            break;
        }

        if (pkt_len == 0) {
            /* Received empty packet.. */
            continue;
        }

        ptr_next = pkt_buf;

        /*
         * Handle one letter commands
         */
        switch (pkt_buf[0]) {

        /* Calculate remaining space in packet from ptr_next position. */
        #define token_remaining_buf (pkt_len-(ptr_next-pkt_buf))

        /* Expecting a seperator. If not present, go to error */
        #define token_expect_seperator(c) \
            { \
                if (!ptr_next || *ptr_next != c) { \
                    goto error; \
                } else { \
                    ptr_next += 1; \
                } \
            }

        /* Expecting an integer argument. If not present, go to error */
        #define token_expect_integer_arg(arg) \
            { \
                arg = gdb_strtol(ptr_next, token_remaining_buf, \
                                 16, &ptr_next); \
                if (!ptr_next) { \
                    goto error; \
                } \
            }

        /*
         * Read Registers
         * Command Format: g
         */
        case 'g':
            /* Encode registers */
            status = gdb_enc_hex(pkt_buf, sizeof(pkt_buf),
                                 (char *)&(state->registers),
                                 sizeof(state->registers));
            if (status == GDB_EOF) {
                goto error;
            }
            pkt_len = status;
            gdb_send_packet(state, pkt_buf, pkt_len);
            break;

        /*
         * Write Registers
         * Command Format: G XX...
         */
        case 'G':
            status = gdb_dec_hex(pkt_buf+1, pkt_len-1,
                                 (char *)&(state->registers),
                                 sizeof(state->registers));
            if (status == GDB_EOF) {
                goto error;
            }
            gdb_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
            break;

        /*
         * Read a Register
         * Command Format: p n
         */
        case 'p':
            ptr_next += 1;
            token_expect_integer_arg(addr);

            if (addr >= GDB_CPU_NUM_REGISTERS) {
                goto error;
            }

            /* Read Register */
            status = gdb_enc_hex(pkt_buf, sizeof(pkt_buf),
                                 (char *)&(state->registers[addr]),
                                 sizeof(state->registers[addr]));
            if (status == GDB_EOF) {
                goto error;
            }
            gdb_send_packet(state, pkt_buf, status);
            break;

        /*
         * Write a Register
         * Command Format: P n...=r...
         */
        case 'P':
            ptr_next += 1;
            token_expect_integer_arg(addr);
            token_expect_seperator('=');

            if (addr < GDB_CPU_NUM_REGISTERS) {
                status = gdb_dec_hex(ptr_next, token_remaining_buf,
                                     (char *)&(state->registers[addr]),
                                     sizeof(state->registers[addr]));
                if (status == GDB_EOF) {
                    goto error;
                }
            }
            gdb_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
            break;

        /*
         * Read Memory
         * Command Format: m addr,length
         */
        case 'm':
            ptr_next += 1;
            token_expect_integer_arg(addr);
            token_expect_seperator(',');
            token_expect_integer_arg(length);

            /* Read Memory */
            status = gdb_mem_read(state, pkt_buf, sizeof(pkt_buf),
                                  addr, length, gdb_enc_hex);
            if (status == GDB_EOF) {
                goto error;
            }
            gdb_send_packet(state, pkt_buf, status);
            break;

        /*
         * Write Memory
         * Command Format: M addr,length:XX..
         */
        case 'M':
            ptr_next += 1;
            token_expect_integer_arg(addr);
            token_expect_seperator(',');
            token_expect_integer_arg(length);
            token_expect_seperator(':');

            /* Write Memory */
            status = gdb_mem_write(state, ptr_next, token_remaining_buf,
                                   addr, length, gdb_dec_hex);
            if (status == GDB_EOF) {
                goto error;
            }
            gdb_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
            break;

        /*
         * Write Memory (Binary)
         * Command Format: X addr,length:XX..
         */
        case 'X':
            ptr_next += 1;
            token_expect_integer_arg(addr);
            token_expect_seperator(',');
            token_expect_integer_arg(length);
            token_expect_seperator(':');

            /* Write Memory */
            status = gdb_mem_write(state, ptr_next, token_remaining_buf,
                                   addr, length, gdb_dec_bin);
            if (status == GDB_EOF) {
                goto error;
            }
            gdb_send_ok_packet(state, pkt_buf, sizeof(pkt_buf));
            break;

        /*
         * Continue
         * Command Format: c [addr]
         */
        case 'c':
            gdb_continue(state);
            return 0;

        /*
         * Single-step
         * Command Format: s [addr]
         */
        case 's':
            gdb_step(state);
            return 0;

        case '?':
            gdb_send_signal_packet(state, pkt_buf, sizeof(pkt_buf),
                                   state->signum);
            break;

        /*
         * Unsupported Command
         */
        default:
            gdb_send_packet(state, NULL, 0);
        }

        continue;

    error:
        gdb_send_error_packet(state, pkt_buf, sizeof(pkt_buf), 0x00);

        #undef token_remaining_buf
        #undef token_expect_seperator
        #undef token_expect_integer_arg
    }

    return 0;
}

/*****************************************************************************
 *
 *  Mock
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_MOCK

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

static char gdb_mem[256];

struct gdb_buffer {
    char   *buf;
    unsigned int pos_write;
    unsigned int pos_read;
    unsigned int size;
} gdb_input, gdb_output;

void gdb_buf_write(struct gdb_buffer *buf, int ch)
{
    if (buf->buf == NULL) {
        buf->pos_write = 0;
        buf->pos_read = 0;
        buf->size = 512;
        buf->buf = malloc(buf->size);
    } else if (buf->pos_write >= buf->size) {
        buf->size *= 2;
        buf->buf = realloc(buf->buf, buf->size);
    }

    assert(buf->buf);
    buf->buf[buf->pos_write++] = ch;
}

int gdb_buf_read(struct gdb_buffer *buf)
{
    if (buf->buf && buf->pos_read < buf->pos_write) {
        return buf->buf[buf->pos_read++];
    }
    return EOF;
}

/*****************************************************************************
 * Debugging System Functions
 ****************************************************************************/

/*
 * Write one character to the debugging stream.
 */
int gdb_sys_putchar(struct gdb_state *state, int ch)
{
#ifdef USE_STDIO
    putchar(ch);
#else
    gdb_buf_write(&gdb_output, ch);
#endif
    return 0;
}

/*
 * Read one character from the debugging stream.
 */
int gdb_sys_getc(struct gdb_state *state)
{
#ifdef USE_STDIO
    int ch = getchar();
    return ch == EOF ? GDB_EOF : ch;
#else
    return gdb_buf_read(&gdb_input);
#endif
}

/*
 * Read one byte from memory.
 */
int gdb_sys_mem_readb(struct gdb_state *state, address addr, char *val)
{
    if (addr >= sizeof(gdb_mem)) {
        return 1;
    }

    *val = gdb_mem[addr];
    return 0;
}

/*
 * Write one byte to memory.
 */
int gdb_sys_mem_writeb(struct gdb_state *state, address addr, char val)
{
    if (addr >= sizeof(gdb_mem)) {
        return 1;
    }

    gdb_mem[addr] = val;
    return 0;
}

/*
 * Continue program execution.
 */
int gdb_sys_continue(struct gdb_state *state)
{
    return 0;
}

/*
 * Single step the next instruction.
 */
int gdb_sys_step(struct gdb_state *state)
{
    return 0;
}

#endif /* GDBSTUB_ARCH_MOCK */


/*****************************************************************************
 *
 *  x86
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_X86

/*****************************************************************************
 * Types
 ****************************************************************************/

#pragma pack(1)
struct gdb_interrupt_state {
    uint32_t ss;
    uint32_t gs;
    uint32_t fs;
    uint32_t es;
    uint32_t ds;
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t vector;
    uint32_t error_code;
    uint32_t eip;
    uint32_t cs;
    uint32_t eflags;
};

struct gdb_idtr
{
    uint16_t len;
    uint32_t offset;
};

struct gdb_idt_gate
{
    uint16_t offset_low;
    uint16_t segment;
    uint16_t flags;
    uint16_t offset_high;
};
#pragma pack()

/*****************************************************************************
 * Const Data
 ****************************************************************************/

extern void const * const gdb_x86_int_handlers[];

/*****************************************************************************
 * Prototypes
 ****************************************************************************/

void gdb_x86_int_handler(struct gdb_interrupt_state *istate);

static void gdb_x86_hook_idt(uint8_t vector, const void *function);
static void gdb_x86_init_gates(void);
static void gdb_x86_init_idt(void);
static void gdb_x86_load_idt(struct gdb_idtr *idtr);
static void gdb_x86_store_idt(struct gdb_idtr *idtr);
static uint32_t gdb_x86_get_cs(void);
static void gdb_x86_interrupt(struct gdb_interrupt_state *istate);
static void gdb_x86_io_write_8(uint16_t port, uint8_t val);
static uint8_t gdb_x86_io_read_8(uint16_t port);
static int gdb_x86_serial_getc(void);
static int gdb_x86_serial_putchar(int ch);

#ifdef __STRICT_ANSI__
#define asm __asm__
#endif

#define SERIAL_COM1 0x3f8
#define SERIAL_COM2 0x2f8
#define SERIAL_PORT SERIAL_COM1

#define NUM_IDT_ENTRIES 32

/*****************************************************************************
 * BSS Data
 ****************************************************************************/

static struct gdb_idt_gate gdb_idt_gates[NUM_IDT_ENTRIES];
static struct gdb_state    gdb_state;

/*****************************************************************************
 * Misc. Functions
 ****************************************************************************/

/*
 * Get current code segment (CS register).
 */
static uint32_t gdb_x86_get_cs(void)
{
    uint32_t cs;

    asm volatile (
        "push    %%cs;"
        "pop     %%eax;"
        /* Outputs  */ : "=a" (cs)
        /* Inputs   */ : /* None */
        /* Clobbers */ : /* None */
        );

    return cs;
}

/*****************************************************************************
 * Interrupt Management Functions
 ****************************************************************************/

/*
 * Initialize idt_gates with the interrupt handlers.
 */
static void gdb_x86_init_gates(void)
{
    unsigned int i;
    uint16_t cs;

    cs = gdb_x86_get_cs();
    for (i = 0; i < NUM_IDT_ENTRIES; i++) {
        gdb_idt_gates[i].flags       = 0x8E00;
        gdb_idt_gates[i].segment     = cs;
        gdb_idt_gates[i].offset_low  =
            ((uint32_t)gdb_x86_int_handlers[i]      ) & 0xffff;
        gdb_idt_gates[i].offset_high =
            ((uint32_t)gdb_x86_int_handlers[i] >> 16) & 0xffff;
    }
}

/*
 * Load a new IDT.
 */
static void gdb_x86_load_idt(struct gdb_idtr *idtr)
{
    asm volatile (
        "lidt    %0"
        /* Outputs  */ : /* None */
        /* Inputs   */ : "m" (*idtr)
        /* Clobbers */ : /* None */
        );
}

/*
 * Get current IDT.
 */
static void gdb_x86_store_idt(struct gdb_idtr *idtr)
{
    asm volatile (
        "sidt    %0"
        /* Outputs  */ : "=m" (*idtr)
        /* Inputs   */ : /* None */
        /* Clobbers */ : /* None */
        );
}

/*
 * Hook a vector of the current IDT.
 */
static void gdb_x86_hook_idt(uint8_t vector, const void *function)
{
    struct gdb_idtr      idtr;
    struct gdb_idt_gate *gates;

    gdb_x86_store_idt(&idtr);
    gates = (struct gdb_idt_gate *)idtr.offset;
    gates[vector].flags       = 0x8E00;
    gates[vector].segment     = gdb_x86_get_cs();
    gates[vector].offset_low  = (((uint32_t)function)      ) & 0xffff;
    gates[vector].offset_high = (((uint32_t)function) >> 16) & 0xffff;
}

/*
 * Initialize IDT gates and load the new IDT.
 */
static void gdb_x86_init_idt(void)
{
    struct gdb_idtr idtr;

    gdb_x86_init_gates();
    idtr.len = sizeof(gdb_idt_gates)-1;
    idtr.offset = (uint32_t)gdb_idt_gates;
    gdb_x86_load_idt(&idtr);
}

/*
 * Common interrupt handler routine.
 */
void gdb_x86_int_handler(struct gdb_interrupt_state *istate)
{
    gdb_x86_interrupt(istate);
}

/*
 * Debug interrupt handler.
 */
static void gdb_x86_interrupt(struct gdb_interrupt_state *istate)
{
    /* Translate vector to signal */
    switch (istate->vector) {
    case 1:  gdb_state.signum = 5; break;
    case 3:  gdb_state.signum = 5; break;
    default: gdb_state.signum = 7;
    }

    /* Load Registers */
    gdb_state.registers[GDB_CPU_I386_REG_EAX] = istate->eax;
    gdb_state.registers[GDB_CPU_I386_REG_ECX] = istate->ecx;
    gdb_state.registers[GDB_CPU_I386_REG_EDX] = istate->edx;
    gdb_state.registers[GDB_CPU_I386_REG_EBX] = istate->ebx;
    gdb_state.registers[GDB_CPU_I386_REG_ESP] = istate->esp;
    gdb_state.registers[GDB_CPU_I386_REG_EBP] = istate->ebp;
    gdb_state.registers[GDB_CPU_I386_REG_ESI] = istate->esi;
    gdb_state.registers[GDB_CPU_I386_REG_EDI] = istate->edi;
    gdb_state.registers[GDB_CPU_I386_REG_PC]  = istate->eip;
    gdb_state.registers[GDB_CPU_I386_REG_CS]  = istate->cs;
    gdb_state.registers[GDB_CPU_I386_REG_PS]  = istate->eflags;
    gdb_state.registers[GDB_CPU_I386_REG_SS]  = istate->ss;
    gdb_state.registers[GDB_CPU_I386_REG_DS]  = istate->ds;
    gdb_state.registers[GDB_CPU_I386_REG_ES]  = istate->es;
    gdb_state.registers[GDB_CPU_I386_REG_FS]  = istate->fs;
    gdb_state.registers[GDB_CPU_I386_REG_GS]  = istate->gs;

    gdb_main(&gdb_state);

    /* Restore Registers */
    istate->eax    = gdb_state.registers[GDB_CPU_I386_REG_EAX];
    istate->ecx    = gdb_state.registers[GDB_CPU_I386_REG_ECX];
    istate->edx    = gdb_state.registers[GDB_CPU_I386_REG_EDX];
    istate->ebx    = gdb_state.registers[GDB_CPU_I386_REG_EBX];
    istate->esp    = gdb_state.registers[GDB_CPU_I386_REG_ESP];
    istate->ebp    = gdb_state.registers[GDB_CPU_I386_REG_EBP];
    istate->esi    = gdb_state.registers[GDB_CPU_I386_REG_ESI];
    istate->edi    = gdb_state.registers[GDB_CPU_I386_REG_EDI];
    istate->eip    = gdb_state.registers[GDB_CPU_I386_REG_PC];
    istate->cs     = gdb_state.registers[GDB_CPU_I386_REG_CS];
    istate->eflags = gdb_state.registers[GDB_CPU_I386_REG_PS];
    istate->ss     = gdb_state.registers[GDB_CPU_I386_REG_SS];
    istate->ds     = gdb_state.registers[GDB_CPU_I386_REG_DS];
    istate->es     = gdb_state.registers[GDB_CPU_I386_REG_ES];
    istate->fs     = gdb_state.registers[GDB_CPU_I386_REG_FS];
    istate->gs     = gdb_state.registers[GDB_CPU_I386_REG_GS];
}

/*****************************************************************************
 * I/O Functions
 ****************************************************************************/

/*
 * Write to I/O port.
 */
static void gdb_x86_io_write_8(uint16_t port, uint8_t val)
{
    asm volatile (
        "outb    %%al, %%dx;"
        /* Outputs  */ : /* None */
        /* Inputs   */ : "a" (val), "d" (port)
        /* Clobbers */ : /* None */
        );
}

/*
 * Read from I/O port.
 */
static uint8_t gdb_x86_io_read_8(uint16_t port)
{
    uint8_t val;

    asm volatile (
        "inb     %%dx, %%al;"
        /* Outputs  */ : "=a" (val)
        /* Inputs   */ : "d" (port)
        /* Clobbers */ : /* None */
        );

    return val;
}

/*****************************************************************************
 * NS16550 Serial Port (IO)
 ****************************************************************************/

#define SERIAL_THR 0
#define SERIAL_RBR 0
#define SERIAL_LSR 5

static int gdb_x86_serial_getc(void)
{
    /* Wait for data */
    while ((gdb_x86_io_read_8(SERIAL_PORT + SERIAL_LSR) & 1) == 0);
    return gdb_x86_io_read_8(SERIAL_PORT + SERIAL_RBR);
}

static int gdb_x86_serial_putchar(int ch)
{
    /* Wait for THRE (bit 5) to be high */
    while ((gdb_x86_io_read_8(SERIAL_PORT + SERIAL_LSR) & (1<<5)) == 0);
    gdb_x86_io_write_8(SERIAL_PORT + SERIAL_THR, ch);
    return ch;
}

/*****************************************************************************
 * Debugging System Functions
 ****************************************************************************/

/*
 * Write one character to the debugging stream.
 */
int gdb_sys_putchar(struct gdb_state *state, int ch)
{
    return gdb_x86_serial_putchar(ch);
}

/*
 * Read one character from the debugging stream.
 */
int gdb_sys_getc(struct gdb_state *state)
{
    return gdb_x86_serial_getc() & 0xff;
}

/*
 * Read one byte from memory.
 */
int gdb_sys_mem_readb(struct gdb_state *state, address addr, char *val)
{
    *val = *(volatile char *)addr;
    return 0;
}

/*
 * Write one byte to memory.
 */
int gdb_sys_mem_writeb(struct gdb_state *state, address addr, char val)
{
    *(volatile char *)addr = val;
    return 0;
}

/*
 * Continue program execution.
 */
int gdb_sys_continue(struct gdb_state *state)
{
    gdb_state.registers[GDB_CPU_I386_REG_PS] &= ~(1<<8);
    return 0;
}

/*
 * Single step the next instruction.
 */
int gdb_sys_step(struct gdb_state *state)
{
    gdb_state.registers[GDB_CPU_I386_REG_PS] |= 1<<8;
    return 0;
}

/*
 * Debugger init function.
 *
 * Hooks the IDT to enable debugging.
 */
void gdb_sys_init(void)
{
    /* Hook current IDT. */
    gdb_x86_hook_idt(1, gdb_x86_int_handlers[1]);
    gdb_x86_hook_idt(3, gdb_x86_int_handlers[3]);

    /* Interrupt to start debugging. */
    asm volatile ("int3");
}

#endif /* GDBSTUB_ARCH_X86 */

/*****************************************************************************
 *
 *  UVM32
 *
 ****************************************************************************/

#ifdef GDBSTUB_ARCH_UVM32

#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>



/*****************************************************************************
 * Debugging System Functions
 ****************************************************************************/

/*
 * Write one character to the debugging stream.
 */
int gdb_sys_putchar(struct gdb_state *state, int ch)
{
  if (0 <= state->socket) {
    ssize_t sent = send(state->socket, (char*)&ch, 1, 0);
    return (sent == 0) ? GDB_EOF : 0;
  } else {
    if (EOF == fputc(ch, stdout)) return GDB_EOF;
    fflush(stdout);
  }
  return 0;
}

/*
 * Read one character from the debugging stream.
 */
int gdb_sys_getc(struct gdb_state *state)
{
  if (0 <= state->socket) {
    uint8_t buffer;
    ssize_t n = recv(state->socket, &buffer, 1, 0);
    return (n == 0) ? GDB_EOF : (int)buffer;
  } else {
    int ch = fgetc(stdin);
    return (EOF == ch) ? GDB_EOF : ch;
  }
}

/*
 * Read one byte from memory.
 */
int gdb_sys_mem_readb(struct gdb_state *state, address addr, char *val)
{
  if (addr >= MINIRV32_RAM_IMAGE_OFFSET) {
    addr -= MINIRV32_RAM_IMAGE_OFFSET;
  }

  if (addr >= UVM32_MEMORY_SIZE) {
    return 1;
  }

  const uint8_t *mem = uvm32_getMemory(state->vmst);
  *val = (char)mem[addr];
  return 0;
}

/*
 * Write one byte to memory.
 */
int gdb_sys_mem_writeb(struct gdb_state *state, address addr, char val)
{
  if (addr >= MINIRV32_RAM_IMAGE_OFFSET) {
    addr -= MINIRV32_RAM_IMAGE_OFFSET;
  }

  if (addr >= UVM32_MEMORY_SIZE) {
    return 1;
  }

  uint8_t *mem = (uint8_t *)uvm32_getMemory(state->vmst);
  mem[addr] = (uint8_t)val;
  return 0;
}

static int gdb_sys_run(struct gdb_state *state, bool cont) {
  bool running = true;
  enum {
    MAX_STEPS = 1000
  };

  while (running) {
    uvm32_evt_t evt = {0};
    uint32_t steps = uvm32_run(state->vmst,
                               &evt,
                               cont ? MAX_STEPS : 1);

    /* Call user callback if available */
    if (NULL != state->event_cb) {
      state->event_cb(state->vmst, &evt);
    }

    switch (evt.typ) {
    case UVM32_EVT_BREAK:
      running = false;
      state->signum = SIGTRAP;
      uvm32_clearError(state->vmst);
      break;

    case UVM32_EVT_ERR:
      switch (evt.data.err.errcode) {
      case UVM32_ERR_HUNG:
        if (!cont) { // single step
          state->signum = SIGTRAP;
          uvm32_clearError(state->vmst);
          running = false;
        } else if (MAX_STEPS == steps) { // continue?
          /* Ran up to max steps, keep going */
          uvm32_clearError(state->vmst);
        } else {
          state->signum = SIGTRAP; // FIXME
          running = false;
        }
        break;

      case UVM32_ERR_MEM_RD:
      case UVM32_ERR_MEM_WR:
        running = false;
        state->signum = SIGSEGV;
        break;

      case UVM32_ERR_INTERNAL_CORE:
        running = false;
        state->signum = SIGILL;
        break;

      default:
        break;
      } // switch(evt.data.err.errcode)
      break;

    default:
      break;
    } // switch(evt.typ)
  } // while

  /* Update registers */
  memcpy(state->registers,
         &state->vmst->_core.regs,
         sizeof(state->registers));

  return 0;
}

/*
 * Continue program execution.
 */
int gdb_sys_continue(struct gdb_state *state) {
  return gdb_sys_run(state, true);
}

/*
 * Single step the next instruction.
 */
int gdb_sys_step(struct gdb_state *state) {
  return gdb_sys_run(state, false);
}

/*
 * Debugger init function.
 *
 */
void gdb_sys_init(void)
{

}

/* Helper function to init gdb_state */
int32_t gdb_sys_init_state(struct gdb_state* state,
                           uvm32_state_t* vmst,
                           int socket,
                           uvm32_event_callback_t event_cb) {
  /* Sanity check argument */
  if ((NULL == state) || (NULL == vmst)) {
    return -1;
  }
  memset(state, 0, sizeof(struct gdb_state));
  state->vmst = vmst;
  state->event_cb = event_cb;
  state->socket = socket;
  /* Update registers */
  memcpy(state->registers,
         vmst->_core.regs,
         sizeof(state->registers));
  return 0;
}

#endif /* GDBSTUB_ARCH_UVM32 */
#endif /* GDBSTUB_IMPLEMENTATION */
#endif /* GDBSTUB_H */
