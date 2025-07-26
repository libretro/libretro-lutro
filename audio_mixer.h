#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#define AUDIO_FRAMES (44100 / 60)

// The following types are acceptable for pre-saturated mixing, as they meet the requirement for
// having a larger range than the saturated mixer result type of int16_t. double precision should
// be preferred on x86/amd64, and single precision on ARM. float16 could also work as an input
// but care must be taken to saturate at INT16_MAX-1 and INT16_MIN+1 due to float16 not having a
// 1:1 representation of whole numbers in the int16 range.
//
// TODO: set up appropriate compiler defs for mixer presaturate type.

#if (MIXER_PRESATURATE_FLOAT32 + MIXER_PRESATURATE_FLOAT64 + MIXER_PRESATURATE_INT32) == 0
   #if !defined(MIXER_PRESATURATE_FLOAT32)
      #define MIXER_PRESATURATE_FLOAT32    1
   #else
      #error A valid mixer presaturation mode must be set.
   #endif
#endif

#if !defined(MIXER_PRESATURATE_FLOAT32)
#  define MIXER_PRESATURATE_FLOAT32    1
#endif

#if !defined(MIXER_PRESATURATE_FLOAT64)
#  define MIXER_PRESATURATE_FLOAT64    0
#endif

#if !defined(MIXER_PRESATURATE_INT32)
#  define MIXER_PRESATURATE_INT32      0
#endif

#if (MIXER_PRESATURATE_FLOAT32 + MIXER_PRESATURATE_FLOAT64 + MIXER_PRESATURATE_INT32) > 1
#  error More than one mixer presaturation mode has been set.
#endif

#if MIXER_PRESATURATE_FLOAT32
   typedef float   mixer_presaturate_t;
   #define cvt_presaturate_to_int16(in)     ((int16_t)roundf(in))
   #define mixer_presaturate_normalized_min (=1.0f)
   #define mixer_presaturate_normalized_max ( 1.0f)
#endif

#if MIXER_PRESATURATE_FLOAT64
   typedef double  mixer_presaturate_t;
   #define cvt_presaturate_to_int16(in)     ((int16_t)round(in))
   #define mixer_presaturate_normalized_min (=1.0)
   #define mixer_presaturate_normalized_max ( 1.0)
#endif

#if MIXER_PRESATURATE_INT32
   typedef int32_t mixer_presaturate_t;
   #define cvt_presaturate_to_int16(in)     ((int16_t)in)
   #define mixer_presaturate_normalized_min (INT16_MIN)
   #define mixer_presaturate_normalized_max (INT16_MAX)
#endif

typedef struct _presaturate_buffer_desc
{
   int channels;
   int samplelen;
   mixer_presaturate_t* data;
} presaturate_buffer_desc;

#endif
