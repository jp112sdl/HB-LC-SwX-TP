//- -----------------------------------------------------------------------------------------------------------------------
// AskSin++
// 2016-10-31 papa Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
// 2021-05-20 jp112sdl Creative Commons - http://creativecommons.org/licenses/by-nc-sa/3.0/de/
//- -----------------------------------------------------------------------------------------------------------------------
// ci-test=yes board=328p aes=no

// define this to read the device id, serial and device type from bootloader section
// #define USE_OTA_BOOTLOADER
#define HIDE_IGNORE_MSG
#define EI_NOTEXTERNAL
#include <EnableInterrupt.h>
#include <AskSinPP.h>
#include <LowPower.h>
#include <TimeLib.h>

#include <Switch.h>

#define LED_PIN             4
#define RELAY1_PIN          5
#define CONFIG_BUTTON_PIN   8
#define PEERS_PER_CHANNEL  12

#define NUM_DAYS          7  //we have 7 days a week, saturday ... friday
#define PROFILES_PER_DAY 13  //number of time profiles per day

#define TIMESYNC_INTERVAL_SECONDS (60UL*60*12)  //usually, the ccu sends a cyclic timestamp message. if this message is missing, request a new time from ccu
#define TIMESYNC_MAX_TRIES         6

const String strDays[7]  =  {"SAT", "SUN", "MON", "TUE", "WED", "THU", "FRI"};
// all library classes are placed in the namespace 'as'
using namespace as;

// define all device properties
const struct DeviceInfo PROGMEM devinfo = {
  {0xf3, 0x23, 0x01},     // Device ID
  "JPSW1TP001",           // Device Serial
  {0xF3, 0x23},           // Device Model
  0x10,                   // Firmware Version
  as::DeviceType::Switch, // Device Type
  {0x01, 0x00}            // Info Bytes
};

typedef AskSin<StatusLed<LED_PIN>, NoBattery, Radio<AvrSPI<10, 11, 12, 13> , 2> > Hal;


#define CREG_TRANSMITTRYMAX_CUSTOM 0x01
//#define  CREG_POWERUPACTION_CUSTOM 0x02
#define     CREG_STATUSINFO_CUSTOM 0x03

DEFREGISTER(MySwitchReg1,CREG_TRANSMITTRYMAX_CUSTOM, /*CREG_POWERUPACTION_CUSTOM,*/ CREG_STATUSINFO_CUSTOM, CREG_AES_ACTIVE, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201)
class MySwitchList1 : public RegisterList<MySwitchReg1> {
public:
  MySwitchList1(uint16_t addr) : RegisterList<MySwitchReg1>(addr) {}

  bool aesActive(bool v) const { return this->writeRegister(CREG_AES_ACTIVE,0x01,0,v); }
  bool aesActive() const { return this->readRegister(CREG_AES_ACTIVE,0x01,0,false); }

  bool transmitTryMax(uint8_t v) const { return this->writeRegister(CREG_TRANSMITTRYMAX_CUSTOM,v); }
  uint8_t transmitTryMax() const { uint8_t v = this->readRegister(CREG_TRANSMITTRYMAX_CUSTOM,6); return v == 0 ? 1 : v;}

  //bool powerUpAction () const { return this->readBit(CREG_POWERUPACTION_CUSTOM,0,false); }
  //bool powerUpAction (bool v) const { return this->writeBit(CREG_POWERUPACTION_CUSTOM,0,v); }

  uint8_t statusInfoMinDly () const { return this->readRegister(CREG_STATUSINFO_CUSTOM,0x1f,0,4); }
  bool statusInfoMinDly (uint8_t v) { return this->writeRegister(CREG_STATUSINFO_CUSTOM,0x1f,0,v); }
  uint8_t statusInfoRandom () const { return this->readRegister(CREG_STATUSINFO_CUSTOM,0x07,5,1); }
  bool statusInfoRandom (uint8_t v) { return this->writeRegister(CREG_STATUSINFO_CUSTOM,0x07,5,v); }

  const uint8_t start = 20;   //index offset

  bool     ENDTIME(uint8_t dayofweek, uint8_t index,uint16_t value) const { return  this->writeRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index, ((value / 5) >> 8) & 0x01) && this->writeRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index +1, (value / 5) & 0xff); }
  uint16_t ENDTIME(uint8_t dayofweek, uint8_t index)                const { return ((this->readRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index, 0x01,0) << 8) + this->readRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index +1, 0))*5; }

  bool       LEVEL(uint8_t dayofweek, uint8_t index,uint8_t  value) const { return  this->writeRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index,0xfe,1,value / 5); }
  uint8_t    LEVEL(uint8_t dayofweek, uint8_t index)                const { return   this->readRegister(start + 2*dayofweek*PROFILES_PER_DAY + 2*index,0xfe,1) * 5; }


  void defaults () {
    clear ();
    //aesActive(false);
    transmitTryMax(6);
    //powerUpAction(false);
    statusInfoMinDly(4);
    statusInfoRandom(1);

    for (uint8_t dow = 0; dow < NUM_DAYS; dow++) {
      for (uint8_t idx = 0; idx < PROFILES_PER_DAY; idx++) {
        LEVEL(dow, idx, 0);

        uint16_t endtime = 1440;
        if (idx == 0) endtime = 360;
        else if (idx == 1) endtime = 1320;
        ENDTIME(dow, idx, endtime);
      }
    }

  }
};

class SwChannel : public ActorChannel<Hal,MySwitchList1,SwitchList3,PEERS_PER_CHANNEL,List0,SwitchStateMachine> {
private:
  class SwitchSchedule : public Alarm {
    SwChannel& swc;
  private:
    uint8_t lastLevelToSet, levelToSet, lastDow;
    struct {
      uint16_t endtime = 0xffff;
      uint8_t  level;
    } dayProfile[PROFILES_PER_DAY];

  public:
    SwitchSchedule (SwChannel& s) : Alarm(0), swc(s), lastLevelToSet(0xff), levelToSet(0), lastDow(0xff) {}
    virtual ~SwitchSchedule () {}

    void start() {
      sysclock.cancel(*this);
      set(seconds2ticks(1));
      sysclock.add(*this);
     }

    void setDirtyDayProfiles() {
        dayProfile[0].endtime = 0xffff;
    }

    void refreshDayProfileFromList1(uint8_t dow) {
      DPRINT("refreshDayProfileFromList1(day=");DPRINT(strDays[dow]);DPRINTLN(")");
      for (uint8_t idx = 0; idx < PROFILES_PER_DAY; idx++) {
        dayProfile[idx].endtime = swc.getList1().ENDTIME(dow, idx);
        dayProfile[idx].level   = swc.getList1().LEVEL  (dow, idx);
      }
    }

    virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
      if (timeStatus() != timeStatus_t::timeNotSet) {
        uint8_t dow = dayOfWeek(now());
        if (dow != timeDayOfWeek_t::dowInvalid) {

          if (dow == 7) dow = 0;
          //            0    1    2    3    4    5    6   7
          //Homematic: SAT, SUN, MON, TUE, WED, THU, FRI
          //TimeLib  : INV, SUN, MON, TUE, WED, THU, FRI, SAT

          if (dayProfile[0].endtime == 0xffff || lastDow != dow) refreshDayProfileFromList1(dow);

          lastDow = dow;

          uint16_t minSinceMidnight = (hour() * 60)+minute();

          for (uint8_t idx = 0; idx < PROFILES_PER_DAY; idx++) {
             if (minSinceMidnight < dayProfile[idx].endtime) {
               levelToSet = dayProfile[idx].level;
               break;
             }
          }

          if (levelToSet != swc.status() && levelToSet != lastLevelToSet) {
            lastLevelToSet = levelToSet;
            swc.status(levelToSet, 0xffff);
          }
        }
      }

      set(seconds2ticks(1));
      clock.add(*this);
    }

  } switchSchedule;

  class SetDirtyDelayAlarm : public Alarm {
    SwChannel& swc;
  public:
    SetDirtyDelayAlarm (SwChannel& s) : Alarm(0), swc(s) {}
    virtual ~SetDirtyDelayAlarm () {}

    void start() {
      sysclock.cancel(*this);
      set(seconds2ticks(5));
      sysclock.add(*this);
     }

    virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
      swc.switchSchedule.setDirtyDayProfiles();
      swc.dumpAllProfilesFromList1();
    }

  } setDirtyDelayAlarm;
protected:
  uint8_t lowact;
  uint8_t pin;
  bool first;
public:
  SwChannel () : switchSchedule(*this), setDirtyDelayAlarm(*this), lowact(false), pin(0), first(true) {}
  virtual ~SwChannel() {}

  void init (uint8_t p,bool value=false) {
    pin=p;
    ArduinoPins::setOutput(pin);
    lowact = value;
    this->set(/*this->getList1().powerUpAction() == true ? 200 :*/ 0, 0, 0xffff );
    this->changed(true);
    switchSchedule.start();
  }

  uint8_t flags () const { return 0;  }

  void dumpDayProfileFromList1(uint8_t dow) {
    for (uint8_t idx = 0; idx < PROFILES_PER_DAY; idx++) {
      DPRINT("ENDTIME_");DPRINT(strDays[dow]);DPRINT("_");DDEC(idx);DPRINT("=\t");DDEC(this->getList1().ENDTIME(dow, idx));
      DPRINT("\t: LEVEL_");DPRINT(strDays[dow]);DPRINT("_");DDEC(idx);DPRINT(":\t");DDECLN(this->getList1().LEVEL(dow, idx));
    }
  }
  void dumpAllProfilesFromList1() {
    for (uint8_t dow = 0; dow < NUM_DAYS; dow++) {
      dumpDayProfileFromList1(dow);
    }
  }

  void configChanged() {
    //if the config was changed, force reloading the time-profile
    if (first == false) setDirtyDelayAlarm.start(); else first = false;
  }

  virtual void switchState(__attribute__((unused)) uint8_t oldstate,uint8_t newstate,__attribute__((unused)) uint32_t delay) {
    if( newstate == AS_CM_JT_ON ) {
      if( lowact == true ) ArduinoPins::setLow(pin);
      else ArduinoPins::setHigh(pin);
    }
    else if ( newstate == AS_CM_JT_OFF ) {
      if( lowact == true ) ArduinoPins::setHigh(pin);
      else ArduinoPins::setLow(pin);
    }
    this->changed(true);
  }
};

class SwitchType : public ChannelDevice<Hal,SwChannel,1,List0>{
  SwChannel swc;

  class InfoTimestampMsg : public Message {
  public:
    void init () {
      Message::init(0x09,0x00,AS_MESSAGE_TIMESTAMP,BIDI|RPTEN,0x00,0x00);
    }
  };

  class TimestampRequestTimer : public Alarm {
    SwitchType& sw;
  private:
    uint8_t timeReqCount;
    uint32_t nextsync;
  public:
    TimestampRequestTimer (SwitchType& s) : Alarm(0), sw(s),timeReqCount(0), nextsync(0)  {}
    virtual ~TimestampRequestTimer () {}

    void startTimeSync() {
      timeReqCount = TIMESYNC_MAX_TRIES;
      sysclock.cancel(*this);
      set(seconds2ticks(1));
      sysclock.add(*this);
    }

    void stopResync() {
      timeReqCount  = 0;
    }

    void nextSync(uint32_t s) {
      nextsync = s;
    }

    virtual void trigger (__attribute__((unused)) AlarmClock& clock) {
      InfoTimestampMsg imsg;
      imsg.init();
      HMID master = sw.getMasterID();
      HMID devid;sw.getDeviceID(devid);
      imsg.from(devid);
      imsg.to(master);

      bool giveup = false;
      if (master != HMID::broadcast) {
        if (timeReqCount > 0) {
          timeReqCount--;
          if (timeReqCount == 0) giveup = true;
          DPRINT("Sending Timestamp request, leaving ");DDEC(timeReqCount); DPRINTLN(" more retries");
          sw.radio().write(imsg, false);
        }
      }

      if (nextsync < now()) {
        if (timeReqCount == 0) {
          if (giveup == false) {
            timeReqCount = TIMESYNC_MAX_TRIES;
          } else {
            nextsync = now() + TIMESYNC_INTERVAL_SECONDS;
          }
        }
      }

      set(millis2ticks(2500));
      clock.add(*this);
    }
  };

public:
  TimestampRequestTimer timestampRequestTimer;
public:
  typedef ChannelDevice<Hal,SwChannel,1,List0> DeviceType;
  SwitchType (const DeviceInfo& i,uint16_t addr) : ChannelDevice<Hal,SwChannel,1,List0>(i,addr), timestampRequestTimer(*this) {
      this->registerChannel(swc,1);
  }
  virtual ~SwitchType () {}

  bool process(Message& msg) {
     if (getMasterID() == msg.from()) {
      if(msg.type() == AS_MESSAGE_TIMESTAMP) {
        if (msg.command() == AS_ACTION_SET) {
         //DPRINTLN("Got TIMESTAMP Message");
         msg.dump();
         static const uint32_t sec2000 = 946684800;

         int8_t timezone = msg.buffer()[10] / 2;

         uint32_t tPayload =
            (((uint32_t)msg.buffer()[11]) << 24) +
            (((uint32_t)msg.buffer()[12]) << 16) +
            (((uint32_t)msg.buffer()[13]) << 8) +
            (((uint32_t)msg.buffer()[14]))       ;

         uint32_t unixtime = sec2000 + tPayload;

         setTime(unixtime);

         timestampRequestTimer.stopResync();

         time_t t = now();
         DPRINT("UTC   Time is ");DDEC(day(t));DPRINT(".");DDEC(month(t));DPRINT(".");DDEC(year(t));DPRINT(" ");DDEC(hour(t));DPRINT(":");DDEC(minute(t));DPRINT(":");DDECLN(second(t));

         adjustTime(timezone * 3600);
         t = now();
         DPRINT("local Time is ");DDEC(day(t));DPRINT(".");DDEC(month(t));DPRINT(".");DDEC(year(t));DPRINT(" ");DDEC(hour(t));DPRINT(":");DDEC(minute(t));DPRINT(":");DDECLN(second(t));

         timestampRequestTimer.nextSync(now() + TIMESYNC_INTERVAL_SECONDS);
        }
      }
    }

    return DeviceType::process(msg);
   }

  void configChanged() {  }
};

Hal hal;
SwitchType sdev(devinfo, 0x20);
ConfigToggleButton<SwitchType> cfgBtn(sdev);

void setup () {
  DINIT(57600, ASKSIN_PLUS_PLUS_IDENTIFIER);
  bool first = sdev.init(hal);

  sdev.channel(1).init(RELAY1_PIN, false);

  buttonISR(cfgBtn, CONFIG_BUTTON_PIN);

  if (first == true) {
    HMID devid;
    sdev.getDeviceID(devid);
    Peer ipeer(devid, 1);
    sdev.channel(1).peer(ipeer);
  }

  sdev.initDone();

  sdev.timestampRequestTimer.startTimeSync();
}

void loop() {
  hal.runready();
  sdev.pollRadio();
}
