/*
Based upon the elchouse CC1101 library
Licenced under MIT licence
Panagiotis Karagiannis <pkarsy@gmail.com>

Modified for ESPHome ESP-IDF compatibility (no Arduino dependency)
*/

#include "cc1101.h"

namespace esphome {
namespace cc1101 {

static const char *const TAG = "cc1101";

// ESP32 default VSPI pins
#define SPI_MOSI_PIN 23
#define SPI_MISO_PIN 19
#define SPI_CLK_PIN  18

CC1101::CC1101(uint8_t csn_pin, uint8_t miso_pin)
    : csn_pin_(csn_pin), miso_pin_(miso_pin), spi_initialized_(false) {
    status[0] = 0;
    status[1] = 0;
}

void CC1101::spiInit() {
#ifdef USE_ESP32
    if (spi_initialized_) return;
    
    // Configure SPI bus
    spi_bus_config_t buscfg = {};
    buscfg.mosi_io_num = SPI_MOSI_PIN;
    buscfg.miso_io_num = SPI_MISO_PIN;
    buscfg.sclk_io_num = SPI_CLK_PIN;
    buscfg.quadwp_io_num = -1;
    buscfg.quadhd_io_num = -1;
    buscfg.max_transfer_sz = 64;
    
    // Initialize the SPI bus
    esp_err_t ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus: %d", ret);
        return;
    }
    
    // Configure SPI device
    spi_device_interface_config_t devcfg = {};
    devcfg.clock_speed_hz = 1000000;  // 1 MHz - conservative for CC1101
    devcfg.mode = 0;                   // SPI mode 0
    devcfg.spics_io_num = -1;          // We handle CS manually
    devcfg.queue_size = 1;
    
    ret = spi_bus_add_device(SPI2_HOST, &devcfg, &spi_handle_);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to add SPI device: %d", ret);
        return;
    }
    
    // Configure CS pin as output
    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << csn_pin_);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
    gpio_set_level((gpio_num_t)csn_pin_, 1);  // CS high (deselected)
    
    // Configure MISO read pin as input
    io_conf.pin_bit_mask = (1ULL << miso_pin_);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    gpio_config(&io_conf);
    
    spi_initialized_ = true;
    ESP_LOGI(TAG, "SPI initialized - CS: GPIO%d, MISO_READ: GPIO%d", csn_pin_, miso_pin_);
#endif
}

uint8_t CC1101::spiTransfer(uint8_t data) {
#ifdef USE_ESP32
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &data;
    uint8_t rx_data = 0;
    t.rx_buffer = &rx_data;
    
    esp_err_t ret = spi_device_transmit(spi_handle_, &t);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI transfer failed: %d", ret);
        return 0;
    }
    return rx_data;
#else
    return 0;
#endif
}

void CC1101::waitMiso() {
#ifdef USE_ESP32
    // Wait for MISO to go low (CC1101 ready)
    int timeout = 1000;
    while (gpio_get_level((gpio_num_t)miso_pin_) && timeout > 0) {
        delayMicroseconds(1);
        timeout--;
    }
    if (timeout == 0) {
        ESP_LOGW(TAG, "waitMiso timeout");
    }
#endif
}

void CC1101::chipSelect() {
#ifdef USE_ESP32
    gpio_set_level((gpio_num_t)csn_pin_, 0);
#endif
}

void CC1101::chipDeselect() {
#ifdef USE_ESP32
    gpio_set_level((gpio_num_t)csn_pin_, 1);
#endif
}

void CC1101::writeRegister(uint8_t addr, uint8_t value) {
    chipSelect();
    waitMiso();
    spiTransfer(addr);
    spiTransfer(value);
    chipDeselect();
}

void CC1101::writeBurstRegister(uint8_t addr, const uint8_t *buffer, uint8_t num) {
    uint8_t temp = addr | WRITE_BURST;
    chipSelect();
    waitMiso();
    spiTransfer(temp);
    for (uint8_t i = 0; i < num; i++) {
        spiTransfer(buffer[i]);
    }
    chipDeselect();
}

uint8_t CC1101::strobe(uint8_t strobe_cmd) {
    chipSelect();
    waitMiso();
    uint8_t reply = spiTransfer(strobe_cmd);
    chipDeselect();
    return reply;
}

uint8_t CC1101::readRegister(uint8_t addr) {
    uint8_t temp = addr | READ_SINGLE;
    chipSelect();
    waitMiso();
    spiTransfer(temp);
    uint8_t value = spiTransfer(0);
    chipDeselect();
    return value;
}

void CC1101::readBurstRegister(uint8_t addr, uint8_t *buffer, uint8_t num) {
    uint8_t temp = addr | READ_BURST;
    chipSelect();
    waitMiso();
    spiTransfer(temp);
    for (uint8_t i = 0; i < num; i++) {
        buffer[i] = spiTransfer(0);
    }
    chipDeselect();
}

uint8_t CC1101::readStatusRegister(uint8_t addr) {
    uint8_t temp = addr | READ_BURST;
    chipSelect();
    waitMiso();
    spiTransfer(temp);
    uint8_t value = spiTransfer(0);
    chipDeselect();
    return value;
}

void CC1101::setCommonRegisters() {
    setIDLEstate();
    writeRegister(CC1101_IOCFG0, 0x01);
    writeRegister(CC1101_FIFOTHR, 0x4F);
    writeRegister(CC1101_MDMCFG3, 0x83);
    writeRegister(CC1101_MCSM0, 0x18);
    writeRegister(CC1101_FOCCFG, 0x16);
    writeRegister(CC1101_AGCCTRL2, 0x43);
    writeRegister(CC1101_WORCTRL, 0xFB);
    writeRegister(CC1101_FSCAL3, 0xE9);
    writeRegister(CC1101_FSCAL2, 0x2A);
    writeRegister(CC1101_FSCAL1, 0x00);
    writeRegister(CC1101_FSCAL0, 0x1F);
    writeRegister(CC1101_TEST2, 0x81);
    writeRegister(CC1101_TEST1, 0x35);
    writeRegister(CC1101_TEST0, 0x09);
    writeRegister(CC1101_PKTLEN, MAX_PACKET_LEN);
    writeRegister(CC1101_MCSM1, 0x30);
}

void CC1101::reset() {
    chipDeselect();
    delayMicroseconds(50);
    chipSelect();
    delayMicroseconds(50);
    chipDeselect();
    delayMicroseconds(50);
    chipSelect();
    waitMiso();
    spiTransfer(CC1101_SRES);
    waitMiso();
    chipDeselect();
}

void CC1101::begin(uint32_t freq) {
    spiInit();
    reset();
    setCommonRegisters();
    enableWhitening();
    setFrequency(freq);
    setBaudrate4800bps();
    optimizeSensitivity();
    setPower10dbm();
    disableAddressCheck();
}

bool CC1101::sendPacketSlowMCU(const uint8_t *txBuffer, uint8_t size) {
    if (txBuffer == nullptr || size == 0) {
        ESP_LOGW(TAG, "sendPacket called with wrong arguments");
        return false;
    }
    if (size > MAX_PACKET_LEN) {
        ESP_LOGW(TAG, "Warning, packet truncated to max packet length");
        size = MAX_PACKET_LEN;
    }
    uint8_t txbytes = readStatusRegister(CC1101_TXBYTES);
    if (txbytes != 0 || getState() != 1) {
        setIDLEstate();
        strobe(CC1101_SFTX);
        strobe(CC1101_SFRX);
        setRXstate();
    }
    writeRegister(CC1101_TXFIFO, size);
    writeBurstRegister(CC1101_TXFIFO, txBuffer, size);
    delayMicroseconds(500);
    strobe(CC1101_STX);
    uint8_t state = getState();
    if (state == 1) {
        return false;
    } else {
        while (1) {
            state = getState();
            if (state == 0) break;
        }
    }
    setIDLEstate();
    strobe(CC1101_SFTX);
    setRXstate();
    return true;
}

bool CC1101::sendPacket(const char* msg) {
    size_t msglen = strlen(msg);
    return sendPacket((const uint8_t*)msg, (uint8_t)msglen);
}

void CC1101::setRXstate() {
    while (1) {
        uint8_t state = getState();
        if (state == 0b001) break;
        else if (state == 0b110) strobe(CC1101_SFRX);
        else if (state == 0b111) strobe(CC1101_SFTX);
        strobe(CC1101_SRX);
    }
}

uint8_t CC1101::getPacket(uint8_t *rxBuffer) {
    uint8_t state = getState();
    if (state == 1) {
        return 0;
    }
    uint8_t rxbytes = readStatusRegister(CC1101_RXBYTES);
    rxbytes = rxbytes & BYTES_IN_RXFIFO;
    uint8_t size = 0;
    if (rxbytes) {
        size = readRegister(CC1101_RXFIFO);
        if (size > 0 && size <= MAX_PACKET_LEN) {
            if ((size + 3) <= rxbytes) {
                readBurstRegister(CC1101_RXFIFO, rxBuffer, size);
                readBurstRegister(CC1101_RXFIFO, status, 2);
            } else {
                size = 0;
            }
        } else {
            ESP_LOGW(TAG, "Wrong rx size=%d", size);
            size = 0;
        }
    }
    setIDLEstate();
    strobe(CC1101_SFRX);
    setRXstate();
    if (size == 0) memset(status, 0, 2);
    return size;
}

void CC1101::optimizeSensitivity() {
    setIDLEstate();
    writeRegister(CC1101_FSCTRL1, 0x06);
    writeRegister(CC1101_MDMCFG2, 0x13);
    setRXstate();
}

void CC1101::optimizeCurrent() {
    setIDLEstate();
    writeRegister(CC1101_FSCTRL1, 0x08);
    writeRegister(CC1101_MDMCFG2, 0x93);
}

void CC1101::disableAddressCheck() {
    setIDLEstate();
    writeRegister(CC1101_PKTCTRL1, 4 + 0);
}

void CC1101::enableAddressCheck(uint8_t addr) {
    setIDLEstate();
    writeRegister(CC1101_ADDR, addr);
    writeRegister(CC1101_PKTCTRL1, 4 + 1);
}

void CC1101::enableAddressCheckBcast(uint8_t addr) {
    setIDLEstate();
    writeRegister(CC1101_ADDR, addr);
    writeRegister(CC1101_PKTCTRL1, 4 + 2);
}

void CC1101::setBaudrate4800bps() {
    setIDLEstate();
    writeRegister(CC1101_MDMCFG4, 0xC7);
    writeRegister(CC1101_DEVIATN, 0x40);
}

void CC1101::setBaudrate38000bps() {
    setIDLEstate();
    writeRegister(CC1101_MDMCFG4, 0xCA);
    writeRegister(CC1101_DEVIATN, 0x35);
}

void CC1101::setBaudrate(uint16_t baudrate) {
    if (baudrate >= 10000) setBaudrate38000bps();
    else setBaudrate4800bps();
}

void CC1101::setPower10dbm() {
    writeRegister(CC1101_PATABLE, 0xC5);
}

void CC1101::setPower5dbm() {
    writeRegister(CC1101_PATABLE, 0x86);
}

void CC1101::setPower0dbm() {
    writeRegister(CC1101_PATABLE, 0x50);
}

int16_t CC1101::getRSSIdbm() {
    uint8_t rssi_dec = status[0];
    int16_t rssi_dBm;
    const int16_t rssi_offset = 74;
    if (rssi_dec >= 128) {
        rssi_dBm = (int16_t)((int16_t)(rssi_dec - 256) / 2) - rssi_offset;
    } else {
        rssi_dBm = (rssi_dec / 2) - rssi_offset;
    }
    return rssi_dBm;
}

bool CC1101::crcok() {
    return status[1] >> 7;
}

uint8_t CC1101::getLQI() {
    return status[1] & 0b01111111;
}

void CC1101::setIDLEstate() {
    strobe(CC1101_SIDLE);
    while (getState() != 0);
}

bool CC1101::printf(const char* fmt, ...) {
    uint8_t pkt[MAX_PACKET_LEN + 1];
    va_list args;
    va_start(args, fmt);
    uint8_t length = vsnprintf((char*)pkt, MAX_PACKET_LEN + 1, fmt, args);
    va_end(args);
    if (length > MAX_PACKET_LEN) length = MAX_PACKET_LEN;
    return sendPacket(pkt, length);
}

void CC1101::setPowerDownState() {
    setIDLEstate();
    strobe(CC1101_SFRX);
    strobe(CC1101_SFTX);
    strobe(CC1101_SPWD);
}

void CC1101::enableWhitening() {
    setIDLEstate();
    writeRegister(CC1101_PKTCTRL0, 0x45);
}

void CC1101::disableWhitening() {
    setIDLEstate();
    writeRegister(CC1101_PKTCTRL0, 0x05);
}

void CC1101::whitening(bool w) {
    if (w) enableWhitening();
    else disableWhitening();
}

uint8_t CC1101::getState() {
    uint8_t old_state = strobe(CC1101_SNOP);
    while (1) {
        uint8_t state = strobe(CC1101_SNOP);
        if (state == old_state) {
            return (state >> 4) & 0b00111;
        }
        old_state = state;
    }
}

void CC1101::setFrequency(uint32_t freq) {
    uint32_t reg_freq = ((uint64_t)freq << 16) / CC1101_CRYSTAL_FREQUENCY;
    uint8_t FREQ2 = (reg_freq >> 16) & 0xFF;
    uint8_t FREQ1 = (reg_freq >> 8) & 0xFF;
    uint8_t FREQ0 = reg_freq & 0xFF;
    setIDLEstate();
    writeRegister(CC1101_CHANNR, 0);
    writeRegister(CC1101_FREQ2, FREQ2);
    writeRegister(CC1101_FREQ1, FREQ1);
    writeRegister(CC1101_FREQ0, FREQ0);
}

void CC1101::setSyncWord(uint8_t sync0, uint8_t sync1) {
    setIDLEstate();
    writeRegister(CC1101_SYNC0, sync0);
    writeRegister(CC1101_SYNC1, sync1);
}

void CC1101::setSyncWord10(uint8_t sync1, uint8_t sync0) {
    setIDLEstate();
    writeRegister(CC1101_SYNC1, sync1);
    writeRegister(CC1101_SYNC0, sync0);
}

void CC1101::setMaxPktSize(uint8_t size) {
    setIDLEstate();
    if (size < 1) size = 1;
    if (size > MAX_PACKET_LEN) size = MAX_PACKET_LEN;
    writeRegister(CC1101_PKTLEN, size);
}

void CC1101::wor(uint16_t timeout) {
    if (timeout < 15) timeout = 15;
    constexpr const uint16_t maxtimeout = 750ul * 0xffff / (CC1101_CRYSTAL_FREQUENCY / 1000);
    if (timeout > maxtimeout) timeout = maxtimeout;
    
    writeRegister(CC1101_WORCTRL, 0x78);
    writeRegister(CC1101_MCSM2, 0b11000 + 6);
    writeRegister(CC1101_MCSM0, 0x38);
    
    uint16_t evt01 = timeout * (CC1101_CRYSTAL_FREQUENCY / 1000) / 750;
    writeRegister(CC1101_WOREVT0, evt01 & 0xff);
    writeRegister(CC1101_WOREVT1, evt01 >> 8);
    strobe(CC1101_SWOR);
}

void CC1101::wor2rx() {
    writeRegister(CC1101_WORCTRL, 0xFB);
    writeRegister(CC1101_MCSM2, 0x07);
    writeRegister(CC1101_MCSM0, 0x18);
    writeRegister(CC1101_WOREVT0, 0x6B);
    writeRegister(CC1101_WOREVT1, 0x87);
}

bool CC1101::sendPacket(const uint8_t *txBuffer, uint8_t size, uint32_t duration) {
    if (txBuffer == nullptr || size == 0) {
        ESP_LOGW(TAG, "sendPacket called with wrong arguments");
        return false;
    }
    if (size > MAX_PACKET_LEN) {
        ESP_LOGW(TAG, "Warning, packet truncated");
        size = MAX_PACKET_LEN;
    }
    uint8_t txbytes = readStatusRegister(CC1101_TXBYTES);
    if (txbytes != 0 || getState() != 1) {
        setIDLEstate();
        strobe(CC1101_SFTX);
        strobe(CC1101_SFRX);
        setRXstate();
    }
    delayMicroseconds(500);
    strobe(CC1101_STX);
    uint8_t state = getState();
    if (state == 1) {
        return false;
    } else {
        uint32_t t = millis();
        while (millis() - t < duration) {}
        writeRegister(CC1101_TXFIFO, size);
        writeBurstRegister(CC1101_TXFIFO, txBuffer, size);
        delayMicroseconds(500);
        while (1) {
            state = getState();
            if (state == 0) break;
        }
    }
    setIDLEstate();
    strobe(CC1101_SFTX);
    setRXstate();
    return true;
}

}  // namespace cc1101
}  // namespace esphome
