#ifndef MRMDATABUFFERTYPE_H
#define MRMDATABUFFERTYPE_H

class epicsShareClass mrmDataBufferType {
public:
    typedef enum type_e {
        type_230 = 0,   // configurable-length data buffer
        type_300        // segmented data buffer
    } type_t;
};

#endif // MRMDATABUFFERTYPE_H
