/***************************************************************************//**
* @file    crc32.h
* @brief   CRC32.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _CRC32_H_
#define _CRC32_H_

#if defined (__cplusplus)
extern "C"
{
#endif

#include "os_config.h"
#include "typedefs.h"

//-----------------------------------------------------------------------------
#define CRC32_POLYNOMIAL        0xFFFFFFFFU

/*****************************************************************************/
/// @brief      CRC32 computation.
/// @param[in]  data_p     Input data.
/// @param[in]  size       Input data size.
/// @return     CRC32.
U32             Crc32(U8* data_p, U32 size);

/// @brief      CRC32 delta computation.
/// @param[in]  data_p     Input data.
/// @param[in]  size       Input data size.
/// @param[in]  init_poly  Input polynom.
/// @return     CRC32.
U32             Crc32Delta(U8* data_p, U32 size, U32 init_poly);

#if defined (__cplusplus)
}
#endif // __cplusplus

#endif // _CRC32_H_
