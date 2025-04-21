#ifndef CCSDS_SECONDARY_HEADER_ABSTRACT_H
#define CCSDS_SECONDARY_HEADER_ABSTRACT_H

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
   * @brief Represents a fixed secondary header used as a data buffer.
   *
   * This class holds the raw data for the secondary header. It provides methods to serialize and deserialize
   * the header data as a vector of bytes. The `DataOnlyHeader` is not associated with any specific PUS type.
   *
   * Field Summary:
   *  - m_data : A buffer holding the header data.
   *  - m-Size : The length of the data.
   *  - m_dataLength : unused / can be set and retrieved, completely standalone from data.
   *
   *  @note The `getSize` method returns the size of the data buffer, while `getDataLength` returns the length
   *  of the data as set. The `m_dataLength` field is not tied to any other data structure or context.
   */
  class BufferHeader final : public SecondaryHeaderAbstract {
  public:
    BufferHeader() = default;

    /**
     * @brief Constructs a DataOnlyHeader object with all fields explicitly set.
     */
    explicit BufferHeader(const std::vector<uint8_t>& data) : m_data(data) {
    };

    void setDataLength(const uint16_t dataLength) override { m_dataLength = dataLength; }

    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data) override {m_data = data; return true;};

    [[nodiscard]] uint16_t getDataLength() const override { return m_dataLength; }
    [[nodiscard]] uint16_t getSize() const override { return m_data.size(); }
    [[nodiscard]] std::string getType() const override { return m_type; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override {return m_data;};

  private:
    std::vector<uint8_t> m_data;
    uint16_t m_dataLength = 0;
    const std::string m_type = "DataOnlyHeader";
  };


}




#endif //CCSDS_SECONDARY_HEADER_ABSTRACT_H
