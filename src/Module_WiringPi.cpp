#include "Module.h"

#include <ctype.h>
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
void hexDumpBuf(unsigned char *buf, int size)
{
  int row;
  int col;
  int j;
  const int cols = 16;
  char line[255];
  char newl[255];
  
  for (row = 0; row <= (size / cols); row++) {
    bzero(line,255);
    line[0] = 0;
    snprintf(line, 255, "%.4x  ", row * cols);
    for (col = 0; col < cols; col++) {
      if (row * cols + col >= size)
        break;
      snprintf(newl, 255, "%s %.2x", line, buf[(row * cols) + col]);
      strncpy(line,newl,255);
    }
    if (col < cols) {           //line up the ascii decoded out for fractional line
      for (j = 0; j < (cols - col); j++) {
        snprintf(newl, 255, "%s   ", line);
        strncpy(line,newl,255);
      }
    }
    snprintf(newl, 255, "%s   ", line);
    strncpy(line,newl,255);
    for (col = 0; col < cols; col++) {
      if (row * cols + col >= size)
        break;
      if (buf[(row * cols) + col] == ' '
          || isgraph(buf[(row * cols) + col])) {
        snprintf(newl, 255, "%s%c", line, buf[(row * cols) + col]);
        strncpy(line,newl,255);
      } else {
        snprintf(newl, 255, "%s%c", line, '.');
        strncpy(line,newl,255);
      }
    }
    printf("%s\r\n", line);
  }
  printf("\r\n");
}
#pragma GCC diagnostic pop


Module::Module(RADIOLIB_PIN_TYPE cs, RADIOLIB_PIN_TYPE irq, RADIOLIB_PIN_TYPE rst, RADIOLIB_PIN_TYPE gpio):
  _cs{cs},
  _irq{irq},
  _rst{rst},
  _gpio{gpio}
{
  _initInterface = true;

  // this is Arduino-like build, pre-set callbacks
  setCb_pinMode(::pinMode);
  setCb_digitalRead(::digitalRead);
  setCb_digitalWrite(::digitalWrite);
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  setCb_tone(::tone);
  setCb_noTone(::noTone);
  #endif
  //setCb_attachInterrupt(::attachInterrupt);
  //setCb_detachInterrupt(::detachInterrupt);
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  setCb_yield(::sched_yield);
  #endif
  setCb_delay(::delay);
  setCb_delayMicroseconds(::delayMicroseconds);
  setCb_millis(::millis);
  setCb_micros(::micros);
  //GPIO7 is CE1, GPIO8 is CE0
  if ( _cs == 7 || _cs == 8)  {
    _spiSettings.IPcontrolsCE=1;
    _spiSettings.channel = (_cs==7 )? 1 : 0;
  } else {
    _spiSettings.gpioCE = _cs;
    _spiSettings.IPcontrolsCE = 0;
    _spiSettings.channel = 0; //the IP is going to assert GPIO8 anyway...
  }
}

Module::Module(const Module& mod) {
  *this = mod;
}

Module& Module::operator=(const Module& mod) {
  this->SPIreadCommand = mod.SPIreadCommand;
  this->SPIwriteCommand = mod.SPIwriteCommand;
  this->_cs = mod.getCs();
  this->_irq = mod.getIrq();
  this->_rst = mod.getRst();
  this->_gpio = mod.getGpio();

  return(*this);
}

void Module::init() {
  if(_initInterface) {
    this->SPIbegin();
  }
}

void Module::term() {
  // stop hardware interfaces (if they were initialized by the library)
  if(!_initInterface) {
    return;
  }

  if(_spi != nullptr) {
    this->SPIend();
  }
}

int16_t Module::SPIgetRegValue(uint8_t reg, uint8_t msb, uint8_t lsb) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t rawValue = SPIreadRegister(reg);
  uint8_t maskedValue = rawValue & ((0b11111111 << lsb) & (0b11111111 >> (7 - msb)));
  return(maskedValue);
}

int16_t Module::SPIsetRegValue(uint8_t reg, uint8_t value, uint8_t msb, uint8_t lsb, uint8_t checkInterval, uint8_t checkMask) {
  if((msb > 7) || (lsb > 7) || (lsb > msb)) {
    return(RADIOLIB_ERR_INVALID_BIT_RANGE);
  }

  uint8_t currentValue = SPIreadRegister(reg);
  uint8_t mask = ~((0b11111111 << (msb + 1)) | (0b11111111 >> (8 - lsb)));
  uint8_t newValue = (currentValue & ~mask) | (value & mask);
  SPIwriteRegister(reg, newValue);

  #if defined(RADIOLIB_SPI_PARANOID)
    // check register value each millisecond until check interval is reached
    // some registers need a bit of time to process the change (e.g. SX127X_REG_OP_MODE)
    uint32_t start = this->micros();
    uint8_t readValue = 0x00;
    while(this->micros() - start < (checkInterval * 1000)) {
      readValue = SPIreadRegister(reg);
      if((readValue & checkMask) == (newValue & checkMask)) {
        // check passed, we can stop the loop
        return(RADIOLIB_ERR_NONE);
      }
    }

    // check failed, print debug info
    RADIOLIB_DEBUG_PRINTLN(F(""));
    RADIOLIB_DEBUG_PRINT(F("address:\t0x"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",reg);
    RADIOLIB_DEBUG_PRINT(F("bits:\t\t"));
    RADIOLIB_DEBUG_PRINT("0x%x",msb);
    RADIOLIB_DEBUG_PRINT(" ");
    RADIOLIB_DEBUG_PRINTLN("0x%x",lsb);
    RADIOLIB_DEBUG_PRINT(F("value:\t\t0b"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",value);
    RADIOLIB_DEBUG_PRINT(F("current:\t0b"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",currentValue);
    RADIOLIB_DEBUG_PRINT(F("mask:\t\t0b"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",mask);
    RADIOLIB_DEBUG_PRINT(F("new:\t\t0b"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",newValue);
    RADIOLIB_DEBUG_PRINT(F("read:\t\t0b"));
    RADIOLIB_DEBUG_PRINTLN("0x%x",readValue);
    RADIOLIB_DEBUG_PRINTLN(F(""));

    return(RADIOLIB_ERR_SPI_WRITE_FAILED);
  #else
    return(RADIOLIB_ERR_NONE);
  #endif
}

void Module::SPIreadRegisterBurst(uint8_t reg, uint8_t numBytes, uint8_t* inBytes) {
  SPItransfer(SPIreadCommand, reg, NULL, inBytes, numBytes);
}

uint8_t Module::SPIreadRegister(uint8_t reg) {
  uint8_t resp = 0;
  SPItransfer(SPIreadCommand, reg, NULL, &resp, 1);
  return(resp);
}

void Module::SPIwriteRegisterBurst(uint8_t reg, uint8_t* data, uint8_t numBytes) {
  SPItransfer(SPIwriteCommand, reg, data, NULL, numBytes);
}

void Module::SPIwriteRegister(uint8_t reg, uint8_t data) {
  SPItransfer(SPIwriteCommand, reg, &data, NULL, 1);
}

#define MAXIOBUF 1024
void Module::SPItransfer(uint8_t cmd, uint8_t reg, uint8_t* dataOut, uint8_t* dataIn, uint8_t numBytes) {
  uint8_t iobuf[MAXIOBUF];
  // start SPI transaction
  this->SPIbeginTransaction();

  // pull CS low
  if (!_spiSettings.IPcontrolsCE) {
    this->pinMode(_cs, OUTPUT);
    this->digitalWrite(_cs, LOW);
  }
  // send SPI register address with access command
  //this->SPItransfer(reg | cmd);
  if (cmd == SPIwriteCommand) {
    memcpy(iobuf+1, dataOut, numBytes);
  } else { 
    memset(iobuf, 0, numBytes+1);
  }
  iobuf[0]=reg | cmd;
  
  #if defined(RADIOLIB_VERBOSE)
    if(cmd == SPIwriteCommand) {
      RADIOLIB_VERBOSE_PRINT("W");
    } else if(cmd == SPIreadCommand) {
      RADIOLIB_VERBOSE_PRINT("R");
    }
    RADIOLIB_VERBOSE_PRINT("\t");
  #endif

  // send data or get response
  if(cmd == SPIwriteCommand) {
    if(dataOut != NULL) {
      RADIOLIB_VERBOSE_DUMP(iobuf,numBytes+1);
      _spi->transfer(iobuf,numBytes+1);
    }
  } else if (cmd == SPIreadCommand) {
    if(dataIn != NULL) {
      _spi->transfer(iobuf,numBytes+1);
      RADIOLIB_VERBOSE_DUMP(iobuf,numBytes+1);
      memcpy(dataIn, iobuf+1, numBytes);
    }
  }

  // release CS
  if (!_spiSettings.IPcontrolsCE) {
    this->digitalWrite(_cs, HIGH);
  }
  // end SPI transaction
  this->SPIendTransaction();
}

void Module::pinMode(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_MODE mode) {
  if((pin == RADIOLIB_NC) || (cb_pinMode == nullptr)) {
    return;
  }
  cb_pinMode(pin, mode);
}

void Module::digitalWrite(RADIOLIB_PIN_TYPE pin, RADIOLIB_PIN_STATUS value) {
  if((pin == RADIOLIB_NC) || (cb_digitalWrite == nullptr)) {
    return;
  }
  cb_digitalWrite(pin, value);
}

RADIOLIB_PIN_STATUS Module::digitalRead(RADIOLIB_PIN_TYPE pin) {
  if((pin == RADIOLIB_NC) || (cb_digitalRead == nullptr)) {
    return((RADIOLIB_PIN_STATUS)0);
  }
  return(cb_digitalRead(pin));
}

#if 0
void Module::tone(RADIOLIB_PIN_TYPE pin, uint16_t value, uint32_t duration) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  if((pin == RADIOLIB_NC) || (cb_tone == nullptr)) {
    return;
  }
  cb_tone(pin, value, duration);
  #else
  if(pin == RADIOLIB_NC) {
    return;
  }
    #if defined(ESP32)
      // ESP32 tone() emulation
      (void)duration;
      ledcAttachPin(pin, RADIOLIB_TONE_ESP32_CHANNEL);
      ledcWriteTone(RADIOLIB_TONE_ESP32_CHANNEL, value);
    #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
      // better tone for mbed OS boards
      (void)duration;
      if(!pwmPin) {
        pwmPin = new mbed::PwmOut(digitalPinToPinName(pin));
      }
      pwmPin->period(1.0 / value);
      pwmPin->write(0.5);
    #else
      (void)value;
      (void)duration;
    #endif
  #endif
}

void Module::noTone(RADIOLIB_PIN_TYPE pin) {
  #if !defined(RADIOLIB_TONE_UNSUPPORTED)
  if((pin == RADIOLIB_NC) || (cb_noTone == nullptr)) {
    return;
  }
  #if defined(ARDUINO_ARCH_STM32)
  cb_noTone(pin, false);
  #else
  cb_noTone(pin);
  #endif
  #else
  if(pin == RADIOLIB_NC) {
    return;
  }
    #if defined(ESP32)
      // ESP32 tone() emulation
      ledcDetachPin(pin);
      ledcWrite(RADIOLIB_TONE_ESP32_CHANNEL, 0);
    #elif defined(RADIOLIB_MBED_TONE_OVERRIDE)
      // better tone for mbed OS boards
      (void)pin;
      pwmPin->suspend();
    #endif
  #endif
}
#endif

void Module::attachInterrupt(RADIOLIB_PIN_TYPE interruptNum, void (*userFunc)(void), RADIOLIB_INTERRUPT_STATUS mode) {
  if((interruptNum == RADIOLIB_NC) || (cb_attachInterrupt == nullptr)) {
    return;
  }
  //Here interruptNum is a pin, mode is INT_EDGE_RISING, INT_EDGE_FALLING or INT_EDGE_BOTH
  //This launches a pthread to watch the line
  wiringPiISR(interruptNum, mode, userFunc);
}

void Module::detachInterrupt(RADIOLIB_PIN_TYPE interruptNum) {
  if((interruptNum == RADIOLIB_NC) || (cb_detachInterrupt == nullptr)) {
    return;
  }
  //Don't know how to do this
}

void Module::yield() {
  if(cb_yield == nullptr) {
    return;
  }
  #if !defined(RADIOLIB_YIELD_UNSUPPORTED)
  cb_yield();
  #endif
}

void Module::delay(uint32_t ms) {
  if(cb_delay == nullptr) {
    return;
  }
  cb_delay(ms);
}

void Module::delayMicroseconds(uint32_t us) {
  if(cb_delayMicroseconds == nullptr) {
    return;
  }
  cb_delayMicroseconds(us);
}

uint32_t Module::millis() {
  if(cb_millis == nullptr) {
    return(0);
  }
  return(cb_millis());
}

uint32_t Module::micros() {
  if(cb_micros == nullptr) {
    return(0);
  }
  return(cb_micros());
}

void Module::begin() {
  this->SPIbegin();
}

void Module::beginTransaction() {
  this->SPIbeginTransaction();
}

uint8_t Module::transfer(uint8_t b) {
  return(this->SPItransfer(b));
}

void Module::endTransaction() {
  this->SPIendTransaction();
}

void Module::end() {
  this->SPIend();
}

void Module::SPIbegin() {
  _spi->begin();
}

void Module::SPIbeginTransaction() {
  _spi->beginTransaction(_spiSettings);
}

uint8_t Module::SPItransfer(uint8_t b) {
  return(_spi->transfer(b));
}

void Module::SPIendTransaction() {
  _spi->endTransaction();
}

void Module::SPIend() {
  _spi->end();
}


uint8_t Module::flipBits(uint8_t b) {
  b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
  b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
  b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
  return b;
}

uint16_t Module::flipBits16(uint16_t i) {
  i = (i & 0xFF00) >> 8 | (i & 0x00FF) << 8;
  i = (i & 0xF0F0) >> 4 | (i & 0x0F0F) << 4;
  i = (i & 0xCCCC) >> 2 | (i & 0x3333) << 2;
  i = (i & 0xAAAA) >> 1 | (i & 0x5555) << 1;
  return i;
}

void Module::setRfSwitchPins(RADIOLIB_PIN_TYPE rxEn, RADIOLIB_PIN_TYPE txEn) {
  _useRfSwitch = true;
  _rxEn = rxEn;
  _txEn = txEn;
  this->pinMode(rxEn, OUTPUT);
  this->pinMode(txEn, OUTPUT);
}

void Module::setRfSwitchState(RADIOLIB_PIN_STATUS rxPinState, RADIOLIB_PIN_STATUS txPinState) {
  // check RF switch control is enabled
  if(!_useRfSwitch) {
    return;
  }

  // set pins
  this->digitalWrite(_rxEn, rxPinState);
  this->digitalWrite(_txEn, txPinState);
}
