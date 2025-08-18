#ifndef PUS_SERVICES_H
#define PUS_SERVICES_H

#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSResult.h"

//exclude includes when building for MCU
#ifndef CCSDS_MCU
  #include "CCSDSConfig.h"
#endif //CCSDS_MCU
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
  class PusA final : public CCSDS::SecondaryHeaderAbstract {
  public:
    PusA() = default;

    /**
     * @brief Constructs a PusA object with all fields explicitly set.
     * @param version PUS version (3 bits).
     * @param serviceType Service type (8 bits).
     * @param serviceSubtype Service subtype (8 bits).
     * @param sourceID Source identifier (8 bits).
     * @param dataLength Length of the telemetry data (16 bits).
     */
    explicit PusA(const std::uint8_t version, const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
                  const std::uint8_t sourceID, const std::uint32_t dataLength) : m_version(version & 0x7),
                                                                       m_serviceType(serviceType),
                                                                       m_serviceSubType(serviceSubtype),
                                                                       m_sourceID(sourceID), m_dataLength(dataLength) {
    }

    [[nodiscard]] std::uint8_t getVersion()        const          { return m_version;          }
    [[nodiscard]] std::uint8_t getServiceType()    const          { return m_serviceType;      }
    [[nodiscard]] std::uint8_t getServiceSubtype() const          { return m_serviceSubType;   }
    [[nodiscard]] std::uint8_t getSourceID()       const          { return m_sourceID;         }
    [[nodiscard]] std::uint16_t getDataLength()    const          { return m_dataLength;       }
    [[nodiscard]] std::uint16_t getSize()          const override { return m_size;             }
    [[nodiscard]] std::string getType()            const override { return m_type;          }

    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<std::uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
    CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif


  private:                               // Field	            Size (bits)	Description
    std::uint8_t m_version{};            // Version	            3	        Version of the PUS standard
    std::uint8_t m_serviceType{};        // Service Type	        8	        Type of service (e.g., 0x01 for telemetry)
    std::uint8_t m_serviceSubType{};     // Service Subtype	    8	        Subtype of the service (e.g., specific telemetry type)
    std::uint8_t m_sourceID{};           // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    std::uint16_t m_dataLength{};        // Data Length	        16	      Length of the telemetry data in bytes

    const std::string m_type = "PusA";   // Static registration (automatically called when the program starts)
    const std::uint16_t m_size = 6;      // bytes
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
  class PusB final : public CCSDS::SecondaryHeaderAbstract {
  public:
    PusB() = default;

    /**
     * @brief Constructs a PusB object with all fields explicitly set.
     * @param version PUS version (3 bits).
     * @param serviceType Service type (8 bits).
     * @param serviceSubtype Service subtype (8 bits).
     * @param sourceID Source identifier (8 bits).
     * @param eventID Event identifier (16 bits).
     * @param dataLength Length of the event data (16 bits).
     */
    explicit PusB(const std::uint8_t version, const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
                  const std::uint8_t sourceID, const std::uint8_t eventID, const std::uint16_t dataLength) : m_version(version & 0x7),
      m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_eventID(eventID),
      m_dataLength(dataLength) {
    }

    [[nodiscard]] std::uint8_t getVersion()        const          { return m_version;          }
    [[nodiscard]] std::uint8_t getServiceType()    const          { return m_serviceType;      }
    [[nodiscard]] std::uint8_t getServiceSubtype() const          { return m_serviceSubType;   }
    [[nodiscard]] std::uint8_t getSourceID()       const          { return m_sourceID;         }
    [[nodiscard]] std::uint16_t getEventID()       const          { return m_eventID;          }
    [[nodiscard]] std::uint16_t getDataLength()    const          { return m_dataLength;       }
    [[nodiscard]] std::uint16_t getSize()          const override { return m_size;             }
    [[nodiscard]] std::string getType()            const override { return m_type;          }

    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<std::uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
    CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif

  private:                                // Field	            Size (bits)	Description
    std::uint8_t m_version{};             // Version	            3	        Version of the PUS standard
    std::uint8_t m_serviceType{};         // Service Type	      8	        Type of service (e.g., 0x02 for event reporting)
    std::uint8_t m_serviceSubType{};      // Service Subtype	    8	        Subtype of the service (e.g., specific event type)
    std::uint8_t m_sourceID{};            // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    std::uint16_t m_eventID{};            // Event ID	          16	      ID of the event being reported
    std::uint16_t m_dataLength{};         // Data Length	        16	      Length of the event data in bytes

    const std::string m_type = "PusB";    // Static registration (automatically called when the program starts)
    const std::uint16_t m_size = 8;       // bytes
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
  class PusC final : public CCSDS::SecondaryHeaderAbstract {
  public:
    PusC() {variableLength=true;}

    /**
     * @brief Constructs a PusC object with all fields explicitly set.
     * @param version PUS version (3 bits).
     * @param serviceType Service type (8 bits).
     * @param serviceSubtype Service subtype (8 bits).
     * @param sourceID Source identifier (8 bits).
     * @param timeCode Time code value (variable).
     * @param dataLength Length of the time data (16 bits).
     */
    explicit PusC(const std::uint8_t version, const std::uint8_t serviceType, const std::uint8_t serviceSubtype,
                  const std::uint8_t sourceID, const std::vector<std::uint8_t>& timeCode,
                  const std::uint16_t dataLength) : m_version(version & 0x7), m_serviceType(serviceType),
                                               m_serviceSubType(serviceSubtype), m_sourceID(sourceID),
                                               m_timeCode(timeCode), m_dataLength(dataLength) {
      variableLength=true;
    }

    [[nodiscard]] std::uint8_t getVersion()                const          { return m_version;                  }
    [[nodiscard]] std::uint8_t getServiceType()            const          { return m_serviceType;              }
    [[nodiscard]] std::uint8_t getServiceSubtype()         const          { return m_serviceSubType;           }
    [[nodiscard]] std::uint8_t getSourceID()               const          { return m_sourceID;                 }
    [[nodiscard]] std::vector<std::uint8_t> getTimeCode()  const          { return m_timeCode;              }
    [[nodiscard]] std::uint16_t getDataLength()            const          { return m_dataLength;               }
    [[nodiscard]] std::uint16_t getSize()                  const override { return m_size + m_timeCode.size(); }
    [[nodiscard]] std::string getType()               const override { return m_type;                  }

    [[nodiscard]] std::vector<std::uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<std::uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

#ifndef CCSDS_MCU
    CCSDS::ResultBool loadFromConfig(const ::Config &cfg) override;
#endif

  private:                                   // Field	            Size (bits)	Description
    std::uint8_t m_version{};                // Version	            3	        Version of the PUS standard
    std::uint8_t m_serviceType{};            // Service Type	        8	        Type of service (e.g., 0x03 for time code)
    std::uint8_t m_serviceSubType{};         // Service Subtype	    8	        Subtype of the service (e.g., specific time type
    std::uint8_t m_sourceID{};               // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    std::vector<std::uint8_t> m_timeCode{};  // Time Code	          16	      Time code value, depending on the system
    std::uint16_t m_dataLength{};            // Data Length	        16	      Length of the time data in bytes

    const std::string m_type = "PusC";       // Static registration (automatically called when the program starts)
    const std::uint16_t m_size = 6;          // bytes minimum size

  };


#endif //PUS_SERVICES_H