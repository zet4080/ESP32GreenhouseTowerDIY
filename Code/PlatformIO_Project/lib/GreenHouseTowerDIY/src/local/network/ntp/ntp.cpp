#include "ntp.hpp"

const char NetworkNTP::ntpServerName[] = "us.pool.ntp.org";
// static const char ntpServerName[] = "time.nist.gov";
// static const char ntpServerName[] = "time-a.timefreq.bldrdoc.gov";
// static const char ntpServerName[] = "time-b.timefreq.bldrdoc.gov";
// static const char ntpServerName[] = "time-c.timefreq.bldrdoc.gov";

/**
 * @brief Get the current time automatically from the NTP server
 * @return String with the current time
 *
 */
// Define NTP Client to get time

/**
 * @brief Get the current time MANUALLY
 *
 */
NetworkNTP::NetworkNTP()
    : _formattedDate(""),
      _dayStamp(""),
      _timeStamp(""),
      localPort(8888),
      timeClient(ntpUDP),
      prevDisplay(0) {}

void NetworkNTP::begin() {
  timeZone = TIME_ZONE_OFFSET;
  timeClient.begin();
  timeClient.setTimeOffset(timeZone);
}

NetworkNTP::~NetworkNTP() {}

#ifdef NTP_MANUAL_ENABLED
void NetworkNTP::NTPSetupManual() {
  log_i("Starting UDP");
  ntpUDP.begin(localPort);
  log_i("waiting for sync");
  setSyncProvider(getNtpTime);
  setSyncInterval(300);
}

void NetworkNTP::NTPLoopManual() {
  if (timeStatus() != timeNotSet) {
    if (now() != prevDisplay) {  // update the display only if time has changed
      prevDisplay = now();
      digitalClockDisplay();
    }
  }
}

void NetworkNTP::digitalClockDisplay() {
  // digital clock display of the time
  Serial.print(hour());
  printDigits(minute());
  printDigits(second());
  Serial.print(" ");
  Serial.print(day());
  Serial.print(".");
  Serial.print(month());
  Serial.print(".");
  Serial.print(year());
  Serial.println();
}

void NetworkNTP::printDigits(int digits) {
  // utility for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if (digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

/*-------- NTP code ----------*/
const int ntp_packet_size = 48;  // NTP time is in the first 48 bytes of message
byte packetBuffer[ntp_packet_size];  // buffer to hold incoming & outgoing
                                     // packets

time_t NetworkNTP::getNtpTime() {
  IPAddress ntpServerIP;  // NTP server's ip address

  while (ntpUDP.parsePacket() > 0)
    ;  // discard any previously received packets
  Serial.println("Transmit NTP Request");
  // get a random server from the pool
  WiFi.hostByName(ntpServerName, ntpServerIP);
  Serial.print(ntpServerName);
  Serial.print(": ");
  Serial.println(ntpServerIP);
  sendNTPpacket(ntpServerIP);
  uint32_t beginWait = millis();
  while (millis() - beginWait < 1500) {
    int size = ntpUDP.parsePacket();
    if (size >= ntp_packet_size) {
      Serial.println("Receive NTP Response");
      ntpUDP.read(packetBuffer,
                  ntp_packet_size);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 = (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      return secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
    }
  }
  Serial.println("No NTP Response :-(");
  return 0;  // return 0 if unable to get the time
}

// send an NTP request to the time server at the given address
void NetworkNTP::sendNTPpacket(IPAddress& address) {
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, ntp_packet_size);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;  // LI, Version, Mode
  packetBuffer[1] = 0;           // Stratum, or type of clock
  packetBuffer[2] = 6;           // Polling Interval
  packetBuffer[3] = 0xEC;        // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12] = 49;
  packetBuffer[13] = 0x4E;
  packetBuffer[14] = 49;
  packetBuffer[15] = 52;
  // all NTP fields have been given values, now
  // you can send a packet requesting a _timestamp:
  ntpUDP.beginPacket(address, 123);  // NTP requests are to port 123
  ntpUDP.write(packetBuffer, ntp_packet_size);
  ntpUDP.endPacket();
}
#else

void NetworkNTP::ntpLoop() {
  if (!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The _formattedDate comes with the following format:
  // 2022-05-28T16:00:13Z
  // We need to extract date and time
  _formattedDate = timeClient.getFormattedDate().c_str();
  log_d("Formatted Date: %s", _formattedDate.c_str());
  int splitT = _formattedDate.find("T");

  if (splitT == std::string::npos) {
    log_e("Error parsing date");
    return;
  }

  _dayStamp = _formattedDate.substr(0, splitT).c_str();
  log_d("DATE: %s", _dayStamp.c_str());
  _timeStamp =
      _formattedDate.substr(splitT + 1, _formattedDate.length() - 1).c_str();
  log_d("HOUR: %s", _timeStamp.c_str());
}

const std::string& NetworkNTP::getFullDate() {
  return _formattedDate;
}

const std::string& NetworkNTP::getDayStamp() {
  return _dayStamp;
}

const std::string& NetworkNTP::getTimeStamp() {
  return _timeStamp;
}

const std::string NetworkNTP::getYear() {
  std::string year;
  year.assign(timeClient.getFormattedDate().substring(0, 4).c_str());
  return year;
}

const std::string NetworkNTP::getMonth() {
  std::string month;
  month.assign(timeClient.getFormattedDate().substring(5, 7).c_str());
  return month;
}

const std::string NetworkNTP::getDay() {
  std::string day;
  day.assign(timeClient.getFormattedDate().substring(8, 10).c_str());
  return day;
}
#endif  // NTP_MANUAL_ENABLED

const std::string& NetworkNTP::getSensorName() {
  static std::string name = "ntp";
  return name;
}

void NetworkNTP::accept(Visitor<SensorInterface<std::string>>& visitor) {
  visitor.visit(this);
}

std::string NetworkNTP::read() {
  return getTimeStamp();
}