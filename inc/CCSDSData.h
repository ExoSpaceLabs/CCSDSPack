#ifndef CCSDSDATA_H
#define CCSDSDATA_H

#include <string>
#include <cstdint>
#include <vector>


namespace CCSDS {
    enum SecondaryHeaderType {
        NA,
        PUS_A,
        PUS_B,
        PUS_C,
        OTHER
    };

    class PacketUtilizationStandardA {
    public:
        PacketUtilizationStandardA();
        PacketUtilizationStandardA(uint8_t version, uint8_t serviceType, uint8_t serviceSubtype, uint8_t sourceID, uint32_t timestamp);
        explicit PacketUtilizationStandardA(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return        m_version; };
        uint8_t                    getServiceType()     const { return    m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return m_serviceSubType; };
        uint8_t                    getSourceID()        const { return       m_sourceID; };
        uint32_t                   getTimeStamp()       const { return      m_timeStamp; };
        uint8_t                    getSize()            const { return           m_size; };
        static SecondaryHeaderType getType()                  { return            PUS_A; };
        std::vector<uint8_t>       getData()            const;

    private:
        uint8_t         m_version{};
        uint8_t     m_serviceType{};
        uint8_t  m_serviceSubType{};
        uint8_t        m_sourceID{};
        uint32_t      m_timeStamp{}; // (mission epoch, often in CCSDS CUC format or mission-defined).

        const uint16_t m_size = 7; // bytes
    };

    class PacketUtilizationStandardB {
    public:
        PacketUtilizationStandardB();
        PacketUtilizationStandardB(uint8_t version, uint8_t serviceType, uint8_t serviceSubtype, uint8_t m_destinationID, uint16_t m_sequenceControl);
        explicit PacketUtilizationStandardB(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return         m_version; };
        uint8_t                    getServiceType()     const { return     m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return  m_serviceSubType; };
        uint8_t                    getDestinationID()   const { return   m_destinationID; };
        uint16_t                   getSequenceControl() const { return m_sequenceControl; };
        static SecondaryHeaderType getType()                  { return            PUS_B; };
        std::vector<uint8_t> getData()                  const;

    private:
        uint8_t          m_version{};
        uint8_t      m_serviceType{};
        uint8_t   m_serviceSubType{};
        uint8_t    m_destinationID{};
        uint16_t m_sequenceControl{}; // alternatively it can be used for sequence control

        const uint16_t m_size = 6; // bytes
    };
    class PacketUtilizationStandardC {
    public:
        PacketUtilizationStandardC();


        static SecondaryHeaderType getType()                  { return            PUS_C; };
        std::vector<uint8_t> getData() const;
    private:
        uint8_t                    m_version{};
        uint8_t                m_serviceType{};
        uint8_t             m_serviceSubType{};
        uint8_t                   m_sourceID{};
        std::vector<uint8_t>   m_missionData{};

        uint16_t m_size = 6; // set to minimum, typically between 6 - 12 bytes
    };

    class DataField {
    public:
        DataField() = default;
        virtual ~DataField() = default;

        void setData(                       const std::vector<uint8_t>& data ) {            m_data =            data; }
        void setData(                  const uint8_t* pData, size_t sizeData );
        void setSecondaryHeader(       const uint8_t* pData, size_t sizeData );
        void setSecondaryHeader(           PacketUtilizationStandardA header );
        void setSecondaryHeader(           PacketUtilizationStandardB header );
        void setSecondaryHeader(           PacketUtilizationStandardC header );
        void setSecondaryHeader( const std::vector<uint8_t>& secondaryHeader ) { m_secondaryHeader = secondaryHeader; } // type has to be set to other

        void setDataPacketSize(                         const uint16_t value ) {  m_dataPacketSize =           value; }


        std::vector<uint8_t> getDataFieldHeader() {return m_secondaryHeader; }
        std::vector<uint8_t> getApplicationData() {return            m_data; }
        std::vector<uint8_t> getFullDataField();

        void printData();

    private:
        SecondaryHeaderType   m_secondaryHeaderType{};
        std::vector<uint8_t>      m_secondaryHeader{};
        std::vector<uint8_t>                 m_data{};
        uint16_t              m_dataPacketSize = 2024;
    };
}
#endif // CCSDSDATA_H

