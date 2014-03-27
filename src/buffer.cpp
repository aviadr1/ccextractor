#include "ccextractor.h"

void ccx_buffered_skip(ccx_context_t::filebuffer_t* fb, int bytes)
{
    if (bytes<=fb->bytesinbuffer-fb->pos) {
        fb->pos+=bytes;
        result=bytes;
    } 
    else {
        ccx_buffered_read_opt (fb, NULL,bytes);
    }
}

void ccx_buffered_read(ccx_context_t::filebuffer_t* fb, uint8_t* buffer, int bytes)
{
    if (bytes<=fb->bytesinbuffer-fb->pos) { 
        if (buffer!=NULL) {
            memcpy (buffer,fb->p+fb->pos,bytes); 
        }

        fb->pos+=bytes;
        result=bytes;
    } 
    else { 
        ccx_buffered_read_opt (fb, buffer,bytes); 
        if (gui_mode_reports && input_source==CCX_DS_NETWORK) {
            net_activity_gui++; 
            if (!(net_activity_gui%1000)) {
                activity_report_data_read();
            }
        }
    }
}

void ccx_buffered_read_byte(ccx_context_t::filebuffer_t* fb, uint8_t* buffer)
{
    if (fb->bytesinbuffer-fb->pos) {
        if (buffer) { 
            *buffer=fb->p[fb->pos];
            fb->pos++;
            result=1; 
        }
    } 
    else {
        result=ccx_buffered_read_opt (fb, buffer,1);
    }
}

void ccx_buffered_seek (ccx_context_t::filebuffer_t* fb, int offset)
{
    position_sanity_check();
    if (offset<0)
    {
        fb->pos+=offset;
        if (fb->pos<0)
        {
            // We got into the start buffer (hopefully)
            if (startbytes_pos+fb->pos < 0)
            {
                fatal (EXIT_BUG_BUG, "PANIC: Attempt to seek before buffer start, this is a bug!");
            }
            startbytes_pos+=fb->pos;
            fb->pos=0;
        }
    }
    else
    {
        result = ccx_buffered_read_opt (fb, NULL, offset);
        position_sanity_check();
    }
}


int ccx_buffered_init(ccx_context_t::filebuffer_t* fb)
{
    fb->start=0;
    fb->pos=0;    
    if (fb->p==NULL)
    {
        fb->p=(unsigned char *) malloc (FILEBUFFERSIZE);
        fb->bytesinbuffer=0;
    }
    if (fb->p==NULL) 
    {
        fatal (EXIT_NOT_ENOUGH_MEMORY, "Not enough memory\n");        
    }
    return 0;
}
