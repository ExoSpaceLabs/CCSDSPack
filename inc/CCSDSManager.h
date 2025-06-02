#ifndef CCSDS_MANAGER_H
#define CCSDS_MANAGER_H

#include <utility>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"
#include "CCSDSValidator.h"

namespace CCSDS {
  /**
   * @class Manager
   * @brief Manages CCSDS packets and their templates.
   *
   * This class provides an interface for managing CCSDS packets, allowing
   * users to set a packet template, configure data field sizes, and retrieve
   * application data and packet instances.
   */
  class Manager {
  public:
    /**
     * @brief Default constructor.
     */
    Manager() = default;

    /**
     * @brief Constructs a Manager with a given packet template.
     *
     * @param packet The packet template to be used as a reference.
     */
    explicit Manager(Packet packet) : m_templatePacket(std::move(packet)) {
      m_templateIsSet = true;
      m_templatePacket.setUpdatePacketEnable(false);
      m_validator.setTemplatePacket(m_templatePacket);
      m_validator.configure(true, true, true);

    }

    /**
     * set sync pattern that should indicate the start of a CCSDS packet. insertion
     * is disabled by default. use setSyncPatternEnable to enable.
     *
     * @param syncPattern uint32_t (default 0x1ACFFC1D)
     */
    void setSyncPattern(uint32_t syncPattern);

    /**
     * returns the currently set sync pattern.
     *
     * @return uint32_t
     */
    uint32_t getSyncPattern() const;

    /**
     * enable sync pattern utilization both in serialization, deserialization, read and write.
     *
     * @param enable bool (default false)
     */
    void setSyncPatternEnable(bool enable);

    /**
     * returns the current settings of the sync pattern enable
     *
     * @return bool
     */
    bool getSyncPatternEnable() const;

    /**
     * @brief Sets a new packet template.
     *
     * @param packet The new packet template to use.
     */
    [[nodiscard]] ResultBool setPacketTemplate(Packet packet);

    /**
    * @brief Loads a template packet from a configuration file.
    *
    * @param configPath  path to the configuration file.
    */
    [[nodiscard]] ResultBool loadTemplateConfigFile(const std::string &configPath);

    /**
    * @brief Loads a template packet from a configuration object.
    *
    * @param cfg  Configuration obj to load template from.
    */
    [[nodiscard]] ResultBool loadTemplateConfig(const Config &cfg);

    /**
     * @brief Sets the size of the data field.
     *
     * @param size The new data field size in bytes.
     */
    void setDataFieldSize( uint16_t size );

    /**
     * @brief retrieves the set data field size (this includes the secondary header if present)
     *
     * @return uint16_t the size of data length in bytes.
     */
    uint16_t getDataFieldSize() const;

    /**
     * @brief Sets the application data for the packet.
     *
     * @param data The application data as a vector of bytes.
     * @return ResultBool indicating success or failure.
     */
    ResultBool setApplicationData( const std::vector<uint8_t> &data );

    /**
     * @brief Enables or disables automatic updates for packets.
     *
     * @param enable Set to true to enable automatic updates, false to disable.
     */
    void setAutoUpdateEnable( bool enable );

    /**
     * @brief Enables or disables automatic validation of packets.
     *
     * @param enable Set to true to enable automatic validation, false to disable.
     */
    void setAutoValidateEnable( bool enable );

    /**
     * @brief Retrieves the packet template in serialized form.
     *
     * @return A ResultBuffer containing the serialized packet template.
     */
    ResultBuffer getPacketTemplate();

    /**
     * @brief Retrieves a packet at the specified index.
     *
     * @param index The index of the packet to retrieve.
     * @return A ResultBuffer containing the requested packet.
     */
    ResultBuffer getPacketBufferAtIndex( uint16_t index );


    /**
     * @brief Retrieves a buffer containing all the stored packets sequentially.
     *
     * @return A vector of bytes containing the packets data.
     */
    std::vector<uint8_t> getPacketsBuffer() const;

    /**
     * @brief Retrieves the application data from the packets.
     *
     * @return A ResultBuffer containing the application data.
     */
    [[nodiscard]] ResultBuffer getApplicationDataBuffer();

    /**
     * @brief Retrieves the application data from a packet at the given index.
     *
     * @param index The index of the packet.
     * @return A ResultBuffer containing the application data of the selected packet.
     */
    ResultBuffer getApplicationDataBufferAtIndex( uint16_t index );

    /**
     * @brief Retrieves the total number of packets managed.
     *
     * @return The total number of stored packets.
     */
    [[nodiscard]] uint16_t getTotalPackets() const;

    /**
     * @brief Checks if automatic updates are enabled.
     *
     * @return True if auto-update is enabled, false otherwise.
     */
    [[nodiscard]] bool getAutoUpdateEnable() const { return m_updateEnable; }

    /**
     * @brief Retrieves the packet template.
     *
     * @return The stored packet template.
     */
    Packet getTemplate() { return m_templatePacket; };

    /**
     * @brief Retrieves all stored packets.
     *
     * @return A vector containing all managed packets.
     */
    std::vector<Packet> getPackets();

    /**
     * @brief Adds a new packet to the list.
     *
     * @param packet The new packet to be added.
     */
    [[nodiscard]] ResultBool addPacket(Packet packet);

    /**
     * @brief Adds a new packet to the list.
     *
     * @param packetBuffer The new packet to be added in the form of a buffer.
     */
    [[nodiscard]] ResultBool addPacketFromBuffer(const std::vector<uint8_t>& packetBuffer);

    /**
     * @brief Load a vector of packets.
     *
     * @param packets The packets
     */
    [[nodiscard]] ResultBool load(const std::vector<Packet>& packets);

    /**
     * @brief Load a packet or a series of packets from a buffer
     *
     * @param packetsBuffer The buffer holding packet data.
     */
    [[nodiscard]] ResultBool load(const std::vector<uint8_t>& packetsBuffer);

    /**
     * @brief Load a packet or a series of packets from a binary file
     *
     * @param binaryFile path to the file holding packet data.
     */
    [[nodiscard]] ResultBool read(const std::string& binaryFile);

    /**
     * @brief Write a packet or a series of packets to a binary file
     *
     * @param binaryFile destination file path for packets data.
     */
    [[nodiscard]] ResultBool write(const std::string& binaryFile) const;

    /**
     * @brief Load a template packet from a binary or configuration file
     *
     * @param filename path to the file holding template.
     */
    [[nodiscard]] ResultBool readTemplate(const std::string& filename);

    /**
     * @brief Clears the manager, removes all packets and template.
     */
    void clear();

    /**
     * @brief Clears the packets and sets the counter to 0.
     */
    void clearPackets();

    /**
     * @brief Returns a reverence to the manager's Validator
     *
     * @note changing settings of this instance will affect the manager
     */
    Validator& getValidatorReference() { return m_validator; }

    /**
     * @brief Returns a reference to the packets vector
     *
     * @note changing the data will affect the packets stored in the manager.
     */
    std::vector<Packet>& getPacketsReference() { return m_packets; }

  private:
    Packet m_templatePacket{};         ///< The template packet used for generating new packets.
    bool m_templateIsSet  { false };   ///< Boolean to indicate if Template has been set or not.
    bool m_updateEnable   {  true };   ///< bool indicating whether automatic updates are enabled (default: true).
    bool m_validateEnable {  true };   ///< bool indicating whether automatic validation is enabled (default: true).
    bool m_syncPattEnable { false };   ///< bool indicating whether automatic sync pattern insertion is enabled (default: false).
    std::vector<Packet> m_packets;     ///< Collection of stored packets.
    uint16_t m_sequenceCount{ 0 };

    Validator m_validator{};
    uint32_t m_syncPattern{0x1ACFFC1D};
  };
} // namespace CCSDS

#endif // CCSDS_MANAGER_H
