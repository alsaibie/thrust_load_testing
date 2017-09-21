/****************************************************************************
 *
 *   Copyright (c) 2017 Ali AlSaibie. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

/**
 * @file 
 * 
 *
 * @author Ali AlSaibie
 */
#include "arduino_interface.h"
arduino_interface::arduino_interface() {

  if(!configure_serial()){
    exit(1);
  }

}

bool arduino_interface::configure_serial() {
  // Instantiate the SerialStream object then open the serial port.
  serial_stream.Open("/dev/ttyACM0");

  if (!serial_stream.good()) {
    std::cerr << "[" << __FILE__ << ":" << __LINE__ << "] "
              << "Error: Could not open serial port."
              << std::endl;
    return false;
  }

  // Set the baud rate of the serial port.
  serial_stream.SetBaudRate(SerialStreamBuf::BAUD_115200);

  if (!serial_stream.good()) {
    std::cerr << "Error: Could not set the baud rate." << std::endl;
    return false;
  }

  // Set the number of data bits.
  serial_stream.SetCharSize(SerialStreamBuf::CHAR_SIZE_8);

  if (!serial_stream.good()) {
    std::cerr << "Error: Could not set the character size." << std::endl;
    return false;
  }

  // Disable parity.
  serial_stream.SetParity(SerialStreamBuf::PARITY_NONE);

  if (!serial_stream.good()) {
    std::cerr << "Error: Could not disable the parity." << std::endl;
    return false;
  }

  // Set the number of stop bits.
  serial_stream.SetNumOfStopBits(1);

  if (!serial_stream.good()) {
    std::cerr << "Error: Could not set the number of stop bits."
              << std::endl;
    return false;
  }

  // Turn off hardware flow control.
  serial_stream.SetFlowControl(SerialStreamBuf::FLOW_CONTROL_NONE);
  if (!serial_stream.good()) {
    std::cerr << "Error: Could not use hardware flow control."
              << std::endl;
    return false;
  }

  std::cout<<"Serial Port Opened Successfully. Mabrook! " << std::endl;
  return true;
}

arduino_interface::~arduino_interface() {
  serial_stream.Close();
}

int arduino_interface::send_command(arduino_interface::COMMANDS com) {
  char c[2] = {33,0};
  switch (com){
    case P:
      c[1] = 1;
      serial_stream.write(c, 2);
      break;
    case S:
      c[1] = 2;
      serial_stream.write(c, 2);
      break;
  }
  return 0;
}

int arduino_interface::receive_data(char * data_buffer) {
  while (serial_stream.rdbuf()->in_avail() > 0) {
    serial_stream.get(*data_buffer);
    std::cerr << static_cast<int>( *data_buffer ) << " ";
    data_buffer++;
    usleep(100);
  }
  return 0;
}

int arduino_interface::receive_string(std::string &string) {

  if(serial_stream.rdbuf()->in_avail() > 0){
    string = readStringUntil('\n');
    return 1;
  }
  else{
    return -1;
  }

}

/* The following two function are adopted from arduino's library: Stream.cpp */
std::string arduino_interface::readStringUntil(char terminator) {
  std::string ret;
  int c = timedRead();
  while (c >= 0 && c != terminator)
  {
    ret += (char)c;
    c = timedRead();
  }
  return ret;
}

int arduino_interface::timedRead() {
  int c;
  int time_out_seconds = 1;
  std::time_t t1 = std::time(0);
  std::time_t t2;
  do {
    char character;
    serial_stream.get(character);
    c = (int)character;
    if (c >= 0) return c;
    t2 = std::time(0);
  } while(t2 - t1 > time_out_seconds);
  return -1;     // -1 indicates timeout
}

int arduino_interface::send_string(std::string &string) {
//  serial_stream.write(string.c_str(), string.length());
  serial_stream << string << "\n";
  return 1;
}


