#ifndef PACKET_HPP
#define PACKET_HPP

#define CHECK_CODE 250130

struct S_Ble_Header_Packet
{
  uint32_t checkCode;
  uint8_t cmd;
  uint8_t parm[3];
};

struct S_Ble_Packet_About
{
  S_Ble_Header_Packet header;
  uint64_t chipId;
  u_int8_t version[3];
  u_int8_t chennelNum;
  uint32_t sampleRate;
};

struct S_Ble_Packet_Data
{
  S_Ble_Header_Packet header; //cmd 0x09
  uint32_t data[8]; //max 8 channel
};

#endif