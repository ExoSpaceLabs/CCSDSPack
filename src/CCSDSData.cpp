
#include "CCSDSData.h"
#include "CCSDSUtils.h"
#include <iostream>

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

void CCSDS::DataField::update() {
    if (!m_dataFieldHeaderUpdated && m_enableDataFieldUpdate) {
        if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
            m_pusHeaderData->setDataLength(m_applicationData.size());;
        }
        m_dataFieldHeaderUpdated = true;
    }
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const uint8_t *pData, const size_t &sizeData) {
    RET_IF_ERR_MSG(!pData,ErrorCode::NULL_POINTER,"Application data is nullptr");
    RET_IF_ERR_MSG(sizeData < 1,ErrorCode::INVALID_APPLICATION_DATA,"Application data size cannot be < 1");
    RET_IF_ERR_MSG(sizeData > getDataFieldAvailableSizeByes(),ErrorCode::INVALID_APPLICATION_DATA,"Application data field exceeds available size");

    if (!m_applicationData.empty()) {
        std::cerr << "[ CCSDS Data ] Warning: Data field is not empty, it has been overwritten." << std::endl;
    }
    m_applicationData.assign(pData, pData + sizeData);
    m_dataFieldHeaderUpdated = false;
    return true;
}

CCSDS::ResultBool CCSDS::DataField::setApplicationData(const std::vector<uint8_t> &applicationData) {
    RET_IF_ERR_MSG(applicationData.size() > getDataFieldAvailableSizeByes(),ErrorCode::INVALID_APPLICATION_DATA,"Application data field exceeds available size.");
    m_applicationData = applicationData;
    m_dataFieldHeaderUpdated = false;
    return true;
}

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

void CCSDS::DataField::setDataFieldHeader(const PusA& header ) {
    m_dataFieldHeaderType = PUS_A;
    m_pusHeaderData = std::make_shared<PusA>(header);
    m_dataFieldHeaderUpdated = false;
}

void CCSDS::DataField::setDataFieldHeader(const PusB& header ) {
    m_dataFieldHeaderType = PUS_B;
    m_pusHeaderData = std::make_shared<PusB>(header);
    m_dataFieldHeaderUpdated = false;
}

void CCSDS::DataField::setDataFieldHeader(const PusC& header ) {
    m_dataFieldHeaderType = PUS_C;
    m_pusHeaderData = std::make_shared<PusC>(header);
    m_dataFieldHeaderUpdated = false;
}

void CCSDS::DataField::setDataPacketSize(const uint16_t &value)  {  m_dataPacketSize = value; }

std::vector<uint8_t> CCSDS::DataField::getDataFieldHeader() {
    update();
    if (m_dataFieldHeaderType != OTHER && m_dataFieldHeaderType != NA) {
        return m_pusHeaderData->serialize();;
    }
    return m_dataFieldHeader;
}
