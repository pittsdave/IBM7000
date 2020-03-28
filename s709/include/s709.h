
uint16 getxr(int);
void setxr(uint32 r);
void traptrace();
int chk_user();
void steal_cycle();
void access1(uint16);
void access2(uint16);
void load(uint16,uint8);
#ifdef USE64
void store(uint16,t_uint64,uint8);
#else
void store(uint16,uint8,uint32,uint8);
#endif
int trace_open (char *);
void trace_close (void);
