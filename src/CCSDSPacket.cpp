
#include "CCSDSPacket.h"
#include "CCSDSData.h"
#include "CCSDSUtils.h"

#include <iostream>



/**
 * @brief Updates Primary headers data field size.
 *
 * Uses the currently set data field size to set the header.
 *
 * @return none.
 */
void CCSDS::Packet::update() {
    if (!m_updateStatus && m_enableUpdatePacket) {
        const auto dataField = m_dataField.getFullDataField();
        const auto dataFiledSize = static_cast<uint16_t>( dataField.size() );
        const auto dataFieldHeaderFlag(m_dataField.getDataFieldHeaderFlag());
        m_primaryHeader.setDataLength(dataFiledSize);
        m_primaryHeader.setDataFieldHeaderFlag(dataFieldHeaderFlag);
        m_primaryHeader.setSequenceCount(m_sequenceCounter);
        m_CRC16 = crc16(dataField);
        m_updateStatus = true;
    }
}

/**
 * @brief Computes and retrieves the CRC-16 checksum of the packet.
 *
 * If the CRC-16 has not already been calculated, this function computes it
 * using the full data field of the packet. The result is cached for
 * future calls to improve performance.
 *
 * @return The 16-bit CRC-16 checksum.
 */
uint16_t CCSDS::Packet::getCRC() {
    update();
    return m_CRC16;
}

uint16_t CCSDS::Packet::getDataFieldMaximumSize() {
    return m_dataField.getDataFieldAvailableSizeByes();
}

bool CCSDS::Packet::getDataFieldHeaderFlag() {
    update();
    return m_primaryHeader.getDataFieldHeaderFlag();
}

CCSDS::Result<CCSDS::DataField> CCSDS::Packet::getDataField() {
    update();
    return m_dataField;
}

/**
 * @brief Retrieves the CRC-16 checksum as a vector of bytes.
 *
 * The checksum is split into its most significant byte (MSB) and
 * least significant byte (LSB) and stored in a two-element vector.
 *
 * @return A vector containing the MSB and LSB of the CRC-16 checksum.
 */
std::vector<uint8_t> CCSDS::Packet::getCRCVector() {
    std::vector<uint8_t> crc(2);
    const auto crcVar = getCRC();
    crc[0] = (crcVar >> 8) & 0xFF; // MSB (Most Significant Byte)
    crc[1] = crcVar & 0xFF;        // LSB (Least Significant Byte)
    return crc;
}

/**
 * @brief Retrieves the primary header of the packet.
 *
 * This function ensures that the primary header is updated by calling the
 * `updatePrimaryHeader()` function, and then returns the full 48-bit primary
 * header as a 64-bit integer.
 *
 * @return The 64-bit primary header of the packet.
 */
uint64_t CCSDS::Packet::getPrimaryHeader64bit() {
    update();
    return  m_primaryHeader.getFullHeader();
};

/**
 * @brief Retrieves the primary header of the packet as a vector of bytes.
 *
 * Extracts the 48-bit primary header and converts it into a six-element
 * vector, ordered from the most significant byte (MSB) to the least
 * significant byte (LSB).
 *
 * @return A vector containing the six bytes of the primary header.
 */
std::vector<uint8_t> CCSDS::Packet::getPrimaryHeader() {
    update();
    return m_primaryHeader.serialize();
}

std::vector<uint8_t> CCSDS::Packet::getDataFieldHeader() {
    update();
    return m_dataField.getDataFieldHeader();
}

std::vector<uint8_t> CCSDS::Packet::getApplicationData() {
    update();
    return m_dataField.getApplicationData();
}

std::vector<uint8_t> CCSDS::Packet::getFullDataField() {
    return   m_dataField.getFullDataField();
}

/**
 * @brief Retrieves the full packet as a vector of bytes.
 *
 * Combines the primary header, data field, and CRC-16 checksum into
 * a single vector. Ensures that the header and data field sizes meet
 * minimum requirements.
 *
 * @note Header size must be 6 bytes and data field size must be greater than 1 byte.
 *
 * @return A vector containing the full packet in byte form.
 */
std::vector<uint8_t> CCSDS::Packet::serialize() {
    auto header         =       getPrimaryHeader();
    auto dataField = m_dataField.getFullDataField();
    const auto crc      =                 getCRCVector();

    std::vector<uint8_t> packet;
    packet.reserve(header.size() + dataField.size() + crc.size());
    packet.insert(packet.end(),    header.begin(),    header.end());
    if (!getFullDataField().empty()) {
        packet.insert(packet.end(), dataField.begin(), dataField.end());
    }
    packet.insert(packet.end(),       crc.begin(),       crc.end());

    return packet;
}

void CCSDS::Packet::deserialize( const std::vector<uint8_t>& data) {
    if (data.size() > 5) {
        std::vector<uint8_t> dataFieldVector;
        if (data.size() > 7) {
            std::copy(data.begin()+6, data.end(), std::back_inserter(dataFieldVector));
        }
        deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector);
    }
}

void CCSDS::Packet::deserialize( const std::vector<uint8_t>& data, const ESecondaryHeaderType PusType) {
    if (data.size() > 5) {
        uint8_t headerDataSizeBytes;
        std::shared_ptr<PusA> secondaryHeader;

        if (PusType == PUS_A) {
            headerDataSizeBytes = 6;
            m_dataField.setDataFieldHeader({data[6], data[7], data[8], data[9], data[10], data[11]}, PUS_A);
        }else {
            headerDataSizeBytes = 8;
            //todo Deal with cases when it is not Pus
            m_dataField.setDataFieldHeader({data[6], data[7], data[8], data[9], data[10], data[11], data[12], data[13]}, PusType);
        }

        if (data.size() > (6 + headerDataSizeBytes)) {
            std::vector<uint8_t> dataFieldVector;

            if (data.size() > (7 + headerDataSizeBytes)) {
                std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
            }
            deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector);
        }
    }
}

void CCSDS::Packet::deserialize( const std::vector<uint8_t>&  data, const uint16_t headerDataSizeBytes) {
    if (data.size() > 5) {
        if (data.size() > (6 + headerDataSizeBytes)) {
            std::vector<uint8_t> secondaryHeader;
            std::vector<uint8_t> dataFieldVector;
            std::copy(data.begin() + 6, data.begin() + 6 + headerDataSizeBytes, std::back_inserter(secondaryHeader));
            m_dataField.setDataFieldHeader(secondaryHeader);
            if (data.size() > (7 + headerDataSizeBytes)) {
                std::copy(data.begin() + 6 + headerDataSizeBytes, data.end(), std::back_inserter(dataFieldVector));
            }
            deserialize({data[0], data[1], data[2], data[3], data[4], data[5]}, dataFieldVector);
        }
    }
}

void CCSDS::Packet::deserialize(const std::vector<uint8_t>& headerData, const std::vector<uint8_t> &data) {
    m_updateStatus = false;
    if (headerData.size() == 6) {
        m_primaryHeader.deserialize(headerData);

        if (data.size() >= 2) {
            std::vector<uint8_t> dataCopy;
            m_CRC16 = (data[data.size()-2] << 8) + data.back();
            if (data.size() > 2) {
                std::copy(data.begin(), data.end()-2, std::back_inserter(dataCopy));
            }
            m_dataField.setApplicationData(dataCopy);
        }
    }
}

uint16_t CCSDS::Packet::getFullPacketLength() {
    // where 8 is derived from 6 bytes for Primary header and 2 bytes for CRC16.
    return 8 + m_dataField.getDataFieldUsedSizeByes();
}

CCSDS::Packet::Packet(const std::vector<uint8_t>& data) {
    deserialize(data);
    m_updateStatus = false;
}

CCSDS::Packet::Packet(const std::vector<uint8_t>& data, const ESecondaryHeaderType PusType) {
    deserialize(data, PusType);
    m_updateStatus = false;
}

CCSDS::Packet::Packet(const std::vector<uint8_t>& data, const uint16_t headerDataSizeBytes) {
    deserialize(data, headerDataSizeBytes);
    m_updateStatus = false;
}

CCSDS::Packet::Packet(const std::vector<uint8_t>& headerData, const std::vector<uint8_t> &data) {
    deserialize(headerData, data);
    m_updateStatus = false;
}

/**
 * @brief Sets the primary header using the provided 64-bit data.
 *
 * This function sets the primary header of the packet using a 64-bit integer
 * as the header data.
 *
 * @param data The 64-bit primary header data.
 * @return none.
 */
void CCSDS::Packet::setPrimaryHeader( const uint64_t data ) {
    m_primaryHeader.setData( data );
    m_updateStatus = false;
}


/**
 * @brief Sets the primary header using the provided vector of uint8_tdata.
 *
 * This function sets the primary header of the packet using vector of 8-bit integers
 * as the header data.
 *
 * @param data The vector of 8-bit integers primary header data.
 * @return none.
 */
void CCSDS::Packet::setPrimaryHeader( const std::vector<uint8_t>& data ) {
    m_primaryHeader.deserialize( data );
    m_updateStatus = false;
}
/**
 * @brief Sets the primary header using the provided PrimaryHeader object.
 *
 * This function sets the primary header of the packet using a `PrimaryHeader`
 * object as the header data.
 *
 * @param data The `PrimaryHeader` object containing the header data.
 * @return none.
 */
void CCSDS::Packet::setPrimaryHeader( const PrimaryHeader data ) {
    m_primaryHeader.setData( data );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header using the provided PusA header.
 *
 * This function sets the data field header of the packet using a `PusA`
 * header object.
 *
 * @param header The `PusA` header object.
 * @return none.
 */
void CCSDS::Packet::setDataFieldHeader(const PusA& header ) {
    m_dataField.setDataFieldHeader( header );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header using the provided PusB header.
 *
 * This function sets the data field header of the packet using a `PusB`
 * header object.
 *
 * @param header The `PusB` header object.
 * @return none.
 */
void CCSDS::Packet::setDataFieldHeader(const PusB& header ) {
    m_dataField.setDataFieldHeader( header );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header using the provided PusC header.
 *
 * This function sets the data field header of the packet using a `PusC`
 * header object.
 *
 * @param header The `PusC` header object.
 * @return none.
 */
void CCSDS::Packet::setDataFieldHeader(const PusC& header ) {
    m_dataField.setDataFieldHeader( header );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header for the packet using a vector of bytes.
 *
 * This method updates the data field header of the packet by providing the
 * data as a vector of bytes and specifying the PUS (Packet Utilization Standard) type if applicable.
 *
 * @param data A vector containing the data for the data field header.
 * @param type The PUS type that specifies the purpose of the data.
 */
void CCSDS::Packet::setDataFieldHeader(const std::vector<uint8_t> &data, const ESecondaryHeaderType type) {
    m_dataField.setDataFieldHeader( data, type );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header for the packet using a raw data pointer.
 *
 * This method updates the data field header of the packet by providing a
 * pointer to the raw data and its size, along with the PUS (Packet Utilization Standard) type if applicable.
 *
 * @note This method is potentially unsafe as it relies on a raw pointer and size.
 * Ensure that the pointer is valid and the size accurately reflects the data length
 * to avoid undefined behavior such as buffer overflows or invalid memory access.
 *
 * @param pData Pointer to the raw data for the data field header.
 * @param sizeData The size of the data pointed to by pData, in bytes.
 * @param type The PUS type that specifies the purpose of the data.
 */
void CCSDS::Packet::setDataFieldHeader(const uint8_t *pData, const size_t sizeData, const ESecondaryHeaderType type) {
    m_dataField.setDataFieldHeader( pData, sizeData, type );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header using the provided vector of bytes.
 *
 * This function sets the data field header of the packet using a vector
 * of bytes.
 *
 * @param data The vector containing the header bytes.
 * @return none.
 */
void CCSDS::Packet::setDataFieldHeader( const std::vector<uint8_t>& data ) {
    m_dataField.setDataFieldHeader( data );
    m_updateStatus = false;
}

/**
 * @brief Sets the data field header using the provided pointer and size.
 *
 * This function sets the data field header of the packet using a pointer
 * to the header data and its size.
 *
 * @note This method is potentially unsafe as it relies on a raw pointer and size.
 * Ensure that the pointer is valid and the size accurately reflects the data length
 * to avoid undefined behavior such as buffer overflows or invalid memory access.
 *
 * @param pData A pointer to the header data.
 * @param sizeData The size of the header data.
 * @return none.
 */
void CCSDS::Packet::setDataFieldHeader( const uint8_t* pData, const size_t sizeData ) {
    m_dataField.setDataFieldHeader( pData,sizeData );
    m_updateStatus = false;
}

/**
 * @brief Sets the application data for the packet.
 *
 * This function sets the application data in the data field of the packet
 * using a vector of bytes.
 *
 * @param data The vector containing the application data.
 * @return none.
 */
void CCSDS::Packet::setApplicationData( const std::vector<uint8_t>& data ) {
    m_dataField.setApplicationData( data );
    m_updateStatus = false;
}

/**
 * @brief Sets the application data for the packet.
 *
 * This function sets the application data in the data field of the packet
 * using a pointer to the data and its size.
 *
 * @note This method is potentially unsafe as it relies on a raw pointer and size.
 * Ensure that the pointer is valid and the size accurately reflects the data length
 * to avoid undefined behavior such as buffer overflows or invalid memory access.
 *
 * @param pData A pointer to the application data.
 * @param sizeData The size of the application data.
 * @return none.
 */
void CCSDS::Packet::setApplicationData( const uint8_t* pData, const size_t sizeData ) {
    m_dataField.setApplicationData( pData,sizeData );
    m_updateStatus = false;
}


/**
 * @brief Sets the sequence flags for the packet's primary header.
 *
 * This method updates the sequence flags in the primary header of the packet.
 * The sequence flags indicate the position or type of the data segment within
 * the CCSDS telemetry transfer frame (e.g., first segment, last segment, etc.).
 *
 * @param flags The sequence flag to be set, represented by the ESequenceFlag enum : uint8_t.
 */
void CCSDS::Packet::setSequenceFlags(const ESequenceFlag flags) {
    m_primaryHeader.setSequenceFlags(flags);
    m_updateStatus = false;
}

void CCSDS::Packet::setSequenceCount(const uint16_t count) {
    m_sequenceCounter = count;
    m_updateStatus = false;
}


/**
 * @brief Sets the maximum data packet size for the CCSDS DataField.
 *
 * This method updates the maximum allowed size for the data packet.
 * The data packet size is used to validate that the combined size of
 * the header and application data does not exceed this limit.
 *
 * @param size The maximum size of the data packet, in bytes.
 */
void CCSDS::Packet::setDataFieldSize(const uint16_t size) {
    m_dataField.setDataPacketSize(size);
}

void CCSDS::Packet::setUpdatePacketEnable(const bool enable) {
    m_enableUpdatePacket = enable;
    m_dataField.setDataFieldHeaderAutoUpdateStatus(enable);
}
