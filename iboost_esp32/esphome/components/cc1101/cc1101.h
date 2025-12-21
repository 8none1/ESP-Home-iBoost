/*
(c) Panagiotis Karagiannis MIT Licence
The original library taken from
https://github.com/simonmonk/CC1101_arduino (Released under MIT licence)
Original creator
http://www.elechouse.com/ thank you for this library

Modified for ESPHome ESP-IDF compatibility (no Arduino dependency)
*/

#ifndef CC1101_RF_h
#define CC1101_RF_h

#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include <cstring>
#include <cstdint>
#include <cstdarg>

// ESP-IDF SPI includes
#ifdef USE_ESP32
#include "driver/spi_master.h"
#include "driver/gpio.h"
#endif

//***************************************CC1101 define**************************************************//
// CC1101 CONFIG REGISTERS
#define CC1101_IOCFG2       0x00        // GDO2 output pin configuration
#define CC1101_IOCFG1       0x01        // GDO1 output pin configuration
#define CC1101_IOCFG0       0x02        // GDO0 output pin configuration
#define CC1101_FIFOTHR      0x03        // RX FIFO and TX FIFO thresholds
#define CC1101_SYNC1        0x04        // Sync word, high uint8_t
#define CC1101_SYNC0        0x05        // Sync word, low uint8_t
#define CC1101_PKTLEN       0x06        // Packet length
#define CC1101_PKTCTRL1     0x07        // Packet automation control
#define CC1101_PKTCTRL0     0x08        // Packet automation control
#define CC1101_ADDR         0x09        // Device address
#define CC1101_CHANNR       0x0A        // Channel number
#define CC1101_FSCTRL1      0x0B        // Frequency synthesizer control
#define CC1101_FSCTRL0      0x0C        // Frequency synthesizer control
#define CC1101_FREQ2        0x0D        // Frequency control word, high uint8_t
#define CC1101_FREQ1        0x0E        // Frequency control word, middle uint8_t
#define CC1101_FREQ0        0x0F        // Frequency control word, low uint8_t
#define CC1101_MDMCFG4      0x10        // Modem configuration
#define CC1101_MDMCFG3      0x11        // Modem configuration
#define CC1101_MDMCFG2      0x12        // Modem configuration
#define CC1101_MDMCFG1      0x13        // Modem configuration
#define CC1101_MDMCFG0      0x14        // Modem configuration
#define CC1101_DEVIATN      0x15        // Modem deviation setting
#define CC1101_MCSM2        0x16        // Main Radio Control State Machine configuration
#define CC1101_MCSM1        0x17        // Main Radio Control State Machine configuration
#define CC1101_MCSM0        0x18        // Main Radio Control State Machine configuration
#define CC1101_FOCCFG       0x19        // Frequency Offset Compensation configuration
#define CC1101_BSCFG        0x1A        // Bit Synchronization configuration
#define CC1101_AGCCTRL2     0x1B        // AGC control
#define CC1101_AGCCTRL1     0x1C        // AGC control
#define CC1101_AGCCTRL0     0x1D        // AGC control
#define CC1101_WOREVT1      0x1E        // High uint8_t Event 0 timeout
#define CC1101_WOREVT0      0x1F        // Low uint8_t Event 0 timeout
#define CC1101_WORCTRL      0x20        // Wake On Radio control
#define CC1101_FREND1       0x21        // Front end RX configuration
#define CC1101_FREND0       0x22        // Front end TX configuration
#define CC1101_FSCAL3       0x23        // Frequency synthesizer calibration
#define CC1101_FSCAL2       0x24        // Frequency synthesizer calibration
#define CC1101_FSCAL1       0x25        // Frequency synthesizer calibration
#define CC1101_FSCAL0       0x26        // Frequency synthesizer calibration
#define CC1101_RCCTRL1      0x27        // RC oscillator configuration
#define CC1101_RCCTRL0      0x28        // RC oscillator configuration
#define CC1101_FSTEST       0x29        // Frequency synthesizer calibration control
#define CC1101_PTEST        0x2A        // Production test
#define CC1101_AGCTEST      0x2B        // AGC test
#define CC1101_TEST2        0x2C        // Various test settings
#define CC1101_TEST1        0x2D        // Various test settings
#define CC1101_TEST0        0x2E        // Various test settings

// CC1101 Strobe commands
#define CC1101_SRES         0x30        // Reset chip.
#define CC1101_SFSTXON      0x31        // Enable and calibrate frequency synthesizer
#define CC1101_SXOFF        0x32        // Turn off crystal oscillator.
#define CC1101_SCAL         0x33        // Calibrate frequency synthesizer and turn it off
#define CC1101_SRX          0x34        // Enable RX.
#define CC1101_STX          0x35        // Enable TX.
#define CC1101_SIDLE        0x36        // Exit RX / TX
#define CC1101_SAFC         0x37        // Perform AFC adjustment
#define CC1101_SWOR         0x38        // Start automatic RX polling sequence
#define CC1101_SPWD         0x39        // Enter power down mode
#define CC1101_SFRX         0x3A        // Flush the RX FIFO buffer.
#define CC1101_SFTX         0x3B        // Flush the TX FIFO buffer.
#define CC1101_SWORRST      0x3C        // Reset real time clock.
#define CC1101_SNOP         0x3D        // No operation.

// CC1101 STATUS REGISTERS
#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_WORTIME1     0x36
#define CC1101_WORTIME0     0x37
#define CC1101_PKTSTATUS    0x38
#define CC1101_VCO_VC_DAC   0x39
#define CC1101_TXBYTES      0x3A
#define CC1101_RXBYTES      0x3B

//CC1101 PATABLE,TXFIFO,RXFIFO
#define CC1101_PATABLE      0x3E
#define CC1101_TXFIFO       0x3F
#define CC1101_RXFIFO       0x3F

// the restriction the library enforces on maximum packet size
#define MAX_PACKET_LEN 61

// Most modules come with 26Mhz crystal
#ifndef CC1101_CRYSTAL_FREQUENCY
#define CC1101_CRYSTAL_FREQUENCY 26000000ul
#endif

// SPI transfer flags
#define WRITE_BURST         0x40
#define READ_SINGLE         0x80
#define READ_BURST          0xC0
#define BYTES_IN_RXFIFO     0x7F

namespace esphome {
namespace cc1101 {

class CC1101 : public Component {
 public:
    // Default constructor with default ESP32 pins (CS=5, MISO_READ=4)
    CC1101() : csn_pin_(5), miso_pin_(4), spi_initialized_(false) {
        status[0] = 0;
        status[1] = 0;
    }
    
    // Constructor with pin configuration
    CC1101(uint8_t csn_pin, uint8_t miso_pin);
    
    // Pin setters for ESPHome component configuration
    void set_cs_pin(uint8_t pin) { csn_pin_ = pin; }
    void set_miso_pin(uint8_t pin) { miso_pin_ = pin; }
    
    // ESPHome component interface
    void setup() override {}
    float get_setup_priority() const override { return setup_priority::HARDWARE; }
    
    // Reset the chip
    void reset();
    
    // Register access
    void writeRegister(uint8_t addr, uint8_t value);
    void writeBurstRegister(uint8_t addr, const uint8_t *buffer, uint8_t num);
    uint8_t readRegister(uint8_t addr);
    void readBurstRegister(uint8_t addr, uint8_t *buffer, uint8_t num);
    uint8_t readStatusRegister(uint8_t addr);
    
    // Initialize the CC1101
    void begin(uint32_t freq);
    
    // Send a strobe command
    uint8_t strobe(uint8_t strobe_cmd);
    
    // Packet operations
    bool sendPacket(const uint8_t *txBuffer, uint8_t size, uint32_t duration = 0);
    bool sendPacket(const char* msg);
    bool sendPacketSlowMCU(const uint8_t *txBuffer, uint8_t size);
    uint8_t getPacket(uint8_t *packet);
    
    // State control
    void setRXstate();
    void setIDLEstate();
    void setPowerDownState();
    
    // Configuration
    void setCommonRegisters();
    void optimizeSensitivity();
    void optimizeCurrent();
    void disableAddressCheck();
    void enableAddressCheck(uint8_t addr);
    void enableAddressCheckBcast(uint8_t addr);
    void setBaudrate4800bps();
    void setBaudrate38000bps();
    void setBaudrate(uint16_t baudrate);
    void setPower10dbm();
    void setPower5dbm();
    void setPower0dbm();
    void enableWhitening();
    void disableWhitening();
    void whitening(bool w);
    void setFrequency(uint32_t freq);
    void setSyncWord(uint8_t sync0, uint8_t sync1);
    void setSyncWord10(uint8_t sync1, uint8_t sync0);
    void setMaxPktSize(uint8_t size);
    
    // Status
    int16_t getRSSIdbm();
    uint8_t getLQI();
    bool crcok();
    uint8_t getState();
    
    // Wake on Radio
    void wor(uint16_t timeout = 1000);
    void wor2rx();
    
    // Printf-style packet sending
    bool printf(const char* fmt, ...);
    
    // Status bytes from last received packet
    uint8_t status[2];
    
    static const uint8_t BUFFER_SIZE = 64;

 protected:
    void waitMiso();
    void chipSelect();
    void chipDeselect();
    uint8_t spiTransfer(uint8_t data);
    void spiInit();
    
    uint8_t csn_pin_;
    uint8_t miso_pin_;
#ifdef USE_ESP32
    spi_device_handle_t spi_handle_;
#endif
    bool spi_initialized_;
};

}  // namespace cc1101
}  // namespace esphome

#endif
