/*
  xsns_36_i2cencoder.ino - I2C rotary encoder support.

  Copyright (C) 2018  FattoreSaimon, sammy2360 and Theo Arends

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef USE_I2C
#ifdef USE_I2CENCODER
/*********************************************************************************************\
 * I2C Encoder V2
 *
 * Source: https://github.com/Fattoresaimon/I2CEncoderV2
 *
 * I2C Address: 0x5A assumes ADDR connected to Gnd, Wake also must be grounded
\*********************************************************************************************/

#define XSNS_36             36

#include "i2cEncoderLibV2.h"
i2cEncoderLibV2 Encoder[4] = {
  i2cEncoderLibV2(0x01),
  i2cEncoderLibV2(0x02),
  i2cEncoderLibV2(0x03),
  i2cEncoderLibV2(0x04),
};


void i2cencoder_setup()
{
  constrain(NUMBER_OF_ENCODERS, 1, MAX_ENCODERS);

  for (int enc_cnt = 0; enc_cnt < NUMBER_OF_ENCODERS; enc_cnt++) {
      Encoder[enc_cnt].begin(INT_DATA | WRAP_DISABLE | DIRE_RIGHT | IPUP_ENABLE | RMOD_X1 | STD_ENCODER);
      Encoder[enc_cnt].writeCounter((int32_t)RtcSettings.encoder_counter[enc_cnt]); //Reset of the CVAL register
      Encoder[enc_cnt].writeMax((int32_t) Settings.encoder_max_limit); //SEt the maximum threshold to 5
      Encoder[enc_cnt].writeMin((int32_t) Settings.encoder_min_limit); //Set the minimum threshold to 0
      Encoder[enc_cnt].writeStep((int32_t) Settings.encoder_steps); //The step at every encoder click is 0.5
      Encoder[enc_cnt].writeInterruptConfig(0xff);
      Encoder[enc_cnt].writeAntibouncingPeriod(20); //250ms of debouncing
      Encoder[enc_cnt].writeDoublePushPeriod(50); //Set the double push period to 500ms
      Encoder[enc_cnt].updateStatus();
    }
}

/********************************************************************************************/


void i2cEncoderRead()  // Perform every n second
{
for (int i = 0; i < NUMBER_OF_ENCODERS; i++) {
    if ( Encoder[i].updateStatus()) {

    char json_event[120];

      if ( Encoder[i].readStatus(RINC)) {
        RtcSettings.encoder_counter[i] = Encoder[i].readCounterByte();
        Settings.light_dimmer = map(Encoder[i].readCounterByte(), Settings.encoder_min_limit, Settings.encoder_max_limit, 0, 100);
        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Dimmer\":{\"State\":%d}}"), Settings.light_dimmer);

          //      Settings.light_scheme = map(Encoder[i].readCounterByte(), Settings.encoder_min_limit, Settings.encoder_max_limit, 0, 10);
          //      snprintf_P(json_event, sizeof(json_event), PSTR("{\"Scheme\":{\"State\":%d}}"), Settings.light_scheme);
      }

      if ( Encoder[i].readStatus(RDEC)) {
        RtcSettings.encoder_counter[i] = Encoder[i].readCounterByte();
        Settings.light_dimmer = map(Encoder[i].readCounterByte(), Settings.encoder_min_limit, Settings.encoder_max_limit, 0, 100);
        snprintf_P(json_event, sizeof(json_event), PSTR("{\"Dimmer\":{\"State\":%d}}"), Settings.light_dimmer);

          //      Settings.light_scheme = map(Encoder[i].readCounterByte(), Settings.encoder_min_limit, Settings.encoder_max_limit, 0, 10);
          //      snprintf_P(json_event, sizeof(json_event), PSTR("{\"Scheme\":{\"State\":%d}}"), Settings.light_scheme);
      }

      if ( Encoder[i].readStatus(RMAX)) {
        RtcSettings.encoder_counter[i] = Encoder[i].readCounterByte();
      }

      if ( Encoder[i].readStatus(RMIN)) {
        RtcSettings.encoder_counter[i] = Encoder[i].readCounterByte();
      }

      if ( Encoder[i].readStatus(PUSHR)) {
          //  encoderButton();
      }

      if ( Encoder[i].readStatus(PUSHP)) {
      }

      if ( Encoder[i].readStatus(PUSHD)) {
          //  encoderDouble();
      }

    }
  }
}

  #ifdef USE_WEBSERVER
  const char HTTP_SNS_ENCODER[] PROGMEM =
    "%s{s}" D_ENCODER "%d{m}%s%s{e}";  // {s} = <tr><th>, {m} = </th><td>, {e} = </td></tr>
  #endif  // USE_WEBSERVER

  void EncoderCounterShow(boolean json)
  {
    char Estemp[10];
    char Ecounter[16];
    byte header = 0;


    for (byte i = 0; i < NUMBER_OF_ENCODERS; i++) {

          dtostrfd(RtcSettings.encoder_counter[i], 0, Ecounter);

        if (json) {
          if (!header) {
            snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s,\"ENCODER\":{"), mqtt_data);
            Estemp[0] = '\0';
          }
          header++;
          snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s%s\"E%d\":%s"), mqtt_data, Estemp, i +1, Ecounter);
          strlcpy(Estemp, ",", sizeof(Estemp));


  #ifdef USE_DOMOTICZ
          if ((0 == tele_period) && (0 == i)) {
            DomoticzSensor(DZ_COUNT, RtcSettings.encoder_counter[i]);
          }


  #endif  // USE_DOMOTICZ
  #ifdef USE_WEBSERVER
     }
      else {
            snprintf_P(mqtt_data, sizeof(mqtt_data), HTTP_SNS_ENCODER, mqtt_data, i +1, Ecounter, "");
  #endif  // USE_WEBSERVER
        }
      }


    if (json) {
      if (header) {
        snprintf_P(mqtt_data, sizeof(mqtt_data), PSTR("%s}"), mqtt_data);
      }
    }
  }

/*********************************************************************************************\
 * Interface
\*********************************************************************************************/

boolean Xsns36(byte function)
{
  boolean result = false;

  if (i2c_flg) {
    switch (function) {
      case FUNC_INIT:
        i2cencoder_setup();
        break;
      case FUNC_EVERY_250_MSECOND:
        i2cEncoderRead();
        break;
      case FUNC_JSON_APPEND:
        EncoderCounterShow(1);
        break;
#ifdef USE_WEBSERVER
      case FUNC_WEB_APPEND:
        EncoderCounterShow(0);
        break;
#endif  // USE_WEBSERVER
    }
  }
  return result;
}

#endif  // USE_CCS811
#endif  // USE_I2C
