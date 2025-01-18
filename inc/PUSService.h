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
    enum PUSServiceType {
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

        uint8_t                    getVersion()         const { return        m_version; };
        uint8_t                    getServiceType()     const { return    m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return m_serviceSubType; };
        uint8_t                    getSourceID()        const { return       m_sourceID; };
        uint32_t                   getTimeStamp()       const { return      m_timeStamp; };
        uint8_t                    getSize()            const { return           m_size; };
        static PUSServiceType      getType()                  { return            PUS_A; };
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

        uint8_t                    getVersion()         const { return         m_version; };
        uint8_t                    getServiceType()     const { return     m_serviceType; };
        uint8_t                    getServiceSubtype()  const { return  m_serviceSubType; };
        uint8_t                    getDestinationID()   const { return   m_destinationID; };
        uint16_t                   getSequenceControl() const { return m_sequenceControl; };
        static PUSServiceType getType()                  { return            PUS_B; };
        std::vector<uint8_t>       getData() const;

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
        PusC(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, uint16_t, std::vector<uint8_t> missionData) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_missionData(std::move(missionData)) {}

        static PUSServiceType getType()                  { return            PUS_C; };
        std::vector<uint8_t> getData();
    private:
        uint8_t                    m_version{};
        uint8_t                m_serviceType{};
        uint8_t             m_serviceSubType{};
        uint8_t                   m_sourceID{};
        std::vector<uint8_t>   m_missionData{};

        uint16_t m_size = 6; // set to minimum, typically between 6 - 12 bytes
    };
}

#endif //PUSSERVICE_H
