#include "ccextractor.h"
#include "wtv_constants.h"
extern int firstcall;
int check_stream_id(int stream_id, int video_streams, int num_streams);

int check_stream_id(int stream_id, int video_streams[], int num_streams) {
    int x;
    for(x=0; x<num_streams; x++) 
        if(video_streams[x]==stream_id)
            return 1;
    return 0;
}
uint64_t time_to_pes_time(uint64_t time) 
{
	return ((time/10000)*90);
}
LLONG wtv_getmoredata(void)
{
    // Generic buffer to hold some data
    static unsigned char *parsebuf = (unsigned char*)malloc(1024);
    static long parsebufsize = 1024;
    static int video_streams[32];
    static int num_streams=0;
    if(firstcall)
    {
        if(wtvmpeg2)
            bufferdatatype=PES;
        else
            bufferdatatype=RAW;
        buffered_read(parsebuf,16);
        past+=result;
        if (result!=16)
        {
            mprint("Premature end of file!\n");
            end_of_file=1;
            return 0;
        }
        // Expecting ASF header
        if( !memcmp(parsebuf, WTV_HEADER, 16 ) )
        {
			dbg_print(CCX_DMT_PARSE, "\nWTV header\n");
        }
        else
        {
            fatal(EXIT_MISSING_ASF_HEADER, "Missing WTV header. Abort.\n");
        }
        buffered_skip(0x40000-(16));
        past=past+0x40000;
        firstcall=0;
    }
    while(1)
    {
        int bytesread = 0;

        buffered_read(parsebuf, 24)
        past+=result;
        if(result==0) {
            end_of_file=1;
            return 0;
        }
        if (result!=24)
        {
            mprint("Premature end of file!\n");
            end_of_file=1;
            return 0;
        }

    	int x;
    	for(x=0; x<16; x++)
            dbg_print(CCX_DMT_PARSE, "%02X ", parsebuf[x]);
        dbg_print(CCX_DMT_PARSE, "\n");
        uint32_t len;
        memcpy(&len, parsebuf+16, 4);
        len-=32;
        dbg_print(CCX_DMT_PARSE, "len %X\n", len);
		uint32_t pad;
		pad = len%8==0 ? 0 : 8- (len % 8);
        dbg_print(CCX_DMT_PARSE, "pad %X\n", pad);
        uint32_t stream_id;
        memcpy(&stream_id, parsebuf+20, 4);
		stream_id = stream_id & 0x7f;
		dbg_print(CCX_DMT_PARSE, "stream_id: %X\n", stream_id);
        buffered_skip(8);
        past+=32;
        for(x=0; x<num_streams; x++)
            dbg_print(CCX_DMT_PARSE, "video stream_id: %X\n", video_streams[x]);
        if( !memcmp(parsebuf, WTV_EOF, 16 ))
        {
            dbg_print(CCX_DMT_PARSE, "WTV EOF\n");
            buffered_skip(len+pad);
            past+=len+pad;
        }
        if( !memcmp(parsebuf, WTV_STREAM2, 16 ) )
        {
			dbg_print(CCX_DMT_PARSE, "WTV STREAM2\n");
            buffered_skip(0xc);
            static unsigned char stream_type[16];
            buffered_read(stream_type, 16);
            const void *stream_guid;
            if(wtvmpeg2)
                stream_guid = WTV_STREAM_VIDEO;
            else
                stream_guid = WTV_STREAM_MSTVCAPTION;
            if(!memcmp(stream_type, stream_guid, 16 ) )
            {
                video_streams[num_streams]=stream_id;
                num_streams++;    
            }
            len-=28;
            past+=28;
        }
        if( !memcmp(parsebuf, WTV_TIMING, 16 )  && check_stream_id(stream_id, video_streams, num_streams))
        {
            dbg_print(CCX_DMT_PARSE, "WTV TIMING\n");
            buffered_skip(0x8);
            int64_t time;
			buffered_read((unsigned char*)&time, 8);
            dbg_print(CCX_DMT_PARSE, "TIME: %ld\n", time);
            if(time!=-1) {
                current_pts = time_to_pes_time(time); 
                pts_set = 1;
                frames_since_ref_time = 0;
                set_fts();  
            }
            len-=16;
            past+=16;
        }
        if( !memcmp(parsebuf, WTV_DATA, 16 )  
            && check_stream_id(stream_id, video_streams, num_streams) && current_pts!=0
            && (wtvmpeg2 || (!wtvmpeg2 && len==2)))
        {
            dbg_print(CCX_DMT_PARSE, "want: %X\n", BUFSIZE);
            dbg_print(CCX_DMT_PARSE, "\nWTV DATA\n");
            buffered_read(buffer+inbuf, len);
            inbuf+=result;
            bytesread+=(int) result;
            buffered_skip(pad);
            past+=len+pad;
            frames_since_ref_time++;
            set_fts(); 
            
            return bytesread;
        }
        else {
            buffered_skip(len+pad);
            past+=len+pad;
        }
    }    
}
