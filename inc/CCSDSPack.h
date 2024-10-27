#ifndef CCSDSPACK_H
#define CCSDSPACK_H

#include <string>
#include <cstdint>
#include <stdexcept>


// functions
std::string getBinaryString(uint32_t value, int bits);


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

    // getters
    uint8_t  getVersionNumber()        { return m_versionNumber;       }
    uint8_t  getType()                 { return m_type;                }
    uint8_t  getDataFieldheaderFlag()  { return m_dataFieldHeaderFlag; }
    uint16_t getAPID()                 { return m_APID;                }
    uint8_t  getSequenceFlags()        { return m_sequenceFlags;       }
    uint16_t getSequenceCount()        { return m_sequenceCount;       }
    uint16_t getDataLength()           { return m_dataLength;          }
    
    uint64_t getFullHeader(){
        m_packetSequenceControl = (m_sequenceFlags << 14) | m_sequenceCount;
        m_packetIdentificationAndVersion = (m_versionNumber << 13) | (m_type << 12) | (m_dataFieldHeaderFlag << 11) | m_APID;
        return (static_cast<uint64_t>(m_packetIdentificationAndVersion) << 32) | (static_cast<uint32_t>(m_packetSequenceControl) << 16) | m_dataLength;
        
    }
    
    
    // setters
    void setVersionNumber(uint8_t value)        { m_versionNumber       = value & 0x0007; }
    void setType(uint8_t value)                 { m_type                = value & 0x0001; }
    void setDataFieldheaderFlag(uint8_t value)  { m_dataFieldHeaderFlag = value & 0x0001; }
    void setAPID(uint16_t value)                { m_APID                = value & 0x07FF; }
    void setSequenceFlags(uint8_t value)        { m_sequenceFlags       = value & 0x0003; }
    void setSequenceCount(uint16_t value)       { m_sequenceCount       = value & 0x3FFF; }
    void setDataLength(uint16_t value)          { m_dataLength          = value;          }
    
    
        
    // Full data setter
    void setData(uint64_t data){
        if (data > 0xFFFFFFFFFFFF) { // check if given header exeeds header size.
            throw std::invalid_argument("[ CCSDS Header ] Error: Input data exceeds expected bit size for version or size.");
        }
        
        // Decompose data using mask and 
        m_dataLength                     = (data & 0xFFFF);               // last 16 bits
        m_packetSequenceControl          = (data >> 16) & 0xFFFF;         // middle 16 bits
        m_packetIdentificationAndVersion = (data >> 32);                  // first 16 bits
        
        // decompose packet identifier
        m_versionNumber       = (m_packetIdentificationAndVersion >> 13);          // First 3 bits for version
        m_type                = (m_packetIdentificationAndVersion >> 12) & 0x1;    // Next 1 bit
        m_dataFieldHeaderFlag = (m_packetIdentificationAndVersion >> 11) & 0x1;    // Next 1 bit
        m_APID                = (m_packetIdentificationAndVersion & 0x07FF);       // Last 11 bits
        
        // decompose sequence control
        m_sequenceFlags = (m_packetSequenceControl >> 14);                // first 2 bits
        m_sequenceCount = (m_packetSequenceControl & 0x3FFF);             // Last 14 bits.
    }
    
    // Full data setter
    void setData(PrimaryHeader data){
    
        m_versionNumber       = data.versionNumber & 0x0007; 
        m_type                = data.type & 0x0001; 
        m_dataFieldHeaderFlag = data.dataFieldHeaderFlag & 0x0001; 
        m_APID                = data.APID & 0x07FF; 
        m_sequenceFlags       = data.sequenceFlags & 0x0003; 
        m_sequenceCount       = data.sequenceCount & 0x3FFF; 
        m_dataLength          = data.dataLength;          
        
        m_packetSequenceControl = (static_cast<uint16_t>(m_sequenceFlags) << 14) | m_sequenceCount;
        m_packetIdentificationAndVersion = (static_cast<uint16_t>(m_versionNumber) << 13) | (m_type << 12) | static_cast<uint16_t>((m_dataFieldHeaderFlag) << 11) | m_APID;
    }
    
    
    // print out the header
    void printHeader();
    

    // constructor
    
    CCSDSHeader(){
        m_dataLength                     = 0;   // last 16 bits
        m_packetSequenceControl          = 0;   // middle 16 bits
        m_packetIdentificationAndVersion = 0;   // first 16 bits
        
        // decompose packet identifier
        m_versionNumber       = 0;              // First 3 bits for version
        m_type                = 0;              // Next 1 bit
        m_dataFieldHeaderFlag = 0;              // Next 1 bit
        m_APID                = 0;              // Last 11 bits
        
        // decompose sequence control
        m_sequenceFlags = 0;                    // first 2 bits
        m_sequenceCount = 0;                    // Last 14 bits.
    }
    
    // Constructor accepting a binary literal
    // template <uint64_t N>
    //constexpr CCSDSHeader(N data) {
    CCSDSHeader(uint64_t data) {
       setData(data);
    }
    
  
private:
    // full packet size 48 bit fixed
    uint16_t m_packetIdentificationAndVersion;
    
    // version and packet identification 16 bit 4 hex
    uint8_t m_versionNumber;         // 3 bit
    
    // packet identification 4 hex
    uint8_t m_type;                  // 1 bit
    uint8_t m_dataFieldHeaderFlag;   // 1 bit
    uint16_t m_APID;                 // 11 bit
    
    //packet sequence control 16 bit 4 hex
    uint16_t m_packetSequenceControl;
    uint8_t m_sequenceFlags;         // 2 bit
    uint16_t m_sequenceCount;        // 14 bit
    
    // data packet length
    uint16_t m_dataLength;           // 16 bits
    
};


class CCSDSPack {
public:
    CCSDSPack();
   
    void setPrimaryHeader(uint64_t data){ m_primaryHeader.setData(data); }
    void setPrimaryHeader(PrimaryHeader data){ m_primaryHeader.setData(data); }
    
    void printPrimaryHeader(){ m_primaryHeader.printHeader(); }
    
private:
    CCSDSHeader m_primaryHeader;
};

#endif // CCSDSPACK_H

