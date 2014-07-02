/***************************************************************************//**
* @file    crc8.h
* @brief   CRC8.
* @author  A. Filyanov
*******************************************************************************/
#ifndef _CRC8_H_
#define _CRC8_H_

#if defined (__cplusplus)
extern "C"
{
#endif

#include "os_config.h"
#include "typedefs.h"

/*****************************************************************************/
/// @brief      CRC8 computation.
/// @param[in]  data_p     Input data.
/// @param[in]  size       Input data size.
/// @return     CRC8.
U8              Crc8(U8* data_p, U16 size);

/// @brief      CRC8 delta computation.
/// @param[in]  value      Input value.
/// @param[in]  init_poly  Input polynom.
/// @return     CRC8.
U8              Crc8Delta(const U8 value, const U8 init_poly);

#if defined (__cplusplus)
}
#endif // __cplusplus

#endif // _CRC8_H_
