#ifndef PACKED
#if defined (__IAR_SYSTEMS_ICC__)
#define XDATA
#define CODE
#define DATA_ALIGN(x)               _Pragma data_alignment=(x)
#define PACKED1                     __packed
#define PACKED                      PACKED1
#define PACKED_STRUCT               PACKED1 struct
#define PACKED_TYPEDEF_STRUCT       PACKED1 typedef struct
#define PACKED_TYPEDEF_CONST_STRUCT PACKED1 typedef const struct
#define PACKED_TYPEDEF_UNION        PACKED1 typedef union

#elif defined __TI_COMPILER_VERSION || defined __TI_COMPILER_VERSION__
#define XDATA
#define CODE
#define DATA
#define NEARFUNC
#define PACKED1                     __attribute__((__packed__))
#define PACKED                      PACKED1
#define PACKED_STRUCT               struct PACKED1
#define PACKED_TYPEDEF_STRUCT       typedef struct PACKED1
#define PACKED_TYPEDEF_CONST_STRUCT typedef const struct PACKED1
#define PACKED_TYPEDEF_UNION        typedef union PACKED1

#elif defined (__GNUC__)
#define PACKED1 __attribute__((__packed__))
#define PACKED_STRUCT               struct PACKED1
#define PACKED_TYPEDEF_STRUCT       typedef struct PACKED1
#define PACKED_TYPEDEF_CONST_STRUCT typedef const struct PACKED1
#define PACKED_TYPEDEF_UNION        typedef union PACKED1

#else
#ifndef PACKED1
#define PACKED1
#endif

#ifndef PACKED2
#define PACKED2
#endif

#ifndef PACKED
#define PACKED PACKED1
#endif

#endif
#endif // PACKED

