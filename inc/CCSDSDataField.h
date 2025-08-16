#ifndef CCSDS_DATA_FIELD_H
#define CCSDS_DATA_FIELD_H

#include <CCSDSResult.h>
#include <vector>
#include <memory>
#include "CCSDSSecondaryHeaderAbstract.h"
#include "CCSDSSecondaryHeaderFactory.h"
#include "PusServices.h"

namespace CCSDS {
  /**
   * @brief Represents the data field of a CCSDS packet.
   *
   * This class encapsulates the structure and operations for handling the data field
   * of a CCSDS (Consultative Committee for Space Data Systems) packet. It allows for
   * setting and retrieving application data, data field headers, and calculating
   * the full data field. Additionally, it provides methods for managing the data field's
   * size and printing its content.
   *
   * The data field consists of headers and application data, and the class supports
   * different header types (PusA, PusB, PusC). It also allows setting and getting
   * the data packet size, as well as managing the data field header flag.
   */
  class DataField {
  public:
    DataField() {
      m_secondaryHeaderFactory.registerType(std::make_shared<BufferHeader>());
      m_secondaryHeaderFactory.registerType(std::make_shared<PusA>());
      m_secondaryHeaderFactory.registerType(std::make_shared<PusB>());
      m_secondaryHeaderFactory.registerType(std::make_shared<PusC>());
    };

    ~DataField() = default;

    /**
    * @brief Registers a new header type with its creation function.
    *
    * This function adds a new header type to the factory by associating the header's type string with a shared pointer to the header.
    *
    * @param header A shared pointer to a `SecondaryHeaderAbstract` object to register.
    */
    template <typename T>
    void RegisterSecondaryHeader() {
      m_secondaryHeaderFactory.registerType(std::make_shared<T>());
    }

    /**
     * @brief Sets the application data using a vector of bytes.
     *
     * Replaces the current application data with the given vector and updates the header.
     *
     * @param applicationData A vector containing the application data bytes.
    *
     * @note  The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data
     * exceeds available size.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setApplicationData(const std::vector<uint8_t> &applicationData);

    /**
     * @brief Sets the application data for the data field.
     *
     * Validates and assigns the given application data to the data field.
     * Ensures the data size is within acceptable limits and does not exceed
     *
     * @param pData A pointer to the application data.
     * @param sizeData The size of the application data in bytes.
    *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setApplicationData(const uint8_t *pData, const size_t &sizeData);

    /**
     * @brief Sets the secondary header data for the data field.
     *
     * Validates and assigns the given secondary header data to the data field.
     * Ensures the data size is within acceptable limits and does not exceed
     * the remaining packet size after accounting for the header.
     *
     * @param pData A pointer to the application data.
     * @param sizeData The size of the application data in bytes.
    *
     * @note The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, const size_t &sizeData);

    /**
     * @brief Sets the secondary header for the data field using a PUS Type.
     *
     * Validates and assigns the given header data to the secondary header field.
     * Ensures the header size is within acceptable limits and does not exceed
     * the remaining packet size after accounting for the application data.
     *
     * @param pData A pointer to the header data.
     * @param sizeData The size of the header data in bytes.
     * @param pType enum of type PUSType to select
    *
     * @note If the PUS type is OTHER, the data is passed to the overloaded setDataFieldHeader method.
     * The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     * @return ResultBool.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const uint8_t *pData, const size_t &sizeData,
                                                const std::string &pType);

    /**
     * @brief Sets the data field header for the CCSDS DataField with a specific PUS type.
     *
     * This method configures the data field header based on the provided data and
     * the specified Packet Utilization Standard (PUS) type. It validates the header
     * size to ensure it does not exceed the maximum allowed packet size and creates
     * the appropriate header object based on the PUS type.
     *
     * @param data A vector containing the data for the data field header.
     * @param pType The PUS type (PUS_A, PUS_B, PUS_C, or OTHER) indicating the header format.
     *
     * @note If the PUS type is OTHER, the data is passed to the overloaded setDataFieldHeader method.
     * The method will log an error to standard error and ErrorCode is returned by ResultBool if provided data is invalid.
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &data, const std::string &pType);

    /**
     * @brief Sets the data field header for the CCSDS DataField.
     *
     * This method updates the data field header with the provided vector of bytes.
     * If the existing data field header is not empty, it clears the current contents
     * and logs a warning to indicate that the header has been overwritten.
     *
     * @param dataFieldHeader A vector containing the new data field header.
     *
     * @note The m_dataFieldHeaderType is set to OTHER after the header is updated. If the current data field header
     * is not empty, it will be cleared. The method will log an error to standard error and ErrorCode is returned
     * by ResultBool if provided data is invalid.
     * @return ResultBool
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const std::vector<uint8_t> &dataFieldHeader);

    /**
     * @brief Sets the data field header using a configuration file as reference.
     *
     * @note The config file must contain all required parameter for the interested secondary header, including
     * the secondary_header_type string indicating which registered secondary header to be parsed.
     *
     * @param cfg configuration file parser object.
     * @return ResultBool
     */
    [[nodiscard]] ResultBool setDataFieldHeader(const Config& cfg);

    /**
     * @brief Sets the secondary header for the data field using a PUS-A header.
     *
     * @param header A SecondaryHeaderAbstract derived object containing the header data.
     * @return None.
     */
    void setDataFieldHeader(std::shared_ptr<SecondaryHeaderAbstract> header);

    /**
     * @brief returns the secondary header factory
     *
     * @return SecondaryHeaderFactory& .
     */
    SecondaryHeaderFactory& getDataFieldHeaderFactory() {return m_secondaryHeaderFactory;}

    /**
     * @brief returns the secondary header
     * A SecondaryHeaderAbstract derived object containing the header data.
     *
     * @return std::shared_ptr<SecondaryHeaderAbstract>& .
     */
    SecondaryHeaderAbstract &getDataFieldHeader() {return *m_secondaryHeader;}

    /**
     * @brief Sets the maximum data packet size for the CCSDS DataField.
     *
     * This method updates the maximum allowed size for the data packet.
     * The data packet size is used to validate that the combined size of
     * the header and application data does not exceed this limit.
     *
     * @param value The maximum size of the data packet, in bytes.
     */
    void setDataPacketSize(const uint16_t &value);

    /** @brief Sets The auto update variable, if disabled the data size in the header field will not be updated.
     *
     * @param enable
     */
    void setDataFieldHeaderAutoUpdateStatus(const bool enable) { m_enableDataFieldUpdate = enable; }

    /**
    * @brief Retrieves the absolute size of the data field in bytes.
    *
    * This method returns the total allocated size for the data field, including both the header
    * and application data.
    *
    * @return The absolute size of the data field in bytes.
    */
    uint16_t getDataFieldAbsoluteBytesSize() const;

    /**
     * @brief Retrieves the size of the application data stored in the data field.
     *
     * @return The size of the application data in bytes
     */
    uint16_t getApplicationDataBytesSize() const;

    /**
     * @brief Retrieves the used size of the data field in bytes.
     *
     * This method returns the amount of space currently occupied by valid data within the data field.
     *
     * @return The used size of the data field in bytes.
     */
    uint16_t getDataFieldUsedBytesSize() const;

    /**
    * @brief Retrieves the available size of the data field in bytes.
    *
    * This method calculates and returns the remaining free space within the data field that can be utilized.
    *
    * @return The available size of the data field in bytes.
    */
    uint16_t getDataFieldAvailableBytesSize() const;

    /**
     * @brief Retrieves the secondary header data as a vector of bytes.
     *
     * If the header type is not OTHER or NA, retrieves the data from the
     * corresponding PUS header object.
     *
     * @return A vector containing the header data bytes.
     */
    std::vector<uint8_t> getDataFieldHeaderBytes();

    /**
     * @brief Retrieves the full data field by combining the data field header and application data.
     *
     * Combines the secondary header (if present) and application data into a single vector.
     * Ensures that the total size does not exceed the maximum allowed data packet size.
     *
     * @return A vector containing the full data field (header + application data).
     */
    std::vector<uint8_t> serialize();

    /**
     * @brief Retrieves the application data from the data field.
     *
     * This method returns a vector containing the raw application data stored in the data field.
     *
     * @return A vector containing the application data bytes.
     */
    std::vector<uint8_t> getApplicationData();

    /** @brief returns true if auto update has been enabled for the secondary header */
    [[nodiscard]] bool getDataFieldHeaderAutoUpdateStatus() const { return m_enableDataFieldUpdate; }

    /**
     * @brief retrieves true if a known secondary header has been set
     *
     * @return boolean
     */
    [[nodiscard]] bool getDataFieldHeaderFlag() const {
      return  m_secondaryHeader != nullptr;
    }

    /**
     * @brief retrieves the known PUS type
     *
     * @return std::shared_ptr<SecondaryHeaderAbstract>
     */
    [[nodiscard]] std::shared_ptr<SecondaryHeaderAbstract> getSecondaryHeader();

    /**
     * @brief Updates the data field header based on the current application data size.
     *
     * Updates the length field in the secondary header to match the size of the
     * application data. Ensures the header reflects the most recent data state.
     *
     * @return None.
     */
    void update();

  private:
    std::shared_ptr<SecondaryHeaderAbstract> m_secondaryHeader{};  ///< Shared pointer to the secondary header class
    SecondaryHeaderFactory m_secondaryHeaderFactory;               ///< secondary header dispatcher factory
    std::vector<uint8_t> m_applicationData{};                      ///< Application data buffer
    std::string m_dataFieldHeaderType{};                           ///< Data field Header type
    uint16_t m_dataPacketSize{2024};                               ///< Data field maximum size in bytes
    bool m_dataFieldHeaderUpdated{false};                          ///< Boolean for secondary header updated status
    bool m_enableDataFieldUpdate{true};                            ///< Boolean for secondary header update enable

  };
}

#endif // CCSDS_DATA_FIELD_H
