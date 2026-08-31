// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPack.h
 * @brief Convenience umbrella header for the complete public CCSDSPack API.
 *
 * Packet-level error control and direction use generic ccsds types. Standards PUS
 * revision/direction identity comes from the concrete rev_a/rev_c TC/TM header,
 * while optional mission-specific PUS layout choices use the direction-specific
 * tailoring structs in PusTailoring.h.
 */
#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include "CCSDSDataField.h"
#include "CCSDSHeader.h"
#include "CCSDSManager.h"
#include "CCSDSPacket.h"
#include "CCSDSPacketTypes.h"
#include "CCSDSTime.h"
#include "CCSDSResult.h"
#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSUtils.h"
#include "CCSDSValidator.h"
#include "CCSDSBuffer.h"
#include "PusTailoring.h"
#include "PusSecondaryHeaderFactory.h"
#include "PusSecondaryHeaders.h"

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

#endif // CCSDSPACK_H
