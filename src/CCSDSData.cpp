

#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <iostream>

/**
 * @brief Retrieves the full data field by combining the data field header and application data.
 *
 * Combines the secondary header (if present) and application data into a single vector.
 * Ensures that the total size does not exceed the maximum allowed data packet size.
 *
 * @return A vector containing the full data field (header + application data).
 */
std::vector<uint8_t> CCSDS::DataField::getFullDataField() {
    update();
    const auto& dataFieldHeader = getDataFieldHeader();
    std::vector<uint8_t> fullData{};
    if(!dataFieldHeader.empty()) {
        fullData.insert(fullData.end(),dataFieldHeader.begin(),dataFieldHeader.end());
    }
    fullData.insert(fullData.end(),m_applicationData.begin(),m_applicationData.end());
    return fullData;
}

std::vector<uint8_t> CCSDS::DataField::getApplicationData() {
    update();
    return m_applicationData;
}

uint16_t CCSDS::DataField::getDataFieldAvailableSizeByes() {
    update();
    return m_dataPacketSize - getDataFieldUsedSizeByes();
}

uint16_t CCSDS::DataField::getDataFieldAbsoluteSizeByes() {
    update();
    return m_dataPacketSize;
}

uint16_t CCSDS::DataField::getDataFieldUsedSizeByes() {
    update();
    if (!getDataFieldHeaderFlag()) {
        return m_applicationData.size();
    }
    if (!m_dataFieldHeader.empty()) {
        return m_applicationData.size() + m_dataFieldHeader.size();
    }
    if (m_pusHeaderData != nullptr) {
        return m_applicationData.size() + m_pusHeaderData->getSize();
    }
    return 0;
}

/**
 * @brief Updates the data field header based on the current application data size.
 *
 * Updates the length field in the secondary header to match the size of the
 * application data. Ensures the header reflects the most recent data state.
 *
 * @return None.
 */
void CCSDS::DataField::update() {
    if (!m_dataFieldHeaderUpdated && m_enableDataFieldUpdate) {
        if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
            m_pusHeaderData->setDataLength(m_applicationData.size());;
        }
        m_dataFieldHeaderUpdated = true;
    }
}

/**
 * @brief Sets the application data for the data field.
 *
 * Validates and assigns the given application data to the data field.
 * Ensures the data size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the header.
 *
 * @param pData A pointer to the application data.
 * @param sizeData The size of the application data in bytes.
 *
 * @return none.
 */
CCSDS::ResultBool CCSDS::DataField::setApplicationData(const uint8_t *pData, const size_t &sizeData) {
    RET_IF_ERR_MSG(!pData,ErrorCode::NULL_POINTER,"Application data is nullptr");
    RET_IF_ERR_MSG(sizeData < 1,ErrorCode::INVALID_APPLICATION_DATA,"Application data size cannot be < 1");
    RET_IF_ERR_MSG(sizeData > getDataFieldAvailableSizeByes(),ErrorCode::INVALID_APPLICATION_DATA,"Application data field exceeds available size.");

    if (!m_applicationData.empty()) {
        std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
    }
    m_applicationData.assign(pData, pData + sizeData);
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the application data using a vector of bytes.
 *
 * Replaces the current application data with the given vector and updates the header.
 *
 * @param applicationData A vector containing the application data bytes.
 *
 * @return None.
 */
CCSDS::ResultBool CCSDS::DataField::setApplicationData(const std::vector<uint8_t> &applicationData) {
    RET_IF_ERR_MSG(applicationData.size() > getDataFieldAvailableSizeByes(),ErrorCode::INVALID_APPLICATION_DATA,"Application data field exceeds available size.");
    m_applicationData = applicationData;
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the secondary header for the data field.
 *
 * Validates and assigns the given header data to the secondary header field.
 * Ensures the header size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the application data.
 *
 * @param pData A pointer to the header data.
 * @param sizeData The size of the header data in bytes.
 *
 * @return none.
 */
CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const uint8_t *pData, const size_t &sizeData) {

    RET_IF_ERR_MSG(!pData,ErrorCode::NULL_POINTER,"Secondary header data is nullptr");
    RET_IF_ERR_MSG(sizeData < 1,ErrorCode::INVALID_SECONDARY_HEADER_DATA,"Secondary header data size cannot be < 1");
    RET_IF_ERR_MSG(sizeData > getDataFieldAvailableSizeByes(),ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Secondary header data exceeds available size.");

    if (!m_dataFieldHeader.empty()) {
        m_dataFieldHeader.clear();
    }
    m_dataFieldHeader.assign(pData, pData + sizeData);
    m_dataFieldHeaderType = OTHER;
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the secondary header for the data field using a PUS Type.
 *
 * Validates and assigns the given header data to the secondary header field.
 * Ensures the header size is within acceptable limits and does not exceed
 * the remaining packet size after accounting for the application data.
 *
 * @param pData A pointer to the header data.
 * @param sizeData The size of the header data in bytes.
 * @param pType enum of type PUSType to select
 *
 * @return none.
 */
CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const uint8_t *pData, const size_t &sizeData,
                                                       const ESecondaryHeaderType &pType) {

    RET_IF_ERR_MSG(!pData, ErrorCode::NULL_POINTER,"Secondary header data is nullptr" );
    RET_IF_ERR_MSG(pType == NA, ErrorCode::INVALID_SECONDARY_HEADER_DATA,"Secondary header type is NA (NOT APPLICABLE).");

    if (!m_dataFieldHeader.empty()) {
        m_dataFieldHeader.clear();
    }
    if (pType != OTHER) {
        std::vector<uint8_t> data;
        data.assign(pData, pData + sizeData);
        FORWARD_RESULT( setDataFieldHeader(data,pType) );
    }else {
        FORWARD_RESULT( setDataFieldHeader(pData, sizeData) );
        m_dataFieldHeaderType = pType;
    }
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the data field header for the CCSDS DataField with a specific PUS type.
 *
 * This method configures the data field header based on the provided data and
 * the specified Packet Utilization Standard (PUS) type. It validates the header
 * size to ensure it does not exceed the maximum allowed packet size and creates
 * the appropriate header object based on the PUS type.
 *
 * @param data A vector containing the data for the data field header.
 * @param pType The PUS type (PUS_A, PUS_B, PUS_C, or OTHER) indicating the header format.
 *
 *
 * @note If the PUS type is OTHER, the data is passed to the overloaded setDataFieldHeader method.
 * @warning The method will log an error to standard error and return an exception if the total
 *          size of the header and application data exceeds the allowed packet size.
 */
CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::vector<uint8_t> &data,
                                                       const ESecondaryHeaderType &pType) {

    RET_IF_ERR_MSG(data.size() > getDataFieldAvailableSizeByes(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Secondary header data exceeds available size" );

    RET_IF_ERR_MSG(data.size() != 6 && pType == ESecondaryHeaderType::PUS_A, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Invalid PUS-A data provided, size != 6" );
    RET_IF_ERR_MSG(data.size() != 8 && pType == ESecondaryHeaderType::PUS_B, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Invalid PUS-B data provided, size != 8" );
    RET_IF_ERR_MSG(data.size() != 8 && pType == ESecondaryHeaderType::PUS_C, ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Invalid PUS-C data provided, size != 8" );

    m_dataFieldHeaderType = pType;

    switch (pType) {
        case PUS_A: {
            PusA PusAHeader;
            FORWARD_RESULT( PusAHeader.deserialize(data) );
            m_pusHeaderData = std::make_unique<PusA>(PusAHeader);
        }
        break;
        case PUS_B: {
            PusB PusBHeader;
            FORWARD_RESULT( PusBHeader.deserialize(data) );
            m_pusHeaderData = std::make_unique<PusB>(PusBHeader);
        }
        break;
        case PUS_C: {
            PusC PusCHeader;
            FORWARD_RESULT( PusCHeader.deserialize(data) );
            m_pusHeaderData = std::make_unique<PusC>(PusCHeader);
        }
        break;
        case OTHER: {
            FORWARD_RESULT( setDataFieldHeader(data) );
        }
        break;
        default: ;
    }
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the data field header for the CCSDS DataField.
 *
 * This method updates the data field header with the provided vector of bytes.
 * If the existing data field header is not empty, it clears the current contents
 * and logs a warning to indicate that the header has been overwritten.
 *
 * @param dataFieldHeader A vector containing the new data field header.
 *
 * @note The m_dataFieldHeaderType is set to OTHER after the header is updated.
 * @warning If the current data field header is not empty, it will be cleared,
 * and a warning message will be printed to the standard error stream.
 */
CCSDS::ResultBool CCSDS::DataField::setDataFieldHeader(const std::vector<uint8_t> &dataFieldHeader) {
    RET_IF_ERR_MSG(dataFieldHeader.size() > getDataFieldAvailableSizeByes(), ErrorCode::INVALID_SECONDARY_HEADER_DATA,
        "Secondary header data exceeds available size");
    if (!m_dataFieldHeader.empty()) {
        m_dataFieldHeader.clear();
    }
    m_dataFieldHeader = dataFieldHeader;
    m_dataFieldHeaderType = OTHER;
    m_dataFieldHeaderUpdated = false;
    return true;
}

/**
 * @brief Sets the secondary header for the data field using a PUS-A header.
 *
 * @param header A PusA object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusA& header ) {
    m_dataFieldHeaderType = PUS_A;
    m_pusHeaderData = std::make_shared<PusA>(header);
    m_dataFieldHeaderUpdated = false;
}

/**
 * @brief Sets the secondary header for the data field using a PUS-B header.
 *
 * @param header A PusB object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusB& header ) {
    m_dataFieldHeaderType = PUS_B;
    m_pusHeaderData = std::make_shared<PusB>(header);
    m_dataFieldHeaderUpdated = false;
}

/**
 * @brief Sets the secondary header for the data field using a PUS-C header.
 *
 * @param header A PusC object containing the header data.
 * @return None.
 */
void CCSDS::DataField::setDataFieldHeader(const PusC& header ) {
    m_dataFieldHeaderType = PUS_C;
    m_pusHeaderData = std::make_shared<PusC>(header);
    m_dataFieldHeaderUpdated = false;
}

/**
 * @brief Sets the maximum data packet size for the CCSDS DataField.
 *
 * This method updates the maximum allowed size for the data packet.
 * The data packet size is used to validate that the combined size of
 * the header and application data does not exceed this limit.
 *
 * @param value The maximum size of the data packet, in bytes.
 */
void CCSDS::DataField::setDataPacketSize(const uint16_t &value)  {  m_dataPacketSize = value; }

/**
 * @brief Retrieves the secondary header data as a vector of bytes.
 *
 * If the header type is not OTHER or NA, retrieves the data from the
 * corresponding PUS header object.
 *
 * @return A vector containing the header data bytes.
 */
std::vector<uint8_t> CCSDS::DataField::getDataFieldHeader() {
    update();
    if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
        return m_pusHeaderData->serialize();;
    }
    return m_dataFieldHeader;
}
