#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <cstdint>
#include <vector>
#include <memory>
#include "PUSService.h"


namespace CCSDS{
    class DataField {
    public:
        DataField() = default;

        ~DataField() = default;

        void setApplicationData(          const std::vector<uint8_t>& applicationData ) { m_applicationData = applicationData; }
        void setApplicationData(                const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader(                const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader( const uint8_t* pData, size_t sizeData, PUSType pType );
        void setDataFieldHeader(                                   const PusA& header );
        void setDataFieldHeader(                                   const PusB& header );
        void setDataFieldHeader(                                   const PusC& header );
        void setDataFieldHeader(          const std::vector<uint8_t>& dataFieldHeader ) { m_dataFieldHeader = dataFieldHeader; } // type has to be set to other

        void setDataPacketSize(                         const uint16_t value ) {  m_dataPacketSize =           value; }

        // template<typename PusType, typename... Args>
        // void setDataFieldHeader(Args&&... args) {
        //     m_pusHeaderData = std::make_shared<PusType>(std::forward<Args>(args)...);
        // }

        std::vector<uint8_t> getDataFieldHeader() {return m_dataFieldHeader; }
        std::vector<uint8_t> getApplicationData() {return m_applicationData; }
        std::vector<uint8_t> getFullDataField();
        bool getDataFieldHeaderFlag() const       { return !m_dataFieldHeader.empty() || m_pusHeaderData != nullptr; }

        void printData();

    private:
        PUSType               m_dataFieldHeaderType{};
        std::shared_ptr<PusHeader>    m_pusHeaderData{};
        std::vector<uint8_t>      m_dataFieldHeader{};
        std::vector<uint8_t>      m_applicationData{};
        uint16_t              m_dataPacketSize = 2024;
    };
}
#endif // CCSDSDATA_H

