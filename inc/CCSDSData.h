#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <cstdint>
#include <vector>


namespace CCSDS {
    enum DataFieldHeaderType {
        NA,
        PUS_A,
        PUS_B,
        PUS_C,
        OTHER
    };

    class PusA {
    public:
        PusA();
        PusA(uint8_t version, uint8_t serviceType, uint8_t serviceSubtype, uint8_t sourceID, uint32_t timestamp);
        explicit PusA(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return        m_version; };
        uint8_t                    getServiceType()     const { return    m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return m_serviceSubType; };
        uint8_t                    getSourceID()        const { return       m_sourceID; };
        uint32_t                   getTimeStamp()       const { return      m_timeStamp; };
        uint8_t                    getSize()            const { return           m_size; };
        static DataFieldHeaderType getType()                  { return            PUS_A; };
        std::vector<uint8_t>       getData()            const;

    private:
        uint8_t         m_version{};
        uint8_t     m_serviceType{};
        uint8_t  m_serviceSubType{};
        uint8_t        m_sourceID{};
        uint32_t      m_timeStamp{}; // (mission epoch, often in CCSDS CUC format or mission-defined).

        const uint16_t m_size = 7; // bytes
    };

    class PusB {
    public:
        PusB();
        PusB(uint8_t version, uint8_t serviceType, uint8_t serviceSubtype, uint8_t m_destinationID, uint16_t m_sequenceControl);
        explicit PusB(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return         m_version; };
        uint8_t                    getServiceType()     const { return     m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return  m_serviceSubType; };
        uint8_t                    getDestinationID()   const { return   m_destinationID; };
        uint16_t                   getSequenceControl() const { return m_sequenceControl; };
        static DataFieldHeaderType getType()                  { return            PUS_B; };
        std::vector<uint8_t>       getData()            const;

    private:
        uint8_t          m_version{};
        uint8_t      m_serviceType{};
        uint8_t   m_serviceSubType{};
        uint8_t    m_destinationID{};
        uint16_t m_sequenceControl{}; // alternatively it can be used for sequence control

        const uint16_t m_size = 6; // bytes
    };
    class PusC {
    public:
        PusC();

        static DataFieldHeaderType getType()                  { return            PUS_C; };
        std::vector<uint8_t> getData() const;
    private:
        uint8_t                    m_version{};
        uint8_t                m_serviceType{};
        uint8_t             m_serviceSubType{};
        uint8_t                   m_sourceID{};
        std::vector<uint8_t>   m_missionData{};

        uint16_t m_size = 6; // set to minimum, typically between 6 - 12 bytes
    };

    class DataField final {
    public:
        DataField() = default;

        ~DataField() = default;

        void setApplicationData( const std::vector<uint8_t>& applicationData ) { m_applicationData = applicationData; }
        void setApplicationData(       const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader(       const uint8_t* pData, size_t sizeData );
        void setDataFieldHeader(                                 PusA header );
        void setDataFieldHeader(                                 PusB header );
        void setDataFieldHeader(                          const PusC& header );
        void setDataFieldHeader( const std::vector<uint8_t>& dataFieldHeader ) { m_dataFieldHeader = dataFieldHeader; } // type has to be set to other

        void setDataPacketSize(                         const uint16_t value ) {  m_dataPacketSize =           value; }


        std::vector<uint8_t> getDataFieldHeader() {return m_dataFieldHeader; }
        std::vector<uint8_t> getApplicationData() {return m_applicationData; }
        std::vector<uint8_t> getFullDataField();
        bool getDataFieldHeaderFlag() const       { return !m_dataFieldHeader.empty(); }

        void printData();

    private:
        DataFieldHeaderType   m_dataFieldHeaderType{};
        std::vector<uint8_t>      m_dataFieldHeader{};
        std::vector<uint8_t>      m_applicationData{};
        uint16_t              m_dataPacketSize = 2024;
    };
}
#endif // CCSDSDATA_H

