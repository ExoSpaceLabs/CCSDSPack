
#ifndef PUSSERVICE_H
#define PUSSERVICE_H

#include <vector>
#include <cstdint>

namespace CCSDS {


    enum PUSType {
        NA,
        PUS_A,
        PUS_B,
        PUS_C,
        OTHER
    };

    class PusHeader {
    public:

        virtual ~PusHeader() = default;

        // virtual overloaded functions require default definition within cpp.

        // public virtual setters
        virtual void setDataLength(uint16_t dataLength);

        // public virtual getters
        virtual uint16_t             getDataLength() const;
        virtual uint8_t              getSize() const;
        virtual std::vector<uint8_t> getData() const; // Pure virtual method for polymorphism
    };

    class PusA final : public PusHeader {
    public:
        explicit PusA(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint32_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_dataLength(dataLength) {};
        explicit PusA(std::vector<uint8_t> data);

        void setDataLength(const uint16_t dataLength)    override  { m_dataLength = dataLength; };

        uint8_t                    getVersion()         const  { return        m_version; }
        uint8_t                    getServiceType()     const  { return    m_serviceType; }
        uint8_t                    getServiceSubtype()  const  { return m_serviceSubType; }
        uint8_t                    getSourceID()        const  { return       m_sourceID; }
        uint16_t                   getDataLength()      const override { return     m_dataLength; }
        uint8_t                    getSize()            const override { return           m_size; }

        std::vector<uint8_t> getData() const override;

    private:                                // Field	            Size (bits)	Description
        uint8_t         m_version{};        // Version	            3	        Version of the PUS standard
        uint8_t     m_serviceType{};        // Service Type	        8	        Type of service (e.g., 0x01 for telemetry)
        uint8_t  m_serviceSubType{};        // Service Subtype	    8	        Subtype of the service (e.g., specific telemetry type)
        uint8_t        m_sourceID{};        // Source ID	        8	        ID of the source (e.g., satellite or sensor)
        uint16_t     m_dataLength{};        // Data Length	        16	        Length of the telemetry data in bytes
                                            // Telemetry Data	    Variable    (based on Data Length)	The actual telemetry data (variable length)

        const uint16_t m_size = 6; // bytes
    };

    class PusB final : public PusHeader {
    public:
        explicit PusB(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint8_t eventID, const uint16_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_eventID(eventID), m_dataLength(dataLength) {}
        explicit PusB(std::vector<uint8_t> data);

        void setDataLength(const uint16_t dataLength)    override  { m_dataLength = dataLength; };

        uint8_t                    getVersion()         const  { return         m_version; }
        uint8_t                    getServiceType()     const  { return     m_serviceType; }
        uint8_t                    getServiceSubtype()  const  { return  m_serviceSubType; }
        uint8_t                    getSourceID()        const  { return        m_sourceID; }
        uint8_t                    getEventID()         const          { return         m_eventID; }
        uint16_t                   getDataLength()      const override { return      m_dataLength; }
        uint8_t                    getSize()            const override { return            m_size; }

        std::vector<uint8_t> getData() const override;

    private:                                 // Field	            Size (bits)	Description
        uint8_t          m_version{};        // Version	            3	        Version of the PUS standard
        uint8_t      m_serviceType{};        // Service Type	    8	        Type of service (e.g., 0x02 for event reporting)
        uint8_t   m_serviceSubType{};        // Service Subtype	    8	        Subtype of the service (e.g., specific event type)
        uint8_t         m_sourceID{};        // Source ID	        8	        ID of the source (e.g., satellite or sensor)
        uint8_t          m_eventID{};        // Event ID	        16	        ID of the event being reported
        uint16_t      m_dataLength{};        // Data Length	        16	        Length of the event data in bytes
                                             // Event Data	        Variable    (based on Data Length)	The actual event data (variable length)

        const uint16_t m_size = 7; // bytes
    };

    class PusC final : public PusHeader {
    public:
        explicit PusC(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint16_t timeCode, const uint16_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_timeCode(timeCode), m_dataLength(dataLength) {}
        explicit PusC(std::vector<uint8_t> data);

        void setDataLength(const uint16_t dataLength)    override  { m_dataLength = dataLength; };

        uint8_t                    getVersion()         const  { return                     m_version; }
        uint8_t                    getServiceType()     const  { return                 m_serviceType; }
        uint8_t                    getServiceSubtype()  const  { return              m_serviceSubType; }
        uint8_t                    getSourceID()        const  { return                    m_sourceID; }
        uint16_t                   getTimeCode()        const          { return                    m_timeCode; }
        uint16_t                   getDataLength()      const override { return                  m_dataLength; }
        uint8_t                    getSize()            const override { return                        m_size; }

        std::vector<uint8_t> getData() const override;

    private:                            // Field	            Size (bits)	Description
        uint8_t        m_version{};     // Version	            3	        Version of the PUS standard
        uint8_t    m_serviceType{};     // Service Type	        8	        Type of service (e.g., 0x03 for time code)
        uint8_t m_serviceSubType{};     // Service Subtype	    8	        Subtype of the service (e.g., specific time type
        uint8_t       m_sourceID{};     // Source ID	        8	        ID of the source (e.g., satellite or sensor)
        uint16_t      m_timeCode{};     // Time Code	        16	        Time code value, depending on the system
        uint16_t    m_dataLength{};     // Data Length	        16	        Length of the time data in bytes
                                        // Time Data	        Variable    (based on Data Length)	The actual time-related data (variable length)

        uint16_t m_size = 8; // bytes
    };
}

#endif //PUSSERVICE_H


/*
Here are the PUS A, B, and C secondary header examples, as used within the CCSDS Packet format. Each of these headers contains fields with specific bit sizes and data that will vary based on the type of PUS used (A, B, or C). We'll show the structure and some typical values for each.

PUS-A (Telemetry) – Secondary Header
PUS-A is used for Telemetry data and typically contains the following fields:

Field	            Size (bits)	Description
Version	            3	        Version of the PUS standard
Service Type	    8	        Type of service (e.g., 0x01 for telemetry)
Service Subtype	    8	        Subtype of the service (e.g., specific telemetry type)
Source ID	        8	        ID of the source (e.g., satellite or sensor)
Data Length	        16	        Length of the telemetry data in bytes
Telemetry Data	    Variable    (based on Data Length)	The actual telemetry data (variable length)
Example for PUS-A:

Version = 0x3 (3 bits)
Service Type = 0x01 (8 bits)
Service Subtype = 0x00 (8 bits)
Source ID = 0x10 (8 bits)
Data Length = 0x0008 (16 bits)
Telemetry Data = 8 bytes (variable)
cpp
Copy
Edit
// Example PUS-A Header
uint8_t m_version = 0x3;  // 3 bits
uint8_t m_serviceType = 0x01;  // 8 bits
uint8_t m_serviceSubType = 0x00;  // 8 bits
uint8_t m_sourceID = 0x10;  // 8 bits
uint16_t m_dataLength = 0x0008;  // 16 bits

// Variable-length telemetry data (here 8 bytes)
uint8_t telemetryData[8] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
Total Size of PUS-A Header (in bits):

3 (Version) + 8 (Service Type) + 8 (Service Subtype) + 8 (Source ID) + 16 (Data Length) + 8 * 8 (Telemetry Data)
3 + 8 + 8 + 8 + 16 + 64 = 107 bits
PUS-B (Event Reporting) – Secondary Header
PUS-B is used for Event Reporting and typically includes the following fields:

Field	            Size (bits)	Description
Version	            3	        Version of the PUS standard
Service Type	    8	        Type of service (e.g., 0x02 for event reporting)
Service Subtype	    8	        Subtype of the service (e.g., specific event type)
Source ID	        8	        ID of the source (e.g., satellite or sensor)
Event ID	        16	        ID of the event being reported
Data Length	        16	        Length of the event data in bytes
Event Data	        Variable    (based on Data Length)	The actual event data (variable length)
Example for PUS-B:

Version = 0x3 (3 bits)
Service Type = 0x02 (8 bits)
Service Subtype = 0x01 (8 bits)
Source ID = 0x20 (8 bits)
Event ID = 0x1234 (16 bits)
Data Length = 0x0010 (16 bits)
Event Data = 16 bytes (variable)
cpp
Copy
Edit
// Example PUS-B Header
uint8_t m_version = 0x3;  // 3 bits
uint8_t m_serviceType = 0x02;  // 8 bits
uint8_t m_serviceSubType = 0x01;  // 8 bits
uint8_t m_sourceID = 0x20;  // 8 bits
uint16_t m_eventID = 0x1234;  // 16 bits
uint16_t m_dataLength = 0x0010;  // 16 bits

// Variable-length event data (here 16 bytes)
uint8_t eventData[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                         0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
Total Size of PUS-B Header (in bits):

3 (Version) + 8 (Service Type) + 8 (Service Subtype) + 8 (Source ID) + 16 (Event ID) + 16 (Data Length) + 16 * 8 (Event Data)
3 + 8 + 8 + 8 + 16 + 16 + 128 = 187 bits
PUS-C (Time Code) – Secondary Header
PUS-C is used for Time Code and contains the following fields:

Field	            Size (bits)	Description
Version	            3	        Version of the PUS standard
Service Type	    8	        Type of service (e.g., 0x03 for time code)
Service Subtype	    8	        Subtype of the service (e.g., specific time type)
Source ID	        8	        ID of the source (e.g., satellite or sensor)
Time Code	        16	        Time code value, depending on the system
Data Length	        16	        Length of the time data in bytes
Time Data	        Variable    (based on Data Length)	The actual time-related data (variable length)
Example for PUS-C:

Version = 0x3 (3 bits)
Service Type = 0x03 (8 bits)
Service Subtype = 0x00 (8 bits)
Source ID = 0x30 (8 bits)
Time Code = 0x5678 (16 bits)
Data Length = 0x0014 (16 bits)
Time Data = 20 bytes (variable)
cpp
Copy
Edit
// Example PUS-C Header
uint8_t m_version = 0x3;  // 3 bits
uint8_t m_serviceType = 0x03;  // 8 bits
uint8_t m_serviceSubType = 0x00;  // 8 bits
uint8_t m_sourceID = 0x30;  // 8 bits
uint16_t m_timeCode = 0x5678;  // 16 bits
uint16_t m_dataLength = 0x0014;  // 16 bits

// Variable-length time data (here 20 bytes)
uint8_t timeData[20] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
                        0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10,
                        0x11, 0x12, 0x13, 0x14};
Total Size of PUS-C Header (in bits):

3 (Version) + 8 (Service Type) + 8 (Service Subtype) + 8 (Source ID) + 16 (Time Code) + 16 (Data Length) + 20 * 8 (Time Data)
3 + 8 + 8 + 8 + 16 + 16 + 160 = 219 bits
Summary of Field Sizes for PUS-A, B, and C Headers:
PUS-A (Telemetry):
Size: Typically 107 bits, variable-length telemetry data
PUS-B (Event Reporting):
Size: Typically 187 bits, variable-length event data
PUS-C (Time Code):
Size: Typically 219 bits, variable-length time data
Each of these headers has a fixed portion with specific field sizes, but the actual size of the header will vary depending on the amount of data contained (telemetry, event data, or time data).

Let me know if you need more details on these fields!
 */