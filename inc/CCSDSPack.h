#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include <string>
#include <cstdint>
#include <vector>
#include <stdexcept>


// functions
std::string getBinaryString(uint32_t value, int bits);

enum SecondaryHeaderType {
    NA,
    PUS_A,
    PUS_B,
    PUS_C,
    OTHER
};


struct PrimaryHeader {
 // version and packet identification 16 bit 4 hex
    uint8_t versionNumber;         // 3 bit
    
    // packet identification 4 hex
    uint8_t type;                  // 1 bit
    uint8_t dataFieldHeaderFlag;   // 1 bit
    uint16_t APID;                 // 11 bit
    
    //packet sequence control 16 bit 4 hex
    uint8_t sequenceFlags;         // 2 bit
    uint16_t sequenceCount;        // 14 bit
    
    // data packet length
    uint16_t dataLength;           // 16 bits
    
    // constructor
    PrimaryHeader(uint8_t versionNumber_value, uint8_t type_value, uint8_t dataFieldHeaderFlag_value, uint16_t APID_value, 
                 uint8_t sequenceFlag_value, uint16_t sequenceCount_value,  uint16_t dataLength_value) :
                  
                  versionNumber(versionNumber_value), type(type_value),  dataFieldHeaderFlag(dataFieldHeaderFlag_value),
                  APID(APID_value), sequenceFlags(sequenceFlag_value), sequenceCount(sequenceCount_value),
                  dataLength(dataLength_value) {}
};


//ToDo 
// To be moved out from the CCSDSPack into its own header and cpp.
class CCSDSHeader {
public:


    // constructor

    CCSDSHeader() = default;
    // getters
    uint8_t  getVersionNumber() const        { return m_versionNumber;       }
    uint8_t  getType() const                 { return m_type;                }
    uint8_t  getDataFieldheaderFlag() const  { return m_dataFieldHeaderFlag; }
    uint16_t getAPID() const                 { return m_APID;                }
    uint8_t  getSequenceFlags() const        { return m_sequenceFlags;       }
    uint16_t getSequenceCount() const        { return m_sequenceCount;       }
    uint16_t getDataLength() const           { return m_dataLength;          }
    
    uint64_t getFullHeader(){
        m_packetSequenceControl = (m_sequenceFlags << 14) | m_sequenceCount;
        m_packetIdentificationAndVersion = (m_versionNumber << 13) | (m_type << 12) | (m_dataFieldHeaderFlag << 11) | m_APID;
        return (static_cast<uint64_t>(m_packetIdentificationAndVersion) << 32) | (static_cast<uint32_t>(m_packetSequenceControl) << 16) | m_dataLength;
    }
    
    
    // setters
    void setVersionNumber(const uint8_t value)        { m_versionNumber       = value & 0x0007; }
    void setType(const uint8_t value)                 { m_type                = value & 0x0001; }
    void setDataFieldheaderFlag(const uint8_t value)  { m_dataFieldHeaderFlag = value & 0x0001; }
    void setAPID(const uint16_t value)                { m_APID                = value & 0x07FF; }
    void setSequenceFlags(const uint8_t value)        { m_sequenceFlags       = value & 0x0003; }
    void setSequenceCount(const uint16_t value)       { m_sequenceCount       = value & 0x3FFF; }
    void setDataLength(const uint16_t value)          { m_dataLength          = value;          }
    

    // Full data setter
    void setData(uint64_t data);
    
    // Full data setter
    void setData(PrimaryHeader data);

    // print out the header
    void printHeader();

    
    // Constructor accepting a binary literal
    // template <uint64_t N>
    //constexpr CCSDSHeader(N data) {
    explicit CCSDSHeader( uint64_t data) {
       setData(data);
    }
    
  
private:
    // full packet size 48 bit fixed 6 byes
    uint16_t m_packetIdentificationAndVersion{};
    
    // version and packet identification 16 bit 4 hex
    uint8_t m_versionNumber{};            // 3 bit
    
    // packet identification 4 hex
    uint8_t m_type{};                     // 1 bit
    uint8_t m_dataFieldHeaderFlag{};      // 1 bit
    uint16_t m_APID{};                    // 11 bit
    
    //packet sequence control 16 bit 4 hex
    uint16_t m_packetSequenceControl{};
    uint8_t m_sequenceFlags{};            // 2 bit
    uint16_t m_sequenceCount{};           // 14 bit
    
    // data packet length
    uint16_t m_dataLength{};              // 16 bits
    
};

class CCSDSDataField {
public:
    CCSDSDataField() = default;
    void setDataPacketSize(                         const uint16_t value ) {      m_dataPacketSize =           value; }
    void setData(                       const std::vector<uint8_t>& data ) {                m_data =            data; }
    void setSecondaryHeader( const std::vector<uint8_t>& secondaryHeader ) {     m_secondaryHeader = secondaryHeader; }


    std::vector<uint8_t> getFullDataField();

    void printData();

private:
    SecondaryHeaderType m_secondaryHeaderType{};
    std::vector<uint8_t>  m_secondaryHeader{};
    std::vector<uint8_t> m_data{};
    uint16_t m_dataPacketSize = 2024; // excluding header
};


class CCSDSPacket {
public:
    CCSDSPacket() = default;

    // setters
    void setPrimaryHeader(                const uint64_t data) {        m_primaryHeader.setData( data ); }
    void setPrimaryHeader(           const PrimaryHeader data) {        m_primaryHeader.setData( data ); }
    void setDataField(       const std::vector<uint8_t>& data) {            m_dataField.setData( data ); }
    void setSecondaryHeader( const std::vector<uint8_t>& data) { m_dataField.setSecondaryHeader( data ); }

    // getters
    uint16_t getCRC() const { return m_CRC16; }

    // other
    void printPrimaryHeader() { m_primaryHeader.printHeader(); }
    void printDataField();

private:
    void calculateCRC16();

    CCSDSHeader m_primaryHeader{};  // 6 bytes / 48 bits / 12 hex
    CCSDSDataField m_dataField{};   // variable
    uint64_t m_CRC16{};               // Cyclic Redundancy check 16 bits
};

#endif // CCSDSPACK_H

