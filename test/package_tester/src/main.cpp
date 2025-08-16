#include "CCSDSPack.h"
#include <iostream>

class CustomSecondaryHeader final : public CCSDS::SecondaryHeaderAbstract {
public:
  CustomSecondaryHeader() {variableLength= true;};

  /**
   * @brief Constructs a CustomSecondaryHeader object with all fields explicitly set.
   */
  explicit CustomSecondaryHeader(const std::vector<uint8_t>& data) : m_data(data) {
    variableLength= true;
  };

  [[nodiscard]] CCSDS::ResultBool deserialize(const std::vector<uint8_t> &data) override {m_data = data; return true;};

  [[nodiscard]] uint16_t getSize() const override { return m_data.size(); }
  [[nodiscard]] std::string getType() const override { return m_type; }

  [[nodiscard]] std::vector<uint8_t> serialize() const override {return m_data;};
  void update(CCSDS::DataField* dataField) override {m_dataLength = m_data.size();}
  CCSDS::ResultBool loadFromConfig(const Config &config) override{return true;};

private:
  std::vector<uint8_t> m_data{};
  uint16_t m_dataLength = 0;
  const std::string m_type = "CustomSecondaryHeader";
};


int main() {
  std::cout << "Stage: Create Manager." << std::endl;
  CCSDS::Manager mgr; // Minimal smoke test: type is visible and linkable
  std::cout << "Stage: Complete." << std::endl;

  std::cout << "Stage: Create Template Package." << std::endl;
  // generate template packet with new registered secondary header
  CCSDS::Packet templatePacket;
  templatePacket.setPrimaryHeader(CCSDS::PrimaryHeader{
    1,
    2,
    1,
    55,
    0,
    0,
    0});
  std::cout << "Stage: Complete." << std::endl;

  std::cout << "Stage: Register custom secondary header." << std::endl;
  const std::vector<uint8_t> data{0x77,0xFA,0xB,0x0,0x0,0xB,0x5};
  templatePacket.RegisterSecondaryHeader<CustomSecondaryHeader>();

  if (const auto res = templatePacket.setDataFieldHeader(data, "CustomSecondaryHeader"); !res.has_value()) {
    std::cerr << "Error: "<< res.error().message() << ". CODE: " << res.error().code() << std::endl;
    return res.error().code();
  }
  std::cout << "Stage: Complete." << std::endl;

  // add template to manager

  std::cout << "Stage: Set template package." << std::endl;
  if (const auto res = mgr.setPacketTemplate(templatePacket); !res.has_value()) {
    std::cerr << "Error: "<< res.error().message() << ". CODE: " << res.error().code() << std::endl;
    return res.error().code();
  }
  std::cout << "Stage: Complete." << std::endl;

  // Add some data to generate packets

  std::cout << "Stage: Generate packets with application data.." << std::endl;
  mgr.setDataFieldSize(1000);
  if (const auto res =mgr.setApplicationData({1,2,3}); !res.has_value()) {
    std::cerr << "Error: "<< res.error().message() << ". CODE: " << res.error().code() << std::endl;
    return res.error().code();
  }
  std::cout << "Stage: Complete." << std::endl;

  // get ccsds packets

  std::cout << "Stage: Get packet buffer." << std::endl;
  const auto packetsData = mgr.getPacketsBuffer();
  printBufferData(packetsData);
  std::cout << "Stage: Complete." << std::endl;


  std::cout << "Stage: Test Generated package." << std::endl;

  const std::vector<uint8_t> expectedPacket{0x28, 0x37, 0x00, 0x00, 0x00, 0x0a, 0x77, 0xfa, 0x0b, 0x00, 0x00,
    0x0b, 0x05, 0x01, 0x02, 0x03, 0x3e, 0x97};

  if (std::equal(packetsData.begin(),packetsData.end(), expectedPacket.begin())) {
    std::cout << "Stage: -- Generated packet as expected." << std::endl;
  }else {
    std::cout << "Stage: -- Generated packet differs from expected." << std::endl;
  }

  std::cout << "Stage: Complete." << std::endl;

  return 0;
}
