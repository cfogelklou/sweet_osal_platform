#ifdef WIN32
// push current alignment rules to internal stack
#pragma pack(push)
// force 1-byte alignment boundary
#pragma pack(1)
// define PACKED to nothing if not already defined

#else

// define PACKED to something gcc understands, if not already defined
#ifndef PACKED1
#define PACKED1 __attribute__((packed))
#endif // PACKED2

#endif // WIN32

#ifndef PACKED1
#define PACKED1
#endif // PACKED1

// define PACKED to something gcc understands, if not already defined
#ifndef PACKED2
#define PACKED2
#endif // PACKED2

#ifndef PACKED
#define PACKED PACKED1
#endif // PACKED
