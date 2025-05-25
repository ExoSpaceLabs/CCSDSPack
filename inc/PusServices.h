#ifndef PUS_SERVICES_H
#define PUS_SERVICES_H

#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "CCSDSResult.h"
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
    explicit PusA(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype,
                  const uint8_t sourceID, const uint32_t dataLength) : m_version(version & 0x7),
                                                                       m_serviceType(serviceType),
                                                                       m_serviceSubType(serviceSubtype),
                                                                       m_sourceID(sourceID), m_dataLength(dataLength) {
    }

    [[nodiscard]] uint8_t getVersion()        const          { return m_version;          }
    [[nodiscard]] uint8_t getServiceType()    const          { return m_serviceType;      }
    [[nodiscard]] uint8_t getServiceSubtype() const          { return m_serviceSubType;   }
    [[nodiscard]] uint8_t getSourceID()       const          { return m_sourceID;         }
    [[nodiscard]] uint16_t getDataLength()    const          { return m_dataLength;       }
    [[nodiscard]] uint16_t getSize()          const override { return m_size;             }
    [[nodiscard]] std::string getType()       const override { return m_type;          }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

  private:                          // Field	            Size (bits)	Description
    uint8_t m_version{};            // Version	            3	        Version of the PUS standard
    uint8_t m_serviceType{};        // Service Type	        8	        Type of service (e.g., 0x01 for telemetry)
    uint8_t m_serviceSubType{};     // Service Subtype	    8	        Subtype of the service (e.g., specific telemetry type)
    uint8_t m_sourceID{};           // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    uint16_t m_dataLength{};        // Data Length	        16	      Length of the telemetry data in bytes

    const std::string m_type = "PusA";  // Static registration (automatically called when the program starts)
    const uint16_t m_size = 6;          // bytes
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
    explicit PusB(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype,
                  const uint8_t sourceID, const uint8_t eventID, const uint16_t dataLength) : m_version(version & 0x7),
      m_serviceType(serviceType), m_serviceSubType(serviceSubtype), m_sourceID(sourceID), m_eventID(eventID),
      m_dataLength(dataLength) {
    }

    [[nodiscard]] uint8_t getVersion()        const          { return m_version;          }
    [[nodiscard]] uint8_t getServiceType()    const          { return m_serviceType;      }
    [[nodiscard]] uint8_t getServiceSubtype() const          { return m_serviceSubType;   }
    [[nodiscard]] uint8_t getSourceID()       const          { return m_sourceID;         }
    [[nodiscard]] uint16_t getEventID()       const          { return m_eventID;          }
    [[nodiscard]] uint16_t getDataLength()    const          { return m_dataLength;       }
    [[nodiscard]] uint16_t getSize()          const override { return m_size;             }
    [[nodiscard]] std::string getType()       const override { return m_type;          }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

  private:                           // Field	            Size (bits)	Description
    uint8_t m_version{};             // Version	            3	        Version of the PUS standard
    uint8_t m_serviceType{};         // Service Type	      8	        Type of service (e.g., 0x02 for event reporting)
    uint8_t m_serviceSubType{};      // Service Subtype	    8	        Subtype of the service (e.g., specific event type)
    uint8_t m_sourceID{};            // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    uint16_t m_eventID{};            // Event ID	          16	      ID of the event being reported
    uint16_t m_dataLength{};         // Data Length	        16	      Length of the event data in bytes

    const std::string m_type = "PusB";  // Static registration (automatically called when the program starts)
    const uint16_t m_size = 8;          // bytes
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
    PusC() = default;

    /**
     * @brief Constructs a PusC object with all fields explicitly set.
     * @param version PUS version (3 bits).
     * @param serviceType Service type (8 bits).
     * @param serviceSubtype Service subtype (8 bits).
     * @param sourceID Source identifier (8 bits).
     * @param timeCode Time code value (16 bits).
     * @param dataLength Length of the time data (16 bits).
     */
    explicit PusC(const uint8_t version, const uint8_t serviceType, const uint8_t serviceSubtype,
                  const uint8_t sourceID, const uint16_t timeCode,
                  const uint16_t dataLength) : m_version(version & 0x7), m_serviceType(serviceType),
                                               m_serviceSubType(serviceSubtype), m_sourceID(sourceID),
                                               m_timeCode(timeCode), m_dataLength(dataLength) {
    }

    [[nodiscard]] uint8_t getVersion()        const          { return m_version;          }
    [[nodiscard]] uint8_t getServiceType()    const          { return m_serviceType;      }
    [[nodiscard]] uint8_t getServiceSubtype() const          { return m_serviceSubType;   }
    [[nodiscard]] uint8_t getSourceID()       const          { return m_sourceID;         }
    [[nodiscard]] uint16_t getTimeCode()      const          { return m_timeCode;         }
    [[nodiscard]] uint16_t getDataLength()    const          { return m_dataLength;       }
    [[nodiscard]] uint16_t getSize()          const override { return m_size;             }
    [[nodiscard]] std::string getType()       const override { return m_type;          }

    [[nodiscard]] std::vector<uint8_t> serialize() const override;
    [[nodiscard]] CCSDS::ResultBool    deserialize( const std::vector<uint8_t> &data ) override;
    void update(CCSDS::DataField* dataField) override;

  private:                           // Field	            Size (bits)	Description
    uint8_t m_version{};             // Version	            3	        Version of the PUS standard
    uint8_t m_serviceType{};         // Service Type	      8	        Type of service (e.g., 0x03 for time code)
    uint8_t m_serviceSubType{};      // Service Subtype	    8	        Subtype of the service (e.g., specific time type
    uint8_t m_sourceID{};            // Source ID	          8	        ID of the source (e.g., satellite or sensor)
    uint16_t m_timeCode{};           // Time Code	          16	      Time code value, depending on the system
    uint16_t m_dataLength{};         // Data Length	        16	      Length of the time data in bytes

    const std::string m_type = "PusC"; // Static registration (automatically called when the program starts)
    const uint16_t m_size = 8;         // bytes
  };


#endif //PUS_SERVICES_H