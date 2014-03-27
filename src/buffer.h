#ifndef CCX_BUFFER_H
#define CCX_BUFFER_H

// compatibility across platforms
#include "platform.h"

struct ccx_filebuffer_context_t {
    unsigned char *p;
    int bytesinbuffer; // Number of bytes we actually have on buffer
    LLONG start; // Position of buffer start relative to file
    int pos; // Position of pointer relative to buffer start
    LLONG number_of_bytes_read_in_last_op; // Number of bytes read/skipped in last read operation
};

inline LLONG ccx_buffered_get_last_num_bytes_processed(ccx_filebuffer_context_t* fb) { return fb->number_of_bytes_read_in_last_op; };
extern LLONG ccx_buffered_read_opt (ccx_filebuffer_context_t* fb, unsigned char *buffer, unsigned int bytes);
extern void ccx_buffered_skip(ccx_filebuffer_context_t* fb, int bytes);
extern void ccx_buffered_read(ccx_filebuffer_context_t* ctx, uint8_t* buffer, int bytes);
extern void ccx_buffered_read_byte(ccx_filebuffer_context_t* ctx, uint8_t* buffer);
extern void ccx_buffered_seek (ccx_filebuffer_context_t* fb, int offset);
extern int ccx_buffered_init( ccx_filebuffer_context_t* fb );

#endif
