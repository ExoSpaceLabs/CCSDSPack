#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <cstdint>
#include <vector>
#include <memory>
#include "CCSDSSecondaryHeader.h"



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

        void setApplicationData(                                     const std::vector<uint8_t>& applicationData );
        void setApplicationData(                                    const uint8_t* pData, const size_t &sizeData );
        void setDataFieldHeader(                                    const uint8_t* pData, const size_t &sizeData );
        void setDataFieldHeader( const uint8_t* pData, const size_t &sizeData, const ESecondaryHeaderType &pType );
        void setDataFieldHeader(                  const std::vector<uint8_t>&, const ESecondaryHeaderType &pType );
        void setDataFieldHeader(                                     const std::vector<uint8_t>& dataFieldHeader );
        void setDataFieldHeader(                                                              const PusA& header );
        void setDataFieldHeader(                                                              const PusB& header );
        void setDataFieldHeader(                                                              const PusC& header );

        void setDataPacketSize(                                                            const uint16_t &value );
        void setDataFieldHeaderAutoUpdateStatus( const bool enable)          { m_enableDataFieldUpdate = enable; }


        std::vector<uint8_t> getDataFieldHeader();
        std::vector<uint8_t> getFullDataField();
        bool getDataFieldHeaderAutoUpdateStatus() const                           { return m_enableDataFieldUpdate; }
        std::vector<uint8_t> getApplicationData();
        bool getDataFieldHeaderFlag() const       { return !m_dataFieldHeader.empty() || m_pusHeaderData != nullptr; }
        uint16_t getDataFieldAbsoluteSizeByes();
        uint16_t getDataFieldUsedSizeByes();
        uint16_t getDataFieldAvailableSizeByes();

        void printData();

        void update();
    private:

        std::shared_ptr<SecondaryHeaderAbstract>                 m_pusHeaderData{};
        std::vector<uint8_t>                                   m_dataFieldHeader{};
        std::vector<uint8_t>                                   m_applicationData{};
        ESecondaryHeaderType                           m_dataFieldHeaderType{ NA };
        uint16_t                                         m_dataPacketSize{  2024 };
        bool                                     m_dataFieldHeaderUpdated{ false };
        bool                                       m_enableDataFieldUpdate{ true };
    };
}
#endif // CCSDSDATA_H

