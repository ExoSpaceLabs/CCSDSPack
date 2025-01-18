#pragma once
#include <utility>
#include <vector>
#include <cstdint>

//
// Created by dev on 1/18/25.
//

#ifndef PUSSERVICE_H
#define PUSSERVICE_H

namespace CCSDS {
    enum PUSType {
        NA,
        PUS_A,
        PUS_B,
        PUS_C,
        OTHER
    };

    class PusA {
    public:
        PusA();
        PusA(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, uint32_t timeStamp) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_timeStamp(timeStamp) {};
        explicit PusA(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return        m_version; }
        uint8_t                    getServiceType()     const { return    m_serviceType; }
        uint8_t                    getServiceSubtype()  const { return m_serviceSubType; }
        uint8_t                    getSourceID()        const { return       m_sourceID; }
        uint32_t                   getTimeStamp()       const { return      m_timeStamp; }
        uint8_t                    getSize()            const { return           m_size; }
        static PUSType             getType()                  { return            PUS_A; }
        std::vector<uint8_t>       getData() const;

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
        PusB(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t destinationID, uint16_t sequenceControl) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_destinationID(destinationID), m_sequenceControl(sequenceControl) {}
        explicit PusB(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return         m_version; }
        uint8_t                    getServiceType()     const { return     m_serviceType; }
        uint8_t                    getServiceSubtype()  const { return  m_serviceSubType; }
        uint8_t                    getDestinationID()   const { return   m_destinationID; }
        uint16_t                   getSequenceControl() const { return m_sequenceControl; }
        uint8_t                    getSize()            const { return            m_size; }
        static PUSType             getType()                  { return             PUS_B; };
        std::vector<uint8_t>       getData() const;

    private:
        uint8_t          m_version{};
        uint8_t      m_serviceType{};
        uint8_t   m_serviceSubType{};
        uint8_t    m_destinationID{};
        uint16_t m_sequenceControl{}; // timestamp or alternatively it can be used for sequence control

        const uint16_t m_size = 6; // bytes
    };
    class PusC {
    public:
        PusC();
        PusC(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, uint16_t, std::vector<uint8_t> missionData) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_missionData(std::move(missionData)) {}
        explicit PusC(std::vector<uint8_t> data);

        uint8_t                    getVersion()         const { return                     m_version; }
        uint8_t                    getServiceType()     const { return                 m_serviceType; }
        uint8_t                    getServiceSubtype()  const { return              m_serviceSubType; }
        uint8_t                    getSourceID()        const { return                    m_sourceID; }
        std::vector<uint8_t>       getMissionData()     const { return              m_missionData; }
        uint8_t                    getSize()            const { return m_size + m_missionData.size(); }
        static PUSType getType()                              { return                         PUS_C; }

        std::vector<uint8_t> getData();
    private:
        uint8_t                    m_version{};
        uint8_t                m_serviceType{};
        uint8_t             m_serviceSubType{};
        uint8_t                   m_sourceID{};
        std::vector<uint8_t>   m_missionData{};

        uint16_t m_size = 4; // set to minimum, typically between 6 - 12 bytes
    };
}

#endif //PUSSERVICE_H


/*Todo
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