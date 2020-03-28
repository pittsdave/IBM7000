
EXTERN uint8  watchstop;
EXTERN uint32 watchloc;

EXTERN uint8  addrstop;
EXTERN uint32 stopic;

#define TRACE_SIZE 64

EXTERN uint8  itrc_idx;
#ifdef USE64
EXTERN t_uint64 itrc[TRACE_SIZE];
#else
EXTERN uint8  itrc_h[TRACE_SIZE];
EXTERN uint32 itrc_l[TRACE_SIZE];
#endif
EXTERN uint32 itrc_buf[TRACE_SIZE];
