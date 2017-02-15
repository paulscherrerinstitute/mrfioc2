#ifndef MRMDATABUFFERTYPE_H
#define MRMDATABUFFERTYPE_H

/**
 * @brief The mrmDataBufferType struct is used to determine the type of data buffer used.
 * This is in a separate header so the users can include as little as needed when building their application for accessing the data buffer.
 */

struct mrmDataBufferType {
    typedef enum type_e {
        type_first = 0,
        type_230 = type_first,   // configurable-length data buffer
        type_300 = 1,   // segmented data buffer
        type_last = type_300
    } type_t;

    static const char* const type_string[]; // definition in mrmDataBuffer.cpp. Contais string representation of enums.
};


#endif // MRMDATABUFFERTYPE_H
