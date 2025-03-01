#ifndef CCSDSMANAGER_H
#define CCSDSMANAGER_H

#include <utility>
#include "CCSDSPacket.h"
#include "CCSDSResult.h"

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
    Manager();

    /**
     * @brief Constructs a Manager with a given packet template.
     *
     * @param packet The packet template to be used as a reference.
     */
    explicit Manager(Packet packet) : m_packetTemplate(std::move(packet)) {}

    /**
     * @brief Sets a new packet template.
     *
     * @param packet The new packet template to use.
     */
    void setPacketTemplate(Packet packet);

    /**
     * @brief Sets the size of the data field.
     *
     * @param size The new data field size in bytes.
     */
    void setDatFieldSize(uint16_t size);

    /**
     * @brief Sets the application data for the packet.
     *
     * @param data The application data as a vector of bytes.
     * @return ResultBool indicating success or failure.
     */
    ResultBool setApplicationData(const std::vector<uint8_t>& data);

    /**
     * @brief Enables or disables automatic updates for packets.
     *
     * @param enable Set to true to enable automatic updates, false to disable.
     */
    void setAutoUpdateEnable(bool enable);

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
    ResultBuffer getPacketAtIndex(uint16_t index);

    /**
     * @brief Retrieves the application data from the current packet.
     *
     * @return A ResultBuffer containing the application data.
     */
    [[nodiscard]] ResultBuffer getApplicationData() const;

    /**
     * @brief Retrieves the application data from a packet at the given index.
     *
     * @param index The index of the packet.
     * @return A ResultBuffer containing the application data of the selected packet.
     */
    ResultBuffer getApplicationDataAtIndex(uint16_t index);

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
    Packet getTemplate() { return m_packetTemplate; };

    /**
     * @brief Retrieves all stored packets.
     *
     * @return A vector containing all managed packets.
     */
    std::vector<CCSDS::Packet> getPackets();

  private:
    Packet m_packetTemplate{};  ///< The template packet used for generating new packets.
    bool m_updateEnable{true};  ///< Flag indicating whether automatic updates are enabled (default: true).
    std::vector<Packet> m_packets; ///< Collection of stored packets.
  };

} // namespace CCSDS

#endif // CCSDSMANAGER_H
