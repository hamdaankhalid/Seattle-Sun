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
const double utcOffset = 480.00;
const int smootheningOffset = 25; // Offset in minutes to make a smoother transition for when the sky goes bright and dark
// ------------------------------------------------------------------------------------------------------------------

const int brightnessUpdateInterval = 60000; // Every minute

class MilitaryTimeFormat : public Printable  {
  public:
  int originalMinutes;
  int hour; // 1 - 24
  int minutes; // 1 - 60
  
  static MilitaryTimeFormat FromMinutes(int minutes) {
    int hour = minutes / 60;
    int remainingMinutes = minutes % 60;
    return MilitaryTimeFormat(hour, remainingMinutes);
  }

  size_t printTo(Print& p) const {
    size_t r = 0;
    r += p.print("Hour: ");
    r += p.print(hour);
    r += p.print(" Minutes: ");
    r += p.print(minutes);
    return r;
  }

  MilitaryTimeFormat operator-(MilitaryTimeFormat rhs) {
    return MilitaryTimeFormat::FromMinutes(originalMinutes - rhs.originalMinutes);
  }
  
  MilitaryTimeFormat(int hr, int mins): hour(hr), minutes(mins) {
    originalMinutes = hr*60 + mins;
  }
};

// https://gml.noaa.gov/grad/solcalc/solareqns.PDF
class SunBrightnessConvertor {
  public:

  SunBrightnessConvertor(double currentLatitude, double currentLongitude):
  latitude(currentLatitude), longitude(currentLongitude) {}

  /** If the time input is before dawn or time now is after dusk return 0
    draw a Hill like distrbution with evenly spaced increments and decrements where solar noon is peak
    From the time between dawn to solarNoon divide the space by 0-100 in increments of 1 minute
    From the time between solarNoon to dusk divide the space by 100-0 in decrements of 1 minute
  **/
  int SunBrightnessNow(int dayOfYear, int currentYear, int hr, int min) const {
    double fractionalYear = calcFractionalYear(currentYear, dayOfYear, hr);
    double equationOfTime = calcEquationOfTime(fractionalYear);
    double solarDeclinationAngle = calcSolarDeclinationAngle(fractionalYear);
    double unsignedHourAngle = calcUnsignedHourAngle(solarDeclinationAngle);
    
    // smoothening offset starts the day just a little earlier so we can start increasing our brightness gradually instead of starting right at sunrise
    MilitaryTimeFormat dawn = MilitaryTimeFormat::FromMinutes((int)calcDawnUtc(unsignedHourAngle, equationOfTime) - utcOffset - smootheningOffset);
    // SolarNoon does not need to be smoothened since its not an extreme end of the clock
    MilitaryTimeFormat solarNoon = MilitaryTimeFormat::FromMinutes((int)calcSolarNoon(equationOfTime) - utcOffset);
    // smoothening offset ends the day just a little later so we can start decreasing our brightness gradually instead of going right to 0 at sundown
    MilitaryTimeFormat dusk = MilitaryTimeFormat::FromMinutes((int)calcDuskUtc(unsignedHourAngle, equationOfTime) - utcOffset +  smootheningOffset);

    MilitaryTimeFormat now(hr, min);

    // before dawn or after dusk
    if ((now - dawn).originalMinutes < 0 || (now - dusk).originalMinutes > 0){
      return 0;
    }

    // b/w dawn and solar noon
    if ((now - solarNoon).originalMinutes < 0) {
      int totalMinsToPeak = (solarNoon - dawn).originalMinutes;
      // where does now fall in this distribution?
      int currentRelativePlacement = (now - dawn).originalMinutes;
      return ((double)currentRelativePlacement/(double)totalMinsToPeak)*100;
    }

    // b/w solarNoon and dusk
    int totalMinsToPeak = (dusk - solarNoon).originalMinutes;
    int currentRelativePlacement = (now - solarNoon).originalMinutes;
    return 100 - ((double)currentRelativePlacement/(double)totalMinsToPeak)*100;
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
  Serial.println("Begin Night Watch");
}

void loop() {
  // Todo get time now
  int dayOfYear = 45;
  int currentYear = 2023;
  int hr = 14;
  int min = 47;

  int sunlightBrightness = sbc.SunBrightnessNow(dayOfYear, currentYear, hr, min);
  Serial.print("Current Brightness percentage: ");
  Serial.println(sunlightBrightness);

  delay(brightnessUpdateInterval);
}
