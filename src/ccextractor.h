#ifndef CCX_CCEXTRACTOR_H
#define CCX_CCEXTRACTOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <fcntl.h> 
#include <stdarg.h>
#include <errno.h>

// compatibility across platforms
#include "platform.h"
#include "buffer.h"

#define VERSION "0.69"



struct ccx_context_t {
    ccx_filebuffer_context_t filebuffer;
};

extern void ccx_init_context(ccx_context_t* ctx);

extern int cc_buffer_saved; // Do we have anything in the CC buffer already? 
extern int ccblocks_in_avc_total; // Total CC blocks found by the AVC code
extern int ccblocks_in_avc_lost; // CC blocks found by the AVC code lost due to overwrites (should be 0)

#include "608.h"
#include "708.h"
#include "bitstream.h"
#include "constants.h"


struct ccx_boundary_time
{
    int hh,mm,ss;
    LLONG time_in_ms;
    int set;
};

struct ccx_s_write
{
    int fh;
    char *filename;
    struct eia608 *data608;
	int my_field; // Used for sanity checks
	long bytes_processed_608; // To be written ONLY by process_608
    void* spupng_data;
};


struct gop_time_code
{
  int drop_frame_flag;
  int time_code_hours;
  int time_code_minutes;
  int marker_bit;
  int time_code_seconds;
  int time_code_pictures;
  int inited;
  LLONG ms;
};



// Stuff for telcc.cpp
struct ccx_s_teletext_config {
    uint8_t verbose : 1; // should telxcc be verbose?
    uint16_t page; // teletext page containing cc we want to filter
    uint16_t tid; // 13-bit packet ID for teletext stream
    double offset; // time offset in seconds
    uint8_t bom : 1; // print UTF-8 BOM characters at the beginning of output
    uint8_t nonempty : 1; // produce at least one (dummy) frame
    // uint8_t se_mode : 1; // search engine compatible mode => Uses CCExtractor's write_format
    // uint64_t utc_refvalue; // UTC referential value => Moved to CCExtractor global, so can be used for 608 too
    uint16_t user_page; // Page selected by user, which MIGHT be different to 'page' depending on autodetection stuff
};

//params.cpp
void parse_parameters (int argc, char *argv[]);
void usage (void);

// general_loop.cpp
void position_sanity_check ();
LLONG ps_getmoredata( ccx_context_t* ctx);
LLONG general_getmoredata( ccx_filebuffer_context_t* fb);
void raw_loop (ccx_context_t* ctx);
LLONG process_raw (void);
void general_loop(ccx_context_t* ctx);
void processhex (char *filename);
void rcwt_loop(ccx_context_t* ctx);

// activity.cpp
void activity_header (void);
void activity_progress (int percentaje, int cur_min, int cur_sec);
void activity_report_version (void);
void activity_input_file_closed (void);
void activity_input_file_open (const char *filename);
void activity_message (const char *fmt, ...);
void  activity_video_info (int hor_size,int vert_size, 
    const char *aspect_ratio, const char *framerate);
void activity_program_number (unsigned program_number);
void activity_xds_program_name (const char *program_name);
void activity_xds_network_call_letters (const char *program_name);
void activity_xds_program_identification_number (unsigned minutes, unsigned hours, unsigned date, unsigned month);
void activity_xds_program_description (int line_num, const char *program_desc);
void activity_report_data_read (void);

extern unsigned long net_activity_gui;
extern int end_of_file;
extern LLONG inbuf;
extern int ccx_bufferdatatype; // Can be RAW or PES

// asf_functions.cpp
LLONG asf_getmoredata( ccx_context_t* ctx );

extern int wtvconvertfix;

// wtv_functions.cpp
LLONG wtv_getmoredata( ccx_context_t* ctx);

extern int wtvmpeg2;

// avc_functions.cpp
LLONG process_avc (unsigned char *avcbuf, LLONG avcbuflen);

extern int usepicorder;

// es_functions.cpp
LLONG process_m2v (unsigned char *data, LLONG length);

extern unsigned top_field_first;

// es_userdata.cpp
int user_data(struct bitstream *ustream, int udtype);

// bitstream.cpp - see bitstream.h

// 608.cpp
int write_cc_buffer (struct ccx_s_write *wb);
unsigned char *debug_608toASC (unsigned char *ccdata, int channel);


// cc_decoders_common.cpp
LLONG get_visible_start (void);
LLONG get_visible_end (void);

// file_functions.cpp
LLONG getfilesize (int in);
LLONG gettotalfilessize (void);
void prepare_for_new_file (ccx_context_t* ctx);
void close_input_file (void);
int switch_to_next_file (LLONG bytesinbuffer);
int init_sockets (void);
void return_to_buffer (ccx_filebuffer_context_t* fb, unsigned char *buffer, unsigned int bytes);

// timing.cpp
void set_fts(void);
LLONG get_fts(void);
LLONG get_fts_max(void);
char *print_mstime( LLONG mstime );
char *print_mstime2buf( LLONG mstime , char *buf );
void print_debug_timing( void );
int gop_accepted(struct gop_time_code* g );
void calculate_ms_gop_time (struct gop_time_code *g);

// sequencing.cpp
void init_hdcc (void);
void store_hdcc(unsigned char *cc_data, int cc_count, int sequence_number, LLONG current_fts);
void anchor_hdcc(int seq);
void process_hdcc (void);
int do_cb (unsigned char *cc_block);

// mp4.cpp
int processmp4 (char *file);

// params_dump.cpp
void params_dump(void);

// output.cpp
void init_write (struct ccx_s_write *wb, int field);
void writeraw (const unsigned char *data, int length, struct ccx_s_write *wb);
void writedata (const unsigned char *data, int length, struct ccx_s_write *wb);
void flushbuffer (struct ccx_s_write *wb, int closefile);
void printdata (const unsigned char *data1, int length1,const unsigned char *data2, int length2);
void writercwtdata (const unsigned char *data);

// stream_functions.cpp
void detect_stream_type (ccx_context_t* ctx);
int detect_myth( void );
int read_video_pes_header (ccx_context_t* ctx, unsigned char *header, int *headerlength, int sbuflen);

// ts_functions.cpp
void init_ts_constants( void );
int ts_readpacket(ccx_context_t* ctx);
long ts_readstream(ccx_context_t* ctx);
LLONG ts_getmoredata( ccx_context_t* ctx);

// myth.cpp
void myth_loop(ccx_filebuffer_context_t* fb);

// mp4_bridge2bento4.cpp
void mp4_loop (char *filename);

// xds.cpp
void process_xds_bytes (const unsigned char hi, int lo);
void do_end_of_xds (unsigned char expected_checksum);
void xds_init();

// ccextractor.cpp
LLONG calculate_gop_mstime (struct gop_time_code *g);
void set_fts(void);
LLONG get_fts ( void );
LLONG get_fts_max ( void );
char *print_mstime( LLONG mstime );
void print_debug_timing( void );
int switch_to_next_file (LLONG bytesinbuffer);

// utility.cpp
void fatal(int exit_code, const char *fmt, ...);
void dvprint(const char *fmt, ...);
void mprint (const char *fmt, ...);
void subsprintf (const char *fmt, ...);
void dbg_print(LLONG mask, const char *fmt, ...);
void fdprintf (int fd, const char *fmt, ...);
void init_boundary_time (ccx_boundary_time *bt);
void sleep_secs (int secs);
void dump (LLONG mask, unsigned char *start, int l, unsigned long abs_start, unsigned clear_high_bit);
bool_t in_array(uint16_t *array, uint16_t length, uint16_t element) ;
int hex2int (char high, char low);
void timestamp_to_srttime(uint64_t timestamp, char *buffer);
void millis_to_date (uint64_t timestamp, char *buffer) ;
int levenshtein_dist (const uint64_t *s1, const uint64_t *s2, unsigned s1len, unsigned s2len);

void init_eia608 (struct eia608 *data);
unsigned encode_line (unsigned char *buffer, unsigned char *text);
void write_subtitle_file_header (struct ccx_s_write *wb);
void write_subtitle_file_footer (struct ccx_s_write *wb);
extern void build_parity_table(void);

void tlt_process_pes_packet(uint8_t *buffer, uint16_t size) ;
void telxcc_init(void);
void telxcc_close(void);

extern struct gop_time_code gop_time, first_gop_time, printed_gop;
extern int gop_rollover;
extern LLONG min_pts, sync_pts, current_pts;
extern uint32_t global_timestamp, min_global_timestamp;
extern int global_timestamp_inited;
extern LLONG fts_now; // Time stamp of current file (w/ fts_offset, w/o fts_global)
extern LLONG fts_offset; // Time before first sync_pts
extern LLONG fts_fc_offset; // Time before first GOP
extern LLONG fts_max; // Remember the maximum fts that we saw in current file
extern LLONG fts_global; // Duration of previous files (-ve mode)
// Count 608 (per field) and 708 blocks since last set_fts() call
extern int cb_field1, cb_field2, cb_708;
extern int saw_caption_block;


extern unsigned char *buffer;
extern LLONG past;
extern LLONG total_inputsize, total_past; // Only in binary concat mode

extern char **inputfile;
extern int current_file;


extern ccx_datasource input_source;

extern in_addr_t udpaddr; // UDP host address if using network instead of files
extern unsigned udpport; // UDP port if using network instead of files
extern struct sockaddr_in servaddr, cliaddr;

extern int strangeheader;

extern unsigned char startbytes[STARTBYTESLENGTH]; 
extern unsigned int startbytes_pos;
extern int startbytes_avail; // Needs to be able to hold -1 result.

extern unsigned char *pesheaderbuf;
extern int pts_set; //0 = No, 1 = received, 2 = min_pts set

extern int MPEG_CLOCK_FREQ; // This is part of the standard

extern unsigned pts_big_change;
extern unsigned total_frames_count;
extern unsigned total_pulldownfields;
extern unsigned total_pulldownframes;

extern int CaptionGap;

extern int live_stream;



extern const char *desc[256];

extern FILE *fh_out_elementarystream;
extern int infd;
extern int false_pict_header;

extern int stat_numuserheaders;
extern int stat_dvdccheaders;
extern int stat_scte20ccheaders;
extern int stat_replay5000headers;
extern int stat_replay4000headers;
extern int stat_dishheaders;
extern int stat_hdtv;
extern int stat_divicom;
extern ccx_stream_mode_enum stream_mode;
extern int use_gop_as_pts;
extern int fix_padding; 
extern int rawmode; 
extern int extract; 
extern int cc_stats[4];
extern LLONG inputsize;
extern int cc_channel;
extern ccx_encoding_type encoding ;
extern int direct_rollup;
extern LLONG subs_delay; 
extern struct ccx_boundary_time extraction_start, extraction_end; 
extern struct ccx_boundary_time startcreditsnotbefore, startcreditsnotafter;
extern struct ccx_boundary_time startcreditsforatleast, startcreditsforatmost;
extern struct ccx_boundary_time endcreditsforatleast, endcreditsforatmost;
extern int startcredits_displayed, end_credits_displayed;
extern LLONG last_displayed_subs_ms; 
extern LLONG screens_to_process;
extern int processed_enough;
extern int nofontcolor;
extern int notypesetting;
extern unsigned char usercolor_rgb[8];
extern color_code default_color;
extern int sentence_cap;
extern int binary_concat;
extern int trim_subs;
extern int norollup;
extern int forced_ru;
extern int gui_mode_reports;
extern int no_progress_bar;
extern const char *extension;
extern unsigned ucla_settings; // Enables convenient settings for UCLA's project.
extern char millis_separator;
extern int levdistmincnt, levdistmaxpct;
extern long FILEBUFFERSIZE; // Uppercase because it used to be a define



/* General (ES stream) video information */
extern unsigned current_hor_size;
extern unsigned current_vert_size;
extern unsigned current_aspect_ratio;
extern unsigned current_frame_rate;
extern double current_fps;


extern unsigned long net_activity_gui;
extern int end_of_file;
extern LLONG inbuf;
extern ccx_bufferdata_type bufferdatatype; // Can be CCX_BUFFERDATA_TYPE_RAW or CCX_BUFFERDATA_TYPE_PES

extern int wtvconvertfix;

extern int wtvmpeg2;

extern int usepicorder;

extern unsigned top_field_first;

extern int firstcall;
extern LLONG minimum_fts; // No screen should start before this FTS

#define MAXBFRAMES 50
#define SORTBUF (2*MAXBFRAMES+1)
extern int cc_data_count[SORTBUF];
extern unsigned char cc_data_pkts[SORTBUF][10*31*3+1];
extern int has_ccdata_buffered;
extern int current_field;

extern int last_reported_progress;
extern int buffer_input;
extern LLONG debug_mask; 
extern LLONG debug_mask_on_debug;
extern int investigate_packets;
extern int messages_target;
extern int cc_to_stdout;

extern unsigned ts_forced_program;
extern unsigned ts_forced_program_selected;
extern unsigned hauppauge_warning_shown;
extern unsigned hauppauge_mode;
extern unsigned mp4vidtrack;
extern int nosync;
extern int fullbin;
extern unsigned char *subline;
extern int saw_gop_header;
extern int max_gop_length;
extern int last_gop_length;
extern int frames_since_last_gop;
extern LLONG fts_at_gop_start;
extern int frames_since_ref_time;
extern ccx_stream_mode_enum auto_stream;
extern int num_input_files;
extern char *basefilename;
extern int do_cea708; // Process 708 data?
extern int cea708services[63]; // [] -> 1 for services to be processed
extern char *sentence_cap_file;
extern int auto_myth;
extern struct ccx_s_write wbout1, wbout2, *wbxdsout;
extern int export_xds;
extern int line_terminator_lf;
extern int autodash;
extern int noautotimeref;

extern char **spell_lower;
extern char **spell_correct;
extern int spell_words;
extern int spell_capacity;

extern char *output_filename;
extern char *out_elementarystream_filename;

extern unsigned char encoded_crlf[16]; // We keep it encoded here so we don't have to do it many times
extern unsigned int encoded_crlf_length;
extern unsigned char encoded_br[16];
extern unsigned int encoded_br_length;
extern ccx_output_format write_format;
extern ccx_output_date_format date_format;

extern ccx_frame_type current_picture_coding_type; 
extern int current_tref; // Store temporal reference of current frame

extern int cc608_parity_table[256]; // From myth

// From ts_functions
extern unsigned cappid ; // PID for stream that holds caption information
extern unsigned forced_cappid ; 
extern unsigned forced_streamtype;
extern int datastreamtype;
extern unsigned autoprogram;

// Credits stuff
extern char *start_credits_text;
extern char *end_credits_text;

#define HAUPPAGE_CCPID	1003 // PID for CC's in some Hauppauge recordings

/* Exit codes. Take this seriously as the GUI depends on them. 
   0 means OK as usual,
   <100 means display whatever was output to stderr as a warning
   >=100 means display whatever was output to stdout as an error
*/

#define EXIT_OK                                 0
#define EXIT_NO_INPUT_FILES                     2
#define EXIT_TOO_MANY_INPUT_FILES               3
#define EXIT_INCOMPATIBLE_PARAMETERS            4
#define EXIT_FILE_CREATION_FAILED               5
#define EXIT_UNABLE_TO_DETERMINE_FILE_SIZE      6
#define EXIT_MALFORMED_PARAMETER                7
#define EXIT_READ_ERROR                         8
#define EXIT_UNSUPPORTED						9
#define EXIT_NOT_CLASSIFIED                     300
#define EXIT_NOT_ENOUGH_MEMORY                  500
#define EXIT_ERROR_IN_CAPITALIZATION_FILE       501
#define EXIT_BUFFER_FULL                        502
#define EXIT_BUG_BUG                            1000
#define EXIT_MISSING_ASF_HEADER                 1001
#define EXIT_MISSING_RCWT_HEADER                1002

extern int PIDs_seen[65536];
extern struct PMT_entry *PIDs_programs[65536];

extern LLONG ts_start_of_xds; 
extern int timestamps_on_transcript;

extern unsigned telext_mode;

extern int temp_debug;

extern uint64_t utc_refvalue; // UTC referential value
extern struct ccx_s_teletext_config tlt_config;
extern uint32_t tlt_packet_counter;
extern uint32_t tlt_frames_produced;

#endif
