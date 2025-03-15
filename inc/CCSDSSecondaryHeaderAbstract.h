#ifndef CCSDSSECONDARYHEADER_H
#define CCSDSSECONDARYHEADER_H

#include <CCSDSResult.h>
#include <vector>
#include <cstdint>

namespace CCSDS {
  /**
   * @brief Abstract base class for a (Packet Utilization Standard) header.
   *
   * Defines the common interface for all header types.
   */
  class SecondaryHeaderAbstract {
  public:
    virtual ~SecondaryHeaderAbstract() = default;

    /**
   * @brief Sets the length of the data associated with the packet.
   * @param dataLength Length of the data in bytes.
   */
    virtual void setDataLength(uint16_t dataLength) = 0;


    /**
     * @brief takes a buffer if data (vector uint8) and creates the header
     * @return Boolean true on success or Error.
     */
    [[nodiscard]] virtual ResultBool deserialize(const std::vector<uint8_t> &data) = 0;

    /**
     * @brief Gets the length of the data associated with the packet if applicable.
     * @return The length of the data in bytes set in the header.
     */
    [[nodiscard]] virtual uint16_t getDataLength() const = 0;

    /**
     * @brief Gets the size of the header in bytes.
     * @return The size of the header in bytes.
     */
    [[nodiscard]] virtual uint16_t getSize() const = 0;

    /**
     * @brief Retrieves the serialized representation of the  header.
     * @return A vector containing the header bytes. (does not include data field)
     */
    [[nodiscard]] virtual std::vector<uint8_t> serialize() const = 0; // Pure virtual method for polymorphism

    /**
     * @brief Retrieves the name of the packet.
     * @return A vector containing the header bytes. (does not include data field)
     */
    [[nodiscard]] virtual std::string getType() const = 0; // Pure virtual method for polymorphism
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
  class DataOnlyHeader final : public CCSDS::SecondaryHeaderAbstract {
  public:
    DataOnlyHeader() = default;

    /**
     * @brief Constructs a DataOnlyHeader object with all fields explicitly set.
     *

     */
    explicit DataOnlyHeader(const std::vector<uint8_t>& data) : m_data(data) {
    };

    void setDataLength(const uint16_t dataLength) override { (void)dataLength; }

    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data) override {m_data = data; return true;};

    [[nodiscard]] uint16_t getDataLength() const override { return 0; }
    [[nodiscard]] uint16_t getSize() const override { return m_data.size(); }
    [[nodiscard]] std::string getType() const override { return m_type; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override {return m_data;};

  private: // Field	            Size (bits)	Description
    std::vector<uint8_t> m_data;
    // Telemetry Data	    Variable    (based on Data Length)	The actual telemetry data (variable length)
    const std::string m_type = "DataOnlyHeader";// Static registration (automatically called when the program starts)
    static bool registered;
  };


}




#endif //CCSDSSECONDARYHEADER_H
