#ifndef MRMDATABUFFERTYPE_H
#define MRMDATABUFFERTYPE_H

struct mrmDataBufferType {
    typedef enum type_e {
        type_230 = 0,   // configurable-length data buffer
        type_300 = 1,   // segmented data buffer
        type_last = type_300
    } type_t;
};

#endif // MRMDATABUFFERTYPE_H
