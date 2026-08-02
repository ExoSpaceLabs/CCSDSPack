// Copyright 2025-2026 ExoSpaceLabs
// SPDX-License-Identifier: Apache-2.0

/**
 * @file CCSDSPack.h
 * @brief Convenience umbrella header for the complete public CCSDSPack API.
 *
 * Include this header when an application uses several library components. More
 * constrained builds may include individual headers instead. Config support is
 * excluded automatically when CCSDS_MCU is defined.
 *
 * @par Typical workflow
 *
 * - Use CCSDS::Packet for one Space Packet.
 * - Use CCSDS::Manager for one packet-identifier stream and sequence counter.
 * - Use CCSDS::Validator for explicit coherence/template checks.
 * - Derive from CCSDS::SecondaryHeaderAbstract for mission-specific headers.
 *
 * @code{.cpp}
 * #include <CCSDSPack.h>
 *
 * CCSDS::Packet packet;
 * packet.setPrimaryHeader({0, 0, 0, 1, CCSDS::UNSEGMENTED, 0, 0});
 * packet.setApplicationData({0x01, 0x02});
 * const auto bytes = packet.serialize();
 * @endcode
 */
#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include "CCSDSDataField.h"
#include "CCSDSHeader.h"
#include "CCSDSManager.h"
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSUtils.h"
#include "CCSDSValidator.h"
#include "PusServices.h"

#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif

#endif // CCSDSPACK_H
