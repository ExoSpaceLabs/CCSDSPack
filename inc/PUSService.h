
#ifndef PUSSERVICE_H
#define PUSSERVICE_H

#include <vector>
#include <cstdint>

namespace CCSDS {

    /**
     * @brief Enumeration representing the different types of PUS headers.
     */
    enum PUSType {
        NA,     ///< Not applicable or undefined.
        PUS_A,  ///< PUS Type A: Telemetry header.
        PUS_B,  ///< PUS Type B: Event Reporting header.
        PUS_C,  ///< PUS Type C: Time Code header.
        OTHER   ///< Other or custom PUS types.
    };

    /**
     * @brief Abstract base class for a PUS (Packet Utilization Standard) header.
     *
     * Defines the common interface for all PUS header types.
     */
    class PusHeader {
    public:

        virtual ~PusHeader() = default;

        /**
       * @brief Sets the length of the data associated with the PUS packet.
       * @param dataLength Length of the data in bytes.
       */
        virtual void setDataLength(uint16_t dataLength);

        /**
         * @brief Gets the length of the data associated with the PUS packet.
         * @return The length of the data in bytes set in the header.
         */
        virtual uint16_t             getDataLength() const;

        /**
         * @brief Gets the size of the PUS header in bytes.
         * @return The size of the header in bytes.
         */
        virtual uint8_t              getSize() const;

        /**
         * @brief Retrieves the serialized representation of the PUS header.
         * @return A vector containing the header bytes. (does not include data field)
         */
        virtual std::vector<uint8_t> getData() const; // Pure virtual method for polymorphism
    };

    /**
     * @brief Represents a PUS Type A (Telemetry) header.
     *
     * Contains fields used for telemetry data.
     *
     * Field Summary:
     * - Version: 3 bits, PUS standard version.
     * - Service Type: 8 bits, type of service (e.g., 0x01 for telemetry).
     * - Service Subtype: 8 bits, subtype of the service.
     * - Source ID: 8 bits, source identifier (e.g., satellite ID).
     * - Data Length: 16 bits, length of the telemetry data in bytes.
     */
    class PusA final : public PusHeader {
    public:
        /**
         * @brief Constructs a PusA object with all fields explicitly set.
         * @param version PUS version (3 bits).
         * @param serviceType Service type (8 bits).
         * @param serviceSubtype Service subtype (8 bits).
         * @param sourceID Source identifier (8 bits).
         * @param dataLength Length of the telemetry data (16 bits).
         */
        explicit PusA(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint32_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_dataLength(dataLength) {};

        /**
         * @brief Constructs a PusA object by parsing a serialized data vector.
         * @param data Vector containing the serialized PUS-A header 6 elements.
         * @throws std::invalid_argument if the data size is invalid or parsing fails.
         */
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

    /**
     * @brief Represents a PUS Type B (Event Reporting) header.
     *
     * Contains fields used for event reporting.
     *
     * Field Summary:
     * - Version: 3 bits, PUS standard version.
     * - Service Type: 8 bits, type of service (e.g., 0x02 for event reporting).
     * - Service Subtype: 8 bits, subtype of the service.
     * - Source ID: 8 bits, source identifier (e.g., satellite ID).
     * - Event ID: 16 bits, identifier of the event.
     * - Data Length: 16 bits, length of the event data in bytes.
     */
    class PusB final : public PusHeader {
    public:

        /**
         * @brief Constructs a PusB object with all fields explicitly set.
         * @param version PUS version (3 bits).
         * @param serviceType Service type (8 bits).
         * @param serviceSubtype Service subtype (8 bits).
         * @param sourceID Source identifier (8 bits).
         * @param eventID Event identifier (16 bits).
         * @param dataLength Length of the event data (16 bits).
         */
        explicit PusB(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint8_t eventID, const uint16_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_eventID(eventID), m_dataLength(dataLength) {}

        /**
         * @brief Constructs a PusB object by parsing a serialized data vector.
         * @param data Vector containing the serialized PUS-B header 7 elements.
         * @throws std::invalid_argument if the data size is invalid or parsing fails.
         */
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

    /**
     * @brief Represents a PUS Type C (Time Code) header.
     *
     * Contains fields used for time synchronization.
     *
     * Field Summary:
     * - Version: 3 bits, PUS standard version.
     * - Service Type: 8 bits, type of service (e.g., 0x03 for time code).
     * - Service Subtype: 8 bits, subtype of the service.
     * - Source ID: 8 bits, source identifier (e.g., satellite ID).
     * - Time Code: 16 bits, value representing the time.
     * - Data Length: 16 bits, length of the time data in bytes.
     */
    class PusC final : public PusHeader {
    public:
        /**
         * @brief Constructs a PusC object with all fields explicitly set.
         * @param version PUS version (3 bits).
         * @param serviceType Service type (8 bits).
         * @param serviceSubtype Service subtype (8 bits).
         * @param sourceID Source identifier (8 bits).
         * @param timeCode Time code value (16 bits).
         * @param dataLength Length of the time data (16 bits).
         */
        explicit PusC(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype, const uint8_t sourceID, const uint16_t timeCode, const uint16_t dataLength) :
        m_version(version), m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_timeCode(timeCode), m_dataLength(dataLength) {}

        /**
         * @brief Constructs a PusC object by parsing a serialized data vector.
         * @param data Vector containing the serialized PUS-C header 8 elements.
         * @throws std::invalid_argument if the data size is invalid or parsing fails.
         */
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