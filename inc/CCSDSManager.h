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
      m_validator.setTemplatePacket(m_templatePacket);
      m_validator.configure(true, true, true);

    }

    /**
     * @brief Sets a new packet template.
     *
     * @param packet The new packet template to use.
     */
    void setPacketTemplate( Packet packet );

    /**
     * @brief Sets the size of the data field.
     *
     * @param size The new data field size in bytes.
     */
    void setDatFieldSize( uint16_t size );

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
    [[nodiscard]] ResultBuffer getApplicationDataBuffer() const;

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
     * @brief Clears the manager, removes all packets and template.
     */
    void clear();

    /**
     * @brief Clears the packets and sets the counter to 0.
     */
    void clearPackets();

  private:
    Packet m_templatePacket{};         ///< The template packet used for generating new packets.
    bool m_templateIsSet  { false };   ///< Boolean to indicate if Template has been set or not.
    bool m_updateEnable   {  true };   ///< bool indicating whether automatic updates are enabled (default: true).
    bool m_validateEnable {  true };   ///< bool indicating whether automatic validation is enabled (default: true).
    std::vector<Packet> m_packets;     ///< Collection of stored packets.
    uint16_t m_sequenceCount{ 0 };

    Validator m_validator{};
  };
} // namespace CCSDS

#endif // CCSDS_MANAGER_H
