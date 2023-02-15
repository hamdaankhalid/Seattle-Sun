#include <math.h>
#include "TimeLib.h"

/*
Seattle sun is essentially a screen that mimics the sun in seattle in real time.
The applciation can be modified to work regardles of location. It just happens to be the case
that i live in Seattle.
*/



// ------------------------------------ Longitude and Latitude For Your Location ------------------------------------
const double lat = 47.36;
const double lon = -122.19;
const double utcOffset = 48000
// ------------------------------------------------------------------------------------------------------------------

const int brightnessUpdateInterval = 60000; // Every minute

class MilitaryTimeFormat : public Printable  {
  public:
  int originalMinutes;
  int hour; // 1 - 24
  int minutes; // 1 - 60
  
  static MilitaryTimeFormat FromMinutes(int minutes): originalMinutes(minutes) {
    int hour = minutes / 60;
    int remainingMinutes = minutes % 60;
    return MilitaryTimeFormat(hour, remainingMinutes);
  }

  size_t printTo(Print& p) const {
    size_t r = 0;
    r += p.print("Hour ");
    r += p.print(hour);
    r += p.print(" Minutes: ");
    r += p.print(minutes);
    return r;
  }

  MilitaryTimeFormat operator-(MilitaryTimeFormat rhs) {
    return MilitaryTimeFormat::FromMinutes(originalMinutes - rhs.originalMinutes);
  }
  
  MilitaryTimeFormat(int hr, int mins): hour(hr), minutes(mins) {}
};

// https://gml.noaa.gov/grad/solcalc/solareqns.PDF
class SunBrightnessConvertor {
  public:

  SunBrightnessConvertor(double currentLatitude, double currentLongitude):
  latitude(currentLatitude), longitude(currentLongitude) {}

  int SunBrightnessNow() const {
    // Todo get time now
    int dayOfYear = 44;
    int currentYear = 2023;
    int hr = 20;
    int min = 15;
  
    double fractionalYear = calcFractionalYear(currentYear, dayOfYear, hr);
    double equationOfTime = calcEquationOfTime(fractionalYear);
    double solarDeclinationAngle = calcSolarDeclinationAngle(fractionalYear);
    double unsignedHourAngle = calcUnsignedHourAngle(solarDeclinationAngle);

    MilitaryTimeFormat solarNoon = MilitaryTimeFormat::FromMinutes((int)calcSolarNoon(equationOfTime) - utcOffset);
    MilitaryTimeFormat dusk = MilitaryTimeFormat::FromMinutes((int)calcDuskUtc(unsignedHourAngle, equationOfTime) - utcOffset);
    MilitaryTimeFormat dawn = MilitaryTimeFormat::FromMinutes((int)calcDawnUtc(unsignedHourAngle, equationOfTime) - utcOffset);

    Serial.println("Dawn: ");
    Serial.println(dawn);

    Serial.println("Solar Noon: ");
    Serial.println(solarNoon);
    
    Serial.println("Dusk: ");
    Serial.println(dusk);

    // If the time now is before dawn or time now is after dusk return 0
    // draw a Hill like distrbution with evenly spaced increments and decrements where solar noon is peak
    // From the time between dawn to solarNoon divide the space by 0-100 in increments of 1 minute
    // From the time between solarNoon to dusk divide the space by 100-0 in decrements of 1 minute
    MilitaryTimeFormat now(hr, min);
    
    // before dawn or after dusk
    if ((now - dawn) < 0 || (now - dusk) > 0){
      return 0;
    }

    // b/w dawn and solar noon
    if ((now - solarNoon) < 0) {
      int minsToPeak = (solarNoon - dawn).originalMinutes;
      double stepSize = (double) minsToPeak / 100.00;
      // where does now fall in this distribution?
      int currentRelativePlacement = (now - dawn).originalMinutes;
      return currentRelativePlacement*stepsize;
    }

    // b/w solarNoon and dusk
    int minsToPeak = (dusk - solarNoon).originalMinutes;
    double stepSize = (double) minsToPeak / 100.00;
    // TODO start from 100 at solarNoon to 0 :)

    return 0;
  }

  private:
  
  double latitude;
  double longitude;

  const double pi = 3.14159265358979323846;
  const double degreesToRadians = pi / 180.0;
  const double radiansToDegrees = 180.0 / pi;
  const double sunriseZenith = 90.83 * degreesToRadians; // degrees

  // Returns in radians unit
  double calcFractionalYear(int currentYear, int dayOfYear, int hour) const {
    int daysInYear = currentYear % 4 == 0 ? 366 : 365;
    return (2*pi)/daysInYear * (dayOfYear - 1 + ((hour-12)/24));
  }

  // Returns in radians unit
  double calcEquationOfTime(double fractionalYear) const {
    // fractional year is already in radians so no need to convert it before passing to trignometric functions
    return 229.18 * (0.000075 + 0.001868*cos(fractionalYear) - 0.032077*sin(fractionalYear) - 0.014615*cos(2*fractionalYear) - 0.040849*sin(2*fractionalYear) );
  }
  
  // Returns in radians unit
  double calcSolarDeclinationAngle(double fractionalYear) const {
    // fractional year is already in radians so no need to convert it before passing to trignometric functions
    return 0.006918 - 0.399912*cos(fractionalYear) + 0.070257*sin(fractionalYear) - 0.006758*cos(2*fractionalYear) + 0.000907*sin(2*fractionalYear) - 0.002697*cos(3*fractionalYear) + 0.00148*sin(3*fractionalYear);
  }

  // Returns in radians unit
  double calcUnsignedHourAngle(double solarDeclination) const {
    // solar declination is already in radians
    return acos(
      (cos(sunriseZenith)/(cos(latitude * degreesToRadians)*cos(solarDeclination)))
       - 
      (tan(latitude * degreesToRadians)*tan(solarDeclination)) 
    );
  }

  double calcSolarNoon(double equationOfTime) const {
    return 720 - 4*longitude - equationOfTime;
  }

  // Dusk and Dawn calculations are based on civil twilight
  // Dusk is in evening
  double calcDuskUtc(double hourAngle, double equationOfTime) const {
    return 720 - (4*(longitude + (-1*hourAngle*radiansToDegrees))) - equationOfTime;
  }

  // dawn is in morning
  double calcDawnUtc(double hourAngle, double equationOfTime) const {
    return 720 - (4*(longitude + (hourAngle*radiansToDegrees))) - equationOfTime;
  }
};


// ------------------------------ Driver ------------------------------

SunBrightnessConvertor sbc(lat, lon);


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  sbc.SunBrightnessNow();

  delay(brightnessUpdateInterval);
}
