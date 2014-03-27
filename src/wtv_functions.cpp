#include "ccextractor.h"
#include "wtv_constants.h"

int check_stream_id(int stream_id, int video_streams, int num_streams);
int add_skip_chunks(ccx_context_t* ctx, wtv_chunked_buffer *cb, uint32_t offset, uint32_t flag);
void init_chunked_buffer(wtv_chunked_buffer *cb);
uint64_t get_meta_chunk_start(uint64_t offset);
uint64_t time_to_pes_time(uint64_t time);
void add_chunk(wtv_chunked_buffer *cb, uint64_t value);
int qsort_cmpint (const void * a, const void * b);
void get_sized_buffer(ccx_context_t* ctx, wtv_chunked_buffer *cb, uint32_t size);

int qsort_cmpint (const void * a, const void * b)
{
   return ( *(int*)a - *(int*)b );
}

int check_stream_id(int stream_id, int video_streams[], int num_streams) {
    int x;
    for(x=0; x<num_streams; x++) 
        if(video_streams[x]==stream_id)
            return 1;
    return 0;
}

void init_chunked_buffer(wtv_chunked_buffer *cb) {
    cb->count=0;
    cb->buffer=NULL;
    cb->buffer_size=0;
    int x;
    for(x=0; x<256; x++)
        cb->skip_chunks[x]=-1;
}

uint64_t get_meta_chunk_start(uint64_t offset) {
	return offset*WTV_CHUNK_SIZE&WTV_META_CHUNK_MASK;
}

uint64_t time_to_pes_time(uint64_t time) 
{
	return ((time/10000)*90);
}

int add_skip_chunks(ccx_context_t* ctx, wtv_chunked_buffer *cb, uint32_t offset, uint32_t flag) 
{

    uint64_t start = ctx->filebuffer.pos; //Not sure this is the best way to do this
    buffered_seek(&ctx->filebuffer, (offset*WTV_CHUNK_SIZE)-start);
    uint64_t seek_back=0-((offset*WTV_CHUNK_SIZE)-start);

	uint32_t value;
	buffered_read(&ctx->filebuffer,(unsigned char*)&value, 4);
    if(result!=4)
        return 0;
    seek_back-=4;
    while(value!=0) {
        dbg_print(CCX_DMT_PARSE, "value: %x\n", get_meta_chunk_start(value));
        buffered_read(&ctx->filebuffer,(unsigned char*)&value, 4);
        if(result!=4)
            return 0;
        add_chunk(cb, get_meta_chunk_start(value));
        seek_back-=4;
    }
    buffered_seek(&ctx->filebuffer, seek_back);
    dbg_print(CCX_DMT_PARSE, "filebuffer_pos: %x\n", ctx->filebuffer.pos);
    return 1;
}

void add_chunk(wtv_chunked_buffer *cb, uint64_t value)
{
    int x;
    for(x=0; x<cb->count; x++)
        if(cb->skip_chunks[x]==value)
            return;
    cb->skip_chunks[cb->count]=value;
    cb->count++;
}

void get_sized_buffer(ccx_context_t* ctx, wtv_chunked_buffer *cb, uint32_t size) {
    if(cb->buffer!=NULL && cb->buffer_size>0) {
        free(cb->buffer);
    }
	cb->buffer = (uint8_t*)malloc(size);
    cb->buffer_size=size;
	uint64_t start = cb->filepos;

	if(cb->skip_chunks[cb->chunk]!=-1 && start+size>cb->skip_chunks[cb->chunk]) {
        buffered_read(&ctx->filebuffer,cb->buffer, cb->skip_chunks[cb->chunk]-start);
        cb->filepos+=cb->skip_chunks[cb->chunk]-start;
        buffered_seek(&ctx->filebuffer, WTV_META_CHUNK_SIZE);
        cb->filepos+=WTV_META_CHUNK_SIZE;
        buffered_read(&ctx->filebuffer,cb->buffer+(cb->skip_chunks[cb->chunk]-start), size-(cb->skip_chunks[cb->chunk]-start));
        cb->filepos+=cb->skip_chunks[cb->chunk]-start;
        cb->chunk++;
	}
	else {
		buffered_read(&ctx->filebuffer,cb->buffer, size);
        cb->filepos+=size;
        if(result!=size) {
            free(buffer);
            cb->buffer_size=0;
            return;
        }           
    }
    past=cb->filepos;
	return;
}

LLONG wtv_getmoredata(ccx_context_t* ctx)
{
    // Generic buffer to hold some data
    static unsigned char *parsebuf = (unsigned char*)malloc(1024);
    static long parsebufsize = 1024;
    static int video_streams[32];
    static int num_streams=0;
    static wtv_chunked_buffer cb;
    if(firstcall)
    {
        init_chunked_buffer(&cb);
        if(wtvmpeg2)
            ccx_bufferdatatype=CCX_PES;
        else
            ccx_bufferdatatype=CCX_RAW;
        buffered_read(&ctx->filebuffer,parsebuf,0x42);
        past+=result;
        if (result!=0x42)
        {
            mprint("Premature end of file!\n");
            end_of_file=1;
            return 0;
        }
        // Expecting WTV header
        if( !memcmp(parsebuf, WTV_HEADER, 16 ) )
        {
			dbg_print(CCX_DMT_PARSE, "\nWTV header\n");
        }
        else
        {
            fatal(EXIT_MISSING_ASF_HEADER, "Missing WTV header. Abort.\n");
        }
        uint32_t filelen;
	    uint32_t root_dir;
        memcpy(&filelen, parsebuf+0x30, 4);
        dbg_print(CCX_DMT_PARSE, "filelen: %x\n", filelen);
        memcpy(&root_dir, parsebuf+0x38, 4);
        dbg_print(CCX_DMT_PARSE, "root_dir: %x\n", root_dir);   
        buffered_skip(&ctx->filebuffer,(root_dir*WTV_CHUNK_SIZE)-0x42);
        past+=(root_dir*WTV_CHUNK_SIZE)-0x42;

        if (result!=(root_dir*WTV_CHUNK_SIZE)-0x42)
        {
            mprint("Premature end of file!\n");
            end_of_file=1;
            return 0;
        }
        int end=0;
        while(!end)
        {
            buffered_read(&ctx->filebuffer,parsebuf, 32);
            int x;
        	for(x=0; x<16; x++)
                dbg_print(CCX_DMT_PARSE, "%02X ", parsebuf[x]);
            dbg_print(CCX_DMT_PARSE, "\n");
            
            if (result!=32)
            {
                mprint("Premature end of file!\n");
                end_of_file=1;
                return 0;
            }
            past+=32;
            if( !memcmp(parsebuf, WTV_EOF, 16 ))
            {
                dbg_print(CCX_DMT_PARSE, "WTV EOF\n");
                end=1;
            }
            else {
                uint16_t len;
                uint64_t file_length;
                memcpy(&len, parsebuf+16, 2);
                dbg_print(CCX_DMT_PARSE, "len: %x\n", len);
                memcpy(&file_length, parsebuf+24, 8);
                dbg_print(CCX_DMT_PARSE, "file_length: %x\n", file_length);
                if(len>1024)
                {
                    mprint("Too large for buffer!\n");
                    end_of_file=1;
                    return 0;
                }
                buffered_read(&ctx->filebuffer,parsebuf, len-32);
                if (result!=len-32)
                {
                    mprint("Premature end of file!\n");
                    end_of_file=1;
                    return 0;
                }
                past+=len-32;
        		uint32_t text_len;
                memcpy(&text_len, parsebuf, 4);
                dbg_print(CCX_DMT_PARSE, "text_len: %x\n", text_len);
        		char *string;
        		string = (char*)malloc(text_len+1);
                string[text_len]='\0';
        		for(x=0; x<text_len; x++) {
                    memcpy(&string[x], parsebuf+8+(x*2), 1);
        		}
                dbg_print(CCX_DMT_PARSE, "string: %s\n", string);
		        if(strstr(string, WTV_TABLE_ENTRIES)!=NULL) {
			        uint32_t value;
			        uint32_t flag;
                    memcpy(&value, parsebuf+(text_len*2)+8, 4);
                    memcpy(&flag, parsebuf+(text_len*2)+4+8, 4);
                    dbg_print(CCX_DMT_PARSE, "value: %x\n", value);
                    dbg_print(CCX_DMT_PARSE, "flag: %x\n", flag);
                    if(!add_skip_chunks(ctx, &cb, value, flag)) {
                        mprint("Premature end of file!\n");
                        end_of_file=1;
                        return 0;
                    }
		        }
                free(string);
            }
        }
        qsort(cb.skip_chunks, cb.count, sizeof(uint64_t), qsort_cmpint);
        dbg_print(CCX_DMT_PARSE, "skip_chunks: ");
        int x;
        for(x=0; x<cb.count; x++)
            dbg_print(CCX_DMT_PARSE, "%x, ", cb.skip_chunks[x]);
        dbg_print(CCX_DMT_PARSE, "\n");

		buffered_skip(&ctx->filebuffer,(cb.skip_chunks[cb.chunk]+WTV_META_CHUNK_SIZE)-past);
        cb.filepos=(cb.skip_chunks[cb.chunk]+WTV_META_CHUNK_SIZE);
		cb.chunk++;
        past=cb.filepos;
        firstcall=0;
    }
    while(1)
    {
        int bytesread = 0;
        get_sized_buffer(ctx, &cb, 24);

        if(cb.buffer==NULL) {
            mprint("Premature end of file!\n");
            end_of_file=1;
            return 0;
        }
        uint8_t guid[16];
        memcpy(&guid, cb.buffer, 16);
    	int x;
    	for(x=0; x<16; x++)
            dbg_print(CCX_DMT_PARSE, "%02X ", guid[x]);
        dbg_print(CCX_DMT_PARSE, "\n");
        uint32_t len;
        memcpy(&len, cb.buffer+16, 4);
        len-=32;
        dbg_print(CCX_DMT_PARSE, "len %X\n", len);
		uint32_t pad;
		pad = len%8==0 ? 0 : 8- (len % 8);
        dbg_print(CCX_DMT_PARSE, "pad %X\n", pad);
        uint32_t stream_id;
        memcpy(&stream_id, cb.buffer+20, 4);
		stream_id = stream_id & 0x7f;
		dbg_print(CCX_DMT_PARSE, "stream_id: %X\n", stream_id);
        
        get_sized_buffer(ctx, &cb, 8);

        for(x=0; x<num_streams; x++)
            dbg_print(CCX_DMT_PARSE, "video stream_id: %X\n", video_streams[x]);
        if( !memcmp(guid, WTV_EOF, 16 ))
        {
            dbg_print(CCX_DMT_PARSE, "WTV EOF\n");
            do {
                buffered_read(&ctx->filebuffer,parsebuf, 1024);
                past+=1024;
            } while (result==1024);
            past+=result;
            end_of_file=1;
            return 0;
        }
        if( !memcmp(guid, WTV_STREAM2, 16 ) )
        {
			dbg_print(CCX_DMT_PARSE, "WTV STREAM2\n");
            get_sized_buffer(ctx, &cb, 0xc);
            static unsigned char stream_type[16];
            get_sized_buffer(ctx, &cb, 16);
            memcpy(&stream_type, cb.buffer, 16);
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
        }
        if( !memcmp(guid, WTV_TIMING, 16 )  && check_stream_id(stream_id, video_streams, num_streams))
        {
            dbg_print(CCX_DMT_PARSE, "WTV TIMING\n");
            get_sized_buffer(ctx, &cb, 0x8);
            int64_t time;
            get_sized_buffer(ctx, &cb, 0x8);
            memcpy(&time, cb.buffer, 8);
            dbg_print(CCX_DMT_PARSE, "TIME: %ld\n", time);
            if(time!=-1) {
                current_pts = time_to_pes_time(time); 
                pts_set = 1;
                frames_since_ref_time = 0;
                set_fts();  
            }
            len-=16;
        }
        if( !memcmp(guid, WTV_DATA, 16 )  
            && check_stream_id(stream_id, video_streams, num_streams) && current_pts!=0
            && (wtvmpeg2 || (!wtvmpeg2 && len==2)))
        {
            dbg_print(CCX_DMT_PARSE, "\nWTV DATA\n");
            get_sized_buffer(ctx, &cb, len);
            memcpy(buffer+inbuf, cb.buffer, len);
            inbuf+=result;
            bytesread+=(int) len;
            if(pad>0)
                get_sized_buffer(ctx, &cb, pad);
            frames_since_ref_time++;
            set_fts(); 
            return bytesread;
        }
        if(len+pad>0)
            get_sized_buffer(ctx, &cb, len+pad);

    }    
}
