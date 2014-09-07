/* 
 * File:   Msgpack.h
 * Author: lieven2
 *
 * Created on 21 september 2013, 10:39
 */

#ifndef MSGPACK_H
#define	MSGPACK_H
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "Bytes.h"

#define SUPPORT_64BIT_VALUE
#define TYPE_POSITIVE_FIXNUM       (0x00)
#define TYPE_POSITIVE_FIXNUM_MASK  (0x80)
#define TYPE_NEGATIVE_FIXNUM       (0xe0)
#define TYPE_NEGATIVE_FIXNUM_MASK  (0xe0)
#define TYPE_FIXRAW                (0xa0)
#define TYPE_FIXRAW_MASK           (0xe0)
#define TYPE_FIXARRAY              (0x90)
#define TYPE_FIXARRAY_MASK         (0xe0)
#define TYPE_FIXMAP                (0x80)
#define TYPE_FIXMAP_MASK           (0xe0)
#define TYPE_VALIABLE              (0xc0)
#define TYPE_VALIABLE_MASK         (0xe0)
#define TYPE_VALIABLE_NIL     (0xc0) ///< �V���A���C�Y�f�[�^�w�b�_(NIL     )
#define TYPE_VALIABLE_FALSE   (0xc2) ///< �V���A���C�Y�f�[�^�w�b�_(FALSE   )
#define TYPE_VALIABLE_TRUE    (0xc3) ///< �V���A���C�Y�f�[�^�w�b�_(TRUE    )
#define TYPE_VALIABLE_FLOAT   (0xca) ///< �V���A���C�Y�f�[�^�w�b�_(FLOAT   )
#define TYPE_VALIABLE_DOUBLE  (0xcb) ///< �V���A���C�Y�f�[�^�w�b�_(DOUBLE  )
#define TYPE_VALIABLE_UINT8   (0xcc) ///< �V���A���C�Y�f�[�^�w�b�_(UINT8   )
#define TYPE_VALIABLE_UINT16  (0xcd) ///< �V���A���C�Y�f�[�^�w�b�_(UINT16  )
#define TYPE_VALIABLE_UINT32  (0xce) ///< �V���A���C�Y�f�[�^�w�b�_(UINT32  )
#define TYPE_VALIABLE_UINT64  (0xcf) ///< �V���A���C�Y�f�[�^�w�b�_(UINT64  )
#define TYPE_VALIABLE_INT8    (0xd0) ///< �V���A���C�Y�f�[�^�w�b�_(INT8    )
#define TYPE_VALIABLE_INT16   (0xd1) ///< �V���A���C�Y�f�[�^�w�b�_(INT16   )
#define TYPE_VALIABLE_INT32   (0xd2) ///< �V���A���C�Y�f�[�^�w�b�_(INT32   )
#define TYPE_VALIABLE_INT64   (0xd3) ///< �V���A���C�Y�f�[�^�w�b�_(INT64   )
#define TYPE_VALIABLE_RAW16   (0xda) ///< �V���A���C�Y�f�[�^�w�b�_(RAW16   )
#define TYPE_VALIABLE_RAW32   (0xdb) ///< �V���A���C�Y�f�[�^�w�b�_(RAW32   )
#define TYPE_VALIABLE_ARRAY16 (0xdc) ///< �V���A���C�Y�f�[�^�w�b�_(ARRAY16 )
#define TYPE_VALIABLE_ARRAY32 (0xdd) ///< �V���A���C�Y�f�[�^�w�b�_(ARRAY32 )
#define TYPE_VALIABLE_MAP16   (0xde) ///< �V���A���C�Y�f�[�^�w�b�_(MAP16   )
#define TYPE_VALIABLE_MAP32   (0xdf) ///< �V���A���C�Y�f�[�^�w�b�_(MAP32   )

class Msgpack : public Bytes {
public:
    Msgpack(int size);
    Msgpack(uint8_t *pb,int size);
    ~Msgpack();
    void pack(uint8_t v);
    void pack(uint16_t v);
    void pack(uint32_t v);
    void pack(uint64_t v);
    void pack(int8_t v);
    void pack(int16_t v);
    void pack(int32_t v);
    void pack(int64_t v);
    void packNil();
    void pack(bool v);
    void pack(float v);
    void pack(double v);
    void packUInt(unsigned int i);
    void packInt(int i);
    void pack(uint8_t* w, uint32_t length);
    void packRawHeader(uint32_t size);
    void packArrayHeader(uint32_t size);
    void packMapHeader(uint32_t size);
};


typedef union unpack_base_value_t {
    int8_t int8_value;
    uint8_t uint8_value;
    int16_t int16_value;
    uint16_t uint16_value;
    int32_t int32_value;
    uint32_t uint32_value;
    float float_value;
#ifdef SUPPORT_64BIT_VALUE
    int64_t int64_value;
    uint64_t uint64_value;
    double double_value;
#endif
    size_t size;
    uint8_t* raw_data;
} unpack_base_value_t;

typedef struct unpack_base_info_t {
    size_t size; ///< Serialized data size.
    uint8_t type; ///< Data type.
    unpack_base_value_t value; ///< Data value.
} unpack_base_info_t;

unpack_base_info_t unpack_base(uint8_t* buf);

typedef struct unpack_raw_t {
    size_t size;
    uint8_t* data;
} unpack_raw_t;

typedef union unpack_value_t {
    bool bool_value;
    int int_value;
    unsigned int uint_value;
    float float_value;
#ifdef SUPPORT_64BIT_VALUE
    double double_value;
#endif
    unpack_raw_t raw_value;
    size_t size_value;
} unpack_value_t;

typedef enum unpack_type_t {
    UNPACK_TYPE_NIL,
    UNPACK_TYPE_BOOL,
    UNPACK_TYPE_INT,
    UNPACK_TYPE_UINT,
    UNPACK_TYPE_FLOAT,
#ifdef SUPPORT_64BIT_VALUE
    UNPACK_TYPE_DOUBLE,
#endif
    UNPACK_TYPE_RAW,
    UNPACK_TYPE_ARRAY,
    UNPACK_TYPE_MAP
} unpack_type_t;

typedef struct unpack_info_t {
    unpack_type_t type; ///< �f�[�^�^�C�v
    unpack_value_t value; ///< �l
} unpack_info_t;

int unpack(unpack_info_t *info, uint8_t* buf);
#endif	/* MSGPACK_H */

