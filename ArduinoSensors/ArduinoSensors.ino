#include "Adafruit_BMP085_U.h"
#include "Adafruit_L3GD20.h"
#include "Adafruit_LSM303_U.h"

#include "llc.h"

// Attach a new CmdMessenger object to the default Serial port
CmdMessenger cmdMessenger (Serial, '^', ';', '/');

Adafruit_BMP085_Unified   bmpSensor(0);
Adafruit_L3GD20           gyroSensor;
Adafruit_LSM303_Accel_Unified   accSensor;
Adafruit_LSM303_Mag_Unified   magSensor;
Stream *gps = NULL;                    // Serial data stream

bool bmpSensorEnabled = false;
bool gyroSensorEnabled = false;
bool accSensorEnabled = false;
bool magSensorEnabled = false;

char    gps_line[2][120];
int     current_gps_line = 0, last_gps_line = 1, gps_index = 0;

uint8_t function2chan[num_functions];

char    output_line[120];

sChannel    channels[MAX_CHANNELS];

void Onsetup_digital_sensor()
{
  int16_t   chan, pin, pullup;
  int32_t   period;
  chan = cmdMessenger.readInt16Arg();
  if ((chan >= NELEMENTS(channels)) || (chan < 0))
  {
    cmdMessenger.sendCmd(nack, "Digital Invalid channel");
  } else
  {
    pin = cmdMessenger.readInt16Arg();
    period = cmdMessenger.readInt32Arg();
    pullup = cmdMessenger.readInt16Arg();
    channels[chan].pin = pin;
    channels[chan].period = period;
    channels[chan].function = 'd';
    channels[chan].next_time = millis() + period;
    if (pullup)
    {
        pinMode (pin, INPUT_PULLUP);
    } else
    {
        pinMode (pin, INPUT);
    }
  }
}

void Onsetup_analog_sensor()
{
  int16_t   chan, pin;
  int32_t   period;
  chan = cmdMessenger.readInt16Arg();
  if ((chan >= NELEMENTS(channels)) || (chan < 0))
  {
    cmdMessenger.sendCmd(nack, "Analog Invalid channel");
  } else
  {
    pin = cmdMessenger.readInt16Arg();
    period = cmdMessenger.readInt32Arg();
    channels[chan].function = 'n';
    channels[chan].pin = pin;
    channels[chan].period = period;
    channels[chan].filter_coefficient[0] = cmdMessenger.readFloatArg();
    channels[chan].filter_coefficient[1] = cmdMessenger.readFloatArg();
    channels[chan].rejection_band = cmdMessenger.readFloatArg();
    channels[chan].secondary_band = cmdMessenger.readFloatArg();
    channels[chan].secondary_filter_duration = (unsigned)cmdMessenger.readInt32Arg();
    channels[chan].secondary_filter_count = 0;
    channels[chan].sample_count = 0;
    channels[chan].reject_count = 0;
    channels[chan].secondary_use_count = 0;
    channels[chan].next_time = millis() + period;
  }
}

void Onsetup_i2c_sensor()
{
  int16_t   chan;
  char      function;
  int32_t   period;
  float     pressure, temperature;
  sensors_event_t   event;
  int       retry;

  chan = cmdMessenger.readInt16Arg();
  if ((chan >= NELEMENTS(channels)) || (chan < 0))
  {
    cmdMessenger.sendCmd(nack, "I2C Invalid channel");
  } else
  {
      bool good = true;
      function = cmdMessenger.readCharArg();
      period = cmdMessenger.readInt32Arg();
      channels[chan].function = function;
      channels[chan].period = period;
      channels[chan].filter_coefficient[0] = cmdMessenger.readFloatArg();
      channels[chan].filter_coefficient[1] = cmdMessenger.readFloatArg();
      channels[chan].secondary_band = cmdMessenger.readFloatArg();
      channels[chan].rejection_band = cmdMessenger.readFloatArg();
      channels[chan].secondary_filter_duration = (unsigned)cmdMessenger.readInt32Arg();
      channels[chan].secondary_filter_count = 0;
      channels[chan].sample_count = 0;
      channels[chan].reject_count = 0;
      channels[chan].secondary_use_count = 0;
      channels[chan].next_time = millis() + period;
      switch (function)
      {
        case 'a':
          if (accSensorEnabled)
          {
              for (retry = 10; retry != 0; retry--)
              {
                  if (accSensor.getEvent(&event)) break;
                  delayMicroseconds(500);
                  accSensor.begin();
              }
              if (retry > 0)
              {
                  channels[chan].state[0] = event.orientation.x;
                  channels[chan].state[1] = event.orientation.y;
                  channels[chan].state[2] = event.orientation.z;
                  function2chan[accel_function] = chan;
              } else
              {
                cmdLog (99, "Failed to initialize acc");
              }
          }
          break;
        case 'r':
          if (gyroSensorEnabled)
          {
              for (retry = 10; retry != 0; retry--)
              {
                  if (gyroSensor.read()) break;
                  delayMicroseconds(500);
                  gyroSensor.begin();
              }
              if (retry > 0)
              {
                  channels[chan].state[0] = gyroSensor.data.x;
                  channels[chan].state[1] = gyroSensor.data.y;
                  channels[chan].state[2] = gyroSensor.data.z;
                  function2chan[rotation_function] = chan;
              } else
              {
                cmdLog (99, "Failed to initialize gyro");
              }
          }
          break;
        case 'm':
          if (magSensorEnabled)
          {
              for (retry = 10; retry != 0; retry--)
              {
                  if (magSensor.getEvent(&event)) break;
                  delayMicroseconds(500);
                  magSensor.begin();
              }
              if (retry > 0)
              {
                  channels[chan].state[0] = event.magnetic.x;
                  channels[chan].state[1] = event.magnetic.y;
                  channels[chan].state[2] = event.magnetic.z;
                  function2chan[magnetic_function] = chan;
              } else
              {
                cmdLog (99, "Failed to initialize mag");
              }
          }
          break;
        case 'p':
          if (bmpSensorEnabled)
          {
              for (retry = 10; retry != 0; retry--)
              {
                  if (bmpSensor.getPressure(&pressure)) break;
                  delayMicroseconds(500);
                  bmpSensor.begin();
              }
              if (retry > 0)
              {
                  channels[chan].state[0] = pressure;
                  function2chan[pressure_function] = chan;
              } else
              {
                cmdLog (99, "Failed to initialize pressure");
              }
          }
          break;
        case 't':
          if (bmpSensorEnabled)
          {
              for (retry = 10; retry != 0; retry--)
              {
                  if (bmpSensor.getTemperature(&temperature)) break;
                  delayMicroseconds(500);
                  bmpSensor.begin();
              }
              if (retry > 0)
              {
                  channels[chan].state[0] = temperature;
                  function2chan[temp_function] = chan;
              } else
              {
                cmdLog (99, "Failed to initialize temp");
              }
          }
          break;
        default:
            good = false;
            cmdMessenger.sendCmd(nack, "Invalid I2C function");
            break;
      }
      if (good) cmdMessenger.sendCmd(ack);
  }
}
void Onsetup_spi_sensor()
{
    cmdMessenger.sendCmd(nack, "SPI sensors unimplemented");
}

void Onsetup_serial_sensor()
{
  int16_t   chan;
  char      function;
  int16_t   port_number;
  char      *stype;

  chan = cmdMessenger.readInt16Arg();
  if ((chan >= NELEMENTS(channels)) || (chan < 0))
  {
    cmdMessenger.sendCmd(nack, "Serial Invalid channel");
  } else
  {
      bool good = true;
      function = cmdMessenger.readCharArg();
      port_number = cmdMessenger.readInt16Arg();
      stype = cmdMessenger.readStringArg();
      channels[chan].function = function;
      channels[chan].pin = port_number;
      switch (function)
      {
        case 'g':
          function2chan[gps_function] = chan;
          break;
        default:
            good = false;
            cmdMessenger.sendCmd(nack, "Invalid Serial funciton");
            break;
      }
      if (good)
      {
        switch (port_number)
        {
          case 1:
            gps = &Serial1;
            Serial1.begin(9600);
            if (!strncmp (stype, "ubx", 3))
            {
                gps->println("$PUBX,41,1,0003,0003,57600,0*2F");
                Serial1.end();
                Serial1.begin(57600);
            }
            break;
          case 2:
            gps = &Serial2;
            Serial2.begin(9600);
            if (!strncmp (stype, "ubx", 3))
            {
                gps->println("$PUBX,41,1,0003,0003,57600,0*2F");
                Serial2.end();
                Serial2.begin(57600);
            }
            break;
          case 3:
            gps = &Serial3;
            Serial3.begin(9600);
            if (!strncmp (stype, "ubx", 3))
            {
                gps->println("$PUBX,41,1,0003,0003,57600,0*2F");
                Serial3.end();
                Serial3.begin(57600);
            }
            break;
          default:
            good = false;
            cmdMessenger.sendCmd(nack, "Invalid Serial Port Number");
        }
        //gps->write (PMTK_SET_BAUD_9600);
        delay(100);
        if (!strncmp (stype, "ubx", 3))
        {
            gps->println ("$PUBX,40,GLL,0,0,0,0,0,0*5C");
            gps->println ("$PUBX,40,GSA,0,0,0,0,0,0*4E");
            gps->println ("$PUBX,40,GSV,0,0,0,0,0,0*59");
            gps->println ("$PUBX,40,VTG,0,0,0,0,0,0*5E");
            gps->println ("$PUBX,40,RMC,0,1,0,0,0,0*46");
        } else if (!strncmp (stype, "mtk", 3))
        {
            gps->write (PMTK_SET_NMEA_OUTPUT_RMCGGA);
            gps->write (PMTK_SET_NMEA_UPDATE_1HZ);
            gps->write (PMTK_API_SET_FIX_CTL_1HZ);
        }
      }
      if (good) cmdMessenger.sendCmd(ack);
  }
}

void Onsetup_analog_output()
{
  int16_t   pin, value;
  pin = cmdMessenger.readInt16Arg();
  value = cmdMessenger.readInt16Arg();
  pinMode (pin, OUTPUT);
  analogWrite (pin, value);
}

void Onsetup_digital_output()
{
  int16_t   pin, value;
  pin = cmdMessenger.readInt16Arg();
  value = cmdMessenger.readInt16Arg();
  pinMode (pin, OUTPUT);
  digitalWrite (pin, value);
}

void Onset_analog_output()
{
  int16_t   pin, value;
  pin = cmdMessenger.readInt16Arg();
  value = cmdMessenger.readInt16Arg();
  analogWrite (pin, value);
}

void Onset_digital_output()
{
  int16_t   pin, value;
  pin = cmdMessenger.readInt16Arg();
  value = cmdMessenger.readInt16Arg();
  digitalWrite (pin, value);
}


void OnUnknownCommand()
{
    cmdMessenger.sendCmd(nack, "Unknown Command");
}

// Callbacks define on which received commands we take action
void attachCommandCallbacks()
{
  // Attach callback methods
  cmdMessenger.attach(OnUnknownCommand);
  cmdMessenger.attach(setup_digital_sensor, Onsetup_digital_sensor);
  cmdMessenger.attach(setup_analog_sensor, Onsetup_analog_sensor);
  cmdMessenger.attach(setup_i2c_sensor, Onsetup_i2c_sensor);
  cmdMessenger.attach(setup_spi_sensor, Onsetup_spi_sensor);
  cmdMessenger.attach(setup_serial_sensor, Onsetup_serial_sensor);
  cmdMessenger.attach(setup_analog_output, Onsetup_analog_output);
  cmdMessenger.attach(setup_digital_output, Onsetup_digital_output);
  cmdMessenger.attach(set_analog_output, Onset_analog_output);
  cmdMessenger.attach(set_digital_output, Onset_digital_output);
  cmdMessenger.printLfCr(false); 
}


void cmdLog(unsigned level, const char *s)
{
  int16_t checksum = 0;
  cmdMessenger.sendCmdStart(send_log);
  cmdMessenger.sendCmdArg (level);
  cmdMessenger.sendCmdEscArg (s);
  cmdMessenger.sendCmdEnd();
}

void pollGps()
{
  static int     max_gps_available = 0;     // Collect buffer fill statistics in case it's needed

  while (gps && (gps->available()))
  {
    if (gps->available() > max_gps_available) max_gps_available = gps->available();
    char c = gps->read();
    if (c == '\r') continue;
    gps_line[current_gps_line][gps_index++] = c;
    
    if ((c == '\n') || (gps_index >= sizeof(gps_line[0])-1) || ((c == '$') && (gps_index > 1)))
    {
      int   temp = last_gps_line;
      if (c == '$')
      {
        gps_line[current_gps_line][gps_index-1] = 0;
        gps_line[last_gps_line][0] = '$';
        gps_index = 1;
      } else
      {
        gps_line[current_gps_line][gps_index] = 0;
        gps_index = 0;
      }
      last_gps_line = current_gps_line;
      current_gps_line = temp;
      cmdMessenger.sendCmdStart (sensor_reading);
      cmdMessenger.sendCmdArg (function2chan [gps_function]);
      cmdMessenger.sendCmdArg (gps_line[last_gps_line]);
      cmdMessenger.sendCmdArg (millis());
      cmdMessenger.sendCmdEnd ();
      max_gps_available = 0;
    }
  }
}

void filter_channel (pChannel pch, int ninputs, float input[])
{
    int     in, filter = 0;
    float   diff[3], adiff;
    bool    trigger_secondary = false;

    for (in = 0; in < ninputs; in++)
    {
        diff[in] = input[in] - pch->state[in];
        adiff = ABS(diff[in]);
        if (adiff > pch->rejection_band)
        {   // Reject all inputs from this sample. They are all suspect
            pch->reject_count++;
            return;
        }
        if (adiff > pch->secondary_band)
        {   trigger_secondary = true; }
    }
    if (trigger_secondary)                  // If we get more large sensor movement,
    {   pch->secondary_filter_count = 1; }  // reset the filter count for more responsiveness
    if (pch->secondary_filter_count > 0)
    {
        filter = 1;
        pch->secondary_use_count++;
    }
    for (in = 0; in < ninputs; in++)
    {
        pch->state[in] += pch->filter_coefficient[filter] * diff[in];
    }
    if (pch->secondary_filter_count > 0)
    {
        pch->secondary_filter_count++;
        if (pch->secondary_filter_count >= pch->secondary_filter_duration)
        {   pch->secondary_filter_count = 0;    }
    }
    pch->sample_count++;
}

void setup()
{
  // put your setup code here, to run once:
  memset (channels, 0, sizeof (channels));
  memset (function2chan, 0xff, sizeof (function2chan));
  gps_line[current_gps_line][0] = 0;
  gps_line[last_gps_line][0] = 0;
  gps_index = 0;
  Serial.begin(115200);
  cmdLog (99, "Sensors Starting...");
  bmpSensorEnabled = bmpSensor.begin();
  gyroSensorEnabled = gyroSensor.begin();
  accSensorEnabled = accSensor.begin();
  magSensorEnabled = magSensor.begin();

  attachCommandCallbacks();
}

void loop()
{
  float   pressure, temperature;
  sensors_event_t   event;
  pChannel          pch;
  int               sensors_configured = 0;
  
  // put your main code here, to run repeatedly:
  cmdMessenger.feedinSerialData();
  pollGps();

  unsigned long ms = millis();
  long          timediff;
  int           channel;
  // Get 10DOF readings
  if (function2chan[pressure_function] != 0xff)
  {
    channel = function2chan[pressure_function];
    pch = &(channels[channel]);
    //cmdLog (99, "P");
    sensors_configured++;
    bmpSensor.getPressure(&pressure);
    filter_channel (pch, 1, &pressure);
    timediff = (long)pch->next_time - (long)ms;
    if (timediff <= 0)
    {
        cmdMessenger.sendCmdStart (sensor_reading);
        cmdMessenger.sendCmdArg (channel);
        cmdMessenger.sendCmdArg (pch->state[0]);
        cmdMessenger.sendCmdArg ((float)0); // Pitot pressure not implemented yet
        cmdMessenger.sendCmdArg (ms);
        cmdMessenger.sendCmdArg (pch->sample_count);
        cmdMessenger.sendCmdArg (pch->secondary_use_count);
        cmdMessenger.sendCmdArg (pch->reject_count);
        cmdMessenger.sendCmdEnd ();
        pch->next_time += pch->period;
        pch->sample_count = 0;
        pch->reject_count = 0;
        pch->secondary_use_count = 0;
    }
  }

  pollGps();
  ms = millis();

  if (function2chan[temp_function] != 0xff)
  {
    channel = function2chan[temp_function];
    pch = &(channels[channel]);
    //cmdLog (99, "T");
    sensors_configured++;
    bmpSensor.getTemperature(&temperature);
    filter_channel (pch, 1, &temperature);
    timediff = (long)pch->next_time - (long)ms;
    if (timediff <= 0)
    {
        cmdMessenger.sendCmdStart (sensor_reading);
        cmdMessenger.sendCmdArg (channel);
        cmdMessenger.sendCmdArg (pch->state[0]);
        cmdMessenger.sendCmdArg (pch->sample_count);
        cmdMessenger.sendCmdArg (pch->secondary_use_count);
        cmdMessenger.sendCmdArg (pch->reject_count);
        cmdMessenger.sendCmdEnd ();
        pch->next_time += pch->period;
        pch->sample_count = 0;
        pch->reject_count = 0;
        pch->secondary_use_count = 0;
    }
  }

  pollGps();
  ms = millis();

  if (function2chan[rotation_function] != 0xff)
  {
    channel = function2chan[rotation_function];
    pch = &(channels[channel]);
    //cmdLog (99, "g");
    sensors_configured++;
    if (gyroSensor.read())
    {
        filter_channel (pch, 3, &(gyroSensor.data.x));
        timediff = (long)pch->next_time - (long)ms;
        if (timediff <= 0)
        {
            cmdMessenger.sendCmdStart (sensor_reading);
            cmdMessenger.sendCmdArg (channel);
            cmdMessenger.sendCmdArg (pch->state[0]);
            cmdMessenger.sendCmdArg (pch->state[1]);
            cmdMessenger.sendCmdArg (pch->state[2]);
            cmdMessenger.sendCmdArg (ms);
            cmdMessenger.sendCmdArg (pch->sample_count);
            cmdMessenger.sendCmdArg (pch->secondary_use_count);
            cmdMessenger.sendCmdArg (pch->reject_count);
            cmdMessenger.sendCmdEnd ();
            pch->next_time += pch->period;
            pch->sample_count = 0;
            pch->reject_count = 0;
            pch->secondary_use_count = 0;
        }
    }
  }

  pollGps();
  ms = millis();

  if (function2chan[accel_function] != 0xff)
  {
    channel = function2chan[accel_function];
    pch = &(channels[channel]);
    //cmdLog (99, "A");
    sensors_configured++;
    if (accSensor.getEvent(&event))
    {
        filter_channel (pch, 3, event.orientation.v);
        timediff = (long)pch->next_time - (long)ms;
        if (timediff <= 0)
        {
            cmdMessenger.sendCmdStart (sensor_reading);
            cmdMessenger.sendCmdArg (channel);
            cmdMessenger.sendCmdArg (pch->state[0]);
            cmdMessenger.sendCmdArg (pch->state[1]);
            cmdMessenger.sendCmdArg (pch->state[2]);
            cmdMessenger.sendCmdArg (event.timestamp);
            cmdMessenger.sendCmdArg (pch->sample_count);
            cmdMessenger.sendCmdArg (pch->secondary_use_count);
            cmdMessenger.sendCmdArg (pch->reject_count);
            cmdMessenger.sendCmdEnd ();
            pch->next_time += pch->period;
            pch->sample_count = 0;
            pch->reject_count = 0;
            pch->secondary_use_count = 0;
        }
    }
  }

  pollGps();
  ms = millis();

  if (function2chan[magnetic_function] != 0xff)
  {
    channel = function2chan[magnetic_function];
    pch = &(channels[channel]);
    //cmdLog (99, "M");
    sensors_configured++;
    if (magSensor.getEvent(&event))
    {
        filter_channel (pch, 3, event.magnetic.v);
        timediff = (long)pch->next_time - (long)ms;
        if (timediff <= 0)
        {
            cmdMessenger.sendCmdStart (sensor_reading);
            cmdMessenger.sendCmdArg (channel);
            cmdMessenger.sendCmdArg (event.magnetic.x);
            cmdMessenger.sendCmdArg (event.magnetic.y);
            cmdMessenger.sendCmdArg (event.magnetic.z);
            cmdMessenger.sendCmdArg (event.timestamp);
            cmdMessenger.sendCmdArg (pch->sample_count);
            cmdMessenger.sendCmdArg (pch->secondary_use_count);
            cmdMessenger.sendCmdArg (pch->reject_count);
            cmdMessenger.sendCmdEnd ();
            pch->next_time += pch->period;
            pch->sample_count = 0;
            pch->reject_count = 0;
            pch->secondary_use_count = 0;
        }
    } else
    {
        magSensor.begin();
    }
  }

  pollGps();
  ms = millis();

  for (channel = 0; channel < NELEMENTS(channels); channel++)
  {
    pch = &(channels[channel]);
    if ((pch->function == 'd') || (pch->function == 'n'))
    {
        timediff = (long)pch->next_time - (long)ms;
        if (timediff <= 0)
        {
            cmdMessenger.sendCmdStart (sensor_reading);
            cmdMessenger.sendCmdArg (channel);
            if (pch->function == 'd')
            {
                cmdMessenger.sendCmdArg (digitalRead(pch->pin));
            } else
            {
                cmdMessenger.sendCmdArg (analogRead(pch->pin));
            }
            cmdMessenger.sendCmdArg (ms);
            cmdMessenger.sendCmdEnd ();
            
            pch->next_time += pch->period;
        }
    }
  }
  //if (sensors_configured >= 5) cmdLog (99, "L");
}
