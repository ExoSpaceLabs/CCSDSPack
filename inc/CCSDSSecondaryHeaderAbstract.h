#ifndef CCSDS_SECONDARY_HEADER_ABSTRACT_H
#define CCSDS_SECONDARY_HEADER_ABSTRACT_H

#include <CCSDSResult.h>
#include "CCSDSConfig.h"
#include <vector>
#include <cstdint>

namespace CCSDS {
  class DataField;
  /**
   * @brief Abstract base class for a (Packet Utilization Standard) header.
   *
   * Defines the common interface for all header types.
   */
  class SecondaryHeaderAbstract {
  public:
    virtual ~SecondaryHeaderAbstract() = default;

    /**
     * @brief takes a buffer if data (vector uint8) and creates the header
     * @return Boolean true on success or Error.
     */
    [[nodiscard]] virtual ResultBool deserialize(const std::vector<uint8_t> &data) = 0;


    /**
     * @brief Defines how the packet secondary header is updated using the data field as reference.
     * This method is called every time a data get is performed from the packet, that is if update is enabled and
     * method successfully registered.
     *
     * @param dataField
     */
    virtual void update(DataField* dataField) = 0;

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


    virtual ResultBool loadFromConfig(const Config &config) = 0;
    void setVariableLength(const bool bEnable){ variableLength =  bEnable;}
    bool variableLength{false};
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

    [[nodiscard]] ResultBool deserialize(const std::vector<uint8_t> &data) override {m_data = data; return true;};

    [[nodiscard]] uint16_t getSize() const override { return m_data.size(); }
    [[nodiscard]] std::string getType() const override { return m_type; }

    [[nodiscard]] std::vector<uint8_t> serialize() const override {return m_data;};
    void update(DataField* dataField) override {m_dataLength = m_data.size();}
    ResultBool loadFromConfig(const Config &config) override{return true;};

  private:
    std::vector<uint8_t> m_data;
    uint16_t m_dataLength = 0;
    const std::string m_type = "DataOnlyHeader";
  };


}




#endif //CCSDS_SECONDARY_HEADER_ABSTRACT_H
