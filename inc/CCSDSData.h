#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <cstdint>
#include <vector>
#include <memory>
#include "PUSService.h"



namespace CCSDS{

    /**
     * @brief Represents the data field of a CCSDS packet.
     *
     * This class encapsulates the structure and operations for handling the data field
     * of a CCSDS (Consultative Committee for Space Data Systems) packet. It allows for
     * setting and retrieving application data, data field headers, and calculating
     * the full data field. Additionally, it provides methods for managing the data field's
     * size and printing its content.
     *
     * The data field consists of headers and application data, and the class supports
     * different header types (PusA, PusB, PusC). It also allows setting and getting
     * the data packet size, as well as managing the data field header flag.
     */
    class DataField {
    public:
        DataField() = default;

        ~DataField() = default;

        void setApplicationData(          const std::vector<uint8_t>& applicationData );
        void setApplicationData(                const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader(                const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader( const uint8_t* pData, size_t sizeData, PUSType pType );
        void setDataFieldHeader(                                   const PusA& header );
        void setDataFieldHeader(                                   const PusB& header );
        void setDataFieldHeader(                                   const PusC& header );
        void setDataFieldHeader(          const std::vector<uint8_t>& dataFieldHeader ) { m_dataFieldHeader = dataFieldHeader; } // type has to be set to other

        void setDataPacketSize(                         const uint16_t value ) {  m_dataPacketSize =           value; }

        std::vector<uint8_t> getDataFieldHeader();
        std::vector<uint8_t> getApplicationData() {return m_applicationData; }
        std::vector<uint8_t> getFullDataField();
        bool getDataFieldHeaderFlag() const       { return !m_dataFieldHeader.empty() || m_pusHeaderData != nullptr; }

        void printData();

    private:
        void updateDataFieldHeader();

        PUSType               m_dataFieldHeaderType{};
        std::shared_ptr<PusHeader>  m_pusHeaderData{};
        std::vector<uint8_t>      m_dataFieldHeader{};
        std::vector<uint8_t>      m_applicationData{};
        uint16_t              m_dataPacketSize = 2024;

        bool m_dataFieldHeaderUpdated{false};
    };
}
#endif // CCSDSDATA_H

